#pragma once
#include <world/world.h>
#include <world/world_pool.h>
#include <algorithm/algorithm.h>

namespace GTServer::events {
    void OnMovement(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_IS_IN))
            return;
        for (const auto& mod : ctx.m_player->GetPlaymods()) {
            if (mod.m_time.GetPassedTime() >= mod.m_time.GetTimeout() && mod.m_time.GetTimeout() != std::chrono::seconds(-1) && mod.m_type != PLAYMOD_TYPE_GHOST_IN_THE_SHELL && mod.m_type != PLAYMOD_TYPE_NICK && mod.m_type != PLAYMOD_TYPE_INVISIBLE) {
                ctx.m_player->RemovePlaymod(mod.m_type);
            }
        }
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        GameUpdatePacket* packet{ ctx.m_update_packet };
        if ((int)(packet->m_pos_x / 32) > world->GetSize().m_x || (int)(packet->m_pos_y / 32) > world->GetSize().m_y)
            return;

        bool invalid_movement = false;
        CL_Vec2f position = { static_cast<float>(ctx.m_player->GetPosition().m_x), static_cast<float>(ctx.m_player->GetPosition().m_y) };
        
        CL_Vec2i current_pos = { static_cast<int>((ctx.m_player->GetPosition().m_x + 10) / 32), static_cast<int>((ctx.m_player->GetPosition().m_y + 15) / 32) };
        CL_Vec2i future_pos = { static_cast<int>((packet->m_pos_x + 10) / 32), static_cast<int>((packet->m_pos_y + 15) / 32) };

        CL_Vec2i top_left = { static_cast<int>(packet->m_pos_x / 32), static_cast<int>(packet->m_pos_y / 32) };
        CL_Vec2i top_right = { static_cast<int>((packet->m_pos_x + 19) / 32), static_cast<int>(packet->m_pos_y / 32) };
        CL_Vec2i buttom_left = { static_cast<int>(packet->m_pos_x / 32), static_cast<int>((packet->m_pos_y + 29) / 32) };
        CL_Vec2i buttom_right = { static_cast<int>((packet->m_pos_x + 19) / 32), static_cast<int>((packet->m_pos_y + 29) / 32) };

        if (world->IsObstacle(ctx.m_player, top_left) && world->IsObstacle(ctx.m_player, top_right) && world->IsObstacle(ctx.m_player, buttom_left) && world->IsObstacle(ctx.m_player, buttom_right) && ctx.m_player->HasPlaymod(PLAYMOD_TYPE_FROZEN)) {      
            ctx.m_player->v_sender.OnSetPos(ctx.m_player->GetNetId(), position);
            return;  
        } else if (world->IsObstacle(ctx.m_player, top_left) || world->IsObstacle(ctx.m_player, top_right) || world->IsObstacle(ctx.m_player, buttom_left) || world->IsObstacle(ctx.m_player, buttom_right) || ctx.m_player->HasPlaymod(PLAYMOD_TYPE_FROZEN)) {
            ctx.m_player->v_sender.OnSetPos(ctx.m_player->GetNetId(), position);
            return; 
        } else {
            int x_diff = std::abs(current_pos.m_x - future_pos.m_x);
            int y_diff = std::abs(current_pos.m_y - future_pos.m_y);
            if (x_diff >= 10 || y_diff >= 10) { 
                ctx.m_player->v_sender.OnSetPos(ctx.m_player->GetNetId(), position);
                    return;
            }
            Tile* current = world->GetTile(current_pos);
            Tile* future = world->GetTile(future_pos);      
            if (current && future) {
                if (!Algorithm::OnFindPath(ctx.m_player, world, current_pos, future_pos)) {
                    ctx.m_player->v_sender.OnSetPos(ctx.m_player->GetNetId(), position);
                    return; 
                }
            } else {
                ctx.m_player->v_sender.OnSetPos(ctx.m_player->GetNetId(), position);
                return; 
            }
        }
        packet->m_net_id = ctx.m_player->GetNetId();
        ctx.m_player->SetPosition(packet->m_pos_x, packet->m_pos_y);
        if (packet->m_flags & NET_GAME_PACKET_FLAGS_FACINGLEFT)
            ctx.m_player->SetFlag(PLAYERFLAG_IS_FACING_LEFT);
        else
            ctx.m_player->RemoveFlag(PLAYERFLAG_IS_FACING_LEFT);

        world->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket));
        });
    }
}