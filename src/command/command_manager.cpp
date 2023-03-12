#include <command/command_manager.h>
#include <ranges>
#include <fmt/core.h>
#include <utils/text.h>
#include <server/server_pool.h>
#include <world/world.h>


namespace GTServer {
    CommandManager::~CommandManager() {
        m_commands.clear();
    }
    
    void CommandManager::register_commands() {
        fmt::print("registering commands for CommandManager\n");

        m_commands.insert({ "help", new Command { 
            "help", { "?" },
            "`oInfo >> listing you all available commands.``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_help(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "who", new Command {
            "who", { "?" },
            "`oInfo >> listing you all players are in the world.``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_who(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "get", new Command { 
            "get", { "getitem", "item" },
            "`oUsage: /get <item_id> <count>``\n"
            " >> item_id -> item's unique id get them by /findid\n"
            " >> count -> maxed by 200, leave this blank will give you 200x of item",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_get(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "manage", new Command { 
            "manage", {},
            "`oUsage: /manage <manage_type> <raw_name>``\n"
            " >> manage_type -> [player, world]\n"
            " >> raw_name -> full name with case sensitivity e.g. BigBoy_123, START and Harry",
            PLAYER_ROLE_DEVELOPER,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_manage(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "ghost", new Command {
            "ghost", {},
            "`oInfo >> toggle your ability to pass through blocks``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_ghost(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "find", new Command { 
            "find", { "search" },
            "`oUsage: /find <item_name>``"
            " >> item_name -> full or first part of item's name",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_find(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "nick", new Command {
            "nick", { "nickname" },
            "`oUsage: /nick <new_name>``"
            " >> nick -> fake name for yourself",
            PLAYER_ROLE_MODERATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_nick(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "warp", new Command {
            "warp", { "warpworld" },
            "`oUsage: /warp <world_name>``"
            " >> warp -> teleports you to world of name you gave",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_warp(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "summon", new Command {
            "summon", {},
            "`oUsage: /summon <player_name>``"
            " >> summon -> teleports player to you",
            PLAYER_ROLE_MODERATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_summon(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "warpto", new Command {
            "warpto", {},
            "`oUsage: /warpto <player_name>``"
            " >> warpto -> teleports you to player",
            PLAYER_ROLE_MODERATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_warpto(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "pull", new Command {
            "pull", {},
            "`oUsage: /pull <player_name>``"
            " >> pull -> pulls player to you",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_pull(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "pullall", new Command {
            "pullall", {},
            "`oUsage: /pullall``"
            " >> pullall -> pulls all players to you",
            PLAYER_ROLE_VIP_PLUS,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_pullall(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "kickall", new Command {
            "kickall", {},
            "`oUsage: /kickall``"
            " >> kickall -> kicks all players",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_kickall(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "clearplaymods", new Command {
            "clearplaymods", {},
            "`oUsage: /clearplaymods``"
            " >> clearplaymods -> removes your playmods",
            PLAYER_ROLE_ADMINISTRATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_clearplaymods(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "kick", new Command {
            "kick", {},
            "`oUsage: /kick <player_name>``"
            " >> kick -> kicks player",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_kick(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "unaccess", new Command {
            "unaccess", {},
            "`oUsage: /unaccess``"
            " >> unaccess -> removes your access from all locks in the world",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_unaccess(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "nuke", new Command {
            "nuke", {},
            "`oUsage: /nuke <world_name>``"
            " >> nuke -> stops players from joining world",
            PLAYER_ROLE_MODERATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_nuke(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "readyall", new Command {
            "readyall", {},
            "`oUsage: /readyall``"
            " >> readyall -> makes all trees and providers in world ready.",
            PLAYER_ROLE_DEVELOPER,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_readyall(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "gban", new Command {
            "gban", {},
            "`oUsage: /gban <player_name> <reason>``"
            " >> gban -> bans a player from BetterGrowtopia",
            PLAYER_ROLE_MODERATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_gban(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "ban", new Command {
            "ban", {},
            "`oUsage: /ban <player_name>``"
            " >> ban -> bans a player from the world",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_ban(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "uba", new Command {
            "uba", {},
            "`oUsage: /uba``"
            " >> uba -> unbans everyone from the world",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_uba(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "spawneffect", new Command {
            "spawneffect", {},
            "`oUsage: /spawneffect <effect_id>``"
            " >> spawneffect -> spawns an effect",
            PLAYER_ROLE_ADMINISTRATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_spawneffect(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "spawnevent", new Command {
            "spawnevent", {},
            "`oUsage: /spawnevent <event_name>``"
            " >> spawnevent -> spawns an event\nEvents: beautiful_crystal",
            PLAYER_ROLE_ADMINISTRATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_spawnevent(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "mods", new Command {
            "mods", {},
            "`oUsage: /mods``"
            " >> mods -> shows list of online moderators and higher",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_mods(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "unban", new Command {
            "unban", {},
            "`oUsage: /unban <player_name>``"
            " >> unban -> removes ban from a player",
            PLAYER_ROLE_ADMINISTRATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_unban(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "unnuke", new Command {
            "unnuke", {},
            "`oUsage: /unnuke <world_name>``"
            " >> unnuke -> allows players to join the world again",
            PLAYER_ROLE_MODERATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_unnuke(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "clearinventory", new Command { 
            "clearinventory", {},
            "`oInfo >> clearing out your inventory items (except Fist and Wrench).``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_clearinventory(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "findplayer", new Command { 
            "findplayer", {},
            "`oUsage: /findplayer <player_name>``\n"
            " >> player_name -> full or first part of player's name",
            PLAYER_ROLE_MODERATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_findplayer(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "renderworld", new Command { 
            "renderworld", {},
            "`oInfo >> render your world data as a png file, this will public to everyone.``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_renderworld(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "punchid", new Command { 
            "punchid", {},
            "`oUsage: /punchid <0-255>``",
            PLAYER_ROLE_ADMINISTRATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_punchid(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "test", new Command { 
            "test", {},
            "`oInfo >> Just a testing command for debug things``",
            PLAYER_ROLE_DEVELOPER,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_test(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "edit", new Command {
            "edit", {},
            "`oUsage: /edit <action> <radius> <item id/name>, use /unsafeedit to work on locked areas too.\n"
            "`oExample: `w/edit add 3 lava``, `w/edit erase 3 dirt``, `w/edit addobject 3 rock`` or `w/edit clearobject 5``",
            PLAYER_ROLE_DEVELOPER,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_edit(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "unsafeedit", new Command {
            "unsafeedit", {},
            "`oUsage: /edit <action> <radius> <item id/name>, use /unsafeedit to work on locked areas too.\n"
            "`oExample: `w/edit add 3 lava``, `w/edit erase 3 dirt``, `w/edit addobject 3 rock`` or `w/edit clearobject 5``",
            PLAYER_ROLE_DEVELOPER,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_edit(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "transferworld", new Command {
            "transferworld", {},
            "`oUsage: /transferworld <userID or name>, changes ownership of all locks in the current world to this new person.\n"
            "`oExample: `w/transferworld #1``, `w/transferworld user`` or `w/transferworld /User``",
            PLAYER_ROLE_DEVELOPER,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_transferworld(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "msg", new Command { 
            "msg", {},
            "`oUsage: /msg <`$full or first part of a name``> <`$your message``> - This will send a private message to someone anywhere in the universe.``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_msg(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "r", new Command { 
            "r", {},
            "`oUsage: /r <`$your message``> - This will reply a private message to a person who recently message you.``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_reply(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "rgo", new Command { 
            "rgo", {},
            "`oInfo >> This will send you to the world of a person who recently message you.``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_rgo(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "sb", new Command {
            "sb", {},
            "`oInfo >> This will send a message to everyone online.``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_sb(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "go", new Command {
            "go", {},
            "`oInfo >> Takes you to most recent super-broadcast world.``",
            PLAYER_ROLE_DEFAULT,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_go(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "1hit", new Command {
            "1hit", {},
            "`oInfo >> Makes you break blocks in 1 hit!``",
            PLAYER_ROLE_DEVELOPER,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_1hit(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "clearworld", new Command {
            "clearworld", {},
            "`oInfo >> Clears the world.``",
            PLAYER_ROLE_DEVELOPER,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_clearworld(std::forward<decltype(PH1)>(PH1)); } }
        } });
        m_commands.insert({ "invis", new Command {
            "invis", {},
            "`oInfo >> Makes the player invisible to other players.``",
            PLAYER_ROLE_MODERATOR,
            std::function<void(const CommandContext&)> { [this](auto&& PH1) { this->command_invis(std::forward<decltype(PH1)>(PH1)); } }
} });

        fmt::print(" - {} commands registered.\n", m_commands.size());
    }


    bool CommandManager::execute__interface(ServerPool* servers, std::shared_ptr<Server> server, std::shared_ptr<Player> player, const std::string& text) {
        std::vector<std::string> args = utils::split(text, " ");
        if (args.empty())
            return false;
        std::string command_name = args[0].substr(1);
        std::transform(command_name.cbegin(), command_name.cend(), command_name.begin(), ::tolower);
        args.erase(args.cbegin());

        return std::ranges::any_of(m_commands, [&](const auto& command) {
            std::vector<std::string> aliases = command.second->GetAliases();
        if (command.second->GetName() == command_name || std::ranges::any_of(aliases, [&](const auto& alias) {
            return alias == command_name;
            })) {
            if (player->GetRole() < command.second->GetRole()) {
                player->SendLog("`4Unknown command. ``Enter `$/help`` for a list of valid commands.");
                return false;
            }
            player->SendLog("`6{}``", text);

            CommandContext ctx{
                .m_player = player,
                .m_server = server,
                .m_servers = servers,
                .m_command = command.second,
                .m_message = text,
                .m_arguments = args
            };
            command.second->GetFunction()(ctx);
            return true;
        }

        return false;
            });
    }
}