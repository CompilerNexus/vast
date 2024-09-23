#include "vast/server/server.hpp"
#include "vast/server/io.hpp"
#include "vast/server/sync_collections.hpp"

#include <gap/coro/sync_wait.hpp>
#include <iostream>
#include <variant>

struct input_request
{
    static constexpr const char *method   = "input";
    static constexpr bool is_notification = false;

    nlohmann::json type;
    std::string text;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(input_request, type, text)

    struct response_type
    {
        nlohmann::json value;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(response_type, value)
    };
};

enum class message_kind {
    info,
    warn,
    err,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    message_kind,
    {
        { message_kind::info, "info" },
        { message_kind::warn, "warn" },
        {  message_kind::err,  "err" },
}
)

struct message_notification
{
    static constexpr const char *method   = "message";
    static constexpr bool is_notification = true;

    message_kind kind;
    std::string text;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(message_notification, kind, text)
};

enum class console_severity { trace, debug, info, warn, err };

NLOHMANN_JSON_SERIALIZE_ENUM(
    console_severity,
    {
        { console_severity::trace, "trace" },
        { console_severity::debug, "debug" },
        {  console_severity::info,  "info" },
        {  console_severity::warn,  "warn" },
        {   console_severity::err,   "err" },
}
)

struct console_notification
{
    static constexpr const char *method   = "console";
    static constexpr bool is_notification = true;

    console_severity severity;
    std::string message;
    nlohmann::json params;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(console_notification, severity, message, params)
};

struct handler
{
    void operator()(vast::server::server_base &server, const console_notification &noti) {
        server.send_notification(message_notification{ message_kind::warn, noti.message });
    }
};

void run_server() {
    vast::server::server< handler, console_notification > server{
        vast::server::sock_adapter::create_unix_socket("/tmp/vast.sock")
    };
    try {
        while (true) {
            auto response = server.send_request(input_request{ "string", "what's your name?" });
            if (auto result = std::get_if< input_request::response_type >(&response)) {
                server.send_notification(message_notification{
                    message_kind::info, "Hello, " + std::string(result->value) });
            } else {
                server.send_notification(message_notification{ message_kind::err,
                                                               "Error received" });
                break;
            }
        }
    } catch (const vast::server::execution_stopped &) {
        std::cerr << "End of connection\n";
    }
}

int main() { run_server(); }
