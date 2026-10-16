#include "local_storage.hpp"


#include <fcntl.h>
#include <openssl/evp.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <algorithm>
#include <cstring>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <sys/file.h>
#include <openssl/rand.h>

#include <openssl/hmac.h>

#include <unistd.h>

namespace core {
namespace storage {

namespace {

void randBytes(std::uint8_t *data, std::size_t size)
{
    if (RAND_bytes(data, static_cast<int>(size)) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }
}

} // namespace


LocalStorage::LocalStorage()
{
    ::mlock(m_key, sizeof m_key); // best effort
}

LocalStorage::LocalStorage(const std::string &path, const std::string &password)
    : core::storage::LocalStorage(path, password, Options{})
{}

LocalStorage::LocalStorage(const std::string &path, const std::string &password, Options opt)
    : m_opt(opt)
{
    m_fd = open(path.c_str (), O_RDWR | O_CREAT, 0600);
    if (m_fd < 0) {
        throw std::runtime_error("cannot open " + path);
    }
    if (::flock(m_fd, LOCK_EX | LOCK_NB) != 0) {
        ::close(m_fd);
        throw std::runtime_error("store is locked by another process");
    }
    ::mlock (m_key, sizeof m_key); // best effort
    try {
        struct stat st;
        if (fstat (m_fd, &st) != 0) {
            throw std::runtime_error("fstat fialed");
        }
        if (st.st_size == 0) {
            createHeader (password);
        } else {
            loadHeader (password);
        }
        scan ();
        loadDirectory ();

    } catch(...) {
        cleanup ();
        throw;
    }
}

LocalStorage::~LocalStorage()
{
    cleanup ();
}

void LocalStorage::setPath(const std::string &path, const std::string &password)
{
    std::unique_lock lk(m_mu);
    if (m_fd >= 0) {
        struct stat current;
        struct stat target;
        if (::fstat(m_fd, &current) != 0) {
            throw std::runtime_error("fstat failed");
        }
        if (::stat(path.c_str(), &target) == 0 &&
            current.st_dev == target.st_dev && current.st_ino == target.st_ino) {
            // Validate the password without reacquiring our own file lock or
            // modifying the live key if authentication fails.
            LocalStorage verifier;
            verifier.m_fd = ::dup(m_fd);
            if (verifier.m_fd < 0) {
                throw std::runtime_error("dup failed");
            }
            verifier.loadHeader(password);
            return;
        }
    }

    LocalStorage replacement(path, password, m_opt);
    if (m_fd >= 0) {
        synceFd();
    }

    // Commit only after the new store has been opened and authenticated.
    // The replacement's destructor closes the old file and clears its key.
    using std::swap;
    swap(m_fd, replacement.m_fd);
    swap(m_bs, replacement.m_bs);
    swap(m_plain, replacement.m_plain);
    swap(m_cap, replacement.m_cap);
    swap(m_nblocks, replacement.m_nblocks);
    swap(m_dirId, replacement.m_dirId);
    swap(m_dirDirty, replacement.m_dirDirty);
    swap(m_key, replacement.m_key);
    m_free.swap(replacement.m_free);
    m_head.swap(replacement.m_head);
    m_byTag.swap(replacement.m_byTag);
    m_roots.swap(replacement.m_roots);
}

std::uint64_t LocalStorage::insert(const std::string &data, std::uint32_t tag)
{
    if (tag == kDirTag) {
        throw std::invalid_argument("tag tag 0xFFFFFFFF is reserved ");
    }
    std::unique_lock lk(m_mu);
    if (m_fd < 0) {
        throw std::logic_error("Call setPath before inserting records");
    }
    return insertLocked(data, tag);
}

std::optional<std::string> LocalStorage::get(std::uint64_t id) const
{
    std::shared_lock lk(m_mu);
    return getLocked (id);
}

bool LocalStorage::update(std::uint64_t id, const std::string &data)
{
    std::unique_lock lk(m_mu);
    if (id == m_dirId || !m_head.count (id)) {
        return false;
    }
    updateLocked (id, data);
    return true;
}

bool LocalStorage::remove(std::uint64_t id)
{
    std::unique_lock lk (m_mu);
    if (id == m_dirId || !m_head.count (id)) {
        return false;
    }
    removeLocked (id);
    return true;

}

bool LocalStorage::exists(std::int64_t id) const
{
    std::shared_lock lk(m_mu);
    return id != m_dirId && m_head.count (id) != 0;
}

std::vector<std::uint64_t> LocalStorage::ids(std::uint32_t tag) const
{
    std::shared_lock lk (m_mu);
    std::vector<std::uint64_t> out;
    if (tag == kDirTag){
        return out;
    }
    auto it = m_byTag.find (tag);
    if (it != m_byTag.end ()) {
        const auto &value = it->second;
        out.assign (value.begin (), value.end ());
    }
    return out;

}

void LocalStorage::forEach(
    std::uint32_t tag,
    const std::function<void(std::uint64_t, const std::string &)> &fn) const
{
    for (std::uint64_t id : ids(tag)) {
        if (auto v = get(id)) {
            fn(id, *v);
        }
    }
}

void LocalStorage::setRoot(const std::string &name, std::uint64_t id)
{
    std::unique_lock lk(m_mu);
    if (id == m_dirId || !m_head.count(id)) {
        throw std::invalid_argument("no such record");
    }
    m_roots[name] = id;
    saveDirectoryLocked ();
}

std::optional<std::uint64_t> LocalStorage::root(const std::string &name) const
{
    std::shared_lock lk(m_mu);
    auto it = m_roots.find (name);
    if (it == m_roots.end ()) {
        return std::nullopt;
    }
    return it->second;
}

bool LocalStorage::removeRoot(const std::string &name)
{
    std::unique_lock lk(m_mu);
    if (!m_roots.erase(name)) {
        return false;
    }
    saveDirectoryLocked ();
    return true;
}

std::size_t LocalStorage::recordCount() const
{
    std::shared_lock lk(m_mu);
    return m_head.size () - (m_dirId ? 1 : 0);
}

std::uint64_t LocalStorage::blockCount() const
{
    std::shared_lock lk(m_mu);
    return m_nblocks;
}

std::size_t LocalStorage::freeBlock() const
{
    std::shared_lock lk(m_mu);
    return m_free.size ();
}

std::uint32_t LocalStorage::blockSize() const
{
    std::shared_lock lk(m_mu);
    return m_bs;
}

std::size_t LocalStorage::maxInline() const
{
    std::shared_lock lk(m_mu);
    return m_cap;
}

void LocalStorage::sync()
{
    std::unique_lock lk(m_mu);
    if (m_fd < 0) {
        throw std::logic_error("Call setPath before syncing storage");
    }
    synceFd ();
}


// MARK: --- Private API ---

const std::uint8_t *LocalStorage::Blk::data() const
{
    return plain.data() + kBlkHdr;
}

void LocalStorage::w32(std::uint8_t *p, std::uint32_t v)
{
    for (int i = 0; i < 4; i++) {
        p[i] = std::uint8_t(v >>(8 *i ));
    }
}

void LocalStorage::w64(std::uint8_t *p, std::uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        p[i] = std::uint8_t(v >> (8 * i));
    }
}

std::uint32_t LocalStorage::r32(const std::uint8_t *p)
{
    std::uint32_t v = 0;
    for (int i = 0; i < 4; i++) {
        v |= std::uint32_t(p[i]) << (8 * i);
    }
    return v;
}

std::uint64_t LocalStorage::r64(const std::uint8_t *p)
{
    std::uint64_t v = 0;
    for (int i = 0; i < 8; i++) {
        v |= std::uint64_t(p[i]) << (8 * i);
    }
    return v;
}

void LocalStorage::preadFull(int fd, void *buf, std::size_t n, std::int64_t off)
{
    auto *p = static_cast<std::uint8_t *>(buf);
    while (n) {
        ssize_t r = ::pread(fd, p, n, off);
        if (r <= 0 ) {
            throw std::runtime_error("read failed / unexspected EOF");
        }
        p += r;
        n -= std::size_t(r);
        off += std::uint64_t(r);
    }
}

void LocalStorage::pwriteFull(int fd, const void *buf, std::size_t n, std::int64_t off)
{
    auto *p = static_cast <const std::uint8_t *>(buf);
    while (n) {
        ssize_t r = ::pwrite (fd, p, n, off);
        if (r <= 0) {
            throw std::runtime_error("write failed");
        }
        p += r;
        n -= std::size_t(r);
        off += std::uint64_t(r);
    }
}

void LocalStorage::synceFd()
{
    if (::fsync (m_fd) != 0) {
        throw std::runtime_error("fsync failed");
    }
}

std::uint64_t LocalStorage::blockOffset(std::uint64_t id) const
{
    return std::uint64_t(m_bs) + (id - 1) * std::uint64_t(m_bs);
}

void LocalStorage::subkey(const std::uint8_t *nonce, std::uint8_t out[]) const
{
    unsigned int len = kKey;
    if (!HMAC(EVP_sha256(), m_key, int(kKey), nonce, kNonce, out, &len) || len != kKey) {
        throw std::runtime_error("HMAC failed");
    }
}

void LocalStorage::aeadEncrypt(const std::uint8_t *nonce, const std::uint8_t *ad, std::size_t adl,
                               const std::uint8_t *in, std::size_t n, std::uint8_t *out) const
{
    std::uint8_t sk[kKey];
    std::uint8_t iv[12] = {0};
    subkey (nonce, sk);
    EVP_CIPHER_CTX *c = EVP_CIPHER_CTX_new ();
    int l = 0;
    bool ok = c &&
              EVP_EncryptInit_ex(c, EVP_aes_256_gcm(), nullptr, sk, iv) == 1 &&
              EVP_EncryptUpdate(c, nullptr, &l, ad, int(adl)) == 1 &&
              EVP_EncryptUpdate(c, out, &l, in, int(n)) == 1 &&
              EVP_EncryptFinal_ex(c, out + l, &l) == 1 &&
              EVP_CIPHER_CTX_ctrl(c, EVP_CTRL_AEAD_GET_TAG, int(kTag), out + n) == 1;
    EVP_CIPHER_CTX_free(c);
    OPENSSL_cleanse(sk, sizeof sk);
    if (!ok) {
        throw std::runtime_error("encryption failed");
    }
}

bool LocalStorage::aeadDecrypt(const std::uint8_t *nonce, const std::uint8_t *ad, std::size_t adl,
                               const std::uint8_t *in, std::size_t n, std::uint8_t *out) const
{
    if (n < kTag) {
        return false;
    }
    std::uint8_t sk[kKey];
    std::uint8_t iv[12] = {0};
    subkey (nonce, sk);
    const std::size_t clen = n - kTag;
    EVP_CIPHER_CTX *c = EVP_CIPHER_CTX_new ();
    int l = 0;
    bool ok = c &&
              EVP_DecryptInit_ex(c, EVP_aes_256_gcm(), nullptr, sk, iv) == 1 &&
              EVP_DecryptUpdate(c, nullptr, &l, ad, int(adl)) == 1 &&
              EVP_DecryptUpdate(c, out, &l, in, int(clen)) == 1 &&
              EVP_CIPHER_CTX_ctrl(c, EVP_CTRL_AEAD_SET_TAG, int(kTag), const_cast<std::uint8_t*>(in + clen)) == 1 &&
              EVP_DecryptFinal_ex(c, out + l, &l) == 1;
    EVP_CIPHER_CTX_free(c);
    OPENSSL_cleanse(sk, sizeof sk);
    return ok;

}

void LocalStorage::deriveKey(const std::string &pw, const std::uint8_t *salt)
{
    if (EVP_PBE_scrypt(pw.data(), pw.size(), salt, kSalt, 1u << 15, 8, 1, 64u * 1024 * 1024, m_key, kKey) != 1) {
        throw std::runtime_error("key derivation failed (out of memory?)");
    }
}

void LocalStorage::setGeometry(std::uint32_t bs)
{
    m_bs = bs;
    m_plain = m_bs - kNonce - kTag; // plaintext bytes per block.
    m_cap = m_plain - kBlkHdr; // data bytes per block.
}

void LocalStorage::createHeader(const std::string &pw)
{
    if (m_opt.block_size < 128 || m_opt.block_size > (1u << 20)) {
        throw std::invalid_argument("block_size must be between 128 and 1048576");
    }
    setGeometry (m_opt.block_size);
    std::uint8_t h[kHdr];
    std::memcpy(h, kMagic, 8);
    randBytes(h + 8, kSalt);
    w32(h + 8 + kSalt, m_bs);
    deriveKey (pw, h + 8);
    std::uint8_t *nonce = h+ kHdrAad;
    randBytes (nonce, kNonce);
    aeadEncrypt (nonce, h, kHdrAad, reinterpret_cast<const std::uint8_t *> (kCheck), sizeof(kCheck), nonce + kNonce);
    pwriteFull (m_fd,h, kHdr, 0);
    if (::ftruncate (m_fd, off_t(m_bs)) != 0) {
        throw std::runtime_error("ftruncate failed"); // header  = block = 0.
    }
    synceFd ();
}

void LocalStorage::loadHeader(const std::string &pw)
{
    std::uint8_t h[kHdr];
    preadFull (m_fd, h, kHdr, 0);
    if (std::memcmp(h, kMagic, 8) != 0) {
        throw std::runtime_error("not a LocalStorage file");
    }
    std::uint32_t bs = r32(h + 8 + kSalt);
    if (bs < 128 || bs > (1u << 20)) {
        throw std::runtime_error("corrupt header (block size)");
    }
    setGeometry (bs);
    deriveKey (pw, h + 8);
    const std::uint8_t *nonce = h + kHdrAad;
    std::uint8_t out[sizeof kCheck];
    if (!aeadDecrypt (nonce, h , kHdrAad, nonce + kNonce, sizeof kCheck + kTag, out)) {
        throw std::runtime_error("wrong password or corrupted header");
    }
}

void LocalStorage::writeBlock(std::uint64_t id, std::uint8_t type, std::uint32_t tag, std::uint64_t next, const std::uint8_t *data, std::size_t used)
{
    std::vector<std::uint8_t> plain(m_plain, 0);
    std::vector<std::uint8_t> raw(m_bs);
    plain[0] = type;
    w32(&plain[1], tag);
    w64(&plain[5], next);
    w32(&plain[13], std::uint32_t(used));
    if (used) {
        std::memcpy(&plain[kBlkHdr], data, used);
    }
    std::uint8_t *nonce = raw.data ();
    randBytes (nonce, kNonce);
    std::uint8_t ad[8];
    w64(ad, id);
    aeadEncrypt (nonce, ad, 8, plain.data (), plain.size (), nonce + kNonce);
    pwriteFull (m_fd, raw.data (), raw.size (), blockOffset (id));
}

void LocalStorage::readBlock(std::uint64_t id, Blk &b) const
{
    std::vector<std::uint8_t> raw(m_bs);
    preadFull(m_fd, raw.data (), raw.size(), blockOffset (id));
    b.plain.resize (m_plain);
    std::uint8_t ad[8];
    w64(ad, id);
    if (!aeadDecrypt (raw.data (), ad, 8, raw.data () + kNonce, m_plain + kTag, b.plain.data ())) {
        throw std::runtime_error("block " + std::to_string (id) + " falied authentication (corrupt, tampered or moved");
    }
    b.type = b.plain[0];
    b.tag = r32(&b.plain[1]);
    b.next = r64(&b.plain[5]);
    b.used = r32(&b.plain[13]);
    if (b.type > 2 || b.used > m_cap) {
        throw std::runtime_error("block " + std::to_string (id) + " is malformed");
    }
}

std::uint64_t LocalStorage::alloc()
{
    if (!m_free.empty ()) {
        std::uint64_t id = m_free.back ();
        m_free.pop_back ();
        return id;
    }
    return ++m_nblocks;
}

void LocalStorage::scan()
{
    struct stat st;
    if (::fstat (m_fd, &st) != 0) {
        throw std::runtime_error("fstart failed");
    }
    const std::uint64_t fsize = std::uint64_t(st.st_size);
    if (fsize < m_bs) {
        throw std::runtime_error("file shorter than its header block");
    }
    std::uint64_t n = (fsize - m_bs) / m_bs; // a partial trailing block (torn extend) is ignored.

    std::vector<Info> info(n + 1, Info { 0, 0, 0});
    Blk b;
    for (std::uint64_t  id = 1; id <= n; id++) {
        readBlock (id, b);
        info[id] = Info{b.type, b.tag, b.next};
    }

    std::vector<char> seen(n + 1, 0);
    for (std::uint64_t id = 1; id <= n; id++) {
        if (info[id].type != 1) {
            continue;
        }
        m_head[id] = HeadInfo{info[id].tag, info[id].next};
        m_byTag[info[id].tag].insert (id);
        for (std::uint64_t c = info[id].next; c; c = info[c].next) {
            if ( c > n || info[c].type != 2 || seen[c]) {
                throw std::runtime_error("broken block chain at record " + std::to_string (id));
            }
            seen[c] = 1;
        }
    }

    // free = explicitly free blocks + continuation blocks nobody points to (leaked by a crash / old versions)
    std::vector<char> is_free(n + 1, 0);
    for (std::uint64_t id = 1; id <= n; id++) {
        if (info[id].type == 0 || (info[id].type == 2 && !seen[id])) {
            is_free[id] = 1;
        }
    }
    while (n > 0 && is_free[n]) {
        n--; // Trim free space at the end of the file.
    }
    if (fsize != m_bs + n * std::uint64_t(m_bs) && ::ftruncate (m_fd, off_t(m_bs + n * std::uint64_t(m_bs))) != 0) {
        throw std::runtime_error("ftruncate failed");
    }
    m_nblocks = n;
    for (std::uint64_t id = n; id >=1; id--) {
        if (is_free[id]) {
            m_free.push_back (id);
        }
    }
}

void LocalStorage::loadDirectory()
{
    auto it = m_byTag.find (kDirTag);
    if (it == m_byTag.end () || it->second.empty ()) {
        return;
    }
    m_dirId = *it->second.begin();
    auto raw = getLocked (m_dirId);
    if (!raw) {
        throw std::runtime_error("directory record missing");
    }
    const std::uint8_t *p = reinterpret_cast<const std::uint8_t *> (raw->data ());
    std::size_t left = raw->size ();
    auto need = [&](std::size_t k) {
        if (left < k) {
            throw std::runtime_error("corrupt directory");
        }
    };
    need(4);
    std::uint32_t count = r32(p);
    p += 4;
    left -= 4;
    for (std::uint32_t i = 0; i < count; i++) {
        need(4);
        std::uint32_t len = r32(p);
        p += 4;
        left -= 4;
        need(std::size_t(len) + 8);
        std::string name(reinterpret_cast<const char *>(p), len);
        p += len;
        left -= len;
        m_roots[name] = r64(p);
        p += 8;
        left -= 8;
    }
}

void LocalStorage::saveDirectoryLocked()
{
    std::string out(4, '\0');

    w32(
        reinterpret_cast<std::uint8_t*>(out.data()),
        static_cast<std::uint32_t>(m_roots.size())
        );

    for (auto it = m_roots.cbegin(); it != m_roots.cend(); ++it) {
        const std::string &name = it->first;

        std::uint8_t t[8];

        w32(t, static_cast<std::uint32_t>(name.size()));
        out.append(reinterpret_cast<const char*>(t), 4);

        out.append(name);

        w64(t, it->second);
        out.append(reinterpret_cast<const char*>(t), 8);
    }

    if (m_dirId == 0) {
        m_dirId = insertLocked (out, kDirTag);
    } else {
        updateLocked (m_dirId, out);
    }
}

std::size_t LocalStorage::chunks(std::size_t bytes) const
{
    return std::max<std::size_t> (1, (bytes + m_cap -1) / m_cap);
}

void LocalStorage::writeChain(const std::vector<std::uint64_t> &ids, const std::string &data, std::uint32_t tag)
{
    const std::size_t n = ids.size();

    const auto *base =
        reinterpret_cast<const std::uint8_t *>(data.data());

    auto len_of = [&](std::size_t i) -> std::size_t {
        const std::size_t offset = i * m_cap;

        return offset < data.size()
                   ? std::min(m_cap,  (static_cast<std::size_t>(data.size ()) - offset))
                   : std::size_t(0);
    };

    for (std::size_t i = n; i-- > 1;) {
        writeBlock(
            ids[i],
            2,
            0,
            i + 1 < n ? ids[i + 1] : 0,
            base + i * m_cap,
            len_of(i)
            );
    }

    if (m_opt.durable && n > 1) {
        synceFd();
    }

    // Write head after all continuation blocks.
    writeBlock(
        ids[0],
        1,
        tag,
        n > 1 ? ids[1] : 0,
        base,
        len_of (0)
        );

    if (m_opt.durable) {
        synceFd();
    }
}

std::uint64_t LocalStorage::insertLocked(const std::string &data, std::uint32_t tag)
{
    std::vector<std::uint64_t> ids(chunks(data.size()));

    for (auto &id : ids) {
        id = alloc();
    }

    writeChain(ids, data, tag);

    m_head[ids[0]] = HeadInfo{
        tag,
        ids.size() > 1 ? ids[1] : 0
    };

    m_byTag[tag].insert(ids[0]);

    return ids[0];
}

std::optional<std::string> LocalStorage::getLocked(std::uint64_t id) const
{
    auto it = m_head.find(id);

    if (it == m_head.cend()) {
        return std::nullopt;
    }

    std::string out;
    Blk block;

    std::uint64_t current = id;
    std::uint64_t guard = 0;

    while (current != 0) {
        if (++guard > m_nblocks + 1) {
            throw std::runtime_error("block chain loop");
        }

        readBlock(current, block);

        const auto expectedType = current == id ? 1 : 2;

        if (block.type != expectedType) {
            throw std::runtime_error("unexpected block type in chain");
        }

        out.append(
            reinterpret_cast<const char *>(block.data()),
            static_cast<std::size_t>(block.used)
            );

        current = block.next;
    }

    return out;
}

std::vector<std::uint64_t> LocalStorage::oldContinuations(const HeadInfo &hi) const
{
    std::vector<std::uint64_t> old;
    Blk block;

    for (std::uint64_t current = hi.next; current != 0;) {
        if (old.size() > m_nblocks) {
            throw std::runtime_error("block chain loop");
        }

        old.push_back(current);

        readBlock(current, block);
        current = block.next;
    }

    return old;
}

void LocalStorage::updateLocked(std::uint64_t id, const std::string &data)
{
    HeadInfo &hi = m_head[id];

    std::vector<std::uint64_t> old = oldContinuations(hi);

    std::vector<std::uint64_t> ids(chunks(data.size()));
    ids[0] = id;

    for (std::size_t i = 1; i < ids.size(); ++i) {
        ids[i] = alloc();
    }

    // Old chain remains valid until writeChain() updates the head.
    writeChain(ids, data, hi.tag);

    hi.next = ids.size() > 1 ? ids[1] : 0;

    for (std::uint64_t blockId : std::as_const(old)) {
        m_free.push_back(blockId);
    }
}

void LocalStorage::removeLocked(std::uint64_t id)
{
    auto it = m_head.find(id);

    if (it == m_head.end()) {
        return; // or throw, depending on your API contract
    }

    std::vector<std::uint64_t> old = oldContinuations(it->second);

    writeBlock(id, 0, 0, 0, nullptr, 0);

    if (m_opt.durable) {
        synceFd();
    }

    // Remove from tag index
    auto ts = m_byTag.find(it->second.tag);

    if (ts != m_byTag.end()) {
        ts->second.erase(id);

        if (ts->second.empty()) {
            m_byTag.erase(ts);
        }
    }

    // Remove head
    m_head.erase(it);

    // Recycle blocks
    m_free.push_back(id);

    for (std::uint64_t blockId : std::as_const(old)) {
        m_free.push_back(blockId);
    }

    // Remove roots pointing to this record
    for (auto r = m_roots.begin(); r != m_roots.end();) {
        if (r->second == id) {
            r = m_roots.erase(r);
            m_dirDirty = true;
        } else {
            ++r;
        }
    }

    if (m_dirDirty) {
        m_dirDirty = false;
        saveDirectoryLocked();
    }
}

void LocalStorage::cleanup()
{
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }

    OPENSSL_cleanse(m_key, sizeof(m_key));
    ::munlock(m_key, sizeof(m_key));
}






} // namespace storage
} // namespace core
