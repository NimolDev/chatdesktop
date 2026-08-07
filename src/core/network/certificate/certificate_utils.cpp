#include "certificate_utils.hpp"


namespace core {
namespace network {
namespace certificate {
namespace {

// Converts a single QSslCertificate -> X509Ptr (declared earlier, reused here)
X509Ptr qSslCertificateToX509Ptr(const QSslCertificate &cert)
{
    if (cert.isNull()) {
        return nullptr;
    }
    const QByteArray der = cert.toDer();
    const unsigned char *data = reinterpret_cast<const unsigned char *>(der.constData());
    X509 *raw = d2i_X509(nullptr, &data, der.size());
    return X509Ptr(raw);
}

/*
* Returns:
*  -1      : not a CA, or BasicConstraints extension absent
*  INT_MAX : CA with no path length limit
*  0..N    : CA, can issue intermediate CAs up to N levels deep
*/
long getBasicConstraints(const X509 *certificate)
{
    BASIC_CONSTRAINTS *constraints = nullptr;
    long path_length = -1;

    if (certificate == nullptr) {
        return path_length;
    }

    // 1. Extract and parse the BASIC_CONSTRAINTS extension structure.
    constraints = static_cast<BASIC_CONSTRAINTS *>(X509_get_ext_d2i (certificate, NID_basic_constraints, nullptr, nullptr));

    if (constraints == nullptr) {
        return path_length;
    }

    // 2. Check if the CA flasg is explicitly set to true
    const bool isCA = constraints->ca != 0;
    if (!isCA) {
        BASIC_CONSTRAINTS_free (constraints);
        return path_length;
    }

    // Extract the path length  if it exists
    if (constraints->pathlen != nullptr) {
        path_length = ASN1_INTEGER_get (constraints->pathlen);
    } else {
        // NO limit set -> treat as unconstrained
        path_length = INT_MAX;
    }


    BASIC_CONSTRAINTS_free(constraints);
    return path_length;


}
} // namespace

/*
 * RootCA
 * Subject and Issuer are the same
 * PathLen > 0
 */
X509Ptr getRootCA(const QList<QSslCertificate> chain)
{
    for (const QSslCertificate &cert : chain) {
        X509Ptr x509 = qSslCertificateToX509Ptr (cert);
        if (!x509) {
            continue;
        }
        long path_length = getBasicConstraints (x509.get ());
        // qDebug() << "path length "<< path_length;
        if (path_length > 0) {
            return x509;
        }
    }
    return nullptr;
}

/*
* Intermediat CA Certificate
* Subject and Issure are difference,
* PathLen = 0
*/
X509Ptr getIntermediateCA(QList<QSslCertificate> chain)
{
    for (const QSslCertificate &cert : chain) {
        X509Ptr x509 = qSslCertificateToX509Ptr (cert);
        if (!x509) {
            continue;
        }
        long path_length = getBasicConstraints (x509.get ());
        // qDebug() << "path length: "<< path_length;
        if (path_length == 0) {
            char *subject = X509_NAME_oneline (X509_get_subject_name (x509.get ()), nullptr, 0);
            char *issuer = X509_NAME_oneline (X509_get_issuer_name (x509.get ()), nullptr, 0);

            if (subject && issuer)  {
                if (subject != issuer) {
                    OPENSSL_free (subject);
                    OPENSSL_free (issuer);
                    return x509;
                }
            }
            OPENSSL_free (subject);
            OPENSSL_free (issuer);
        }
    }
    return nullptr;
}

/*
 * Server Certificate
 * Subject and Issuer are difference,
 * PathLen  = -1
 */
X509Ptr getServerCertificate(QList<QSslCertificate> chain)
{

    for (const QSslCertificate &cert : chain) {
        X509Ptr x509 = qSslCertificateToX509Ptr (cert);
        if (!x509) {
            continue;
        }
        long path_length = getBasicConstraints (x509.get ());
        if (path_length == -1 ) {
            char *subject = X509_NAME_oneline (X509_get_subject_name (x509.get ()), nullptr, 0);
            char *issuer = X509_NAME_oneline (X509_get_issuer_name (x509.get ()), nullptr, 0);
            if (subject && issuer) {
                if (subject != issuer) {
                    OPENSSL_free (subject);
                    OPENSSL_free (issuer);
                    return x509;
                }
            }
            OPENSSL_free (subject);
            OPENSSL_free (issuer);
        }
    }
    return nullptr;
}

bool compareIntermediateCAWithRootCA(X509 &intermediate, X509 &root)
{
    return X509_NAME_cmp(
               X509_get_issuer_name(&intermediate),
               X509_get_subject_name(&root) ) == 0;
}

bool compareServerWithIntermediate(X509 &server, X509 &intermediate)
{
    return X509_NAME_cmp (
               X509_get_issuer_name (&server),
               X509_get_subject_name (&intermediate)
               ) == 0;
}

bool intermedaiteAndRootSignautreVerify(X509 &intermediate, X509 &root)
{

    return X509_verify (
               &intermediate,
               X509_get_pubkey (&root)
               ) != 0;
}

bool intermediateAndServerSignatureVerify(X509 &intermediate, X509 &server)
{
    return X509_verify (
               &server,
               X509_get_pubkey (&intermediate)
               ) != 0;
}


} // namespace certificate
} // namespace network
} // namespace core