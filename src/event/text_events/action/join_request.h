#pragma once
#include <world/world.h>
#include <player/player_pool.h>

namespace GTServer::events {
    void join_request(EventContext& ctx) {
        std::shared_ptr<Player> player{ ctx.m_player };
        std::string world_name = ctx.m_parser.Get("name", 1);
        std::transform(world_name.begin(), world_name.end(), world_name.begin(), ::toupper);
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(world_name) };
        if (player->HasPlaymod(PLAYMOD_TYPE_BAN)) {
            player->v_sender.OnConsoleMessage("You have been banned from BetterGrowtopia.");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (player->GetRole() >= PLAYER_ROLE_MODERATOR) {
            player->SetPunchRange(500);
            player->SetBuildRange(500);
        }
        if (world->IsFlagOn(WORLDFLAG_NUKED) && player->GetRole() < PLAYER_ROLE_MODERATOR) {
            player->v_sender.OnConsoleMessage("Sorry, that world has been removed from BetterGrowtopia to keep our players safe.");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (!player->IsFlagOn(PLAYERFLAG_IS_IN)) {
            player->Disconnect(0U);
            return;
        }
        for (const auto& mod : ctx.m_player->GetPlaymods()) {
            if (mod.m_time.GetPassedTime() >= mod.m_time.GetTimeout() && mod.m_time.GetTimeout() != std::chrono::seconds(-1) && mod.m_type != PLAYMOD_TYPE_GHOST_IN_THE_SHELL && mod.m_type != PLAYMOD_TYPE_NICK && mod.m_type != PLAYMOD_TYPE_INVISIBLE) {
                ctx.m_player->RemovePlaymod(mod.m_type);
            }
        }
        if (world->HasBan(player) && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("`4Oops`o, you are currently banned from that world, come back to that world soon!");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }

        if (world_name.empty())
            world_name = std::string{ "START" };

        if (world_name.length() > 24) {
            player->v_sender.OnConsoleMessage("Sorry, the world name is too long!");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }

        if (world_name == "EXIT" || !std::regex_match(world_name, std::regex{ "^[A-Z0-9]+$" })) {
            player->v_sender.OnConsoleMessage("Sorry, spaces and special characters are not allowed in world or door names. Try again.");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if ((world_name == "QUIZZI" || world_name == "SCAMMER" || world_name == "KAAN" || world_name == "OWNER" || world_name == "HARRY" || world_name == "BETTERGROWTOPIA") && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("Sorry, that world is reserved.");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (world_name.length() <= 2 && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("Sorry, worlds that are 2 letters or less are currently locked right now! ");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (world_name.find("BUY", 0) == 0 && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("Sorry, BUY worlds are currently locked right now! ");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (world_name.find("SELL", 0) == 0 && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("Sorry, SELL worlds are currently locked right now! ");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (player->HasPlaymod(PLAYMOD_TYPE_CURSE) && world_name != "HELL") {
            player->v_sender.OnConsoleMessage("You are only able to go to HELL since you broke the rules!");
            std::shared_ptr<World> worldHELL{ world_pool->GetWorld("HELL")};
            world = worldHELL;
        }

        if (!world) {
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (world->HasPlayer(player)) {
            world_pool->OnPlayerLeave(world, player, false);
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        world_pool->OnPlayerJoin(ctx.m_servers, world, player, world->GetTilePos(ITEMTYPE_MAIN_DOOR));
        world_pool->OnPlayerSyncing(world, player);
        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
        if (ply->GetRawName() != player->GetRawName() && ply->HasPlaymod(PLAYMOD_TYPE_INVISIBLE)) {
            if (player->GetRole() >= ply->GetRole()) {
                player->v_sender.OnInvis(ply->GetNetId(), 0, 0);
                player->v_sender.OnNameChanged(ply->GetNetId(), fmt::format("{} (V)", ply->GetDisplayName(world)));
            }
            else {
                player->v_sender.OnInvis(ply->GetNetId(), 0, 1);
            }
        }
            });
        if (player->HasPlaymod(PLAYMOD_TYPE_INVISIBLE)) {
            player->v_sender.OnInvis(player->GetNetId(), 0, 0);
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->v_sender.OnInvis(player->GetNetId(), 0, 0);
                });
            player->v_sender.OnInvis(player->GetNetId(), 0, 1);

            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                if (ply->GetRawName() != player->GetRawName()) {
                    if (ply->GetRole() < player->GetRole()) {
                        ply->v_sender.OnInvis(player->GetNetId(), 0, 1);
                    }
                    else {
                        ply->v_sender.OnInvis(player->GetNetId(), 0, 0);
                        ply->v_sender.OnNameChanged(player->GetNetId(), fmt::format("{} (V)", player->GetDisplayName(world)));
                    }
                }
                });
            WorldTable* db{ (WorldTable*)Database::GetTable(Database::DATABASE_WORLD_TABLE) };
            if (!db->save(world))
                fmt::print("WorldTable::save, Failed to save {}\n", world->GetName());
            world->SyncPlayerData(player);
        }
        else {
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
                });
        }
        if (player->CharacterState::IsFlagOn(STATEFLAG_PINEAPPLE_FLAG)) {
            player->CharacterState::RemoveFlag(STATEFLAG_PINEAPPLE_FLAG);
        }
        player->v_sender.OnSetClothing(player->GetClothes(), player->GetSkinColor(), true, player->GetNetId());
        player->SendCharacterState(player);
        player->v_sender.OnSetClothing(player->GetClothes(), player->GetSkinColor(), true, player->GetNetId());
    }
}