#pragma once
#include <functional>
#include <string>
#include <player/player.h>
#include <server/server.h>

namespace GTServer {
    class ServerPool;
    class Database;
    class Command;
    struct CommandContext {
        std::shared_ptr<Player> m_player;
        std::shared_ptr<Server> m_server;
        ServerPool* m_servers;

        Command* m_command;
        std::string m_message;
        std::vector<std::string> m_arguments; 
    };
}