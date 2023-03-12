#pragma once
#include <world/world.h>
#include <world/world_pool.h>

namespace GTServer::events {
    void respawn_spike(EventContext& ctx) {
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        world->SendKick(ctx.m_player, true);
    }
    void respawn_lava(EventContext& ctx) {
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        world->SendKick(ctx.m_player, true);
    }
}