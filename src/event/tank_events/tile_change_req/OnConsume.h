#pragma once
#include <random>
#include <command/command_manager.h>
#include <ranges>
#include <fmt/core.h>
#include <utils/text.h>
#include <server/server_pool.h>
#include <world/world.h>

namespace GTServer::events::tile_change_req {
    void OnConsume(EventContext& ctx, std::shared_ptr<World> world, CL_Vec2i position) {
        std::shared_ptr<Player> player{ ctx.m_player };
        if (!player->m_inventory.Contain(ctx.m_update_packet->m_item_id))
            return;
        Tile* tile{ world->GetTile(position.m_x, position.m_y) };
        if (!tile)
            return;

        std::shared_ptr<Player> closestPlayer;
        int minDistance = std::numeric_limits<int>::max();
        for (const auto& player2 : world->GetPlayers(false)) {
            CL_Vec2i playerPosition = player2->GetPosition();
            int distance = sqrt(std::pow(playerPosition.m_x - tile->GetPosition().m_x, 2) + std::pow(playerPosition.m_y - tile->GetPosition().m_y, 2));
            if (distance < minDistance) {
                minDistance = distance;
                closestPlayer = player2;
            }
        }
        ItemInfo* item{ ItemDatabase::GetItem(ctx.m_update_packet->m_item_id) };
        ItemInfo* base{ tile->GetBaseItem() };
        if (!item || !base)
            return;

        switch (item->m_id)
        {
        case ITEM_GROW_SPRAY_FERTILIZER:
        case ITEM_MY_FIRST_GROW_SPRAY_FERTILIZER:
        case ITEM_DELUXE_GROW_SPRAY:
        case ITEM_ULTRA_GROW_SPRAY: {
            if (!(base->m_item_type == ITEMTYPE_SEED || base->m_item_type == ITEMTYPE_PROVIDER) || base->m_id == ITEM_MAGIC_EGG) {
                player->v_sender.OnTalkBubble(player->GetNetId(), "Use this on a growing tree or provider to speed it's growth.", true);
                return;
            }
            if ((high_resolution_clock::now() - tile->GetPlantedDate()) >= std::chrono::seconds(base->m_grow_time)) {
                player->v_sender.OnTalkBubble(player->GetNetId(), fmt::format("`wThis {}, don't waste your spray on it!``", base->m_item_type == ITEMTYPE_SEED ? "tree has already bloomed" : fmt::format("{} is ready to collect", base->m_name)), true);
                return;
            }
            if (!player->m_inventory.Erase(item->m_id, 1, true))
                return;
            std::string spray_text;
            uint32_t spray_seconds;
            switch (item->m_id) {
            case ITEM_GROW_SPRAY_FERTILIZER:
            case ITEM_MY_FIRST_GROW_SPRAY_FERTILIZER:
            { spray_text = "1 `whour"; spray_seconds = 3600; } break;
            case ITEM_DELUXE_GROW_SPRAY:
            { spray_text = "24 `whours"; spray_seconds = 86400; } break;
            case ITEM_ULTRA_GROW_SPRAY:
            { spray_text = "30 `wdays"; spray_seconds = 2592000; } break;
            default:
                return;
            }
            tile->m_planted_date = tile->GetPlantedDate() - std::chrono::seconds(spray_seconds);
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->v_sender.OnPlayPositioned("spray", player->GetNetId());
                });
            world->SendTileUpdate(tile, 0);

            player->v_sender.OnTextOverlay(fmt::format("`w{} aged `${}``", fmt::format("{}", base->m_item_type == ITEMTYPE_SEED ? fmt::format("{} Tree", ItemDatabase::GetItem(base->m_id - 1)->m_name) : base->m_name), spray_text));
        } break;
        case ITEM_BEACH_BLAST: {

        } break;
        case ITEM_BAN_WAND: {
            int distance2 = sqrt((closestPlayer->GetPosition().m_x - closestPlayer->GetPosition().m_x) ^ 2 + (tile->GetPosition().m_y - tile->GetPosition().m_y) ^ 2);
            if (distance2 < 2) {
                if (!player->m_inventory.Erase(item->m_id, 1, true))
                    return;
                closestPlayer->AddPlaymod(PLAYMOD_TYPE_BAN, ITEM_BAN_WAND, steady_clock::now(), std::chrono::seconds(300));
                std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                for (auto& playerthing2 : ctx.m_servers->GetPlayers()) {
                    playerthing2->SendLog("`5** `$The Ancient Ones `ohave `4banned`o {} `5** `w(`4/rules`o to see the rules!)", closestPlayer->GetDisplayName(world));
                }
                auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                std::ofstream ban_log_file("banlogs.txt", std::ios_base::app);
                ban_log_file << closestPlayer->GetRawName() << " was banned (BAN WAND) by " << player->GetRawName() << " at " << std::put_time(std::localtime(&now), "%c %Z") << std::endl;
                fmt::print("{} was banned (BAN WAND) by {}\n", closestPlayer->GetRawName(), player->GetRawName());

                PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                const auto& result{ database->Save(closestPlayer) };
                closestPlayer->PlaySfx("already_used", 0);
                closestPlayer->PlaySfx("hub_open", 0);
                closestPlayer->PlaySfx("bgt_ban", 0);
                closestPlayer->Disconnect(0U);
                closestPlayer->SendLog("`oWarning from `4System`o: You've been `4BANNED `ofrom `wBetterGrowtopia`o.");
                closestPlayer->v_sender.OnAddNotification("`wWarning from `4System``: You've been `4BANNED`` from `wBetterGrowtopia for 5 minutes!", "interface/atomic_button.rttex", "audio/hub_open.wav");
            }

        } break;
        case ITEM_DUCT_TAPE: {
            int distance2 = sqrt((closestPlayer->GetPosition().m_x - closestPlayer->GetPosition().m_x) ^ 2 + (tile->GetPosition().m_y - tile->GetPosition().m_y) ^ 2);
            if (distance2 < 2 && !closestPlayer->HasPlaymod(PLAYMOD_TYPE_DUCT_TAPE)) {
                if (!player->m_inventory.Erase(item->m_id, 1, true))
                    return;
                std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                for (auto& playerthing2 : ctx.m_servers->GetPlayers()) {
                    playerthing2->SendLog("`5** `$The Ancient Ones `ohave `bduct-taped`o {} `5** `w(`4/rules`o to see the rules!)", closestPlayer->GetDisplayName(world));
                }
                closestPlayer->AddPlaymod(PLAYMOD_TYPE_DUCT_TAPE, ITEM_DUCT_TAPE, steady_clock::now(), std::chrono::seconds(300));

                closestPlayer->SetCloth(CLOTHTYPE_FACE, ITEM_DUCT_TAPE, false);
                world->Broadcast([&](const std::shared_ptr<Player>& player2) {
                    player2->v_sender.OnSetClothing(closestPlayer->GetClothes(), closestPlayer->GetSkinColor(), false, closestPlayer->GetNetId());
                    });
                closestPlayer->PlaySfx("already_used", 0);
                closestPlayer->PlaySfx("hub_open", 0);
                closestPlayer->PlaySfx("bgt_ban", 0);
                closestPlayer->SendLog("Warning from `4System``: You've been `4duct-taped`` `ofor 5 minutes.");
                closestPlayer->v_sender.OnAddNotification("`wWarning from `4System``: You've been `4duct-taped`` for 5 minutes!", "interface/atomic_button.rttex", "audio/hub_open.wav");

                PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                const auto& result{ database->Save(closestPlayer) };
            }
        } break;
        case ITEM_FREEZE_WAND: {
            int distance2 = sqrt((closestPlayer->GetPosition().m_x - closestPlayer->GetPosition().m_x) ^ 2 + (tile->GetPosition().m_y - tile->GetPosition().m_y) ^ 2);
            if (distance2 < 2 && !closestPlayer->HasPlaymod(PLAYMOD_TYPE_FROZEN)) {
                if (!player->m_inventory.Erase(item->m_id, 1, true))
                    return;
                closestPlayer->AddPlaymod(PLAYMOD_TYPE_FROZEN, ITEM_FREEZE_WAND, steady_clock::now(), std::chrono::seconds(10));
                closestPlayer->SendCharacterState(closestPlayer);

                world->Broadcast([&](const std::shared_ptr<Player>& player) {
                    player->v_sender.OnSetClothing(closestPlayer->GetClothes(), closestPlayer->GetSkinColor(), false, closestPlayer->GetNetId());
                    });

                PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                const auto& result{ database->Save(closestPlayer) };
            }
        } break;
        case ITEM_CURSE_WAND: {
            int distance2 = sqrt((closestPlayer->GetPosition().m_x - closestPlayer->GetPosition().m_x) ^ 2 + (tile->GetPosition().m_y - tile->GetPosition().m_y) ^ 2);
            if (distance2 < 2 && !closestPlayer->HasPlaymod(PLAYMOD_TYPE_CURSE)) {
                if (!player->m_inventory.Erase(item->m_id, 1, true))
                    return;

                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };

                closestPlayer->AddPlaymod(PLAYMOD_TYPE_CURSE, ITEM_CURSE_WAND, steady_clock::now(), std::chrono::seconds(600));

                auto world2{ ctx.m_server->GetWorldPool()->GetWorld("HELL")};
                // world->RemovePlayer(player);
                ctx.m_server->GetWorldPool()->OnPlayerLeave(world, closestPlayer, false);
                // player->SetNetId(world2->AddPlayer(player));
                // world2->AddPlayer(player);
                ctx.m_server->GetWorldPool()->OnPlayerJoin(ctx.m_servers, world2, closestPlayer, world2->GetTilePos(ITEMTYPE_MAIN_DOOR));
                ctx.m_server->GetWorldPool()->OnPlayerSyncing(world2, closestPlayer);

                PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                const auto& result{ database->Save(closestPlayer) };
            }
        } break;
        case ITEM_WATER_BUCKET: {
            if (tile->GetBaseItem()->m_id == ITEM_BLANK)
                return;
            if (!player->m_inventory.Erase(item->m_id, 1, true))
                return;
            if (tile->IsFlagOn(TILEFLAG_FIRE)) {
                tile->RemoveFlag(TILEFLAG_FIRE);
                world->SendTileUpdate(tile, 0);
                return;
            }
            else {
                if (tile->IsFlagOn(TILEFLAG_WATER)) {
                    tile->RemoveFlag(TILEFLAG_WATER);
                    world->SendTileUpdate(tile, 0);
                    return;
                }
                else {
                    tile->SetFlag(TILEFLAG_WATER);
                }
            }
            world->SendTileUpdate(tile, 0);
        } break;
        case ITEM_POCKET_LIGHTER: {
            if (base->m_id == ITEM_BLANK || base->m_item_type == ITEMTYPE_LOCK || base->m_item_type == ITEMTYPE_MAIN_DOOR || base->m_item_type == ITEMTYPE_BEDROCK) {
                player->v_sender.OnTalkBubble(player->GetNetId(), "`wThere's nothing to burn!``", true);
                return;
            }
            else if (tile->IsFlagOn(TILEFLAG_WATER)) {
                player->v_sender.OnTalkBubble(player->GetNetId(), "`wYou can't burn water.``", true);
                return;
            }
            else if (tile->IsFlagOn(TILEFLAG_FIRE))
                return;
            if (!player->m_inventory.Erase(item->m_id, 1, true))
                return;
            tile->SetFlag(TILEFLAG_FIRE);

            GameUpdatePacket effect_packet{
                .m_type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
                .m_particle_alt_id = 150
            };
            effect_packet.m_pos_x = (position.m_x * 32) + 15;
            effect_packet.m_pos_y = (position.m_y * 32) + 15;

            world->SendTileUpdate(tile, 0);
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->SendPacket(NET_MESSAGE_GAME_PACKET, &effect_packet, sizeof(GameUpdatePacket));
            ply->v_sender.OnConsoleMessage("`7[```4MWAHAHAHA!! FIRE FIRE FIRE```7]``");
            ply->v_sender.OnTalkBubble(player->GetNetId(), "`7[```4MWAHAHAHA!! FIRE FIRE FIRE```7]``");
                });
        } break;
        case ITEM_ELDRITCH_FLAME: {
            if (base->m_id == ITEM_BLANK) {
                player->v_sender.OnTalkBubble(player->GetNetId(), "`wThere's nothing to burn!``", true);
                return;
            }
            if (!player->m_inventory.Erase(item->m_id, 1, true))
                return;
            int index = position.m_x + position.m_y * world->GetSize().m_x;

            std::vector<int> tiles = {
                index - world->GetSize().m_x,
                index - (world->GetSize().m_x * 2),
                index,
                index + world->GetSize().m_x,
                index + (world->GetSize().m_x * 2),
                index - 1,
                index - 2,
                index + 1,
                index + 2,
                (index - world->GetSize().m_x) - 1,
                (index - world->GetSize().m_x) - 2,
                (index - (world->GetSize().m_x * 2)) - 1,
                (index - (world->GetSize().m_x * 2)) - 2,
                (index - world->GetSize().m_x) + 1,
                (index - world->GetSize().m_x) + 2,
                (index - (world->GetSize().m_x * 2)) + 1,
                (index - (world->GetSize().m_x * 2)) + 2,
                (index + world->GetSize().m_x) - 1,
                (index + world->GetSize().m_x) - 2,
                (index + (world->GetSize().m_x * 2)) - 1,
                (index + (world->GetSize().m_x * 2)) - 2,
                (index + world->GetSize().m_x) + 1,
                (index + world->GetSize().m_x) + 2,
                (index + (world->GetSize().m_x * 2)) + 1,
                (index + (world->GetSize().m_x * 2)) + 2,
            };

            auto rd = std::random_device{};
            auto rng = std::default_random_engine{ rd() };
            std::shuffle(std::begin(tiles), std::end(tiles), rng);
            std::vector<Tile*> packs{};

            for (int i = 0; i < 10; i++) {
                int ind = tiles[i];
                if (ind < 0 || ind > world->GetTiles().size())
                    continue;
                auto* tile = world->GetTile(ind);
                ItemInfo* item2 = tile->GetBaseItem();
                if (!tile)
                    return;
                if (item2->m_item_type != ITEMTYPE_LOCK && item2->m_item_type != ITEMTYPE_MAIN_DOOR && item2->m_item_type != ITEMTYPE_BEDROCK && item2->m_id != ITEM_BLANK) {
                    tile->SetFlag(TILEFLAG_FIRE);
                    packs.push_back(tile);

                    GameUpdatePacket effect_packet{
                        .m_type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
                        .m_particle_alt_id = 150
                    };
                    effect_packet.m_pos_x = (tile->GetPosition().m_x * 32) + 15;
                    effect_packet.m_pos_y = (tile->GetPosition().m_y * 32) + 15;

                    world->Broadcast([&](const std::shared_ptr<Player>& ply) { ply->SendPacket(NET_MESSAGE_GAME_PACKET, &effect_packet, sizeof(GameUpdatePacket)); });
                }
            }
            world->SendTileUpdate(packs);
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->v_sender.OnConsoleMessage("`7[```4BURN, PUNY MORTALS!```7]``");
            ply->v_sender.OnTalkBubble(player->GetNetId(), "`7[```4BURN, PUNY MORTALS!```7]``");
                });
        } break;
        default:
            break;
        }
    }
}