#pragma once
#include <world/world.h>
#include <world/world_pool.h>

namespace GTServer::events {
    void friends(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_LOGGED_ON))
            return;
        ctx.m_player->SendDialog(Player::DIALOG_TYPE_SOCIAL_PORTAL, TextScanner{});
    }
}