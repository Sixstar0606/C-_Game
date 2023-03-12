#pragma once
#include <command/command_context.h>
#include <command/command.h>
#include <world/world.h>

namespace GTServer {
    class CommandManager {
    public:
        CommandManager() = default;
        ~CommandManager();

        void register_commands();

    public:
        static CommandManager& get() { static CommandManager ret; return ret; }
        static std::unordered_map<std::string, Command*> get_commands() { return get().m_commands; }
        static bool execute(ServerPool* servers, std::shared_ptr<Server> server, std::shared_ptr<Player> player, const std::string& text) { return get().execute__interface(servers, server, player, text); }

    private:
        bool execute__interface(ServerPool* servers, std::shared_ptr<Server> server, std::shared_ptr<Player> player, const std::string& text);
        void send_item(const uint16_t& item_id, const uint8_t& count,
            std::pair<std::shared_ptr<Player>, std::shared_ptr<Player>> player, std::shared_ptr<World> world);
        
        void command_help(const CommandContext& ctx);
        void command_who(const CommandContext& ctx);
        void command_get(const CommandContext& ctx);
        void command_manage(const CommandContext& ctx);
        void command_find(const CommandContext& ctx);
        void command_clearinventory(const CommandContext& ctx);
        void command_findplayer(const CommandContext& ctx);
        void command_renderworld(const CommandContext& ctx);
        void command_punchid(const CommandContext& ctx);
        void command_nick(const CommandContext& ctx);
        void command_warp(const CommandContext& ctx);
        void command_summon(const CommandContext& ctx);
        void command_unaccess(const CommandContext& ctx);
        void command_clearplaymods(const CommandContext& ctx);
        void command_warpto(const CommandContext& ctx);
        void command_pull(const CommandContext& ctx);
        void command_pullall(const CommandContext& ctx);
        void command_kickall(const CommandContext& ctx);
        void command_kick(const CommandContext& ctx);
        void command_nuke(const CommandContext& ctx);
        void command_readyall(const CommandContext& ctx);
        void command_spawneffect(const CommandContext& ctx);
        void command_spawnevent(const CommandContext& ctx);
        void command_ban(const CommandContext& ctx);
        void command_uba(const CommandContext& ctx);
        void command_gban(const CommandContext& ctx);
        void command_unban(const CommandContext& ctx);
        void command_unnuke(const CommandContext& ctx);
        void command_ghost(const CommandContext& ctx);
        void command_mods(const CommandContext& ctx);
        void command_test(const CommandContext& ctx);
        void command_edit(const CommandContext& ctx);
        void command_transferworld(const CommandContext& ctx);
        void command_msg(const CommandContext& ctx);
        void command_reply(const CommandContext& ctx);
        void command_rgo(const CommandContext& ctx);
        void command_sb(const CommandContext& ctx);
        void command_go(const CommandContext& ctx);
        void command_1hit(const CommandContext& ctx);
        void command_clearworld(const CommandContext& ctx);

        void command_invis(const CommandContext& ctx);

    private:
        std::unordered_map<std::string, Command*> m_commands{};
    };
}