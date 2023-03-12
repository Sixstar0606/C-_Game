#pragma once
#include <string>
#include <memory>
#include <dpp/dpp.h>
#include <discord/objects/discord_settings.h>

#define GROWXYZ_OPERATOR_ROLE   1006014646190342265

namespace GTServer {
    class ServerPool;
    class DiscordBot {
    public:
        enum eBotType {
            BOT_TYPE_VANGUARD
        };

    public:
        DiscordBot() = default;
        ~DiscordBot();

        void initialize(const BotConfiguration& settings, std::shared_ptr<ServerPool> server_pool);

        static void* GetBot(const eBotType& type) { return get().get_bot__interface(type); }
        static BotConfiguration GetConfiguration() { return get().m_configuration; }
    public:
        static DiscordBot& get() { static DiscordBot ret; return ret; }

        void* get_bot__interface(const eBotType& type) {
            switch (type) {
            case BOT_TYPE_VANGUARD: return m_vanguard;
            default: return nullptr;    
            }
        }

    private:
        void on_commands_register();
        void on_message_create(const eBotType& type, const dpp::message_create_t& event);
        void on_interaction_create(const dpp::interaction_create_t& event);
        void on_button_click(const dpp::button_click_t & event);
    public:
        BotConfiguration m_configuration;

    private:     
        dpp::cluster* m_vanguard;
        
    private:
        std::shared_ptr<ServerPool> m_server_pool;
    };
}