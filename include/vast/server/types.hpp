// Copyright (c) 2024-present, Trail of Bits, Inc.

#pragma once

#include <concepts>
#include <cstdint>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

namespace vast::server {
    template< typename T >
    concept json_convertible = requires(T obj, nlohmann::json &json) {
        {
            nlohmann::to_json(json, obj)
        };
        {
            nlohmann::from_json(json, obj)
        };
    };

    template< typename T >
    concept message_like = json_convertible< T > && requires {
        {
            T::is_notification
        } -> std::convertible_to< bool >;
        {
            T::method
        } -> std::convertible_to< std::string >;
    };

    template< typename message >
    concept notification_like = message_like< message > && message::is_notification;

    template< typename message >
    concept request_like = message_like< message > && !message::is_notification
        && json_convertible< typename message::response_type >;

    template< typename message >
    concept request_with_error_like =
        request_like< message > && json_convertible< typename message::error_type >;

    template< typename T >
    struct error;

    template< request_with_error_like request >
    struct error< request >
    {
        int64_t code;
        std::string message;
        typename request::error_type body;
    };

    template< request_like request >
    struct error< request >
    {
        int64_t code;
        std::string message;
    };

    template< request_with_error_like request >
    void to_json(nlohmann::json &json, const error< request > &err) {
        json["code"]    = err.code;
        json["message"] = err.message;
        to_json(json["data"], err.body);
    }

    template< request_like request >
    void to_json(nlohmann::json &json, const error< request > &err) {
        json["code"]    = err.code;
        json["message"] = err.message;
    }

    template< request_with_error_like request >
    void from_json(const nlohmann::json &json, error< request > &err) {
        from_json(json["code"], err.code);
        from_json(json["message"], err.message);
        from_json(json["data"], err.body);
    }

    template< request_like request >
    void from_json(const nlohmann::json &json, error< request > &err) {
        from_json(json["code"], err.code);
        from_json(json["message"], err.message);
    }

    template< request_like request >
    using result_type = std::variant< typename request::response_type, error< request > >;
} // namespace vast::server
