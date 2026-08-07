#ifndef CORE_NETWORK_CERTIFICATE_CERTIFICATE_UTILS_HPP
#define CORE_NETWORK_CERTIFICATE_CERTIFICATE_UTILS_HPP

#include <QSslCertificate>
#include <openssl/x509v3.h>

namespace core {
namespace network {
namespace certificate {

struct X509Deleter {
    void operator() (X509 *cert) const {
        if (cert) X509_free (cert);
    }
};

using X509Ptr = std::unique_ptr<X509, X509Deleter>;

X509Ptr getRootCA(const QList<QSslCertificate> chain);
X509Ptr getIntermediateCA(QList<QSslCertificate> chain );
X509Ptr getServerCertificate(QList<QSslCertificate> chain);

bool compareIntermediateCAWithRootCA(X509 &intermediate, X509 &root);
bool compareServerWithIntermediate(X509 &server, X509 &intermediate);
bool intermedaiteAndRootSignautreVerify(X509 &intermediate, X509 &root);
bool intermediateAndServerSignatureVerify(X509 &intermediate, X509 &server);

} // namespace certificate
} // namespace network
} // namespace core



#endif // CORE_NETWORK_CERTIFICATE_CERTIFICATE_UTILS_HPP
