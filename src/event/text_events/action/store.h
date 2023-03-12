#pragma once
#include <store/store_manager.h>

namespace GTServer::events {
    void store(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_LOGGED_ON))
            return;
        StoreManager::Send(ctx.m_player, STOREMENU_TYPE_NONE);
    }
}