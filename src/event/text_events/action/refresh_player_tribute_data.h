#pragma once
#include <database/player_tribute.h>

namespace GTServer::events {
    void refresh_player_tribute_data(EventContext& ctx) {
        ctx.m_player->SendLog("One moment, updating player tribute data...");

        const auto& pair = PlayerTribute::get().get_packet();
        ctx.m_player->SendPacket(pair.second, sizeof(TankUpdatePacket) + sizeof(GameUpdatePacket) + pair.first);
    }
}