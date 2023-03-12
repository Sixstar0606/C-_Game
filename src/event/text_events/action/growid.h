#pragma once
#include <proton/utils/dialog_builder.h>

namespace GTServer::events {
    void growid(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_IS_IN)) {
            ctx.m_player->SendLog("TankID: \"{}\"", ctx.m_player->GetLoginDetail()->m_tank_id_name);
            return;
        }
        ctx.m_player->SendDialog(Player::DIALOG_TYPE_REGISTRATION, TextScanner{});
        return;
    }
}