#pragma once
#include <player/player.h>
#include <world/world.h>

#define SMALL_LOCK_SIZE     10
#define BIG_LOCK_SIZE       48
#define HUGE_LOCK_SIZE      200

#define STEAM_POWER_LENGTH  49

namespace GTServer {
    enum eSteamDirection {
        STEAM_DIRECTION_NONE,
        STEAM_DIRECTION_LEFT,
        STEAM_DIRECTION_RIGHT,
        STEAM_DIRECTION_UP,
        STEAM_DIRECTION_DOWN
    };
    enum eSteamEffect : uint8_t {
        STEAM_EFFECT_NONE,
        STEAM_EFFECT_UNK1,
        STEAM_EFFECT_UNK2,
        STEAM_EFFECT_ACTIVATE_VENT,
        STEAM_EFFECT_OPEN_DOOR,
        STEAM_EFFECT_CLOSE_DOOR,
        STEAM_EFFECT_OPEN_LAUNCHER = 14,
        STEAM_EFFECT_CLOSE_LAUNCHER = 15,
        STEAM_EFFECT_ENGINE_SPURT = 19,
        STEAM_EFFECT_SSU_EFF = 20,
        STEAM_EFFECT_SSU_EXPLODE = 22,
        STEAM_EFFECT_ACTIVATE_LAMP = 23,
        STEAM_EFFECT_OPEN_SPIKE = 24,
        STEAM_EFFECT_CLOSE_SPIKE = 25
    };

    class Algorithm {
    public:
        static bool IsLockNeighbour(std::shared_ptr<World> world, CL_Vec2i tile, CL_Vec2i lock, bool ignoreAir);

    public:
        static void OnLockApply(std::shared_ptr<Player> player, std::shared_ptr<World> world, GameUpdatePacket* update_packet);
        static void OnLockReApply(std::shared_ptr<Player> player, std::shared_ptr<World> world, GameUpdatePacket* update_packet);
        static void OnSteamPulse(std::shared_ptr<Player> player, std::shared_ptr<World> world, GameUpdatePacket* update_packet, eSteamDirection direction = STEAM_DIRECTION_DOWN);
        
        static bool OnFindPath(std::shared_ptr<Player> player, std::shared_ptr<World> world, const CL_Vec2i& current_pos, const CL_Vec2i& future_pos);
    public:
        static bool IsSteamPowered(const uint16_t& foreground);
        static void OnSteamActive(std::shared_ptr<World> world, Tile* tile, int x, int y, int delay);
    };
}