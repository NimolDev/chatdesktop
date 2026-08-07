#include "certificate_chain_validator.hpp"
#include "certificate_utils.hpp"

namespace core {
namespace network {
namespace certificate {
ValidatorResult certificate::CertificateChainValidator::validate(const QList<QSslCertificate> &chain) const
{
    if (chain.isEmpty ()) {
        return {
            ValidatorError::EmptyChain,
            QStringLiteral ("The peer certificate chain is empty")
        };
    }

    X509Ptr root_ca = getRootCA (chain);
    if (!root_ca.get ()) {
        return {
            ValidatorError::RootCerttificateNotFound,
            QStringLiteral ("Root CA certificate could not found")
        };
    }

    X509Ptr intermediate_ca = getIntermediateCA (chain);
    if (!intermediate_ca.get ()) {
        return {
            ValidatorError::IntermediateCertificateNotFound,
            QStringLiteral ("Intermdiate CA certificate could not found")
        };
    }

    X509Ptr server_certificate = getServerCertificate (chain);
    if (!server_certificate) {
        return {
            ValidatorError::ServerCertificateNotFound,
            QStringLiteral ("Server certificate could not found")
        };
    }

    if (!compareIntermediateCAWithRootCA (
            *intermediate_ca.get (),
            *root_ca.get ())
        ) {
        return {
            ValidatorError::IntermediateIssuerMismath,
            QStringLiteral ("Intermediate and Root Mismatch")
        };
    }

    if (!compareServerWithIntermediate (
            *server_certificate.get (),
            *intermediate_ca.get ())
        )
    {
        return {
            ValidatorError::ServerIssuerMismatch,
            QStringLiteral ("Server and Intermediate Mismatch")
        };
    }
    if (!intermedaiteAndRootSignautreVerify (
            *intermediate_ca.get (),
            *root_ca.get ())
        )
    {
        return {
            ValidatorError::IntermediateSignatureInvalid,
            QStringLiteral ("Intermeidate and Root Signature invalid")
        };
    }
    if (!intermediateAndServerSignatureVerify (
            *intermediate_ca.get (),
            *server_certificate.get ())
        )
    {
        return {
            ValidatorError::ServerSignatureInvalid,
            QStringLiteral ("Server and Intermdiate Signature invalid")
        };
    }
    return {
        ValidatorError::None,
        QStringLiteral ("Make connect to trust connection")
    };
}

} // namespace certificate
} // namespace network
} // namespace core