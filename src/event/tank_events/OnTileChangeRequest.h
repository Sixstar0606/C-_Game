#pragma once
#include <world/world.h>
#include <world/world_pool.h>
#include <event/tank_events/tile_change_req/OnPlace.h>
#include <event/tank_events/tile_change_req/OnPunch.h>
#include <event/tank_events/tile_change_req/OnWrench.h>

namespace GTServer::events {
    void OnTileChangeRequest(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_IS_IN))
            return;
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        switch (ctx.m_update_packet->m_item_id) {
        case ITEM_FIST: {
            //OnTileApplyDamage
            tile_change_req::OnPunch(ctx, world, CL_Vec2i{ ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y });
        } break;
        case ITEM_WRENCH: {
            //OnTileWrenchRequest
            tile_change_req::OnWrench(ctx, world, CL_Vec2i{ ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y });
        } break;
        default: {
            //OnTilePlaceRequest
            tile_change_req::OnPlace(ctx, world, CL_Vec2i{ ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y });
        } break;
        }
    }
}