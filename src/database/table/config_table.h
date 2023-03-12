#pragma once
#include <type_traits>
#include <variant>
#include <fmt/core.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <database/interface/config_i.h>

namespace GTServer {
    class ConfigTable {
    public:
        ConfigTable(sqlpp::mysql::connection* con) : m_connection(con) { }
        ~ConfigTable() = default;

        std::string get_row_varchar(const uint8_t& index) const {
            ConfigDB config_db{};
            for (auto &row : (*m_connection)(select(all_of(config_db)).from(config_db).unconditionally())) {
                if (!row._is_valid)
                    return std::string{};
                switch (index) {
                case 1: return row.vanguard_token.value();
                default: return std::string{};
                }
            }
            return std::string{};
        }
        int64_t get_row_integer(const uint8_t& index) const {
            ConfigDB config_db{};
            for (auto &row : (*m_connection)(select(all_of(config_db)).from(config_db).unconditionally())) {
                if (!row._is_valid)
                    return 0;
                switch (index) {
                case 1: return row.discord_server.value();
                default: return 0;
                }
            }
            return 0;
        }
        
    private:
        sqlpp::mysql::connection* m_connection;
    };
}