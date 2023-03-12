#pragma once
#include <unordered_map>
#include <player/player.h>
#include <world/world.h>

namespace GTServer {
    class ServerPool;
    class WorldPool {
    public:
        struct RandomWorld {
            std::size_t m_players;
            std::string m_name;

            RandomWorld(const std::size_t& players, const std::string& name) : m_players{ players }, m_name{ name } {}
            bool operator>(const RandomWorld& data) const { return m_players > data.m_players; }
        };

    public:
        WorldPool() = default;
        ~WorldPool();

        std::unordered_map<std::string, std::shared_ptr<World>> GetWorlds() const { return m_worlds; }
        std::vector<RandomWorld> GetRandomWorlds(const bool& required_players, const bool& jammed);
        void SendDefaultOffers(std::shared_ptr<Player> invoker);
        void SendCategorySelection();

        std::shared_ptr<World> NewWorld(const std::string& name);
        void RemoveWorld(const std::string& name);
        std::shared_ptr<World> GetWorld(const std::string& name);

        void OnPlayerJoin(ServerPool* pool, std::shared_ptr<World> world, std::shared_ptr<Player> player, const CL_Vec2i& pos);
        void OnPlayerLeave(std::shared_ptr<World> world, std::shared_ptr<Player> player, const bool& send_offers);
        void OnPlayerSyncing(std::shared_ptr<World> world, std::shared_ptr<Player> player);
        
    private:
        std::unordered_map<std::string, std::shared_ptr<World>> m_worlds{};
    };
}