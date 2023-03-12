#pragma once
#include <world/world.h>
#include <world/world_pool.h>
#include <command/command_manager.h>

namespace GTServer::events {
    void input(EventContext& ctx) {
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        if (ctx.m_parser.size() <= 1)
            return;
        auto splited_text = utils::split(ctx.m_parser.get_data()[1], "|text|");
        if (splited_text.empty() || splited_text.size() < 2)
            return;

        std::string text{ splited_text[1] };
        if (text.find("player_chat=") != std::string::npos)
            return;

        if (text.empty()) {
            return;
        }
        bool prev_is_space = true;
        text.erase(std::remove_if(text.begin(), text.end(), [&prev_is_space](unsigned char curr) {
            bool r = std::isspace(curr) && prev_is_space;
            prev_is_space = std::isspace(curr);
            return r;
        }), text.end());
        if (text.length() <= 1 || text.length() > 120 || text.empty() || (std::all_of(text.begin(), text.end(), [](char c) { return std::isspace(c); })))
            return;

        if (text[0] == '/' && text != "/omg") {
            ctx.m_player->v_sender.OnAction(ctx.m_player->GetNetId(), text);
            ctx.m_player->v_sender.OnAction(ctx.m_player->GetNetId(), splited_text[1]);
            if (!CommandManager::execute(ctx.m_servers, ctx.m_server, ctx.m_player, text)) {
                ctx.m_player->SendLog("`4Unknown command. ``Enter `$/help`` for a list of valid commands.");
            }
            return;
        }
        ctx.m_player->v_sender.OnAction(ctx.m_player->GetNetId(), text);
        ctx.m_player->v_sender.OnAction(ctx.m_player->GetNetId(), splited_text[1]);
        if (ctx.m_player->HasPlaymod(PLAYMOD_TYPE_DUCT_TAPE)) {
            ctx.m_player->SendLog("You can't speak while muted!");
            return;
        }
        if (text[0] == ' ') {
            if (!CommandManager::execute(ctx.m_servers, ctx.m_server, ctx.m_player, text)) {
                ctx.m_player->SendLog("I should actually put a message first!");
            }
            return;
        }
        if (ctx.m_player->GetRole() > PLAYER_ROLE_MODERATOR && !ctx.m_player->HasPlaymod(PLAYMOD_TYPE_NICK)) {
            world->Broadcast([&](const std::shared_ptr<Player>& player) {
                player->v_sender.OnConsoleMessage(fmt::format
                (
                    "CP:0_PL:4_OID:_CT:[W]_ `o<`w{}`o> `5{}",
                    ctx.m_player->GetDisplayName(world), text
                )
                );
            player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), fmt::format("`5{}", text));
                });
        }
        if (ctx.m_player->GetRole() == PLAYER_ROLE_MODERATOR && !ctx.m_player->HasPlaymod(PLAYMOD_TYPE_NICK)) {
            world->Broadcast([&](const std::shared_ptr<Player>& player) {
                player->v_sender.OnConsoleMessage(fmt::format
                (
                    "CP:0_PL:4_OID:_CT:[W]_ `o<`w{}`o> `^{}",
                    ctx.m_player->GetDisplayName(world), text
                )
                );
            player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), fmt::format("`^{}", text));
                });
        }
        if (ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR || (ctx.m_player->HasPlaymod(PLAYMOD_TYPE_NICK) && ctx.m_player->GetRole() >= PLAYER_ROLE_MODERATOR)) {
            world->Broadcast([&](const std::shared_ptr<Player>& player) {
                player->v_sender.OnConsoleMessage(fmt::format
                (
                    "CP:0_PL:4_OID:_CT:[W]_ `o<`w{}`o> `o{}",
                    ctx.m_player->GetDisplayName(world), text
                )
                );
            player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), fmt::format("`w{}", text));
                });
        }
    }
}