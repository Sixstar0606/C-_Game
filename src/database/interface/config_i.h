#pragma once
#include <cstdint>
#include <sqlpp11/table.h>
#include <sqlpp11/char_sequence.h>
#include <sqlpp11/column_types.h>

namespace GTServer {
    namespace config_i {
        struct VanguardToken {
            struct _alias_t {
                static constexpr const char _literal[] = "vanguard_token";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T vanguard_token;
                    T& operator()() {
                        return vanguard_token;
                    }

                    const T& operator()() const {
                        return vanguard_token;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };
        struct DiscordServer {
            struct _alias_t {
                static constexpr const char _literal[] = "discord_server";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T discord_server;
                    T& operator()() {
                        return discord_server;
                    }

                    const T& operator()() const {
                        return discord_server;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integer>;
        };
    }
    struct ConfigDB 
    : sqlpp::table_t<ConfigDB, 
        config_i::VanguardToken,
        config_i::DiscordServer> {
        struct _alias_t {
            static constexpr const char _literal[] = "config";
            using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T>
            struct _member_t {
                T config_db;
                T& operator()() {
                    return config_db;
                }
                const T& operator()() const {
                    return config_db;
                }
            };
        };
    };
}