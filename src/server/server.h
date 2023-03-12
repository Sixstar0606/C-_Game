#pragma once
#include <deque>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <enet/enet.h>
#include <server/objects/queues.h>

namespace GTServer {
    class PlayerPool;
    class WorldPool;
    class Server {
    public:
        Server(const uint8_t& instanceId, const std::string& address, const uint16_t& port, const size_t& max_peers);
        ~Server();
        
        bool Start();
        bool Stop();
    
        [[nodiscard]] uint8_t GetInstanceId() const { return m_instance_id; }
        [[nodiscard]] std::string GetAddress() const { return m_address; }
        [[nodiscard]] uint16_t GetPort() const { return m_port; }
        [[nodiscard]] ENetHost* GetHost() const { return m_host; }

        std::shared_ptr<PlayerPool> GetPlayerPool() { return m_player_pool; }
        std::shared_ptr<WorldPool> GetWorldPool() { return m_world_pool; }

        void AddQueue(const eQueueType& queue_type, ServerQueue data) {
            data.m_queue_type = queue_type;
            m_queue_worker.push_back(data);
        }
    public:
        std::deque<ServerQueue> m_queue_worker{};

    private:
        uint8_t m_instance_id;
        std::string m_address{ "0.0.0.0" };
        uint16_t m_port;

        ENetHost* m_host;
        size_t m_max_peers;

        std::shared_ptr<PlayerPool> m_player_pool;
        std::shared_ptr<WorldPool> m_world_pool;
    };
}