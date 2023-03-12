#pragma once
#include <world/world.h>
#include <world/world_pool.h>
#include <algorithm/algorithm.h>

namespace GTServer::events {
    void OnItemActiveObjectRequest(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_IS_IN))
            return;
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        auto it = world->GetObjects().find(ctx.m_update_packet->m_object_id);
        if (it == world->GetObjects().end())
            return;
        CL_Vec2f position = CL_Vec2f{ ctx.m_update_packet->m_pos_x, ctx.m_update_packet->m_pos_y };

        float x = (it->second.m_pos.m_x) / 32;
        float y = (it->second.m_pos.m_y) / 32;
        if (x - static_cast<int>(x) >= 0.75f)
            x = static_cast<float>(std::round(x));
        if (y - static_cast<int>(y) >= 0.75f)
            y = static_cast<float>(std::round(y));
        
        CL_Vec2i player_pos = { (ctx.m_player->GetPosition().m_x + 10) / 32, (ctx.m_player->GetPosition().m_y + 15) / 32 };
        CL_Vec2i current_pos = { player_pos.m_x, player_pos.m_y};
        CL_Vec2i future_pos = { static_cast<int>(x), static_cast<int>(y) };
        int x_diff = std::abs(current_pos.m_x - future_pos.m_x);
        int y_diff = std::abs(current_pos.m_y - future_pos.m_y);

        if (x_diff >= 25 || y_diff >= 25) {
            ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "`w(Too Far Away)``", true);
            return;
        }
        if (!Algorithm::OnFindPath(ctx.m_player, world, current_pos, future_pos)) {
            ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "`w(Too Far Away)``", true);
            return;
        }
        world->CollectObject(ctx.m_player, ctx.m_update_packet->m_object_id, position);
    }
}