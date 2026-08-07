#ifndef CORE_NETWORK_CERTIFICATE_CERTIFICATE_CHAIN_VALIDATOR_HPP
#define CORE_NETWORK_CERTIFICATE_CERTIFICATE_CHAIN_VALIDATOR_HPP

#include <QList>
#include <QString>
#include <QSslCertificate>

namespace core {
namespace network {
namespace certificate {

enum class ValidatorError
{
    None,
    EmptyChain,
    ServerCertificateNotFound,
    IntermediateCertificateNotFound,
    RootCerttificateNotFound,
    ServerIssuerMismatch,
    IntermediateIssuerMismath,
    ServerSignatureInvalid,
    IntermediateSignatureInvalid
};

struct ValidatorResult
{
    ValidatorError error = ValidatorError::None;
    QString message;

    [[nodiscard]]
    bool isValid() const noexcept
    {
        return error == ValidatorError::None;
    }
};

class CertificateChainValidator
{
public:
    [[nodiscard]]
    ValidatorResult validate(const QList<QSslCertificate> &chain) const;
};

} // namespace certificate
} // namespace network
} // namespace core

class CertificateChainValidator
{
public:
    CertificateChainValidator();
};

#endif // CORE_NETWORK_CERTIFICATE_CERTIFICATE_CHAIN_VALIDATOR_HPP
