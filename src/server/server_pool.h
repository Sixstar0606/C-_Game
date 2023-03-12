#pragma once
#include <atomic>
#include <deque>
#include <string>
#include <vector>
#include <unordered_map>
#include <magic_enum.hpp>
#include <server/objects/queues.h>
#include <server/server.h>
#include <player/player_pool.h>
#include <world/world_pool.h>
#include <discord/discord_bot.h>
#include <utils/timing_clock.h>
#include <proton/utils/dialog_builder.h>
#include <proton/packet.h>

namespace GTServer {
    class EventPool;
    class ServerPool : PacketDecoder {
    public:
        explicit ServerPool(std::shared_ptr<EventPool> events);
        ~ServerPool();

        bool InitializeENet();
        void DeInitializeENet();
        
        std::shared_ptr<Server> StartInstance();
        void StopInstance(std::shared_ptr<Server> server);

        void StartService();
        void StopService();

        void ServicePoll();
        
    public:
        void SetUserID(const int& uid) { user_id = uid; }
        [[nodiscard]] int GetUserID(bool increase = true) { return increase ? ++user_id : user_id; }

        bool HasPlayer(const uint32_t& user_id) const;
        std::shared_ptr<Player> GetPlayerByUserID(const uint32_t& user_id);
        std::shared_ptr<Player> GetPlayerByName(const std::string& name);
        std::shared_ptr<Player> GetPlayerByFormat(const std::string& data);

    public:
        [[nodiscard]] bool IsRunning() const { return m_running.load(); }
        [[nodiscard]] std::vector<std::shared_ptr<Server>> GetServers() { return m_servers; }
        [[nodiscard]] std::vector<std::shared_ptr<Player>> GetPlayers() {
            std::vector<std::shared_ptr<Player>> ret{};
            for (auto& server : this->GetServers()) {
                for (auto& [connect_id, player] : server->GetPlayerPool()->GetPlayers())
                    ret.push_back(player);
            }
            return ret;
        }
        [[nodiscard]] std::size_t GetActiveServers() { return this->GetServers().size(); }
        [[nodiscard]] std::size_t GetActiveWorlds() {
            std::size_t ret{};
            for (auto& server : this->GetServers())
                ret += server->GetWorldPool()->GetWorlds().size();
            return ret;
        }
        [[nodiscard]] std::size_t GetActivePlayers() {
            std::size_t ret{};
            for (auto& server : this->m_servers) 
                ret += server->GetPlayerPool()->GetPlayers().size();
            return ret;
        }
        std::shared_ptr<EventPool> GetEvents() const { return m_events; }
        
        void AddQueue(const eQueueType& queue_type, ServerQueue data) {
            data.m_queue_type = queue_type;
            m_queue_worker.push_back(data);
        }
        
    public:
        std::shared_ptr<World> GetWorld(const std::string& name) {
            for (auto& server : this->GetServers()) {
                for (auto& [world_name, world] : server->GetWorldPool()->GetWorlds()) {
                    if (world_name != name)
                        continue;
                    return world;
                }
            }
            return nullptr;
        }

    private:
        std::string m_address{ "0.0.0.0" };
        uint16_t m_port{ 17091 };
        size_t m_max_peers{ 0xFF };
        int user_id{ 0 };

        std::atomic<bool> m_running{ false };
        std::vector<std::thread> m_threads{};
        
    private:
        std::vector<std::shared_ptr<Server>> m_servers{};
        std::shared_ptr<EventPool> m_events;

        std::deque<ServerQueue> m_queue_worker{};

    public:
        std::unordered_map<dpp::snowflake, std::pair<uint32_t, TimingClock>> m_account_verify{};
    };
}