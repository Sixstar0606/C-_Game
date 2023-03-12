#pragma once
#include <regex>
#include <event/event_pool.h>
#include <proton/utils/text_scanner.h>
#include <utils/random.h>
#include <player/objects/enums.h>
#include <player/objects/login_information.h>
#include <database/database.h>
#include <database/table/player_table.h>

namespace GTServer::events {
    void requested_name(EventContext& ctx) {
        auto login_info{ ctx.m_player->GetLoginDetail() };
        if (!ctx.m_parser.TryGet<int32_t>("platformID", login_info->m_platform) ||
            !ctx.m_parser.TryGet("country", login_info->m_country) ||
            ctx.m_parser.Get("meta", 1).empty() || 
            ctx.m_parser.Get("requestedName", 1).empty() ||
            ctx.m_player->IsFlagOn(PLAYERFLAG_LOGGED_ON)) {
            ctx.m_player->Disconnect(0U);
            return;
        }
        ctx.m_player->SetRawName(ctx.m_parser.Get("requestedName", 1));
        ctx.m_player->SetDisplayName(ctx.m_parser.Get("requestedName", 1));

        std::regex regex{ "^[a-zA-Z0-9]+$" };
        if (!std::regex_match(ctx.m_player->GetRawName(), regex)) {
            ctx.m_player->SendLog("`4Oops! `oYour name is including invalid characters, please try again.``");
            ctx.m_player->Disconnect(0U);
            return;
        }
         if (!login_info->Serialize(ctx.m_parser)) {
            ctx.m_player->Disconnect(0U);
            return;
        }
        ctx.m_player->SendDialog(Player::DIALOG_TYPE_REGISTRATION, TextScanner{});

    }
}