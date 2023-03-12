#pragma once
#include <cstdint>
#include <sqlpp11/table.h>
#include <sqlpp11/char_sequence.h>
#include <sqlpp11/column_types.h>

namespace GTServer {
    namespace player_i {
        struct ID {
            struct _alias_t {
                static constexpr const char _literal[] = "id";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T id;
                    T& operator()() {
                        return id;
                    }

                    const T& operator()() const {
                        return id;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integral, ::sqlpp::tag::must_not_insert, ::sqlpp::tag::must_not_update>;
        };
        struct RequestedName {
            struct _alias_t {
                static constexpr const char _literal[] = "requested_name";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T requested_name;
                    T& operator()() {
                        return requested_name;
                    }

                    const T& operator()() const {
                        return requested_name;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar, ::sqlpp::tag::must_not_update>;
        };
        struct TankIDName {
            struct _alias_t {
                static constexpr const char _literal[] = "tank_id_name";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T tank_id_name;
                    T& operator()() {
                        return tank_id_name;
                    }

                    const T& operator()() const {
                        return tank_id_name;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };
        struct TankIDPass {
            struct _alias_t {
                static constexpr const char _literal[] = "tank_id_pass";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T tank_id_pass;
                    T& operator()() {
                        return tank_id_pass;
                    }

                    const T& operator()() const {
                        return tank_id_pass;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };
        struct RawName {
            struct _alias_t {
                static constexpr const char _literal[] = "raw_name";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T raw_name;
                    T& operator()() {
                        return raw_name;
                    }

                    const T& operator()() const {
                        return raw_name;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };
        struct DisplayName {
            struct _alias_t {
                static constexpr const char _literal[] = "display_name";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T display_name;
                    T& operator()() {
                        return display_name;
                    }

                    const T& operator()() const {
                        return display_name;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };
        struct Email {
            struct _alias_t {
                static constexpr const char _literal[] = "email";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T email;
                    T& operator()() {
                        return email;
                    }

                    const T& operator()() const {
                        return email;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };
        struct Discord {
            struct _alias_t {
                static constexpr const char _literal[] = "discord";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T discord;
                    T& operator()() {
                        return discord;
                    }

                    const T& operator()() const {
                        return discord;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::bigint>;
        };
        struct RelativeIdentifier {
            struct _alias_t {
                static constexpr const char _literal[] = "relative_identifier";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T relative_identifier;
                    T& operator()() {
                        return relative_identifier;
                    }

                    const T& operator()() const {
                        return relative_identifier;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };     
        struct MachineAddress {
            struct _alias_t {
                static constexpr const char _literal[] = "machine_address";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T machine_address;
                    T& operator()() {
                        return machine_address;
                    }

                    const T& operator()() const {
                        return machine_address;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };
        struct Role {
            struct _alias_t {
                static constexpr const char _literal[] = "role";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T role;
                    T& operator()() {
                        return role;
                    }

                    const T& operator()() const {
                        return role;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integer>;
        };
        struct Inventory {
            struct _alias_t {
                static constexpr const char _literal[] = "inventory";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T inventory;
                    T& operator()() {
                        return inventory;
                    }

                    const T& operator()() const {
                        return inventory;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::blob>;
        };
        struct Clothes {
            struct _alias_t {
                static constexpr const char _literal[] = "clothes";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T clothes;
                    T& operator()() {
                        return clothes;
                    }

                    const T& operator()() const {
                        return clothes;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::blob>;
        };
        struct LastActive {
            struct _alias_t {
                static constexpr const char _literal[] = "last_active";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T last_active;
                    T& operator()() {
                        return last_active;
                    }

                    const T& operator()() const {
                        return last_active;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::time_point>;
        };  
        struct Gems {
            struct _alias_t {
                static constexpr const char _literal[] = "gems";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T gems;
                    T& operator()() {
                        return gems;
                    }

                    const T& operator()() const {
                        return gems;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integer>;
        };  
        struct Playmods {
            struct _alias_t {
                static constexpr const char _literal[] = "playmods";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T playmods;
                    T& operator()() {
                        return playmods;
                    }

                    const T& operator()() const {
                        return playmods;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::blob>;
        };
        struct CharacterState {
            struct _alias_t {
                static constexpr const char _literal[] = "character_state";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T character_state;
                    T& operator()() { return character_state; }
                    const T& operator()() const { return character_state; }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::blob>;
        };
        struct Country {
            struct _alias_t {
                static constexpr const char _literal[] = "country";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T country;
                    T& operator()() { return country; }
                    const T& operator()() const { return country; }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };
    }

    struct PlayerDB 
    : sqlpp::table_t<PlayerDB, 
        player_i::ID, 
        player_i::RequestedName, 
        player_i::TankIDName, 
        player_i::TankIDPass, 
        player_i::RawName,
        player_i::DisplayName, 
        player_i::Email, 
        player_i::Discord, 
        player_i::RelativeIdentifier,
        player_i::MachineAddress,
        player_i::Role,
        player_i::Inventory,
        player_i::Clothes,
        player_i::LastActive,
        player_i::Gems,
        player_i::Playmods,
        player_i::CharacterState,
        player_i::Country> {
        struct _alias_t {
            static constexpr const char _literal[] = "players";
            using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T>
            struct _member_t {
                T player_db;
                T& operator()() {
                    return player_db;
                }
                const T& operator()() const {
                    return player_db;
                }
            };
        };
    };
}