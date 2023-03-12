#pragma once
#include <player/player.h>
#include <world/world.h>
#include <database/database.h>
#include <database/item/item_database.h>
#include <algorithm/algorithm.h>
#include <render/world_render.h>
#include <event/tank_events/tile_change_req/OnConsume.h>

namespace GTServer::events::tile_change_req {
    void OnPlace(EventContext& ctx, std::shared_ptr<World> world, CL_Vec2i position) {
        std::shared_ptr<Player> player{ ctx.m_player };
        GameUpdatePacket* packet{ ctx.m_update_packet };
        if (!player->m_inventory.Contain(packet->m_item_id))
            return;
        if (std::abs((int)packet->m_int_x - player->GetPosition().m_x / 32) > 7 &&
            ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
            return;
        if (std::abs((int)packet->m_int_y - player->GetPosition().m_y / 32) > 7 &&
            ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
            return;

        Tile* tile{ world->GetTile(position.m_x, position.m_y) };
        if (!tile)
            return; 
        packet->m_type = NET_GAME_PACKET_TILE_CHANGE_REQUEST;
        packet->m_net_id = player->GetNetId();
        packet->m_tile_pos_x = position.m_x;
        packet->m_tile_pos_y = position.m_y; 

        ItemInfo* item{ ItemDatabase::GetItem(packet->m_item_id) };
        ItemInfo* base{ tile->GetBaseItem() };
        if (!item || !base)
            return;

        if (item->m_item_type == ITEMTYPE_CLOTHES && base->m_item_type != ITEMTYPE_MANNEQUIN) {
            player->v_sender.OnTalkBubble(player->GetNetId(), "To wear clothes, double tap them in your inventory.", true);
            return;
        }

        switch (item->m_item_type) {
        case ITEMTYPE_CONSUMABLE: {
            if (base->m_item_type == ITEMTYPE_DISPLAY_BLOCK)
                break;
            events::tile_change_req::OnConsume(ctx, world, position);
        } return;
        default:
            break;
        }

        if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
            PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
            uint32_t user_id{ 0 };
            if (world->IsTileOwned(tile)) {
                Tile* parent = world->GetParentTile(tile);
                if (!parent)
                    return;
                user_id = parent->GetOwnerId();
            } else {
                if (world->IsOwned()) {
                    Tile* main_lock = world->GetTile(world->GetMainLock());
                    if (!main_lock)
                        return;
                    user_id = main_lock->GetOwnerId();
                }
            }
            if (user_id == 0) return;
            player->v_sender.OnTalkBubble(player->GetNetId(), fmt::format("`wThis area is owned by `w{}``", database->GetRowVarchar(user_id, 4)), true);
            world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
            return;
        }
        
        if (item->IsBackground() ){
            if (!player->m_inventory.Erase(item->m_id, 1, false))
                return;
            tile->SetBackground(item->m_id);
        } else {
            if (tile->GetForeground() != ITEM_BLANK) {
                switch (base->m_item_type) {
                case ITEMTYPE_DISPLAY_BLOCK: {
                    if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                        world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                        return;
                    }
                    if (tile->GetItemId() != ITEM_BLANK) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`wRemove what's in there first!``", true);
                        return;
                    }
                    if (item->m_item_type == ITEMTYPE_LOCK || item->m_item_type == ITEMTYPE_DISPLAY_BLOCK) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`wSorry, no displaying Display Blocks or Locks.``", true);
                        return;
                    } else if ((item->m_item_category & ITEMFLAG2_UNTRADABLE) || (item->m_item_category & ITEMFLAG2_MOD)) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`wSorry, no displaying untradeable items.``", true);
                        return; 
                    } else if (item->m_id == ITEM_WORLD_KEY || item->m_id == ITEM_GUILD_KEY) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "No no no.``", true);
                        return; 
                    }
                    if (!player->RemoveItemSafe(item->m_id, 1, true))
                        return;
                    tile->m_item_id = item->m_id;
                    GameUpdatePacket effect_packet{ NET_GAME_PACKET_ITEM_EFFECT };
                    effect_packet.m_pos_x = static_cast<float>(position.m_x * 32) + 15;
                    effect_packet.m_pos_y = static_cast<float>(position.m_y * 32) + 15;
                    effect_packet.m_particle_id = 4;
                    effect_packet.m_item_id_alt = item->m_id;
                    effect_packet.m_target_net_id = player->GetNetId();
                    effect_packet.m_particle_size_alt = 1;
                    world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->SendPacket(NET_MESSAGE_GAME_PACKET, &effect_packet, sizeof(GameUpdatePacket)); });
                    world->SendTileUpdate(tile, 0);
                } return;
                case ITEMTYPE_MANNEQUIN: {
                    for (auto& ply : world->GetPlayers(false)) {
                        if (ply->GetPosition().m_x + 12 > (position.m_x * 32) && ply->GetPosition().m_y + 10 > (position.m_y * 32) && ply->GetPosition().m_x + 8 < ((position.m_x + 1) * 32) && ply->GetPosition().m_y + 8 < ((position.m_y + 1) * 32) && (item->m_collision_type == ITEMCOLLISION_GATEWAY || item->m_collision_type == ITEMCOLLISION_NORMAL|| item->m_collision_type == ITEMCOLLISION_VIP_GATEWAY || item->m_collision_type == ITEMCOLLISION_SWITCHEROO)) {
                            player->v_sender.OnTextOverlay(fmt::format("Somebody is in the way of the {}!", base->m_name));
                            return;
                        }
                    }
                    if (tile->GetCloth(item->m_clothing_type) == item->m_id) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`5[`2You giggle as you swap two identical items.``]``", true);
                        return;
                    }
                    else if (item->m_item_category & ITEMFLAG2_UNTRADABLE) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`5[`2That item is simply too valuable to part with.``]``", true);
                        return;
                    }
                    else if (item->m_item_type == ITEMTYPE_CONSUMABLE) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`5[`2That will be weird. Try putting clothes on your mannequin instead.``]``");
                        return;
                    }
                    else if (tile->GetBackground() == ITEM_DARK_CAVE_BACKGROUND) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`5[`2It's too dark to use this mannequin!``]``", true);
                        return;
                    }
                    if (item->m_item_type != ITEMTYPE_CLOTHES)
                        return;
                    DialogBuilder db{};
                    db.set_default_color('o')
                        ->add_label_with_icon(fmt::format("`wEdit {}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                        ->add_spacer()
                        ->embed_data<int32_t>("tilex", position.m_x)
                        ->embed_data<int32_t>("tiley", position.m_y)
                        ->embed_data<uint32_t>("itemId", item->m_id)
                        ->add_spacer()
                        ->add_textbox(fmt::format("Do you really want to put your {} on the {}?", item->m_name, base->m_name))
                        ->end_dialog("mannequin_edit", "Cancel", "OK");
                    player->v_sender.OnDialogRequest(db.get());
                } return;
                default: {
                    switch (item->m_item_type) {
                    case ITEMTYPE_CONSUMABLE: {
                        events::tile_change_req::OnConsume(ctx, world, position);
                    } return;
                    case ITEMTYPE_SEED: {
                        if (base->m_item_type != ITEMTYPE_SEED && base->m_item_type != ITEMTYPE_MAGIC_EGG) {
                            player->v_sender.OnTalkBubble(player->GetNetId(), "`wYou can only use seeds on blank tiles or existing trees.``", true);
                            return;
                        }
                        if (item->m_id == ITEM_MAGIC_EGG && base->m_item_type == ITEMTYPE_MAGIC_EGG) {
                            if (tile->GetEggsPlaced() >= 2000) {
                                player->v_sender.OnTalkBubble(player->GetNetId(), "`9This egg already at maxed size.``", true);
                                return;
                            }
                            if (!player->m_inventory.Erase(item->m_id, 1, true))
                                return;
                            tile->m_eggs_placed++;
                            GameUpdatePacket update_packet {
                                .m_type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
                                .m_net_id = -1,
                                .m_particle_variable = 10,
                                .m_particle_alt_id = 66
                            };
                            update_packet.m_pos_x = (position.m_x * 32) + 15;
                            update_packet.m_pos_y = (position.m_y * 32) + 15;
                            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                                ply->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
                            });
                            world->SendTileUpdate(tile, 0);
                            return;
                        }
                        if (high_resolution_clock::now() - tile->GetPlantedDate() >= std::chrono::seconds(base->m_grow_time)) {
                            player->v_sender.OnTalkBubble(player->GetNetId(), "This tree is already too big to splice another seed with it.", true);
                            return;
                        } else if (tile->IsSpliced()) {
                            player->v_sender.OnTalkBubble(player->GetNetId(), "It would be too dangerous to try to mix three seeds.", true);
                            return;
                        } else if (tile->GetForeground() == item->m_id || base->m_rarity == 999 || item->m_rarity == 999) {
                            player->v_sender.OnTalkBubble(player->GetNetId(), fmt::format("Hmm, it looks like `w{}`` and `w{}`` can't be spliced.", item->m_name, base->m_name), true);
                            return;
                        }
                        ctx.m_player->SendLog("Unimplemented LOL");
                    } return;
                    case ITEMTYPE_MAGIC_EGG: return;
                    default:
                        break;
                    }
                } break;
                }
                return;
            }
            
            for (auto& ply : world->GetPlayers(false)) {
                if (ply->GetPosition().m_x + 12 > (position.m_x * 32) && ply->GetPosition().m_y + 10 > (position.m_y * 32) && ply->GetPosition().m_x + 8 < ((position.m_x + 1) * 32) && ply->GetPosition().m_y + 8 < ((position.m_y + 1) * 32) && (item->m_collision_type == ITEMCOLLISION_GATEWAY || item->m_collision_type == ITEMCOLLISION_NORMAL|| item->m_collision_type == ITEMCOLLISION_VIP_GATEWAY || item->m_collision_type == ITEMCOLLISION_SWITCHEROO))
                    return; 
            }
            switch (item->m_item_type) {
            case ITEMTYPE_LOCK: {
                if (item->IsWorldLock()) {
                    if (world->IsOwned()) {
                        if (world->IsOwner(player)) {
                            player->v_sender.OnTalkBubble(player->GetNetId(), "Only one `$World Lock`` can be placed in a world, you'd have to remove the other one first.", true);
                            world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                        }
                        else {
                            PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                            player->v_sender.OnTalkBubble(player->GetNetId(),
                                fmt::format(
                                    "`w{}`4 allows `9Public Building`4 here but locks are not allowed",
                                    database->GetRowVarchar(world->GetOwnerId(), 4)),
                                true);
                        }
                        return;
                    }
                    for (auto& t : world->GetTiles()) {
                        if (t.GetBaseItem()->m_item_type != ITEMTYPE_LOCK)
                            continue;
                        if (t.GetOwnerId() == player->GetUserId())
                            continue;
                        player->v_sender.OnTalkBubble(player->GetNetId(), "Your `$World Lock`` can't be placed in this world unless everyone else's locks are removed.", true);
                        world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                        return;
                    }

                    if (!tile->SetForeground(item->m_id))
                        return;
                    if (!player->m_inventory.Erase(item->m_id, 1, true))
                        return;
                    ctx.m_update_packet->m_int_x = position.m_x;
                    ctx.m_update_packet->m_int_y = position.m_y;
                    tile->ApplyLockOwner(player->GetUserId());
                    Algorithm::OnLockApply(player, world, ctx.m_update_packet);
                    return;
                }
                if (!tile->SetForeground(item->m_id))
                    return;
                if (!player->m_inventory.Erase(item->m_id, 1, true))
                    return;
                ctx.m_update_packet->m_int_x = position.m_x;
                ctx.m_update_packet->m_int_y = position.m_y;
                tile->ApplyLockOwner(player->GetUserId());
                tile->SetLockFlag(LOCKFLAG_AREA_LOCK);

                player->v_sender.OnTalkBubble(player->GetNetId(), "Area locked.", true);
                Algorithm::OnLockApply(player, world, ctx.m_update_packet);
                if (tile->GetBaseItem()->m_id == ITEM_SMALL_LOCK || tile->GetBaseItem()->m_id == ITEM_BIG_LOCK || tile->GetBaseItem()->m_id == ITEM_HUGE_LOCK || tile->GetBaseItem()->m_id == ITEM_BUILDERS_LOCK) {
                    Algorithm::OnLockReApply(player, world, ctx.m_update_packet);
                }
                return;
            } return;
            case ITEMTYPE_HEART_MONITOR: {
                tile->set_sign_data(fmt::format("{}: `2Online", player->GetDisplayName()), -1);
                tile->m_label = player->GetUserId();
                tile->m_owner_id = player->GetUserId();
                tile->SetBackground(item->m_id);
                ctx.m_update_packet->m_int_x = position.m_x;
                ctx.m_update_packet->m_int_y = position.m_y;
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size); });
                world->SendTileUpdate(tile, 0);
            } return;
            case ITEMTYPE_BULLETIN:
            case ITEMTYPE_WEATHER_MACHINE:
            case ITEMTYPE_WEATHER_SPECIAL:
            case ITEMTYPE_WEATHER_SPECIAL2:
            case ITEMTYPE_WEATHER_INFINITY: {
                for (auto& t : world->GetTiles()) {
                    if (t.GetBaseItem()->m_id != item->m_id)
                        continue;
                    player->v_sender.OnTalkBubble(player->GetNetId(), fmt::format("`wThis world already has a `o{} `wsomewhere on it, installing two would be dangerous!", item->m_name), true);
                    return;
                }
                if (item->m_id == ITEM_WEATHER_MACHINE_GUILD) {
                    if (!world->IsOwned()) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`wYou can only place it on a locked world.``", true);
                        return;
                    } else if (world->GetTile(world->GetMainLock())->GetForeground() != ITEM_GUILD_LOCK) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`wThis item only be placed in a Guild Locked world.``", true);
                        return;
                    }
                }
                if (!player->m_inventory.Erase(item->m_id, 1, true))
                    return;
                if (!tile->SetForeground(item->m_id))
                    return;
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size); });
                world->SendTileUpdate(tile);
            } return;
            case ITEMTYPE_CONSUMABLE: {
                events::tile_change_req::OnConsume(ctx, world, position);
            } return;
            default: {
                if (tile->SetForeground(item->m_id)) {
                    if (player->IsFlagOn(PLAYERFLAG_IS_FACING_LEFT) && (item->m_editable_type & ITEMFLAG1_FLIPPED))
                        tile->SetFlag(TILEFLAG_FLIPPED);
                    switch (item->m_item_type) {
                    case ITEMTYPE_LOCK: return;
                    case ITEMTYPE_SEED: {
                        if (!player->m_inventory.Erase(item->m_id, 1, false))
                            return;
                        if (item->m_id == ITEM_MAGIC_EGG) {
                            GameUpdatePacket update_packet {
                                .m_type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
                                .m_net_id = -1,
                                .m_particle_variable = 10,
                                .m_particle_alt_id = 66
                            };
                            update_packet.m_pos_x = (position.m_x * 32) + 15;
                            update_packet.m_pos_y = (position.m_y * 32) + 15;
                            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                                ply->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
                            });
                            world->SendTileUpdate(tile, 0);
                            return;
                        } 
                        packet->m_fruit_count = tile->GetFruitCount();
                        
                        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                            ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size);
                        });
                        world->SendTileUpdate(tile, 0);
                    } return;
                    case ITEMTYPE_MAGIC_EGG: {
                        if (!player->m_inventory.Erase(item->m_id, 1, true))
                            return;
                        GameUpdatePacket update_packet {
                            .m_type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
                            .m_net_id = -1,
                            .m_particle_alt_id = 67
                        };
                        update_packet.m_pos_x = (position.m_x * 32) + 15;
                        update_packet.m_pos_y = (position.m_y * 32) + 15;
                        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                            ply->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
                            ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size);
                        });

                        world->SendTileUpdate(tile, 0);
                    } return;
                    case ITEMTYPE_PROVIDER: {
                        if (!player->m_inventory.Erase(item->m_id, 1, false))
                            return;
                        world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size); });
                        world->SendTileUpdate(tile, 0);
                    } return;
                    case ITEMTYPE_VIP_ENTRANCE: {
                        if (!player->m_inventory.Erase(item->m_id, 1, false))
                            return;
                        world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size); });
                        tile->SetFlag(TILEFLAG_OPEN);
                        world->SendTileUpdate(tile, 0);
                        tile->SetFlag(TILEFLAG_OPEN);
                    } return;
                    case ITEMTYPE_FLAG: {
                        if (!WorldRender::get_texture_from_cache(fmt::format("{}.rttex", player->GetLoginDetail()->m_country)))
                            return;
                        if (!player->m_inventory.Erase(item->m_id, 1, false))
                            return;
                        tile->m_label = player->GetLoginDetail()->m_country;
                        world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size); });
                        world->SendTileUpdate(tile, 0);
                    } return;
                    default: {
                        if (!player->m_inventory.Erase(item->m_id, 1, false) && player->GetRole() == PLAYER_ROLE_DEVELOPER)
                            fmt::print("OnTileChangeRequest::OnPunch -> foreground were set but cannot take item from inventory, {} - [{}] {}\n", item->m_name, world->GetName(), player->GetRawName());
                    } break;
                    }
                    break;
                }
                player->SendLog("`4Oops, `wit seems that `5{} `wis not implimented yet, please let the Developers know!", item->m_name);
                player->v_sender.OnTalkBubble(player->GetNetId(), std::format("`4Oops, `wit seems that `5{} `wis not implimented yet, please let the Developers know!", item->m_name), true);
            } return;
            }
        }
        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
            ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size);
        });
    }
}