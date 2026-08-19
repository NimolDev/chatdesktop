#ifndef FEATURES_AUTHENTICATION_DATA_MAPPING_LOGIN_MAPPER_HPP
#define FEATURES_AUTHENTICATION_DATA_MAPPING_LOGIN_MAPPER_HPP

#include "domain/entity/login_request.hpp"
#include "data/dto/login_request_dto.hpp"

namespace data {
namespace mapping {

inline data::dto::LoginRequest toDto(const domain::entity::LoginRequest &request) {
    return {
        .username = request.username,
        .password = request.password
    };
}

} // namespace mapping
} // namespace data



#endif // FEATURES_AUTHENTICATION_DATA_MAPPING_LOGIN_MAPPER_HPP
