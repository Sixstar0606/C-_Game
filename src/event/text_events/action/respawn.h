#pragma once
#include <world/world.h>
#include <world/world_pool.h>

namespace GTServer::events {
    void respawn(EventContext& ctx) {
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        if (ctx.m_player->m_respawn_time.GetPassedTime() < ctx.m_player->m_respawn_time.GetTimeout()) {
            ctx.m_player->SendLog("I can't respawn right now!");
            return;
        }
        ctx.m_player->m_respawn_time.UpdateTime();
        world->SendKick(ctx.m_player, false, 2000);
    }
}