#pragma once
#include <string>
#include <sqlpp11/table.h>
#include <sqlpp11/char_sequence.h>
#include <sqlpp11/column_types.h>

namespace GTServer {
    namespace world_i {
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
        struct Name {
            struct _alias_t {
                static constexpr const char _literal[] = "name";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T name;
                    T& operator()() {
                        return name;
                    }

                    const T& operator()() const {
                        return name;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::varchar>;
        };
        struct Flags {
            struct _alias_t {
                static constexpr const char _literal[] = "flags";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T flags;
                    T& operator()() {
                        return flags;
                    }

                    const T& operator()() const {
                        return flags;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integer>;
        };
        struct Width {
            struct _alias_t {
                static constexpr const char _literal[] = "width";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T width;
                    T& operator()() {
                        return width;
                    }

                    const T& operator()() const {
                        return width;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integer>;
        };
        struct Height {
            struct _alias_t {
                static constexpr const char _literal[] = "height";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T height;
                    T& operator()() {
                        return height;
                    }

                    const T& operator()() const {
                        return height;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integer>;
        };
        struct CreatedAt {
            struct _alias_t {
                static constexpr const char _literal[] = "created_at";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T created_at;
                    T& operator()() {
                        return created_at;
                    }

                    const T& operator()() const {
                        return created_at;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::time_point>;
        };    
        struct UpdatedAt {
            struct _alias_t {
                static constexpr const char _literal[] = "updated_at";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T updated_at;
                    T& operator()() {
                        return updated_at;
                    }

                    const T& operator()() const {
                        return updated_at;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::time_point>;
        };
        struct Objects {
            struct _alias_t {
                static constexpr const char _literal[] = "objects";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T objects;
                    T& operator()() {
                        return objects;
                    }

                    const T& operator()() const {
                        return objects;
                    }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::blob>;
        };
        struct OwnerID {
            struct _alias_t {
                static constexpr const char _literal[] = "owner_id";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T owner_id;
                    T& operator()() { return owner_id; }
                    const T& operator()() const { return owner_id; }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integral>;
        };
        struct MainLock {
            struct _alias_t {
                static constexpr const char _literal[] = "main_lock";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T main_lock;
                    T& operator()() { return main_lock; }
                    const T& operator()() const { return main_lock; }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integral>;
        };
        struct WeatherID {
            struct _alias_t {
                static constexpr const char _literal[] = "weather_id";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T weather_id;
                    T& operator()() { return weather_id; }
                    const T& operator()() const { return weather_id; }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integral>;
        };
        struct BaseWeatherID {
            struct _alias_t {
                static constexpr const char _literal[] = "base_weather_id";
                using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
                template <typename T>
                struct _member_t {
                    T base_weather_id;
                    T& operator()() { return base_weather_id; }
                    const T& operator()() const { return base_weather_id; }
                };
            };

            using _traits = ::sqlpp::make_traits<::sqlpp::integral>;
        };
    }
        
    struct WorldDB 
    : sqlpp::table_t<WorldDB, 
        world_i::ID, 
        world_i::Name, 
        world_i::Flags,
        world_i::Width, 
        world_i::Height, 
        world_i::CreatedAt,
        world_i::UpdatedAt,
        world_i::Objects,
        world_i::OwnerID,
        world_i::MainLock,
        world_i::WeatherID,
        world_i::BaseWeatherID> {
        struct _alias_t {
            static constexpr const char _literal[] = "worlds";
            using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template <typename T>
            struct _member_t {
                T world_db;
                T& operator()() {
                    return world_db;
                }
                const T& operator()() const {
                    return world_db;
                }
            };
        };
    };
}