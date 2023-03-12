#pragma once
#include <world/world.h>
#include <world/world_pool.h>

namespace GTServer::events {
    void OnSetIconState(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_IS_IN))
            return;
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        GameUpdatePacket* packet{ ctx.m_update_packet };
        packet->m_net_id = ctx.m_player->GetNetId();

        world->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket));
        });
    }
}