#pragma once
#include <string>
#include <dpp/dpp.h>

namespace GTServer {
    struct BotConfiguration {
        dpp::snowflake m_guild_id;
        std::string m_vanguard_token;
    };
}