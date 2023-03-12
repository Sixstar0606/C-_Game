#include <database/table/player_table.h>
#include <fmt/chrono.h>
#include <utils/text.h>

namespace GTServer {
    bool PlayerTable::IsAccountExist(const std::string& name) const {
        PlayerDB player_db{};
        for (const auto &row : (*m_connection)(select(all_of(player_db)).from(player_db).where(
            player_db.raw_name == name and player_db.tank_id_name != std::string{}
        ))) {
            if (row._is_valid)
                return true;
        }
        return false;
    }
    std::string PlayerTable::GetName(const int32_t& uid) const {
        PlayerDB player_db{};
        for (const auto &row : (*m_connection)(select(all_of(player_db)).from(player_db).where(player_db.id == uid).limit(1u))) {
            if (row._is_valid)
                return row.raw_name.value();
        }
        return {};
    }
    std::unordered_map<uint32_t, std::string> PlayerTable::GetPlayersMatchingName(const std::string& name) {
        std::unordered_map<uint32_t, std::string> ret{};
        PlayerDB player_db{};
        for (const auto &row : (*m_connection)(select(all_of(player_db)).from(player_db).where(player_db.raw_name.like(
            fmt::format("%{}%", name)
        )).limit(20u))) {
            if (row._is_valid)
                ret.insert_or_assign(static_cast<uint32_t>(row.id), row.raw_name.value());
        }
        return ret;
    }

    uint32_t PlayerTable::Insert(std::shared_ptr<Player> player) {
        std::shared_ptr<LoginInformation> login{ player->m_login_info };

        if (this->IsAccountExist(login->m_tank_id_name))
            return 0;
        PlayerDB player_db{};
        auto id = (*m_connection)(insert_into(player_db).set(
            player_db.requested_name = player->GetRawName(),
            player_db.tank_id_name = login->m_tank_id_name,
            player_db.tank_id_pass = login->m_tank_id_pass,
            player_db.raw_name = player->GetRawName(),
            player_db.display_name = player->GetDisplayName(nullptr),
            player_db.relative_identifier = login->m_rid,
            player_db.machine_address = login->m_mac,
            player_db.email = login->m_email,
            player_db.role = player->GetRole(),
            player_db.inventory = player->Pack(PLAYER_DATA_INVENTORY),
            player_db.clothes = player->Pack(PLAYER_DATA_CLOTHES),
            player_db.gems = player->GetGems(),
            player_db.playmods = player->Pack(PLAYER_DATA_PLAYMODS),
            player_db.character_state = player->Pack(PLAYER_DATA_CHARACTER_STATE),
            player_db.country = player->GetLoginDetail()->m_country
        ));
        return id;
    }
    bool PlayerTable::Save(std::shared_ptr<Player> player) {
        try {
            PlayerDB player_db{};
            (*m_connection)(update(player_db).set(
                player_db.tank_id_name = player->GetLoginDetail()->m_tank_id_name,
                player_db.tank_id_pass = player->GetLoginDetail()->m_tank_id_pass,
                player_db.raw_name = player->GetRawName(),
                player_db.display_name = player->GetDisplayName(nullptr),
                player_db.discord = player->GetDiscord(),
                player_db.role = player->GetRole(),
                player_db.inventory = player->Pack(PLAYER_DATA_INVENTORY),
                player_db.clothes = player->Pack(PLAYER_DATA_CLOTHES),
                player_db.last_active = player->get_last_active(),
                player_db.gems = player->GetGems(),
                player_db.playmods = player->Pack(PLAYER_DATA_PLAYMODS),
                player_db.character_state = player->Pack(PLAYER_DATA_CHARACTER_STATE),
                player_db.country = player->GetLoginDetail()->m_country
            ).where(player_db.id == player->GetUserId()));
            return true;
        }
        catch(const std::exception &e) {
            fmt::print("exception from PlayerTable::save -> {}\n", e.what());
            return false;
        }
        return false;
    } 
    bool PlayerTable::Load(std::shared_ptr<Player> player) {
        try {
            PlayerDB player_db{};
            for (const auto &row : (*m_connection)(select(all_of(player_db)).from(player_db).where(
                player_db.tank_id_name == player->GetLoginDetail()->m_tank_id_name &&
                player_db.tank_id_pass == player->GetLoginDetail()->m_tank_id_pass
            ).limit(1u))) {
                if (!row._is_valid)
                    continue;
                player->SetUserId(static_cast<uint32_t>(row.id));
                player->SetRawName(row.raw_name);
                player->SetDisplayName(row.raw_name);
                player->SetEmail(row.email);
                player->SetDiscord(row.discord);
                player->SetRole(row.role);
                player->Serialize(PLAYER_DATA_INVENTORY, row.inventory.value());
                player->Serialize(PLAYER_DATA_CLOTHES, row.clothes.value());

                player->set_last_active(row.last_active.value());
                player->SetGems(row.gems.value());

                player->Serialize(PLAYER_DATA_PLAYMODS, row.playmods.value());
                player->Serialize(PLAYER_DATA_CHARACTER_STATE, row.character_state.value());
                return true;
            }
            return false;
        }
        catch(const std::exception &e) {
            return false;
        }
        return false;
    }
    bool PlayerTable::SerializeByName(std::shared_ptr<Player>& player, const std::string& name) {
        PlayerDB player_db{};
        for (const auto& row : (*m_connection)(select(all_of(player_db)).from(player_db).where(player_db.raw_name.like(
            fmt::format("%{}%", name)
        )).limit(1u))) {
            if (!row._is_valid)
                continue;
            return this->SerializeByUserID(player, static_cast<uint32_t>(row.id));
        }
        return false;
    }
    bool PlayerTable::SerializeByUserID(std::shared_ptr<Player>& player, const uint32_t& user_id) {
        PlayerDB player_db{};
        for (const auto &row : (*m_connection)(select(all_of(player_db)).from(player_db).where(player_db.id == user_id).limit(1u))) {
            if (row._is_valid) {
                player->GetLoginDetail()->m_tank_id_name = row.tank_id_name;
                player->GetLoginDetail()->m_tank_id_pass = row.tank_id_pass;
                return this->Load(player);
            }
        }
        return false;
    }

