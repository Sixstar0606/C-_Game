#pragma once
#include <fmt/core.h>
#include <world/world_pool.h>
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace GTServer::events {
    void enter_game(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_LOGGED_ON))
            return;

        for (const auto& mod : ctx.m_player->GetPlaymods()) {
            if (mod.m_time.GetPassedTime() >= mod.m_time.GetTimeout() && mod.m_time.GetTimeout() != std::chrono::seconds(-1) && mod.m_type != PLAYMOD_TYPE_GHOST_IN_THE_SHELL && mod.m_type != PLAYMOD_TYPE_NICK && mod.m_type != PLAYMOD_TYPE_INVISIBLE) {
                ctx.m_player->RemovePlaymod(mod.m_type);
            }
        }
        if (ctx.m_player->HasPlaymod(PLAYMOD_TYPE_BAN) && ctx.m_player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            ctx.m_player->SendLog("`oOops, you are currently `4BANNED `ofrom BetterGrowtopia, join when you have been unbanned.");
            std::ifstream input_file(fmt::format("bans/{}.txt", ctx.m_player->GetRawName()));
            if (input_file.is_open()) {
                std::string line;
                while (std::getline(input_file, line)) {
                    ctx.m_player->SendLog(fmt::format("`4Reason: `o{}", line));
                }
                input_file.close();
            }
            return;
        }

        ctx.m_player->SetFlag(PLAYERFLAG_IS_IN);
        std::string folder_path = fmt::format("PlayerData/{}", ctx.m_player->GetRawName());
        std::string file_name = "PlayerData.txt";
        std::string file_path = folder_path + "/" + file_name;

        if (!fs::is_directory(folder_path)) {
            fs::create_directory(folder_path);
        }
        if (!fs::exists(file_path)) {
            std::ofstream outfile(file_path);
            outfile.close();
        }

        std::ifstream infile(file_path);
        std::string line;
        std::string custom_nickname;
        bool custom_nickname_found = false;
        while (std::getline(infile, line)) {
            if (line.find("CustomNickname:") == 0) {
                custom_nickname = line.substr(16);
                custom_nickname_found = true;
                break;
            }
        }
        infile.close();

        if (!custom_nickname_found) {
            std::ofstream outfile(file_path, std::ios::app);
            outfile << "CustomNickname: " << ctx.m_player->GetDisplayName() << std::endl;
            outfile.close();
            custom_nickname = custom_nickname;
        }

        infile.open(file_path);
        while (std::getline(infile, line)) {
            if (line.find("CustomNickname:") == 0) {
                custom_nickname = line.substr(16);
                break;
            }
        }
        infile.close();
        std::string friends_file_path = std::format("PlayerData/{}/friends.txt", ctx.m_player->GetUserId());

        // Check if the file exists and create it if it doesn't
        std::ifstream friends_file(friends_file_path);
        if (!friends_file.good()) {
            std::ofstream new_file(friends_file_path);
            new_file.close();
        }

        // Re-open the file for reading
        friends_file.open(friends_file_path);

        std::string global_variables_file = "GlobalGameVariables.txt";
        std::ofstream global_variables(global_variables_file, std::ios::app);
        std::ifstream check_file(global_variables_file);
        bool recent_sb_location_exists = false;
        if (check_file.is_open()) {
            std::string line;
            while (std::getline(check_file, line)) {
                if (line.find("RecentSBLocation:") == 0) {
                    recent_sb_location_exists = true;
                    break;
                }
            }
            check_file.close();
        }
        if (!recent_sb_location_exists) {
            global_variables << "RecentSBLocation: START\n";
        }

        ctx.m_player->SetDisplayName(custom_nickname);
        ctx.m_player->SendLog("`oWelcome back ``{}`o, BetterGrowtopia `wV{}``", 
            custom_nickname,
            SERVER_VERSION);
        PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
        const auto& result{ database->Save(ctx.m_player) };

        int32_t supporter_ranks = 0;
        if (ctx.m_player->GetRole() >= PLAYER_ROLE_VIP)
            supporter_ranks |= (1 << 0);
        
        ctx.m_server->GetWorldPool()->SendDefaultOffers(ctx.m_player);
        ctx.m_player->v_sender.OnSetBux(ctx.m_player->GetGems(), supporter_ranks);
        ctx.m_player->set_last_active(system_clock::now());

        ctx.m_player->SendDialog(Player::DIALOG_TYPE_NEWS, TextScanner{});

        if (ctx.m_player->GetDiscord() == 0)
            ctx.m_player->SendDialog(Player::DIALOG_TYPE_ACCOUNT_VERIFY, TextScanner{});
    }
}