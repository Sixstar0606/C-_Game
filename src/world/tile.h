#pragma once
#include <string>
#include <vector>
#include <database/item/item_database.h>
#include <utils/binary_writer.h>
#include <utils/binary_reader.h>
#include <utils/timing_clock.h>
#include <proton/utils/common.h>
#include <world/objects/enums.h>
#include <world/tile_extra.h>
#include <player/player.h>

namespace GTServer {
    class Tile : public TileExtra {
    public:
        Tile();
        Tile(const uint16_t& fg, const uint16_t& bg, const uint16_t& lp, const uint16_t& fl);
        ~Tile() = default;

        void SetPosition(const CL_Vec2i& pos);
        [[nodiscard]] CL_Vec2i GetPosition() const { return m_position; }
        bool SetForeground(uint16_t fg);
        [[nodiscard]] uint16_t GetForeground() const { return m_foreground; }
        void SetBackground(const uint16_t& bg);
        [[nodiscard]] uint16_t GetBackground() const { return m_background; }
        void SetParent(const uint16_t& parent);
        [[nodiscard]] uint16_t GetParent() const { return m_parent; }
        
        ItemInfo* GetBaseItem();
        void RemoveBase();
        
        bool IsFlagOn(const eTileFlags& flag) const;
        void SetFlag(const eTileFlags& flag);
        void SetFlags(const uint16_t& flags) { m_flags = flags; }
        void RemoveFlag(const eTileFlags& flag);
        uint32_t DevPunchAdd(const std::shared_ptr<Player>& player);
        void DevPunchRemove(const std::shared_ptr<Player>& player);
        bool HasDevPunch(const std::shared_ptr<Player>& player);

        [[nodiscard]] TimingClock& GetLastHitten() { return m_last_hitten; }
        [[nodiscard]] TimingClock& GetPunchDelay() { return m_punch_delay; }
        
        void SetHitCount(const uint8_t& count) { m_hit_count = count; }
        [[nodiscard]] uint8_t GetHitCount() { return m_hit_count; }
        void ResetHits();
        uint8_t IndicateHit();

        void RemoveLock();
        void ApplyLockOwner(const uint32_t& uid);

    public:
        std::size_t GetMemoryUsage(const bool& to_database);
        void Pack(BinaryWriter& buffer, const bool& to_database);
        void Serialize(BinaryReader& br);

    private:
        CL_Vec2i m_position;
        uint16_t m_foreground;
        uint16_t m_background;
        uint16_t m_parent;
        uint16_t m_flags;

        uint8_t m_hit_count;
        TimingClock m_last_hitten;
        TimingClock m_punch_delay{ std::chrono::seconds(2) };

        uint32_t m_net_id;
        std::unordered_map<uint32_t, std::shared_ptr<Player>> m_DevBreak;
    
    public: // Path Finding
        int32_t m_path_parent;
        bool m_visited;

        float m_local;
        float m_global;
    };
}