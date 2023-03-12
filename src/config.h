#pragma once
#include <string>
#include <string_view>

#define SERVER_NAME "BetterGrowtopia"
#define SERVER_VERSION "1.0.0"
#define HTTP_SERVER

namespace GTServer {
    namespace config {
        namespace http {
            constexpr std::string_view address = "0.0.0.0";
            constexpr uint16_t port = 443;
            namespace gt {
                constexpr std::string_view address = "94.76.230.35";
            }
        }
        namespace server_default {
            constexpr std::string_view address = "94.76.230.35";
            constexpr uint16_t port = 17091;
        }
        namespace database {
            inline const std::string& host = "127.0.0.1";
            inline const uint16_t& port = 3306;
            inline const std::string& user = "root";
            inline const std::string& password = "";
            inline const std::string& database = "gtserver";
            inline const bool& auto_reconnect{ true };
            inline const bool& debug{ false };
        }
        namespace server {
            constexpr std::string_view worlds_dir       { "worlds/" };
            constexpr std::string_view utils_dir        { "utils/" };
            constexpr std::string_view renders_dir      { "renders/" };
            inline const std::string& discord           { "https://discord.gg/4XZQnQNwGn" };

            inline const std::string& cache_server      { "ubistatic-a.akamaihd.net" };
            inline const std::string& cache_path        { "0098/95135/cache/" };
        }
    }
}