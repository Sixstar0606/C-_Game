#pragma once
#include <type_traits>
#include <variant>
#include <fmt/core.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <player/player.h>
#include <utils/timing_clock.h>
#include <database/interface/player_i.h>

namespace GTServer {
    class PlayerTable {
    public:
        enum class RegistrationResult {
            SUCCESS,
            EXIST_GROWID,
            INVALID_GROWID,
            INVALID_PASSWORD, //
            INVALID_EMAIL_OR_DISCORD,
            INVALID_GROWID_LENGTH,
            INVALID_PASSWORD_LENGTH,
            MISMATCH_VERIFY_PASSWORD,
            BAD_CONNECTION
        };

    public:
        PlayerTable(sqlpp::mysql::connection* con) : m_connection(con) { }
        ~PlayerTable() = default;

        bool IsAccountExist(const std::string& name) const;
        std::string GetName(const int32_t& uid) const;
        std::unordered_map<uint32_t, std::string> GetPlayersMatchingName(const std::string& name);
        
        std::string GetRowVarchar(const uint32_t& uid, const uint8_t& index) const {
            PlayerDB player_db{};
            for (auto &row : (*m_connection)(select(all_of(player_db)).from(player_db).where(player_db.id == uid).limit(1u))) {
                if (!row._is_valid)
                    return std::string{};
                switch (index) {
                case 1: return row.requested_name.value();
                case 2: return row.tank_id_name.value();
                case 3: return row.tank_id_pass.value();
                case 4: return row.raw_name.value();
                case 5: return row.display_name.value();
                default: return std::string{};
                }
            }
            return std::string{};
        }
        system_clock::time_point GetRowTimestamp(const uint32_t& uid, const uint8_t& index) const {
            PlayerDB player_db{};
            for (auto &row : (*m_connection)(select(all_of(player_db)).from(player_db).where(player_db.id == uid).limit(1u))) {
                if (!row._is_valid)
                    return system_clock::now();
                switch (index) {
                case 1: return row.last_active.value();
                default: return system_clock::now();
                }
            }
            return system_clock::now();
        }
        
        uint32_t Insert(std::shared_ptr<Player> player);
        bool Save(std::shared_ptr<Player> player);
        bool Load(std::shared_ptr<Player> player);

        bool SerializeByName(std::shared_ptr<Player>& player, const std::string& name);
        bool SerializeByUserID(std::shared_ptr<Player>& player, const uint32_t& user_id);
        
        std::pair<RegistrationResult, std::string> RegisterPlayer(
            const std::string& name, 
            const std::string& password, 
            const std::string& verify_password
        );
        
    private:
        sqlpp::mysql::connection* m_connection;
    };
}