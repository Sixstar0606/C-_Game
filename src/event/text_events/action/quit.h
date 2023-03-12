#pragma once

namespace GTServer::events {
    void quit(EventContext& ctx) {
        ctx.m_player->Disconnect(0U);
    }
}