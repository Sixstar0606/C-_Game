#pragma once

namespace GTServer::events {
    void tank_id_name(EventContext& ctx) {
        auto login_info{ ctx.m_player->GetLoginDetail() };
        if (!ctx.m_parser.TryGet<int32_t>("platformID", login_info->m_platform) ||
            !ctx.m_parser.TryGet("country", login_info->m_country) ||
            ctx.m_parser.Get("meta", 1).empty() || 
            ctx.m_parser.Get("requestedName", 1).empty() ||
            ctx.m_parser.Get("tankIDName").empty() ||
            ctx.m_parser.Get("tankIDPass").empty() ||
            ctx.m_player->IsFlagOn(PLAYERFLAG_LOGGED_ON)) {
            ctx.m_player->Disconnect(0U);
            return;
        }
        auto name = ctx.m_parser.Get("tankIDName", 1);
        auto lower_name = name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        ctx.m_player->SetRawName(name);
        ctx.m_player->SetDisplayName(name);
        login_info->m_tank_id_name = lower_name;
        login_info->m_tank_id_pass = ctx.m_parser.Get("tankIDPass", 1);

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
        ctx.m_server->AddQueue(SERVERQUEUE_TYPE_LOGIN, ServerQueue{ .m_player = ctx.m_player });
    }
}