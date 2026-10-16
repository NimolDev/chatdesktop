#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <shared_mutex>

namespace core {
namespace storage {

class LocalStorage
{
public:

    static constexpr std::uint32_t kDirTag = 0xFFFFFFFFu;
    struct Options {
        std::uint32_t block_size = 4096;
        bool durable = false;
    };

    // Create an unopened store; call setPath before inserting or syncing.
    explicit LocalStorage();
    explicit LocalStorage(const std::string &path, const std::string &password);
    explicit LocalStorage(const std::string &path, const std::string &password, Options opt);

    ~LocalStorage();
    LocalStorage(const LocalStorage &) = delete;
    LocalStorage &operator=(const LocalStorage &) = delete;

    // Open/create another store using its password. Existing records stay in the old file.
    // Throws on failure and keeps the current store usable. Does not change a file's password.
    // Reusing the current file validates the password and keeps the open store.
    void setPath(const std::string &path, const std::string &password);

    // --- record -----
    // Create a record. `tag` groups records inot a collection (any value except 0xFFFFFFFF).
    // Strings contain bytes; use UTF-8 when storing text.
    std::uint64_t insert(const std::string &data, std::uint32_t tag = 0);
    std::optional<std::string> get(std::uint64_t id) const ;
    bool update(std::uint64_t id, const std::string &data);
    bool remove(std::uint64_t id);
    bool exists(std::int64_t id) const;

    // IDS of all recores with this tag, ascending.
    std::vector<std::uint64_t> ids(std::uint32_t tag) const;

    // Call fn(id, data) for each record with this tag. fn may call any LocalStorage Method.
    void forEach(std::uint32_t tag,
                 const std::function<void(std::uint64_t, const std::string &)> &fn) const;

    // ---- named roots (optional convenience: name -> record ID) --------
    void setRoot(const std::string &name, std::uint64_t id);
    std::optional<std::uint64_t> root(const std::string &name) const;
    bool removeRoot(const std::string &name);

    // --- info / control --------------
    std::size_t recordCount() const;
    std::uint64_t blockCount() const;
    std::size_t freeBlock() const;
    std::uint32_t blockSize() const;
    std::size_t maxInline() const;

    void sync();

private:
    static constexpr std::size_t kNonce = 24, kTag = 16, kKey = 32, kSalt = 16;
    static constexpr char kMagic[8] = {'E', 'S', 'B', 'S', '0', '0', '0', '1'};
    static constexpr char kCheck[16] = {'E', 'S', 'B', 'S', '-', 'C', 'H', 'E', 'C', 'K', '-', 'O', 'K', '!', '!', '!'};
    static constexpr std::size_t kHdrAad = 8 + kSalt + 4;                          // magic, salt, block size
    static constexpr std::size_t kHdr = kHdrAad + kNonce + sizeof(kCheck) + kTag;  // 84
    static constexpr std::size_t kBlkHdr = 17;                                     // type u8, tag u32, next u64, used u32

    struct Blk
    {
        std::uint8_t type = 0;  // 0 free, 1 record head, 2 continuation
        std::uint32_t tag = 0;
        std::uint64_t next = 0;
        std::uint32_t used = 0;
        std::vector<std::uint8_t> plain;
        const std::uint8_t* data() const;
    };
    struct HeadInfo
    {
        std::uint32_t tag;
        std::uint64_t next;
    };
    struct Info
    {
        std::uint8_t type;
        std::uint32_t tag;
        std::uint64_t next;
    };

    // ---- little-endian helpers -------------------------------------------------
    static void w32(std::uint8_t* p, std::uint32_t v);
    static void w64(std::uint8_t* p, std::uint64_t v);
    static std::uint32_t r32(const std::uint8_t* p);
    static std::uint64_t r64(const std::uint8_t* p);

    static void preadFull(int fd, void *buf, std::size_t n, std::int64_t off);
    static void pwriteFull(int fd, const void *buf, std::size_t n, std::int64_t off);

    void synceFd();
    std::uint64_t blockOffset(std::uint64_t id) const;

    // Per-block subkey = HMAC-SHA256(master, nonce24); fixed zero IV is safe becuase the key is
    // unique pre block write;
    void subkey(const std::uint8_t *nonce, std::uint8_t out[kKey]) const;
    void aeadEncrypt(const std::uint8_t *nonce, const std::uint8_t *ad, std::size_t adl,
                     const std::uint8_t * in, std::size_t n, std::uint8_t *out) const;
    bool aeadDecrypt(const std::uint8_t *nonce, const std::uint8_t *ad, std::size_t adl,
                     const std::uint8_t *in, std::size_t n, std::uint8_t *out) const;
    void deriveKey(const std::string &pw, const std::uint8_t *salt);

    // --- header --------------
    void setGeometry(std::uint32_t bs);
    void createHeader(const std::string &pw);
    void loadHeader(const std::string &pw);

    // ----- Block IO -------------------------
    // On disk: [24B nonce][ciphertext (plain_ bytes) || 16B tag] = block_size bytes. AAD = block number.
    void writeBlock(std::uint64_t id, std::uint8_t type, std::uint32_t tag, std::uint64_t next, const std::uint8_t *data, std::size_t used);
    void readBlock(std::uint64_t id, Blk &b) const;
    std::uint64_t alloc();

    // --- open-time scan ---------------------
    void scan();
    void loadDirectory();
    void saveDirectoryLocked();

    // ---- record operations (caller holds the lock) -------------
    std::size_t chunks(std::size_t bytes) const;

    // Write a record across ids[0..n]: continuation blocks first, head (ids[0]) last.
    void writeChain(const std::vector<std::uint64_t> &ids, const std::string &data, std::uint32_t tag);

    std::uint64_t insertLocked(const std::string &data, std::uint32_t tag);
    std::optional<std::string> getLocked(std::uint64_t id) const;
    std::vector<std::uint64_t> oldContinuations(const HeadInfo &hi) const;

    void updateLocked(std::uint64_t id, const std::string &data);
    void removeLocked(std::uint64_t id);
    void cleanup();

    Options m_opt;
    int m_fd= -1;
    std::uint32_t m_bs = 0;
    std::size_t m_plain = 0;
    std::size_t m_cap = 0;
    std::uint64_t m_nblocks = 0; // number of data blocks (ids 1..m_nblocks)
    std::uint64_t m_dirId = 0;
    bool m_dirDirty = false;
    std::uint8_t m_key[kKey] = {};

    std::vector<std::uint64_t> m_free;
    std::unordered_map<std::uint64_t, HeadInfo> m_head;
    std::map<std::uint32_t, std::set<std::uint64_t>> m_byTag;
    std::map<std::string, std::uint64_t> m_roots;

    mutable std::shared_mutex m_mu;

};



} // namespace storage

} // namespace core


























