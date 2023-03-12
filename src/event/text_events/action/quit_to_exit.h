#pragma once
#include <world/world.h>
#include <player/player_pool.h>

namespace GTServer::events {
    void quit_to_exit(EventContext& ctx) {
        std::shared_ptr<Player> player{ ctx.m_player };
        if (!player->IsFlagOn(PLAYERFLAG_IS_IN)) {
            player->Disconnect(0U);
            return;
        }
        if (player->GetWorld().empty() || player->GetWorld() == std::string{ "EXIT" })
            return;

        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(player->GetWorld()) };
        if (!world)
            return;
        if (!world->HasPlayer(player))
            return;
        world_pool->OnPlayerLeave(world, player, true);
        player->SendLog("`wWhere would you like to go?");
    }
}