    std::pair<PlayerTable::RegistrationResult, std::string> PlayerTable::RegisterPlayer(
        const std::string& name, 
        const std::string& password, 
        const std::string& verify_password) 
    {
        if (password.length() < 8 || password.length() > 24)
            return { 
                RegistrationResult::INVALID_PASSWORD_LENGTH,
                "`4Oops!``  Your password must be between `$8`` and `$24`` characters long."
            };
        if (verify_password != password)
            return {
                RegistrationResult::MISMATCH_VERIFY_PASSWORD,
                "`4Oops!``  Passwords don't match.  Try again."
            };
        std::string lower_case_name = name;
        if (!utils::to_lowercase(lower_case_name))
            return {
                RegistrationResult::INVALID_GROWID,
                "`4Oops!``  the name is includes invalid characters."
            };
        if (lower_case_name.length() < 3 || lower_case_name.length() > 18)
            return {
                RegistrationResult::INVALID_GROWID_LENGTH,
                "`4Oops!``  Your `wGrowID`` must be between `$3`` and `$18`` characters long."
            };
        if (this->IsAccountExist(lower_case_name))
            return {
                RegistrationResult::EXIST_GROWID,
                fmt::format("`4Oops!``  The name `w{}`` is so cool someone else has already taken it.  Please choose a different name.", name)
            };
        if (!m_connection->is_valid()) {
            m_connection->reconnect();
            return {
                RegistrationResult::BAD_CONNECTION,
                "`4Oops!``  Server's database had bad connection, please try again."
            };
        }
        return { RegistrationResult::SUCCESS, "" };
    }
}