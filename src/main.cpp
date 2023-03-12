#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <thread>
#include <vector>
#include <filesystem>
#include <fmt/chrono.h>

#include <discord/discord_bot.h>
#include <database/database.h>
#include <database/item/item_database.h>
#include <event/event_pool.h>
#include <server/http.h>
#include <server/server.h>
#include <server/server_pool.h>
#include <store/store_manager.h>
#include <render/world_render.h>
#include <command/command_manager.h>

using namespace GTServer;
std::shared_ptr<ServerPool> g_servers;
std::shared_ptr<EventPool> g_events;

int main() {
    fmt::print("starting {} V{}\n", SERVER_NAME, SERVER_VERSION);
    if (!std::filesystem::is_directory(config::server::worlds_dir))
        std::filesystem::create_directory(config::server::worlds_dir);
    if (!std::filesystem::is_directory(config::server::utils_dir))
        std::filesystem::create_directory(config::server::utils_dir);
    if (!std::filesystem::is_directory(config::server::renders_dir))
        std::filesystem::create_directory(config::server::renders_dir);
        
#ifdef HTTP_SERVER
    auto http_server{ std::make_unique<HTTPServer>(
        std::string{ config::http::address.begin(), config::http::address.end() }, 
        config::http::port
    ) };
    if (!http_server->listen())
        fmt::print("failed to starting http server, please run an external http service.\n");
#endif

    Database& database{ Database::Get() };
    if (!database.Connect()) {
        fmt::print(" - failed to connect MySQL server, please check server configuration.\n");
        return EXIT_FAILURE;
    }

    PlayerTribute& player_tribute{ PlayerTribute::get() }  ;
    if (!player_tribute.build())
        fmt::print(" - failed to build PlayerTribute\n");
    else 
        fmt::print(" - PlayerTribute is built with hash {}\n", player_tribute.get_hash());

    ItemDatabase& items{ ItemDatabase::Get() };
    if (!items.Serialize()) {
        fmt::print("ItemDatabase::Serialize -> failed to load items.dat, please make sure the file is on /cache\n");
        return EXIT_FAILURE;
    }
    items.ModifyIOSSupport();
    items.Encode();
    items.Kill();
    if (!items.Serialize()) {
        fmt::print(" - failed to serialization items.dat\n");
    } else {
        items.SerializeDetails();
        fmt::print(" - items.dat -> {} items loaded with hash {}\n", items.GetItems().size(), items.GetHash());
    }

    StoreManager& store_manager{ StoreManager::Get() };
    if (store_manager.Init()) {
        uint16_t size = 0;
        for (auto& [type, items] : store_manager.GetItems())
            size += items.size();
        fmt::print("StoreManager Initialized, {} items are loaded.\n", size);
    }

    WorldRender& world_render{ WorldRender::get() };
    world_render.load_caches();

    CommandManager& commands{ CommandManager::get() };
    commands.register_commands();

    g_events = std::make_shared<EventPool>();
    g_events->load_events();

    g_servers = std::make_shared<ServerPool>(g_events);
    if (!g_servers->InitializeENet()) {
        fmt::print("failed to initialize enet, shutting down the server.\n");
        return EXIT_FAILURE;
    }
    /*
    DiscordBot& discord_bot{ DiscordBot::get() };
    discord_bot.initialize(BotConfiguration{
        .m_guild_id = dpp::snowflake{ (unsigned long long)((ConfigTable*)database.GetTable(Database::DATABASE_CONFIG_TABLE))->get_row_integer(1) },
        .m_vanguard_token = ((ConfigTable*)database.GetTable(Database::DATABASE_CONFIG_TABLE))->get_row_varchar(1)
    }, g_servers); */ // crash, not configured perfectly for now.

    g_servers->StartService();  
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return EXIT_SUCCESS;
}