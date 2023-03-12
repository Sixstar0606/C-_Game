#pragma once
#include <chrono>
#include <player/player.h>
#include <world/world.h>
#include <server/server_pool.h>
#include <database/database.h>
#include <database/item/item_database.h>
#include <utils/random.h>

namespace GTServer::events::tile_change_req {
    void process_harvest_tree(std::shared_ptr<Player> player, std::shared_ptr<World> world, Tile* tile, CL_Vec2f position) {
        ItemInfo* item = ItemDatabase::GetItem(tile->GetForeground() - 1);
        ItemInfo* seed_info = tile->GetBaseItem();
        if (!item) return;
        WorldObject object {
            .m_item_id = (uint16_t)item->m_id,
            .m_item_amount = (uint8_t)(tile->GetFruitCount()),
            .m_pos = position
        };
        tile->RemoveBase();

        switch (seed_info->m_id) {
        case ITEM_LEGENDARY_WIZARD_SEED: {   
            tile->SetForeground(ITEM_LEGENDARY_WIZARD);
            if (player->GetPosition().m_x / 32 < tile->GetPosition().m_x)
                tile->SetFlag(TILEFLAG_FLIPPED);
            else if (player->GetPosition().m_x / 32 > tile->GetPosition().m_x)
                tile->RemoveFlag(TILEFLAG_FLIPPED);

            GameUpdatePacket update_packet {
                .m_type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
                .m_net_id = -1,
                .m_particle_variable = 10,
                .m_particle_alt_id = 48
            };
            update_packet.m_pos_x = (tile->GetPosition().m_x * 32) + 15;
            update_packet.m_pos_y = (tile->GetPosition().m_y * 32) + 15;
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
            });
            world->SendTileUpdate(tile, 0);
            return;
        };
        default: {
            world->AddObject(object, true);
        } break;
        }

        uint16_t rarity = item->m_rarity;
        float penalty = 0.010f;
        int per = 10;

        if (rarity < 30) {
            rarity -= 19;
            per = 2;
        } else if (rarity < 100) {
            rarity += 20;
        } else {
            rarity += 110;
            per = 20;
        }

        float cn = 0.16f;
        cn -= (penalty * (rarity / per));

        float seed_chance = 4.0f / (item->m_rarity + 12);
        if (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) <= seed_chance)
        {
            uint16_t seed = seed_info->m_id;
            switch (item->m_id) {
            case ITEM_BUSH: { seed = std::rand() % 100 > 70 ? ITEM_HEDGE_SEED : seed_info->m_id; } break;
            default:
                break;
            }
            if (tile->GetBaseItem()->m_name != "Blank") {
                object.m_item_id = seed,
                    object.m_item_amount = 1;
                world->AddObject(object, true);
                player->v_sender.OnTalkBubble(player->GetNetId(), fmt::format("`w{} falls out!``", tile->GetBaseItem()->m_name), true);
            }
        }
        if (item->m_rarity == 999)
            return;
        int max_gems = 0;
        if (item->m_rarity > 13) {
            max_gems = item->m_rarity / 3;
        } else {
            switch (std::rand() % 2) {
            case 1: {
                max_gems = 1;
            } break;
            case 2: {
                max_gems = 2;
            } break;
            default: {
                max_gems = 0;
            } break;
            } 
        } 
        if (max_gems < 1) return;
        position.m_x += 6;
        position.m_y += 6;
        world->AddGemsObject(std::rand() % max_gems + 1, position);
    }
    void add_block_objects(std::shared_ptr<Player> player, std::shared_ptr<World> world, Tile* tile, CL_Vec2f position) {
        static float seed_chance = 1.0f / 4.0f;
        static float gems_chance = 2.0f / 3.0f;

        float value = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        ItemInfo* item = tile->GetBaseItem();
        if (!item) return;
        if (item->m_rarity == 999) return;

        position.m_x += 6;   
        position.m_y += 6;   

        switch(item->m_id) {
            case ITEM_SURG_E: return;
            default: break;
        }

        if (value <= gems_chance) {
            int max_gems = 0;
            if (item->m_rarity > 13) {
                max_gems = item->m_rarity / 3;
            } else {
                switch (std::rand() % 2) {
                case 1: {
                    max_gems = 1;
                } break;
                case 2: {
                    max_gems = 2;
                } break;
                default: {
                    max_gems = 0;
                } break;
                } 
            } 
            if (max_gems < 1) return;
            position.m_x += 6;
            position.m_y += 6;
            world->AddGemsObject(std::rand() % max_gems + 1, position); 
        } else if (value <= gems_chance + seed_chance) {     
            if (item->m_item_type != ITEMTYPE_SEED && !(item->m_editable_type & ITEMFLAG1_SEEDLESS)) {   
                uint16_t seed = item->m_id + 1;
                switch(item->m_id) {
                case ITEM_BUSH: { seed = rand() % 100 > 95 ? ITEM_HEDGE_SEED : item->m_id + 1; break; }
                default:
                    break;
                }

                WorldObject object{};
                object.m_pos = position;
                object.m_item_id = seed;
                object.m_item_amount = 1;
                world->AddObject(object, true);
            }
        } else {
            WorldObject object{};
            object.m_pos = position;
            object.m_item_id = item->m_id;
            object.m_item_amount = 1;
            world->AddObject(object, true);
        }
    }
    
    void OnPunch(EventContext& ctx, std::shared_ptr<World> world, CL_Vec2i position) {
        std::shared_ptr<Player> player{ ctx.m_player };
        GameUpdatePacket* packet{ ctx.m_update_packet };
        Tile* tile{ world->GetTile(position.m_x, position.m_y) };
        if (!tile)
            return;

        packet->m_type = NET_GAME_PACKET_TILE_APPLY_DAMAGE;
        packet->m_net_id = player->GetNetId();
        packet->m_tile_pos_x = position.m_x;
        packet->m_tile_pos_y = position.m_y;
        packet->m_tile_damage = 8;

        ItemInfo* base{ tile->GetBaseItem() };

        if (player->HasPlaymod(PLAYMOD_TYPE_1HIT)) {
            packet->m_type = NET_GAME_PACKET_TILE_CHANGE_REQUEST;
            packet->m_item_id = ITEM_FIST;

            if (base->m_item_type != ITEMTYPE_LOCK) {
                int lock_index = position.m_x + position.m_y * world->GetSize().m_x;
                for (auto index = 0; index < world->GetTiles().size(); ++index) {
                    if (index == lock_index)
                        continue;
                    Tile* neighbour = world->GetTile(index);
                    if (neighbour->IsFlagOn(TILEFLAG_LOCKED) && neighbour->GetParent() == lock_index) {
                        neighbour->RemoveLock();
                        world->SendTileUpdate(neighbour);
                    }
                }
                tile->ClearAccess();
                tile->RemoveLock();
                tile->RemoveBase();
                world->SendTileUpdate(tile);
                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                    ply->v_sender.OnPlayPositioned("metal_destroy", player->GetNetId());
                ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size);
                    });
            }
        }

        if (base->m_id == ITEM_BLANK)
            return;
        if (tile->IsFlagOn(TILEFLAG_FIRE) && player->GetCloth(CLOTHTYPE_HAND) == ITEM_FIRE_HOSE) {
            tile->RemoveFlag(TILEFLAG_FIRE);
            world->SendTileUpdate(tile);
            return;
        }
        if ((base->m_item_type == ITEMTYPE_BEDROCK || base->m_item_type == ITEMTYPE_MAIN_DOOR) && (player->GetRole() < PLAYER_ROLE_DEVELOPER)) {
            player->v_sender.OnTalkBubble(player->GetNetId(), "`wIt's too strong to break.", true);
            player->PlaySfx("cant_break_tile", 0);
            return;
        }

        if (player->GetRole() == PLAYER_ROLE_DEVELOPER && !tile->HasDevPunch(player)) {
            tile->DevPunchAdd(player);
            player->v_sender.OnTalkBubble(player->GetNetId(), "`wSent fake punch as you are dev.", true);
            player->PlaySfx("cant_break_tile", 0);
            return;
        }

        if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
            packet->m_tile_damage = 0;
        } else {
            if (tile->GetLastHitten().GetPassedTime().count() >= base->m_reset_time)
                tile->ResetHits();
        }

        if (tile->IsFlagOn(TILEFLAG_LOCKED) && !world->IsTileOwner(tile, player)) {
            Tile* parent = world->GetParentTile(tile);
            if (!parent)
                return;
            if (parent->GetBaseItem()->m_id == ITEM_BUILDERS_LOCK) {
                if ((world->HasTileAccess(tile, player)) && (player->GetRole() < PLAYER_ROLE_DEVELOPER)) {
                    if (parent->IsLockFlagOn(LOCKFLAG_RESTRICT_ADMIN) && parent->IsLockFlagOn(LOCKFLAG_ONLY_BUILDING)) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`wThis lock allows placing only``", true);
                        player->PlaySfx("cant_break_tile", 0);
                        return;
                    }
                } else if (parent->IsFlagOn(TILEFLAG_PUBLIC)) {
                    if (parent->IsLockFlagOn(LOCKFLAG_ONLY_BUILDING)) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "`wThis lock allows placing only``", true);
                        player->PlaySfx("cant_break_tile", 0);
                        return;
                    }
                }
            }
        }

        switch (base->m_item_type) {
        case ITEMTYPE_LOCK: {
            if (tile->GetOwnerId() != player->GetUserId()) {
                PlayerTable* db = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                if (base->IsWorldLock()) {
                    std::string owner_name = db->GetRowVarchar(tile->GetOwnerId(), 4);
                    std::string lock_action{ "`w(`4No Access`w)" };
                    std::string lock_message{ fmt::format("``{}`w's `o{}`w. {}", owner_name, base->m_name, lock_action) };

                    if (tile->IsFlagOn(TILEFLAG_PUBLIC))
                        lock_action = std::string{ "`w(Open to public)" };
                    if (tile->HasAccess(player->GetUserId()))
                        lock_action = std::string{ "`w(`2Access Granted`w)" };
                    player->v_sender.OnTalkBubble(player->GetNetId(), lock_message, true);
                } else {
                    XYZClock last_active = XYZClock{ db->GetRowTimestamp(tile->GetOwnerId(), 1) };

                    if (last_active.get_days_passed() >= 30) {
                        player->SendLog("`5INACTIVELOCK: `o{}`o's lock `4disintegrates`o due to last playing `5{}`o days ago!", 
                            db->GetRowVarchar(tile->GetOwnerId(), 4), last_active.get_days_passed());
                        player->v_sender.OnTalkBubble(player->GetNetId(),
                            fmt::format(
                                "`5INACTIVELOCK: `o{}`o's lock `4disintegrates`o due to last playing `5{}`o days ago!",  
                                db->GetRowVarchar(tile->GetOwnerId(), 4), last_active.get_days_passed()
                            ), false);
                        packet->m_type = NET_GAME_PACKET_TILE_CHANGE_REQUEST;
                        packet->m_item_id = ITEM_FIST;

                        int lock_index = position.m_x + position.m_y * world->GetSize().m_x;
                        for (auto index = 0; index < world->GetTiles().size(); ++index) {
                            if (index == lock_index)
                                continue;
                            Tile* neighbour = world->GetTile(index);
                            if (neighbour->IsFlagOn(TILEFLAG_LOCKED) && neighbour->GetParent() == lock_index) {
                                neighbour->RemoveLock();
                                world->SendTileUpdate(neighbour);
                            }
                        }
                        tile->ClearAccess();
                        tile->RemoveLock();
                        tile->RemoveBase();

                        world->SendTileUpdate(tile);
                        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                            ply->v_sender.OnPlayPositioned("metal_destroy", player->GetNetId());
                            ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size);
                        });
                        return;
                    }
                    std::string owner_name = db->GetRowVarchar(tile->GetOwnerId(), 4);
                    std::string lock_action{ "`w(`4No Access`w)" };
                    std::string lock_message{ fmt::format("``{}`w's `o{}`w. {} (Last played {} days ago)", 
                        owner_name, base->m_name, lock_action, last_active.get_days_passed())
                    };

                    if (tile->IsFlagOn(TILEFLAG_PUBLIC))
                        lock_action = std::string{ "`w(Open to public)" };
                    if (tile->HasAccess(player->GetUserId()))
                        lock_action = std::string{ "`w(`2Access Granted`w)" };
                    if (last_active.get_hours_passed() < 2)
                        lock_message = std::string{ fmt::format("``{}`w's `o{}`w. {} (Last played a few minutes ago)", owner_name, base->m_name, lock_action) };
                    else if (last_active.get_hours_passed() >= 1 && last_active.get_hours_passed() < 25)
                        lock_message = std::string{ fmt::format("``{}`w's `o{}`w. {} (Last played {} hours ago)", owner_name, base->m_name, lock_action, last_active.get_hours_passed()) };
                    if (ctx.m_servers->HasPlayer(tile->GetOwnerId()))
                        lock_message = std::string{ fmt::format("``{}`w's `o{}`w. {}", owner_name, base->m_name, lock_action) };
                    player->v_sender.OnTalkBubble(player->GetNetId(), lock_message, true);
                }
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                if (player->GetRole() != PLAYER_ROLE_DEVELOPER)
                    return;
            }
        } break;
        case ITEMTYPE_BOOMBOX:
        case ITEMTYPE_GREEN_FOUNTAIN: {
            if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                return;
            }
            if (tile->IsFlagOn(TILEFLAG_OPEN)) {
                tile->RemoveFlag(TILEFLAG_OPEN);
            } else {
                tile->SetFlag(TILEFLAG_OPEN);
            }
        } break;
        case ITEMTYPE_SEED: {
            if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                return;
            }
            if (base->m_item_type == ITEMTYPE_MAGIC_EGG)
                return;
            if ((high_resolution_clock::now() - tile->GetPlantedDate()) < std::chrono::seconds(base->m_grow_time))
                break;
            packet->m_type = NET_GAME_PACKET_SEND_TILE_TREE_STATE;
            packet->m_item = -1;
            packet->m_vec_x = 0, packet->m_vec_y = 0;
            packet->m_vec2_x = 0, packet->m_vec2_y = 0;
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size);
            });

            CL_Vec2f drop_position = { static_cast<float>(position.m_x * 32), static_cast<float>(position.m_y * 32) };
            process_harvest_tree(player, world, tile, drop_position);
            if (base->m_rarity != 999 && base->m_rarity > 0)
                player->SendExperience(world, ((base->m_rarity / 10) + 1) + ((base->m_rarity / 10) + 1));
                int min = 15;
                int max = std::rand() % 100 > 50 ? 140 : 202;
                std::random_device rd;   
                std::mt19937 rng(rd());    
                std::uniform_int_distribution<int> uni(min, max);
                world->AddGemsObject(((base->m_rarity / 10) + 1) + ((base->m_rarity / 10) + 1), drop_position);
            return;
        } break;
        case ITEMTYPE_SWITCHEROO:
        case ITEMTYPE_CHEST:
        case ITEMTYPE_SWITCHEROO2: {
            if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                return;
            }
            if (tile->IsFlagOn(TILEFLAG_OPEN)) {
                tile->RemoveFlag(TILEFLAG_OPEN);
            } else {
                tile->SetFlag(TILEFLAG_OPEN);
            }
        } break;
        case ITEMTYPE_DICE: {
            if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                return;
            }
            if (tile->GetPunchDelay().GetPassedTime() < tile->GetPunchDelay().GetTimeout())
                break;
            static randutils::pcg_rng gen{ utils::random::get_generator_local() };
            tile->GetPunchDelay().UpdateTime();
            tile->m_random_value = gen.uniform(0, 5);
            packet->m_dice_result = tile->GetDiceResult();
        } break;
        case ITEMTYPE_PROVIDER: {
            if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                return;
            }
            if ((high_resolution_clock::now() - tile->GetPlantedDate()) < std::chrono::seconds(base->m_grow_time))
                break;
            auto rewards = ItemDatabase::GetRewards(REWARD_TYPE_PROVIDER, base->m_id);
            if (rewards.empty()) {
                player->SendLog("Unhandled ItemDatabase::Rewards(REWARD_TYPE_PROVIDER) -> Id: {}, Name: {}", base->m_id, base->m_name);
                return;
            }
            auto reward = rewards[std::rand() % rewards.size()];
            uint8_t reward_amount = (std::rand() % reward.second) + 1;
            CL_Vec2f drop_position = { static_cast<float>(position.m_x * 32) + 6, static_cast<float>(position.m_y * 32) + 6 };

            switch (base->m_id) {
            case ITEM_ATM_MACHINE: {
                int min = 15;
                int max = std::rand() % 100 > 50 ? 140 : 202;
                std::random_device rd;   
                std::mt19937 rng(rd());    
                std::uniform_int_distribution<int> uni(min, max);
                world->AddGemsObject(uni(rng), drop_position);
            } break;
            default: {
                world->AddObject(reward.first, reward_amount, drop_position);
            } break;
            }
            tile->m_planted_date = high_resolution_clock::now() - std::chrono::seconds(base->m_grow_time / 2);
            world->SendTileUpdate(tile, 0);
            return;
        } break;
        case ITEMTYPE_WEATHER_MACHINE:
        case ITEMTYPE_WEATHER_SPECIAL:
        case ITEMTYPE_WEATHER_SPECIAL2: {
            if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                return;
            }
            if (tile->IsFlagOn(TILEFLAG_OPEN)) {
                tile->RemoveFlag(TILEFLAG_OPEN);
                world->SetWeatherId(world->GetBaseWeatherId());
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnSetCurrentWeather(world->GetBaseWeatherId()); });
            } else {
                for (auto& t : world->GetTiles()) {
                    if (!(t.GetBaseItem()->m_item_type == ITEMTYPE_WEATHER_MACHINE || t.GetBaseItem()->m_item_type == ITEMTYPE_WEATHER_SPECIAL || t.GetBaseItem()->m_item_type == ITEMTYPE_WEATHER_SPECIAL2))
                        continue;
                    if (!t.IsFlagOn(TILEFLAG_OPEN) || t.GetPosition() == position)
                        continue;
                    t.RemoveFlag(TILEFLAG_OPEN);
                    world->SendTileUpdate(&t, 0);
                }
                tile->SetFlag(TILEFLAG_OPEN);
                world->SetWeatherId(base->m_weather_id);
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnSetCurrentWeather(world->GetWeatherId()); });
            }
        } break;
        case ITEMTYPE_MANNEQUIN: {
            if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                return;
            }
            for (auto& cloth : tile->GetClothes()) {
                if (cloth == ITEM_BLANK)
                    continue;
                if (!player->m_inventory.Add(cloth, 1, true)) {
                    player->v_sender.OnTalkBubble(player->GetNetId(), "I better not break that, I have no room to pick it up!", true);
                    return;
                }
                GameUpdatePacket effect_packet {
                    .m_type = NET_GAME_PACKET_ITEM_EFFECT,
                    .m_animation_type = 0x5,
                    .m_target_net_id = static_cast<int32_t>(player->GetNetId()),
                    .m_item_id_alt = cloth,
                    .m_item_count = 1
                };
                effect_packet.m_pos_x = static_cast<float>((position.m_x * 32) + 15);
                effect_packet.m_pos_y = static_cast<float>((position.m_y * 32) + 15);

                cloth = ITEM_BLANK;
                world->SendTileUpdate(tile);
                world->Broadcast([&](const std::shared_ptr<Player>& player) { player->SendPacket(NET_MESSAGE_GAME_PACKET, &effect_packet, sizeof(GameUpdatePacket)); });
                return;
            }
        } break;
        default: {
            if ((world->IsOwned() && !world->IsOwner(player) && !player->HasAccess(world, position, ITEM_FIST) && !tile->HasAccess(player->GetUserId()) && (player->GetRole() != PLAYER_ROLE_DEVELOPER))) {
                world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->v_sender.OnPlayPositioned("punch_locked", player->GetNetId()); });
                return;
            }

            switch (base->m_id) {
            case ITEM_LEGENDARY_WIZARD: {
                if (player->GetPosition().m_x / 32 < position.m_x) {
                    if (tile->IsFlagOn(TILEFLAG_FLIPPED))
                        break;
                    tile->SetFlag(TILEFLAG_FLIPPED);
                    world->SendTileUpdate(tile, 0);
                } else if (player->GetPosition().m_x / 32 > position.m_x) {
                    if (!tile->IsFlagOn(TILEFLAG_FLIPPED))
                        break;
                    tile->RemoveFlag(TILEFLAG_FLIPPED);
                    world->SendTileUpdate(tile, 0);
                }
            } break;
            default:
                break;
            }
        } break;
        }
        if (packet->m_tile_damage == 0) {
            world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size); });
            return;
        }

        if (tile->IndicateHit() >= base->m_break_hits) {
            packet->m_type = NET_GAME_PACKET_TILE_CHANGE_REQUEST;
            packet->m_item_id = ITEM_FIST;

            if (base->m_item_category & ITEMFLAG2_AUTOPICKUP) {
                if (player->m_inventory.GetItemCount(base->m_id) + 1 > base->m_max_amount) {
                    player->v_sender.OnTalkBubble(player->GetNetId(), "I better not break that, I have no room to pick it up!", true);
                    return;
                }
            }

            switch (base->m_item_type) {
            case ITEMTYPE_LOCK: {
                if (base->IsWorldLock()) {
                    if (!world->IsOwner(player) && player->GetRole() != PLAYER_ROLE_DEVELOPER) {
                        player->v_sender.OnTalkBubble(player->GetNetId(), "You can't break that!", true);
                        return;
                    }
                    world->SetOwnerId(-1);
                    world->SetMainLock(-1);
                    world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                        ply->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
                        ply->v_sender.OnTalkBubble(player->GetNetId(), fmt::format("`5[```w{}`` has had its `$World Lock`` removed!`5]``", world->GetName()), true);
                    });
                }
                int lock_index = position.m_x + position.m_y * world->GetSize().m_x;
                for (auto index = 0; index < world->GetTiles().size(); ++index) {
                    if (index == lock_index)
                        continue;
                    Tile* neighbour = world->GetTile(index);
                    if (neighbour->IsFlagOn(TILEFLAG_LOCKED) && neighbour->GetParent() == lock_index)
                        neighbour->RemoveLock();
                }
                tile->ClearAccess();
                tile->RemoveLock();
            } break;
            case ITEMTYPE_CHECKPOINT: {
                CL_Vec2i main_door = world->GetTilePos(ITEMTYPE_MAIN_DOOR);
                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                    if (!(ply->get_respawn_pos().m_x == position.m_x && ply->get_respawn_pos().m_y == position.m_y))
                        return;
                    ply->set_respawn_pos({ main_door.m_x, main_door.m_y });
                    world->Broadcast([&](const std::shared_ptr<Player>& p) {
                        p->v_sender.SetRespawnPos(ply->GetNetId(), main_door.m_x + main_door.m_y * world->GetSize().m_x, 0);
                    });
                });
            } break;     
            case ITEMTYPE_WEATHER_MACHINE:
            case ITEMTYPE_WEATHER_SPECIAL:
            case ITEMTYPE_WEATHER_SPECIAL2: {
                if (tile->IsFlagOn(TILEFLAG_OPEN) || player->GetRole() == PLAYER_ROLE_DEVELOPER) {
                    world->SetWeatherId(WORLD_WEATHER_SUNNY);
                    world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                        ply->v_sender.OnSetCurrentWeather(world->GetWeatherId());
                    });
                }
            } break;
            case ITEMTYPE_SPOTLIGHT: {
                for (auto& current_star : world->GetPlayers(true)) {
                    if (current_star->GetNetId() == tile->GetOwnerId()) {
                        if (current_star->HasPlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT))
                            current_star->HasPlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT);
                        tile->m_owner_id = 0;
                        break;
                    }
                }
            } break;
            case ITEMTYPE_DISPLAY_BLOCK: {
                if (tile->GetItemId() != ITEM_BLANK) {
                    ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "`wRemove what's in there first!``", true);
                    return;
                }
            } break;
            default:
                break;
            }
            if (!(base->m_editable_type & ITEMFLAG1_DROPLESS)) {
                CL_Vec2f drop_position = { static_cast<float>(position.m_x * 32), static_cast<float>(position.m_y * 32) };
                add_block_objects(player, world, tile, drop_position);
            }
            tile->RemoveBase();
            if (player->GetRole() == PLAYER_ROLE_DEVELOPER && tile->HasDevPunch(player)) {
                tile->DevPunchRemove(player);
            }
            if (base->m_rarity != 999 && base->m_rarity > 0)
                player->SendExperience(world, ((base->m_rarity / 10) + 1) + ((base->m_rarity / 10) + 1));
            if (base->m_item_type == ITEMTYPE_LOCK || base->m_item_type == ITEMTYPE_SECURITY_CAMERA || base->m_item_type == ITEMTYPE_XENONITE || base->m_item_type == ITEMTYPE_WEATHER_MACHINE || base->m_item_type == ITEMTYPE_WEATHER_SPECIAL || (base->m_item_category & ITEMFLAG2_AUTOPICKUP)) {
                player->m_inventory.Add(base->m_id, 1, true);
            }
            else {
                if (base->m_rarity != 999 && base->m_rarity > 0) {
                    CL_Vec2f drop_position2 = { static_cast<float>(position.m_x * 32) + 6, static_cast<float>(position.m_y * 32) + 6 };
                    int min2 = 1;
                    int max2 = std::rand() % base->m_rarity > 50 ? 30 : 132;
                    std::random_device rd2;
                    std::mt19937 rng2(rd2());
                    std::uniform_int_distribution<int> uni2(min2, max2);
                    world->AddGemsObject(uni2(rng2), drop_position2);
                }
            }

            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size);
            });
            return;
        }
        
        world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size); });
    }
}