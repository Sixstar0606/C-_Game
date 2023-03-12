#pragma once
#include <string>
#include <vector>
#include <functional>
#include <player/player.h>
#include <world/tile.h>
#include <world/world_object.h>
#include <utils/timing_clock.h>

namespace GTServer {
    class World {
    public:
        explicit World(const std::string& name, const uint32_t& width = 100, const uint32_t& height = 60);
        ~World();

        uint32_t AddPlayer(const std::shared_ptr<Player>& player);
        void RemovePlayer(const std::shared_ptr<Player>& player);
        uint32_t DevPunchAdd(const std::shared_ptr<Player>& player);
        void DevPunchRemove(const std::shared_ptr<Player>& player);
        bool HasDevPunch(const std::shared_ptr<Player>& player);
        bool HasPlayer(const std::shared_ptr<Player>& player);
        bool BanPlayer(const int32_t& player_id);
        bool HasBan(const std::shared_ptr<Player>& player);
        bool ClearBans();
        std::vector<std::shared_ptr<Player>> GetPlayers(const bool& invis);
        void Broadcast(const std::function<void(const std::shared_ptr<Player>&)>& func);

        bool IsFlagOn(const eWorldFlags& flag) const;
        void SetFlag(const eWorldFlags& flag);
        void SpawnEvent(const std::string& eventname);
        void SetFlags(const uint32_t& flags) { m_flags = flags; }
        void RemoveFlag(const eWorldFlags& flag);
        [[nodiscard]] uint32_t GetFlags() const { return m_flags; }
        
        [[nodiscard]] int32_t GetID() const { return m_id; }
        void SetID(const int32_t& id) { m_id = id; }
        [[nodiscard]] uint16_t GetVersion() const { return m_version; }
        void SetVersion(const uint16_t& version) { m_version = version; }
        [[nodiscard]] std::string GetName() const { return m_name; }
        void SetName(const std::string& name) { m_name = name; }
        [[nodiscard]] CL_Vec2i GetSize() const { return CL_Vec2i{ m_width, m_height }; }
        void SetSize(const uint32_t& width, const uint32_t& height) { m_width = width; m_height = height; }
        [[nodiscard]] time_point GetCreatedAt() const { return m_created_at; }
        void SetCreatedAt(const time_point& time) { m_created_at = time; }
        [[nodiscard]] time_point GetUpdatedAt() const { return m_updated_at; }
        void SetUpdatedAt(const time_point& time) { m_updated_at = time; }

        void Generate(const eWorldType& type);

        Tile* GetTile(uint16_t x, uint16_t y);
        Tile* GetTile(CL_Vec2i vec2i) { return GetTile(vec2i.m_x, vec2i.m_y); }
        Tile* GetTile(std::size_t index) { return &m_tiles[index]; }
        std::vector<Tile>& GetTiles() { return m_tiles; }
        CL_Vec2i GetTilePos(const uint16_t& id) const;
        CL_Vec2i GetTilePos(const eItemTypes& type) const;

        bool IsOwned() const { return m_owner_id != -1; }
        bool IsOwner(std::shared_ptr<Player> player) const { return this->GetOwnerId() == player->GetUserId(); }
        bool IsObstacle(std::shared_ptr<Player> player, CL_Vec2i position);

        std::unordered_map<int32_t, WorldObject>& GetObjects() { return m_objects; }

        [[nodiscard]] uint32_t GetObjectId() const { return m_object_id; }
        void SetObjectId(const uint32_t& object_id) { m_object_id = object_id; }
        [[nodiscard]] int32_t GetOwnerId() const { return m_owner_id; }
        void SetOwnerId(const int32_t& owner_id) { m_owner_id = owner_id; }
        [[nodiscard]] int32_t GetMainLock() const { return m_main_lock; }
        void SetMainLock(const int32_t& main_lock) { m_main_lock = main_lock; }
        [[nodiscard]] uint32_t GetWeatherId() const { return m_weather_id; }
        void SetWeatherId(const uint32_t& weather_id) { m_weather_id = weather_id; }
        [[nodiscard]] uint32_t GetBaseWeatherId() const { return m_base_weather_id; }
        void SetBaseWeatherId(const uint32_t& weather_id) { m_base_weather_id = weather_id; }

        std::size_t GetMemoryUsage();
        std::size_t GetTilesMemoryUsage(const bool& to_database);
        std::size_t GetObjectsMemoryUsage();
        std::vector<uint8_t> Pack();
        std::vector<uint8_t> PackTiles(const bool& to_database);
        std::vector<uint8_t> PackObjects(const bool& to_database);

        void SyncPlayerData(std::shared_ptr<Player> player);
        void SendTileUpdate(Tile* tile, const int32_t& delay = 0);
        void SendTileUpdate(std::vector<Tile*> tiles);
        
        void SendWho(std::shared_ptr<Player> player, bool show_self);
        void SendPull(std::shared_ptr<Player> player, std::shared_ptr<Player> target);
        void SendKick(std::shared_ptr<Player> player, bool killed, int delay = -1);

        bool EditTile(std::string action, CL_Vec2i center, float radius, ItemInfo* item, bool ignore_areas = false);
    public:
        Tile* GetParentTile(Tile* neighbour);
        bool HasTileAccess(Tile* neighbour, const std::shared_ptr<Player>& player);
        bool IsTileOwner(Tile* neighbour, const std::shared_ptr<Player>& player);
        bool IsTileOwned(Tile* neighbour);
    
    public:
        std::unordered_map<int32_t, WorldObject> GetObjectsOnPos(const CL_Vec2f& pos);
        std::unordered_map<int32_t, WorldObject> GetLastObjectById(std::unordered_map<int32_t, WorldObject> &obj_list, uint32_t item_id);
        void AddObject(uint32_t item_id, uint8_t amount, CL_Vec2f position);
        void AddObject(WorldObject& object, bool check, bool randomize_pos = true, bool magplant = false);
        void AddGemsObject(int32_t gems, const CL_Vec2f& position);
        void ModifyObject(const std::pair<int32_t, WorldObject>& object);
        void CollectObject(std::shared_ptr<Player> player, const int32_t& obj_id, const CL_Vec2f& position);
        void RemoveObject(const int32_t& id);

    private:
        int32_t m_id{ -1 };
        uint16_t m_version{ 0x14 };
        uint32_t m_flags;

        std::string m_name;
        uint32_t m_width;
        uint32_t m_height;

        time_point m_created_at{ system_clock::now() };
        time_point m_updated_at{ system_clock::now() };

        std::vector<Tile> m_tiles;
        
        int32_t m_owner_id{ -1 };
        int32_t m_main_lock{ -1 };

        uint32_t m_object_id{ 1 };
        uint32_t m_weather_id{ WORLD_WEATHER_SUNNY };
        uint32_t m_base_weather_id{ WORLD_WEATHER_SUNNY };
        uint32_t m_net_id;
        std::unordered_map<uint32_t, std::shared_ptr<Player>> m_players;
        std::unordered_map<uint32_t, std::shared_ptr<Player>> m_DevBreak;
        std::unordered_map<int32_t, WorldObject> m_objects;
        std::unordered_map<int32_t, time_point> m_banned_players;
    };
}