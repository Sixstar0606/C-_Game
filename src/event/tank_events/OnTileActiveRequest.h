#pragma once
#include <world/world.h>
#include <world/world_pool.h>
#include <algorithm/algorithm.h>
#include <database/item/item_database.h>
#include <database/database.h>

namespace GTServer::events {
    void OnTileActiveRequest(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_IS_IN))
            return;
        std::shared_ptr<WorldPool> worldpool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ worldpool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world) return;
        if (!world->HasPlayer(ctx.m_player))
            return;
        CL_Vec2i current_pos = { ctx.m_player->GetPosition().m_x / 32, ctx.m_player->GetPosition().m_y / 32 };
        CL_Vec2i future_pos = { static_cast<int>(ctx.m_update_packet->m_int_x), static_cast<int>(ctx.m_update_packet->m_int_y) };
        Tile* current = world->GetTile(current_pos);
        Tile* tile = world->GetTile(future_pos);
        if (!current || !tile) return;
        ItemInfo* base = tile->GetBaseItem();
        if (!base) return;

        if (std::abs(current_pos.m_x - future_pos.m_x) >= 10 || std::abs(current_pos.m_y - future_pos.m_y) >= 10) { 
            ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "`w(Too far away)``", true);
            if (base->m_item_type == ITEMTYPE_MAIN_DOOR || base->m_item_type == ITEMTYPE_PORTAL || base->m_item_type == ITEMTYPE_DOOR) {
                ctx.m_player->v_sender.OnZoomCamera(10000.000000f, 1000, 200);
                ctx.m_player->v_sender.OnSetFreezeState(ctx.m_player->GetNetId(), 0, 200);
            } 
            return;  
        }
        if (!Algorithm::OnFindPath(ctx.m_player, world, current_pos, future_pos) && current != tile) {
            ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "`w(Too far away)``", true);
            if (base->m_item_type == ITEMTYPE_MAIN_DOOR || base->m_item_type == ITEMTYPE_PORTAL || base->m_item_type == ITEMTYPE_DOOR) {
                ctx.m_player->v_sender.OnZoomCamera(10000.000000f, 1000, 200);
                ctx.m_player->v_sender.OnSetFreezeState(ctx.m_player->GetNetId(), 0, 200);
            } 
            return;  
        }

        switch (base->m_item_type) {
        case ITEMTYPE_DOOR:
        case ITEMTYPE_GATEWAY:
        case ITEMTYPE_PORTAL: {
            if ((world->IsOwned() && !world->IsOwner(ctx.m_player) && !ctx.m_player->HasAccess(world, tile->GetPosition(), ITEM_FIST) && !tile->HasAccess(ctx.m_player->GetUserId()) && (ctx.m_player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                if (!tile->IsFlagOn(TILEFLAG_PUBLIC)) {
                    ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "``The door is `4locked``!", true);
                    ctx.m_player->v_sender.OnZoomCamera(10000.000000f, 1000, 200);
                    ctx.m_player->v_sender.OnSetFreezeState(ctx.m_player->GetNetId(), 0, 200);
                    return;
                }
            }
            auto destination_parse = utils::split(tile->GetDestination(), ":");
            std::string dest_world = destination_parse[0],
                dest_id{};
            if (destination_parse.size() > 1)
                dest_id = destination_parse[1];

            dest_world.erase(std::remove_if(dest_world.begin(), dest_world.end(), [&](char c) { return std::isalnum(c) == 0; }), dest_world.end());
            dest_id.erase(std::remove_if(dest_id.begin(), dest_id.end(), [&](char c) { return std::isalnum(c) == 0; }), dest_id.end());
            utils::uppercase(dest_world);
            utils::uppercase(dest_id);

            if (dest_world.empty() || dest_world == world->GetName()) {
                ctx.m_player->v_sender.OnZoomCamera(10000.000000f, 1000, 200);

                ctx.m_player->v_sender.OnSetFreezeState(ctx.m_player->GetNetId(), 0, 200);
                ctx.m_player->v_sender.OnPlayPositioned("door_open", ctx.m_player->GetNetId());
    
                CL_Vec2i main_door = world->GetTilePos(ITEMTYPE_MAIN_DOOR);
                CL_Vec2f position = { static_cast<float>(main_door.m_x * 32), static_cast<float>(main_door.m_y * 32) };
                if (dest_id.empty()) {
                    ctx.m_player->SetPosition(position.m_x, position.m_y);
                    ctx.m_player->v_sender.OnSetPos(ctx.m_player->GetNetId(), position, 200);
                    return;
                }
                for (auto& tile : world->GetTiles()) {
                    auto* item = tile.GetBaseItem();
                    if (!item) continue;

                    if (((item->m_item_type == ITEMTYPE_DOOR || item->m_item_type == ITEMTYPE_PORTAL) && tile.GetDoorUniqueId() == dest_id)
                    || (item->m_id == ITEM_PATH_MARKER || item->m_id == ITEM_OBJECTIVE_MARKER || item->m_id == ITEM_CARNIVAL_LANDING) && tile.GetLabel() == dest_id) {
                        position = CL_Vec2f{ static_cast<float>(tile.GetPosition().m_x * 32), static_cast<float>(tile.GetPosition().m_y * 32) };
                        break;
                    }
                }
                ctx.m_player->SetPosition(position.m_x, position.m_y);
                ctx.m_player->v_sender.OnSetPos(ctx.m_player->GetNetId(), position, 200);
                return;
            }
            ctx.m_player->v_sender.OnZoomCamera(10000.000000f, 1000, 200);
            ctx.m_player->v_sender.OnSetFreezeState(ctx.m_player->GetNetId(), 0, 200);
            ctx.m_player->v_sender.OnPlayPositioned("door_open", ctx.m_player->GetNetId());

            CL_Vec2i main_door = world->GetTilePos(ITEMTYPE_MAIN_DOOR);
            CL_Vec2f position = { static_cast<float>(main_door.m_x * 32), static_cast<float>(main_door.m_y * 32) };
        } break;
        case ITEMTYPE_MAIN_DOOR: {     
            worldpool->OnPlayerLeave(world, ctx.m_player, true);
        } break;
        case ITEMTYPE_HEART_MONITOR: {
            std::shared_ptr<Player> person{ ctx.m_servers->GetPlayerByUserID(tile->GetOwnerId())};
            if (!person->IsFlagOn(PLAYERFLAG_IS_IN)) {
                tile->m_label = (fmt::format("{}: `4Offline", person->GetDisplayName()));
            }
            else {
                tile->m_label = (fmt::format("{}: `2Online", person->GetDisplayName()));
            }
            world->SendTileUpdate(tile);
        } break;
        case ITEMTYPE_FOREGROUND: {
            switch(base->m_id) {
                case ITEM_STEAM_REVOLVER: {
                    if (ctx.m_update_packet->m_int_y + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y + 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK)
                        Algorithm::OnSteamPulse(ctx.m_player, world, ctx.m_update_packet, STEAM_DIRECTION_DOWN);
                    else if (ctx.m_update_packet->m_int_y - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y - 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK)
                        Algorithm::OnSteamPulse(ctx.m_player, world, ctx.m_update_packet, STEAM_DIRECTION_UP);   
                    else
                    {
                        if (ctx.m_update_packet->m_int_y + 1 < world->GetSize().m_x && Algorithm::IsSteamPowered(world->GetTile(ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y + 1)->GetForeground()))
                            Algorithm::OnSteamActive(world, world->GetTile(ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y + 1), ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y + 1, 0);
                        else if (ctx.m_update_packet->m_int_y - 1 >= 0 && Algorithm::IsSteamPowered(world->GetTile(ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y - 1)->GetForeground()))
                            Algorithm::OnSteamActive(world, world->GetTile(ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y - 1), ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y - 1, 0);
                    }
                } break;
                case ITEM_STEAM_STOMPER:
                {
                    if (ctx.m_update_packet->m_int_y + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y + 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK)
                        Algorithm::OnSteamPulse(ctx.m_player, world, ctx.m_update_packet, STEAM_DIRECTION_DOWN);
                    else if (ctx.m_update_packet->m_int_x - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(ctx.m_update_packet->m_int_x - 1, ctx.m_update_packet->m_int_y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK)
                        Algorithm::OnSteamPulse(ctx.m_player, world, ctx.m_update_packet, STEAM_DIRECTION_LEFT);
                    else if (ctx.m_update_packet->m_int_x + 1 >= 0 && ItemDatabase::GetItem(world->GetTile(ctx.m_update_packet->m_int_x + 1, ctx.m_update_packet->m_int_y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK)
                        Algorithm::OnSteamPulse(ctx.m_player, world, ctx.m_update_packet, STEAM_DIRECTION_RIGHT);
                    else
                    {
                        if (ctx.m_update_packet->m_int_y + 1 < world->GetSize().m_x && Algorithm::IsSteamPowered(world->GetTile(ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y + 1)->GetForeground()))
                            Algorithm::OnSteamActive(world, world->GetTile(ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y + 1), ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y + 1, 0);
                        else if (ctx.m_update_packet->m_int_x - 1 >= 0 && Algorithm::IsSteamPowered(world->GetTile(ctx.m_update_packet->m_int_x - 1, ctx.m_update_packet->m_int_y)->GetForeground()))                              
                            Algorithm::OnSteamActive(world, world->GetTile(ctx.m_update_packet->m_int_x - 1, ctx.m_update_packet->m_int_y), ctx.m_update_packet->m_int_x - 1, ctx.m_update_packet->m_int_y, 0);
                        else if (ctx.m_update_packet->m_int_x + 1 >= 0 && Algorithm::IsSteamPowered(world->GetTile(ctx.m_update_packet->m_int_x + 1, ctx.m_update_packet->m_int_y)->GetForeground()))
                            Algorithm::OnSteamActive(world, world->GetTile(ctx.m_update_packet->m_int_x + 1, ctx.m_update_packet->m_int_y), ctx.m_update_packet->m_int_x + 1, ctx.m_update_packet->m_int_y, 0);
                    } 
                } break;
                default: {
                    goto unhandled;
                } break;
            }
        } break;
        case ITEMTYPE_CHECKPOINT: {
            ctx.m_player->set_respawn_pos({ tile->GetPosition().m_x, tile->GetPosition().m_y });
            world->Broadcast([&](const std::shared_ptr<Player>& player) {
                player->v_sender.SetRespawnPos(ctx.m_player->GetNetId(), tile->GetPosition().m_x + tile->GetPosition().m_y * world->GetSize().m_x, 0);
            });
        } break;
        default: {
        unhandled:
            ctx.m_player->SendLog("unhandled TankPacket::OnTileActiveRequest: `w{}``[IT: `2{}``] (X: `w{}``, Y: `w{}``)", base->m_name, base->m_item_type, ctx.m_update_packet->m_int_x, ctx.m_update_packet->m_int_y);
            break;
        } break;
        }
    }
}