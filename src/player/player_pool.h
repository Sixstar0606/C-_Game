#pragma once
#include <memory>
#include <unordered_map>
#include <enet/enet.h>
#include <player/player.h>

namespace GTServer {
    class PlayerPool {
    public:
        PlayerPool() = default;
        ~PlayerPool() = default;

        std::shared_ptr<Player> NewPlayer(ENetPeer* peer) {
            m_players[peer->connectID] = std::make_shared<Player>(peer);
            return m_players[peer->connectID];
        }
        void RemovePlayer(uint32_t connect_id) {
            m_players[connect_id].reset();
            m_players.erase(connect_id);
        }
        bool HasPlayer(const uint32_t& user_id) const { 
            for (auto& [connect_id, player] : m_players) {
                if (player->GetUserId() != user_id)
                    continue;
                return true;
            }
            return false;  
        }
        bool HasPlayer(const std::string& name) const {
            for (auto& [connect_id, player] : m_players) {
                std::string person_name = player->GetLoginDetail()->m_tank_id_name;
                std::transform(person_name.begin(), person_name.end(), person_name.begin(), ::tolower);
                if (person_name != name)
                    continue;
                return true;
            }
            return false;
        }

        std::shared_ptr<Player> GetPlayer(const uint32_t& cid) {
            for (auto& [connect_id, player] : m_players) {
                if (connect_id == cid)
                    return player;
            }
            return nullptr;
        }
        std::shared_ptr<Player> GetPlayerByUserId(const uint32_t& user_id) {
            for (auto& [connect_id, player] : m_players) {
                if (player->GetUserId() != user_id)
                    continue;
                return player;
            }
            return nullptr;
        }
        std::shared_ptr<Player> GetPlayerByName(const std::string& name) {
            for (auto& [connect_id, player] : m_players) {
                std::string person_name = player->GetLoginDetail()->m_tank_id_name;
                std::transform(person_name.begin(), person_name.end(), person_name.begin(), ::tolower);
                if (person_name != name)
                    continue;
                return this->GetPlayerByUserId(player->GetUserId());
            }
            return nullptr;
        }
    public:
        [[nodiscard]] std::unordered_map<uint32_t, std::shared_ptr<Player>> const GetPlayers() { return m_players; }
        [[nodiscard]] std::size_t GetPlayerCount() const { return m_players.size(); }

    private:
        std::unordered_map<uint32_t, std::shared_ptr<Player>> m_players{};
    };
}