#pragma once

namespace GTServer::events {
    void refresh_item_data(EventContext& ctx) {
        ctx.m_player->SendLog("One moment, updating item data...");

        auto* packet = ItemDatabase::GetPacket();
        ctx.m_player->SendPacket(NET_MESSAGE_GAME_PACKET, packet, sizeof(GameUpdatePacket) + packet->m_data_size);
    }
}