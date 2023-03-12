#include <discord/discord_bot.h>
#include <cstdlib>
#include <stdio.h>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <server/server_pool.h>
#include <database/database.h>
#include <utils/text.h>
#include <utils/timing_clock.h>

namespace GTServer {
    DiscordBot::~DiscordBot() {
        delete m_vanguard;
    }

    void DiscordBot::initialize(const BotConfiguration& settings, std::shared_ptr<ServerPool> server_pool) {
        m_server_pool = server_pool;
        m_vanguard = new dpp::cluster(settings.m_vanguard_token, dpp::i_all_intents);

        m_vanguard->on_message_create([this](const dpp::message_create_t& event) { this->on_message_create(BOT_TYPE_VANGUARD, event); });
        m_vanguard->on_interaction_create([this](const dpp::interaction_create_t& event) { this->on_interaction_create(event); });
        m_vanguard->on_button_click([this](const dpp::button_click_t & event) { this->on_button_click(event); } );
        m_vanguard->on_ready([&](const dpp::ready_t& event) { fmt::print("DiscordBot::Initialize -> BetterGrowtopia Vanguard [ShardID: {}] is now ready!\n", event.shard_id); });
        std::thread([&] { m_vanguard->start(true); while(true); }).detach();
    }

    void DiscordBot::on_commands_register() { }
    void DiscordBot::on_message_create(const eBotType& type, const dpp::message_create_t& event) {
        auto content = event.msg.content;
        std::transform(content.begin(), content.end(), content.begin(), ::tolower);
        switch (type) {
        case BOT_TYPE_VANGUARD: {
            if (content.find(";serverinfo") != std::string::npos) {
                auto arguments = utils::split(event.msg.content, " ");
                if (arguments.size() < 2) {
                    dpp::message reply{}; dpp::embed embed{};
                    embed.set_title("BetterGrowtopia")
                        .set_color(0)
                        .set_footer(fmt::format("BetterGrowtopia - {}", system_clock::now()), m_vanguard->me.get_avatar_url())
                        .add_field("Active Servers", std::to_string(m_server_pool->GetActiveServers()), true)
                        .add_field("Active Worlds", std::to_string(m_server_pool->GetActiveWorlds()), true)
                        .add_field("Active Players", std::to_string(m_server_pool->GetActivePlayers()), true)
                        .add_field("Uptime", "Since you were born") // lol
                        .add_field("Server Load", "0.24 0.17 0.08")
                        .set_thumbnail("https://i.imgur.com/EQdgGwV.jpg");
                    reply.add_embed(embed);
                    event.reply(reply);
                    return;
                }
                uint8_t server_id = std::atoi(arguments[1].c_str());
                if (server_id < 1 || server_id > m_server_pool->GetActiveServers())
                    return;
                auto server = m_server_pool->GetServers()[server_id - 1];
                dpp::message reply{}; dpp::embed embed{};
                embed.set_title(fmt::format("BetterGrowtopia (Server - #{})", server->GetInstanceId() + 1))
                    .set_color(0)
                    .set_footer(fmt::format("BetterGrowtopia - {}", system_clock::now()), m_vanguard->me.get_avatar_url());
                auto active_worlds = server->GetWorldPool()->GetWorlds();
                if (active_worlds.size() > 10) {
                    embed.add_field("Active Worlds", std::to_string(active_worlds.size()), true);
                } else { 
                    if (active_worlds.size() > 0) {
                        std::vector<std::string> ret{};
                        for (auto& [name, world] : active_worlds)
                            ret.push_back(name);
                        embed.add_field("Active Worlds", fmt::format("{}", ret));
                    } else {
                        embed.add_field("Active Worlds", "None");
                    }
                }
                auto active_players = server->GetPlayerPool()->GetPlayers();
                if (active_players.size() > 10) {
                    embed.add_field("Active Players", std::to_string(active_players.size()), true);
                } else { 
                    if (active_players.size() > 0) {
                        std::vector<std::pair<uint32_t, std::string>> ret{};
                        for (auto& [connect_id, player] : active_players)
                            ret.push_back({ player->GetUserId(), player->GetRawName() });
                        embed.add_field("Active Players", fmt::format("{}", ret));
                    } else {
                        embed.add_field("Active Players", "None");
                    }
                }
                embed.set_thumbnail("https://i.imgur.com/EQdgGwV.jpg");
                reply.add_embed(embed);
                event.reply(reply);
        } 
        } break;
        }
    }
    void DiscordBot::on_interaction_create(const dpp::interaction_create_t& event) { }
    void DiscordBot::on_button_click(const dpp::button_click_t & event) {
        auto content = event.custom_id;
        if (content.find("verify_account_") != std::string::npos) {
            dpp::snowflake client_id = dpp::snowflake(std::stoull(content.substr(15)));
            auto* client = dpp::find_user(client_id);
            if (!client)
                return;
            auto& token_cache = this->m_server_pool->m_account_verify;
            if (token_cache.find(client->id) == token_cache.end())
                return;
            auto& [user_id, time] = token_cache[client->id];
            if (time.GetPassedTime() > time.GetTimeout()) {
                event.reply(dpp::interaction_response_type::ir_update_message, "Sorry, this account verification message has been expired, please request a new one.");
                return;
            }
            auto player = this->m_server_pool->GetPlayerByUserID(user_id);
            player->SetDiscord(static_cast<uint64_t>(client->id));
            if(((PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE))->Save(player))
                event.reply(dpp::interaction_response_type::ir_update_message, 
                fmt::format("Successfully, linked your discord account with **BetterGrowtopia** account (UID: {} | Name: {})", player->GetUserId(), player->GetRawName()));
            m_vanguard->message_create(dpp::message(dpp::snowflake(1022078096964341821), fmt::format("Discord Account **{}** is now linked with GrowID **{}**", client->format_username(), player->GetRawName())));
            token_cache.erase(client_id);
            if (!this->m_server_pool->HasPlayer(user_id)) {
                player.reset();
                return;
            }
            DialogBuilder db{};
            db.set_default_color('o')
                ->add_label_with_icon("`wAccount Verification``", ITEM_WARNING_BLOCK, DialogBuilder::LEFT, DialogBuilder::BIG)
                ->add_spacer()
                ->add_textbox(fmt::format("Successfully, linked your account with `w{}`` (ClientID: `w{}``)", client->format_username(), static_cast<uint64_t>(client->id)))
                ->end_dialog("account_verify", "`wOkay``", "");
            player->v_sender.OnDialogRequest(db.get(), 0); 
        }
    }
}