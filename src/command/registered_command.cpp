#include <command/command_manager.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <magic_enum.hpp>
#include <command/command.h>
#include <world/world_pool.h>
#include <database/database.h>
#include <render/world_render.h>
#include <proton/utils/dialog_builder.h>
#include <utils/timing_clock.h>
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;

namespace GTServer {
    void CommandManager::send_item(const uint16_t& item_id, const uint8_t& count,
        std::pair<std::shared_ptr<Player>, std::shared_ptr<Player>> player, std::shared_ptr<World> world) {
        auto& [invoker, receiver] { player };
        ItemInfo* item{ ItemDatabase::GetItem(item_id) };
        if (!item)
            return;
        if (receiver->m_inventory.GetItemCount(item_id) + count > item->m_max_amount) {
            invoker->SendLog("`4Oops, ``You can only add `w{}`` items more!``", 
                item->m_max_amount - ((receiver->m_inventory.GetItemCount(item_id) + count) - count));
            return;
        }
        if (receiver->m_inventory.GetItems().size() >= receiver->m_inventory.GetSize() ||
            !receiver->m_inventory.Add(item_id, count, true)) {
            invoker->SendLog("`4Oops, `w{}`` doesn't have enough free space in his/her inventory.``", receiver->GetDisplayName(world));
            return;
        }
        std::string inv_message = std::string { 
            fmt::format("Given `w{} ``of `w{}``{}", count, item->m_name, item->m_rarity != 999 ? fmt::format(", rarity: `w{}``", item->m_rarity) : "")
        };
        std::string rec_message = std::string {
            fmt::format("You have received `w{} ``of `w{}``{}", count, item->m_name, item->m_rarity != 999 ? fmt::format(", rarity: `w{}``", item->m_rarity) : "")
        };
        if (receiver->GetUserId() != invoker->GetUserId()) {
            inv_message.append(fmt::format(" `oto `w{}`o.", receiver->GetDisplayName(world)));
            rec_message.append(fmt::format(" `ofrom `w{}`o.", invoker->GetDisplayName(world)));
            receiver->v_sender.OnConsoleMessage(rec_message);
        }
        invoker->v_sender.OnConsoleMessage(inv_message);
        if (invoker->IsFlagOn(PLAYERFLAG_IS_INVISIBLE))
            return;

        GameUpdatePacket update_packet {
            .m_type = NET_GAME_PACKET_ITEM_EFFECT,
            .m_animation_type = 0x3,
            .m_net_id = static_cast<int32_t>(receiver->GetNetId()),
            .m_target_net_id = static_cast<int32_t>(invoker->GetNetId()),
            .m_delay = 150,
            .m_item_id_alt = item_id
        };
        world->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
        });
    }


    void CommandManager::command_help(const CommandContext& ctx) {
        if (!ctx.m_arguments.empty()) {
            auto it = std::find_if(m_commands.begin(), m_commands.end(), [&](const auto& command) {
                if (command.second->GetName() != ctx.m_arguments[0]) {
                    std::vector<std::string> aliases = command.second->GetAliases();
                    return std::find(aliases.cbegin(), aliases.cend(), ctx.m_arguments[0]) != aliases.cend();
                }
            return command.second->GetName() == ctx.m_arguments[0];
                });

            if (it != m_commands.end()) {
                if (ctx.m_player->GetRole() >= (*it).second->GetRole()) {
                    ctx.m_player->v_sender.OnConsoleMessage((*it).second->GetDescription());
                    if ((*it).second->GetAliases().size() > 0)
                        ctx.m_player->v_sender.OnConsoleMessage(fmt::format("Aliases >> {}", (*it).second->GetAliases()));
                    return;
                }
            }
            return;
        }
        std::string commands{ "Supported commands are: " };
        for (const auto& command : m_commands) {
            if (ctx.m_player->GetRole() < command.second->GetRole())
                continue;
            commands.append(fmt::format("/{}, ", command.second->GetName()));
        }

        commands.resize(commands.size() - 2);
        ctx.m_player->SendLog(commands);
    }
    void CommandManager::command_who(const CommandContext& ctx) {
        auto world{ ctx.m_servers->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        std::string players_list{};
        for (auto& player : world->GetPlayers(true)) {
            if (player->HasPlaymod(PLAYMOD_TYPE_INVISIBILITY) && !player->IsFlagOn(PLAYERFLAG_IS_INVISIBLE) && ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
                continue;
            players_list.append(fmt::format("`w{}``{}, ", player->GetDisplayName(nullptr), ctx.m_player->GetRole() >= PLAYER_ROLE_MODERATOR ? " (ID: {})" : ""));
        }

        players_list.resize(players_list.size() - 2);
        ctx.m_player->v_sender.OnConsoleMessage(fmt::format("`wWho's in `o{}``: {}", world->GetName(), players_list));
        world->SendWho(ctx.m_player, true);
    }
    void CommandManager::command_mods(const CommandContext& ctx) {
        auto world{ ctx.m_servers->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        std::shared_ptr<Player> player = ctx.m_player;
        std::string players_list{};
        for (auto& playerthing : ctx.m_servers->GetPlayers()) {
            if (playerthing->GetRole() >= PLAYER_ROLE_MODERATOR) {
                if (!playerthing->HasPlaymod(PLAYMOD_TYPE_NICK)) {
                    players_list.append(fmt::format("{}`o, ", playerthing->GetDisplayName(world)));
                }
                else {
                    if (player->GetRole() >= PLAYER_ROLE_MODERATOR) {
                        if (playerthing->GetRole() == PLAYER_ROLE_MODERATOR) {
                            players_list.append(fmt::format("`#@{}`o (Undercover as {}`o), ", playerthing->GetRawName(), playerthing->GetDisplayName(world)));
                        }
                        if (playerthing->GetRole() == PLAYER_ROLE_ADMINISTRATOR && player->GetRole() >= PLAYER_ROLE_ADMINISTRATOR) {
                            players_list.append(fmt::format("`9@{}`o (Undercover as {}`o), ", playerthing->GetRawName(), playerthing->GetDisplayName(world)));
                        }
                        if (playerthing->GetRole() == PLAYER_ROLE_DEVELOPER && player->GetRole() >= PLAYER_ROLE_DEVELOPER) {
                            players_list.append(fmt::format("`6@{}`o (Undercover as {}`o), ", playerthing->GetRawName(), playerthing->GetDisplayName(world)));
                        }
                    }
                }
            }
        }
        ctx.m_player->SendLog(fmt::format("`oMods online: {}", players_list));
    }
    void CommandManager::command_get(const CommandContext& ctx) {
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 1) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        const uint16_t& item_id{ static_cast<uint16_t>(std::atoi(ctx.m_arguments[0].c_str())) };
        ItemInfo* item{ ItemDatabase::GetItem(item_id) };
        if (!item) {
            ctx.m_player->SendLog("`4Oops, ``Invalid ItemId was given, please enter between `w0 - {}``.``", ItemDatabase::GetItems().size());
            return;
        }

        const uint8_t& quantity{ static_cast<uint8_t>(ctx.m_arguments.size() == 1 ? 
            item->m_max_amount : std::atoi(ctx.m_arguments[1].c_str())) };
        if (quantity > item->m_max_amount) {
            ctx.m_player->SendLog("`4Oops, ``The maximum amount of {} is {}``", item->m_name, item->m_max_amount);
            return;
        }
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        this->send_item(item_id, quantity, { ctx.m_player, ctx.m_player }, world);
    }
    void CommandManager::command_manage(const CommandContext& ctx) {
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 2) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        std::string manage_type{ ctx.m_arguments[0] },
            raw_name{ ctx.m_arguments[1] };
        const auto& type_hash = utils::quick_hash(manage_type);
        switch (type_hash) {
        case "world"_qh: {
            if (!utils::to_uppercase(raw_name)) {
                ctx.m_player->SendLog("`4Oops!`` the name `w`` is containing illegal characters.", ctx.m_arguments[0]);
                return;
            }
            auto world{ ctx.m_server->GetWorldPool()->GetWorld(raw_name) };
            if (!world)
                return;
            PlayerTable* db{ (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE) };
            DialogBuilder dialog{};
            dialog.set_default_color('o')
                ->add_label_with_icon(fmt::format("`w{}'s management``", world->GetName()), ITEM_SCROLL_BULLETIN, DialogBuilder::LEFT, DialogBuilder::BIG)
                ->text_scaling_string("dirttttttttttttttttttttttttt")
                ->embed_data("manage_type", "world")
                ->embed_data("world_name", world->GetName())
                ->add_spacer()
                ->add_textbox(fmt::format("Name: `w{}`` (Id: `w{}``)", world->GetName(), world->GetID()))
                ->add_textbox(fmt::format("Flags: `w{:#04x}``", world->GetFlags()))
                ->add_textbox(fmt::format("Created At: `w{}``", world->GetCreatedAt()))
                ->add_textbox(fmt::format("Recently Update: `w{}``", world->GetUpdatedAt()))
                ->add_textbox(fmt::format("OwnerID: `w{}``, OwnerName: `w{}``",
                    world->GetOwnerId() != -1 ? std::to_string(world->GetOwnerId()) : std::string{ "`4undefinded``" },
                    db->GetName(world->GetOwnerId()).empty() ? "`4undefinded``" : db->GetName(world->GetOwnerId())))
                ->add_textbox(fmt::format("Active Players (Including Invisible Person): `w{}``", world->GetPlayers(true).size()))
                ->add_textbox(fmt::format("Size: `w{}``*`w{}``, `w{}`` tiles", world->GetSize().m_x, world->GetSize().m_y, world->GetTiles().size()))
                ->add_textbox(fmt::format("Base Weather: [`w{}``] `w{}``", world->GetBaseWeatherId(), magic_enum::enum_name(static_cast<eWorldWeather>(world->GetBaseWeatherId()))))
                ->add_textbox(fmt::format("Active Weather: [`w{}``] `w{}``", world->GetWeatherId(), magic_enum::enum_name(static_cast<eWorldWeather>(world->GetWeatherId()))))
                ->add_spacer()
                ->add_button_with_icon("list_players", "Players List", ITEM_MINI_GROWTOPIAN)
                ->add_button_with_icon("list_objects", "Objects List", ITEM_GEMS)
                ->add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)
                ->add_spacer()
                ->add_smalltext("Please don't abuse, we store your actions into server logs for every action you do!")
                ->add_quick_exit()
                ->add_button_with_icon("rename", "Rename", ITEM_CHANGE_OF_ADDRESS, DialogBuilder::STATIC_BLUE_FRAME)
                ->add_button_with_icon("render", "Render", ITEM_GLOBE, DialogBuilder::STATIC_BLUE_FRAME)
                ->add_button_with_icon("clear_world", "Clear World", ITEM_DIGGERS_SPADE, DialogBuilder::STATIC_BLUE_FRAME)
                ->add_button_with_icon("toggle_nuke", "Toggle Nuke", ITEM_BAN_WAND, DialogBuilder::STATIC_BLUE_FRAME);
            dialog.add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)->end_dialog("manage_menu", "Okay", "");
            ctx.m_player->v_sender.OnDialogRequest(dialog.get());
        } break;
        case "player"_qh: {
            std::shared_ptr<Player> target = ctx.m_servers->GetPlayerByName(raw_name);
            ctx.m_player->SendDialog(Player::DIALOG_TYPE_PUNISHMENT, TextScanner{}, nullptr, target);
        } break;
        }
    }
    void CommandManager::command_find(const CommandContext& ctx) {
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 1) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        std::string keyword = ctx.m_message.substr(6);
        if (keyword.length() < 3 || keyword.length() > 20) {
            ctx.m_player->SendLog("`4Oops! `oyou've to enter at least first `w3 `ocharacters of item's name, this is not clear enough to find.");
            return;
        }
        ctx.m_servers->AddQueue(QUEUE_TYPE_FINDING_ITEMS, ServerQueue {
            .m_keyword = keyword,
            .m_player = ctx.m_player
        });
    }
    void CommandManager::command_clearinventory(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        std::size_t items_count = ctx.m_player->m_inventory.GetItems().size() - 2;
        if (items_count < 1)
            return;
        for (auto& [id, count] : ctx.m_player->m_inventory.GetItems()) {
            if (id == ITEM_FIST || id == ITEM_WRENCH)
                continue;
            ItemInfo* item = ItemDatabase::GetItem(id);
            if (!item)
                continue;
            ctx.m_player->m_inventory.Erase(id, count, false);
            if (item->m_item_type == ITEMTYPE_CLOTHES)
                ctx.m_player->SetCloth(item->m_clothing_type, ITEM_BLANK, true);
        }
        ctx.m_player->m_inventory.Send();
        ctx.m_player->v_sender.OnSetClothing(ctx.m_player->GetClothes(), ctx.m_player->GetSkinColor(), true, ctx.m_player->GetNetId());

        world->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->v_sender.OnSetClothing(ctx.m_player->GetClothes(), ctx.m_player->GetSkinColor(), false, ctx.m_player->GetNetId());
        });
    }
    void CommandManager::command_findplayer(const CommandContext& ctx) {
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 1) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        std::string keyword = ctx.m_message.substr(12);
        if (keyword.length() < 3 || keyword.length() > 24) {
            ctx.m_player->SendLog("`4Oops! `oyou've to enter at least first `w3 `ocharacters of player's name, this is not clear enough to find.");
            return;
        }
        ctx.m_servers->AddQueue(QUEUE_TYPE_FINDING_PLAYERS, ServerQueue {
            .m_keyword = keyword,
            .m_player = ctx.m_player
        });
    }
    void CommandManager::command_renderworld(const CommandContext& ctx) {
        DialogBuilder db{};
        db.set_default_color('o')
            ->add_label_with_icon("`wWorld Render``", ITEM_MAILBOX, DialogBuilder::LEFT, DialogBuilder::BIG)
            ->add_textbox("World rendering means we'll make a picture of your `5ENTIRE WORLD`` and host it on our server publicly, for anybody to view. <CR><CR>`4Warning:`` this picture will also include you and all co-owners on your `5World Lock``. <CR>if you'd like to keep that information private, Press `wCancel!``")
            ->end_dialog("renderworld", "Cancel", "Render It!");
        ctx.m_player->v_sender.OnDialogRequest(db.get());
    }
    void CommandManager::command_punchid(const CommandContext& ctx) {
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 1) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        uint8_t punch_id = static_cast<uint8_t>(std::atoi(ctx.m_arguments[0].c_str()));
        ctx.m_player->SetPunchID(punch_id);
        ctx.m_player->SendLog("You've changed your punch id to `w{}``", punch_id);
        world->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->SendCharacterState(ctx.m_player);
        });
        
    }
    void CommandManager::command_test(const CommandContext& ctx) {
        ctx.m_player->m_inventory.UpgradeBackpack();
    }
    void CommandManager::command_edit(const CommandContext& ctx) {
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 3) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        auto world{ ctx.m_servers->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        auto action = ctx.m_arguments[0];
        auto circle_radius = std::stof(ctx.m_arguments[1].c_str());
        if (circle_radius < 1.0f || circle_radius > 100.0f) {
            ctx.m_player->SendLog("`4Oops``, the given radius must between 1 - 100.");
            return;
        }
        auto item_name = ctx.m_arguments[2];
        CL_Vec2i devided_position = CL_Vec2i{ ctx.m_player->GetPosition().m_x / 32, ctx.m_player->GetPosition().m_y / 32 };

        ItemInfo* item = ItemDatabase::GetItemByName(item_name);
        if (std::all_of(item_name.begin(), item_name.end(), ::isdigit))
            item = ItemDatabase::GetItem(std::atoi(item_name.c_str()));
        if (!item) {
            ctx.m_player->SendLog("`4Oops``, couldn't find item with given keyword '`w{}``', please make sure this is valid item ID/name.", item_name);
            return;
        }

        ctx.m_player->SendLog("Terraforming world...");
        if (!world->EditTile(action, devided_position, circle_radius, item, ctx.m_command->GetName() == "unsafeedit" ? true : false))
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
    }
    void CommandManager::command_transferworld(const CommandContext& ctx) {
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 1) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        auto world{ ctx.m_servers->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (!world)
            return;
        std::shared_ptr<Player> person{ ctx.m_servers->GetPlayerByFormat(ctx.m_arguments[0]) };
        if (!person) {
            ctx.m_player->SendLog("`4Oops``, couldn't find a player with userId/name starting with `w{}``.", ctx.m_arguments[0]);
            return;
        }
        if (!world->IsOwner(player) && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            ctx.m_player->SendLog("You are not the world owner!");
            return;
        }
        world->SetOwnerId(person->GetUserId());
        Tile* main_lock = world->GetTile(world->GetMainLock());
        main_lock->ApplyLockOwner(person->GetUserId());
        for (auto& t : world->GetTiles()) {
            if (t.GetBaseItem()->m_item_type != ITEMTYPE_LOCK) {
                t.ClearAccess();
                t.AddAccess(person->GetUserId());
            }
            else {
                t.ApplyLockOwner(person->GetUserId());
            }
        }
        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
            if (!person->HasPlaymod(PLAYMOD_TYPE_INVISIBLE)) {
                ply->v_sender.OnNameChanged(person->GetNetId(), person->GetDisplayName(world));
            }
            else {
                ply->v_sender.OnNameChanged(person->GetNetId(), fmt::format("{} (V)", person->GetDisplayName(world)));
            }
            });
        ctx.m_player->SendLog("Trasnfering to Id: {}, Name: {}", person->GetUserId(), person->GetDisplayName(nullptr));
    }

    void CommandManager::command_msg(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 2) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        std::string name = ctx.m_arguments[0];
        std::string message = ctx.m_message.substr(6 + name.length());
        std::vector<std::string> messages_sent{};

        if (ctx.m_player->HasPlaymod(PLAYMOD_TYPE_DUCT_TAPE)) {
            ctx.m_player->v_sender.OnConsoleMessage("CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `$Nothing!`o)`` (`4Note: ``You can't send messages to people when you are muted)");
            return;
        }
        else if (name.length() < 3 || name.length() > 120) {
            ctx.m_player->v_sender.OnConsoleMessage("`4Oops, `oyou've to enter at least first `w3 `ocharacters of person's name, this is not clear enough to find.");
            return;
        }
        else if (message.find("player_chat=") != std::string::npos) {
            ctx.m_player->v_sender.OnConsoleMessage("`4Oops, `ousing old exploit trick huh!? Well it doesn't work here.");
            return;
        }
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        for (auto& player : ctx.m_servers->GetPlayers()) {
            auto target_name = player->GetRawName();
            std::string new_name = name.substr(2);
            std::transform(target_name.begin(), target_name.end(), target_name.begin(), ::tolower);
            if (name != player->GetDisplayName() && player->GetDisplayName().substr(2) != new_name)
                continue;
            if (messages_sent.size() >= 5)
                break;
            auto world_name = "`4JAMMED!``";
            auto target_world = ctx.m_servers->GetWorld(player->GetWorld());
            if (!target_world)
                continue;
            if (!player->IsFlagOn(PLAYERFLAG_LOGGED_ON)) {
                ctx.m_player->SendLog("They aren't online right now!");
                break;
            }
            player->GetReceiveMessage().SetUserId(ctx.m_player->GetUserId());
            if (world->IsFlagOn(WORLDFLAG_JAMMED) || ctx.m_player->HasPlaymod(PLAYMOD_TYPE_INVISIBLE)) {
                player->v_sender.OnConsoleMessage(fmt::format("CP:_PL:0_OID:_CT:[MSG]_ `c>> from (`w{}`c) in [`o{}`c] > `o{}", ctx.m_player->GetDisplayName(world), world_name, message));
            }
            else {
                player->v_sender.OnConsoleMessage(fmt::format("CP:_PL:0_OID:_CT:[MSG]_ `c>> from (`w{}`c) in [`o{}`c] > `o{}", ctx.m_player->GetDisplayName(world), world->GetName(), message));
            }
            if (player->GetRole() >= PLAYER_ROLE_MODERATOR) {
                ctx.m_player->SendLog("`4Note: `oMessage a mod `4ONLY ONCE `oabout an issue. Mods don't fix scams or replace items, they punish players who break the /rules. For issues related to account recovery or purchasing, contact support at the BetterGrowtopia Discord Server.");
            }
            player->PlaySfx("pay_time", 0);
            messages_sent.push_back(player->GetDisplayName(target_world));
        }
        if (messages_sent.size() != 0) {
            std::string name_list{};
            for (auto& it : messages_sent)
                name_list += fmt::format("{}, ", it);
            name_list.resize(name_list.size() - 2);
            ctx.m_player->v_sender.OnConsoleMessage(fmt::format("CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `${}`o)", name_list));
            return;
        }            
        ctx.m_player->SendLog("`6>> No one online who has a name starting with {}`8.", ctx.m_arguments[0]);
    }
    void CommandManager::command_sb(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 1) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        std::shared_ptr<Player> player = ctx.m_player;
        std::string message = ctx.m_message.substr(4);
        if (message.length() > 70 && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->SendLog("The super broadcast cannot be longer than 70 characters.");
            return;
        }
        std::vector<std::string> messages_sent{};

        if (ctx.m_player->HasPlaymod(PLAYMOD_TYPE_DUCT_TAPE)) {
            ctx.m_player->v_sender.OnConsoleMessage("CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `$Nothing!`o)`` (`4Note: ``You can't send super broadcasts when you are muted)");
            return;
        }
        else if (message.find("player_chat=") != std::string::npos) {
            ctx.m_player->v_sender.OnConsoleMessage("`4Oops, `ousing old exploit trick huh!? Well it doesn't work here.");
            return;
        }


        std::string file_path = "GlobalGameVariables.txt";

        if (!fs::exists(file_path)) {
            return; // File does not exist, do nothing
        }

        std::ifstream infile(file_path);
        std::string line;
        std::vector<std::string> lines;
        bool found_recent_sb_location = false;
        while (std::getline(infile, line)) {
            if (line.find("RecentSBLocation:") == 0) {
                lines.push_back(std::format("RecentSBLocation: {}", world->GetName()));
                found_recent_sb_location = true;
            }
            else {
                lines.push_back(line);
            }
        }
        infile.close();

        if (!found_recent_sb_location) {
            lines.push_back(std::format("RecentSBLocation: {}", world->GetName()));
        }

        std::ofstream outfile(file_path);
        for (const auto& line : lines) {
            outfile << line << std::endl;
        }
        outfile.close();

        for (auto& player2 : ctx.m_servers->GetPlayers()) {
            player2->SendLog(std::format("`w** `5Super-Broadcast `wfrom {} `w(in {}) `w** : {}", player->GetDisplayName(), world->GetName(), message));
        }
    }
    void CommandManager::command_go(const CommandContext& ctx) {
        auto world{ ctx.m_servers->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;

        std::shared_ptr<Player> player = ctx.m_player;

        // Get the RecentSBLocation from the GlobalGameVariables.txt file
        std::string recent_sb_location = "";
        std::ifstream file("GlobalGameVariables.txt");
        if (file.is_open()) {
            std::string line;
            while (getline(file, line)) {
                if (line.find("RecentSBLocation:") != std::string::npos) {
                    recent_sb_location = line.substr(line.find(":") + 2);
                    break;
                }
            }
            file.close();
        }
        auto world2{ ctx.m_server->GetWorldPool()->GetWorld(recent_sb_location) };
        if (world2->IsFlagOn(WORLDFLAG_NUKED) && player->GetRole() < PLAYER_ROLE_MODERATOR) {
            player->SendLog("You can't warp there, sorry!");
            return;
        }
        if (player->HasPlaymod(PLAYMOD_TYPE_CURSE)) {
            player->SendLog("You can't warp right now, sorry!");
            return;
        }
        else {
            world->RemovePlayer(player);
            ctx.m_server->GetWorldPool()->OnPlayerLeave(world, player, false);
            // player->SetNetId(world2->AddPlayer(player));
            // world2->AddPlayer(player);
            ctx.m_server->GetWorldPool()->OnPlayerJoin(ctx.m_servers, world2, player, world2->GetTilePos(ITEMTYPE_MAIN_DOOR));
            ctx.m_server->GetWorldPool()->OnPlayerSyncing(world2, player);
            world2->AddPlayer(player);
            ctx.m_player->PlaySfx("object_spawn", 0);
        }
    }
    void CommandManager::command_reply(const CommandContext& ctx) {
        auto world{ ctx.m_servers->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 1 || ctx.m_player->GetReceiveMessage().GetUserId() == -1) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        std::string message = ctx.m_message.substr(3);
        if (ctx.m_player->HasPlaymod(PLAYMOD_TYPE_DUCT_TAPE)) {
            ctx.m_player->v_sender.OnConsoleMessage("CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `$Nothing!`o)`` (`4Note: ``You can't send messages to people when you are muted)");
            return;
        }
        else if (message.length() > 120) {
            ctx.m_player->v_sender.OnConsoleMessage("`4Oops, `oyou've to enter at least first `w3 `ocharacters of person's name, this is not clear enough to find.");
            return;
        }
        else if (message.find("player_chat=") != std::string::npos) {
            ctx.m_player->v_sender.OnConsoleMessage("`4Oops, `ousing old exploit trick huh!? Well it doesn't work here.");
            return;
        }
        for (auto& player : ctx.m_servers->GetPlayers()) {
            if (player->GetUserId() != ctx.m_player->GetReceiveMessage().GetUserId())
                continue;
            auto world_name = "`4JAMMED!``";
            auto target_world = ctx.m_servers->GetWorld(player->GetWorld());
            if (!player->IsFlagOn(PLAYERFLAG_LOGGED_ON)) {
                ctx.m_player->SendLog("They aren't online right now!");
                break;
            }
            player->GetReceiveMessage().SetUserId(ctx.m_player->GetUserId());
            if (world->IsFlagOn(WORLDFLAG_JAMMED) || ctx.m_player->HasPlaymod(PLAYMOD_TYPE_INVISIBLE)) {
                player->v_sender.OnConsoleMessage(fmt::format("CP:_PL:0_OID:_CT:[MSG]_ `c>> from (`w{}`c) in [`o{}`c] > `o{}", ctx.m_player->GetDisplayName(world), world_name, message));
                ctx.m_player->v_sender.OnConsoleMessage(fmt::format("CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `${}`o)", player->GetDisplayName(target_world)));
            }
            else {
                player->v_sender.OnConsoleMessage(fmt::format("CP:_PL:0_OID:_CT:[MSG]_ `c>> from (`w{}`c) in [`o{}`c] > `o{}", ctx.m_player->GetDisplayName(world), world->GetName(), message));
                ctx.m_player->v_sender.OnConsoleMessage(fmt::format("CP:_PL:0_OID:_CT:[MSG]_ `o(Sent to `${}`o)", player->GetDisplayName(target_world)));
            }
            if (player->GetRole() >= PLAYER_ROLE_MODERATOR) {
                ctx.m_player->SendLog("`4Note: `oMessage a mod `4ONLY ONCE `oabout an issue. Mods don't fix scams or replace items, they punish players who break the /rules. For issues related to account recovery or purchasing, contact support at the BetterGrowtopia Discord Server.");
            }
            player->PlaySfx("pay_time", 0);
            return;
        }
        ctx.m_player->SendLog("`4Oops, unable to track target's data.``");
    }
    void CommandManager::command_rgo(const CommandContext& ctx) {
        auto world{ ctx.m_servers->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        for (auto& player : ctx.m_servers->GetPlayers()) {
            if (player->GetUserId() != ctx.m_player->GetReceiveMessage().GetUserId())
                continue;
            auto world_name = "`4JAMMED!``";
            auto target_world = ctx.m_servers->GetWorld(player->GetWorld());
            if (!target_world)
                break;
            player->GetReceiveMessage().SetUserId(ctx.m_player->GetUserId());
            if (!world->IsFlagOn(WORLDFLAG_JAMMED) && target_world->HasPlayer(player) && !player->HasPlaymod(PLAYMOD_TYPE_INVISIBLE))
                player->GetReceiveMessage().SetWorld(world->GetName());
                world->RemovePlayer(player);
                ctx.m_server->GetWorldPool()->OnPlayerLeave(world, ctx.m_player, false);
                ctx.m_server->GetWorldPool()->OnPlayerJoin(ctx.m_servers, target_world, ctx.m_player, target_world->GetTilePos(ITEMTYPE_MAIN_DOOR));
                ctx.m_server->GetWorldPool()->OnPlayerSyncing(target_world, ctx.m_player);
                ctx.m_player->PlaySfx("object_spawn", 0);
            return;
        }
        ctx.m_player->SendLog("`4Oops, unable to track target's data.``");
    }
    void CommandManager::command_unaccess(const CommandContext& ctx) {
        auto world{ ctx.m_servers->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        std::shared_ptr<Player> player = ctx.m_player;
        if (world->IsOwner(player)) {
            return;
        }
        Tile* main_lock = world->GetTile(world->GetMainLock());
        if (!main_lock->HasAccess(player->GetUserId())) {
            return;
        }
        for (auto& t : world->GetTiles()) {
            if (t.GetBaseItem()->m_item_type != ITEMTYPE_LOCK && t.HasAccess(player->GetUserId())) {
                t.RemoveAccess(player->GetUserId());
            }
        }
        world->SyncPlayerData(player);
        ctx.m_player->SendLog("Removed your access from world");
    }
    void CommandManager::command_ghost(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        /*if (world->IsFlagOn(WORLDFLAG_RESTRICT_NOCLIP) && !ctx.m_player->IsPermissionFlagOn(WORLDPERMISSION_OWNER)) {
            ctx.m_player->SendLog("`4Oops`o, Only world owner and players within access list are allowed here.``");
            return;
        }*/
        std::shared_ptr<Player> player = ctx.m_player;
        if (player->HasPlaymod(PLAYMOD_TYPE_GHOST_IN_THE_SHELL)) {
            player->RemoveFlag(PLAYERFLAG_IS_GHOST);
            Color firstcolor = { 0xB4, 0x8A, 0x78, 0xFF };
            player->SetSkinColor(firstcolor); // change skin to 0x7B 
            player->RemovePlaymod(PLAYMOD_TYPE_GHOST_IN_THE_SHELL);
            world->SyncPlayerData(player);
            if (player->GetRole() < PLAYER_ROLE_DEVELOPER) {
                world->SendKick(player, true);
            }
        }
        else {
            player->SetFlag(PLAYERFLAG_IS_GHOST);
            player->SetSkinColor(-0x7B); // change skin to 0x7B 
            player->AddPlaymod(PLAYMOD_TYPE_GHOST_IN_THE_SHELL, ITEM_NINJA_MASK, steady_clock::now(), std::chrono::seconds(-1));
            world->SyncPlayerData(player);
        }
        ctx.m_player->v_sender.OnSetClothing(ctx.m_player->GetClothes(), ctx.m_player->GetSkinColor(), true, ctx.m_player->GetNetId());

        world->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->v_sender.OnSetClothing(ctx.m_player->GetClothes(), ctx.m_player->GetSkinColor(), false, ctx.m_player->GetNetId());
            });
    }
    void CommandManager::command_nick(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        world->Broadcast([&](const std::shared_ptr<Player>& playerthing) {
            playerthing->v_sender.SendDanceAnimation(player->GetNetId());
            });
        if (ctx.m_arguments.empty()) {
            player->SetDisplayName(player->GetRawName());
            if (player->HasPlaymod(PLAYMOD_TYPE_NICK)) {
                player->RemovePlaymod(PLAYMOD_TYPE_NICK);
            }
            player->SendLog("Your display name has been changed back to normal.");
            player->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
            world->SyncPlayerData(player);
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                if (!player->HasPlaymod(PLAYMOD_TYPE_INVISIBLE)) {
                    ply->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
                }
                else {
                    ply->v_sender.OnNameChanged(player->GetNetId(), fmt::format("{} (V)", player->GetDisplayName(world)));
                }
                });
            return;
        }
        std::string new_display_name = ctx.m_message.substr(6);
        if (new_display_name.length() > 20 && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->SendLog("The display name cannot be longer than 20 characters.");
            return;
        }
        if (new_display_name.length() < 3) {
            player->SendLog("The display name cannot be shorter than 3 characters.");
            return;
        }
        player->v_sender.OnTextOverlay("`wYou change your nickname.``");
        player->SetDisplayName(new_display_name);
        std::string folder_path = fmt::format("PlayerData/{}", ctx.m_player->GetRawName());
        std::string file_name = "PlayerData.txt";
        std::string file_path = folder_path + "/" + file_name;

        if (!fs::is_directory(folder_path)) {
            fs::create_directory(folder_path);
        }
        if (!fs::exists(file_path)) {
            std::ofstream outfile(file_path);
            outfile.close();
        }

        std::ifstream infile(file_path);
        std::string line;
        std::string custom_nickname;
        bool custom_nickname_found = false;
        while (std::getline(infile, line)) {
            if (line.find("CustomNickname:") == 0) {
                custom_nickname = line.substr(15);
                custom_nickname_found = true;
                break;
            }
        }
        infile.close();

        if (!custom_nickname_found) {
            std::ofstream outfile(file_path, std::ios::app);
            outfile << "CustomNickname: " << new_display_name << std::endl;
            outfile.close();
            custom_nickname = new_display_name;
        }
        else {
            std::ifstream infile(file_path);
            std::stringstream buffer;
            std::string line;
            while (std::getline(infile, line)) {
                if (line.find("CustomNickname:") == 0) {
                    buffer << "CustomNickname: " << new_display_name << std::endl;
                }
                else {
                    buffer << line << std::endl;
                }
            }
            infile.close();

            std::ofstream outfile(file_path);
            outfile << buffer.str();
            outfile.close();

            custom_nickname = new_display_name;
        }

        infile.open(file_path);
        while (std::getline(infile, line)) {
            if (line.find("CustomNickname:") == 0) {
                custom_nickname = line.substr(15);
                break;
            }
        }
        infile.close();
        if (!player->HasPlaymod(PLAYMOD_TYPE_NICK)) {
            player->AddPlaymod(PLAYMOD_TYPE_NICK, ITEM_TROLL_MASK, steady_clock::now(), std::chrono::seconds(-1));
        }
        player->SendLog("Your display name has been changed to `w{}`o.", new_display_name);
        player->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
        world->SyncPlayerData(player);
        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
            if (!player->HasPlaymod(PLAYMOD_TYPE_INVISIBLE)) {
                ply->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
            }
            else {
                ply->v_sender.OnNameChanged(player->GetNetId(), fmt::format("{} (V)", player->GetDisplayName(world)));
            }
            });
    }
    void CommandManager::command_pull(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (ctx.m_arguments.empty()) {
            player->SendLog("The second argument should be the player name you want to pull.");
            return;
        }
        std::string new_display_name = ctx.m_arguments[0];
        utils::to_lowercase(new_display_name); // fix lower letter world name
        for (auto& playerthing : ctx.m_servers->GetPlayers()) {
            std::string targetname = playerthing->GetRawName();
            utils::to_lowercase(targetname); // fix lower letter world name
            if (targetname == new_display_name && (world->IsOwner(player) || player->GetRole() >= PLAYER_ROLE_MODERATOR)) {
                if (world->HasPlayer(playerthing)) {
                    world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                        ply->SendLog("{} `5pulls`w {}", player->GetDisplayName(world), playerthing->GetDisplayName(world));
                        });
                    world->SendPull(playerthing, player);
                    int32_t effect_delay = 300;
                    GameUpdatePacket update_packet;
                    update_packet.m_type = NET_GAME_PACKET_ITEM_EFFECT;
                    update_packet.m_animation_type = 0x5;
                    update_packet.m_pos_x = playerthing->GetPosition().m_x;
                    update_packet.m_pos_y = playerthing->GetPosition().m_y;
                    update_packet.m_target_net_id = playerthing->GetNetId();
                    update_packet.m_delay = effect_delay;
                    update_packet.m_particle_size_alt = 200;
                    update_packet.m_item_id_alt = 0;
                    update_packet.m_item_count = 10;

                    world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                        ply->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
                    ply->v_sender.OnParticleEffect(20, CL_Vec2f{ update_packet.m_pos_x + 15, update_packet.m_pos_y + 15 }, effect_delay - 30);
                        });
                    effect_delay += 300;
                }
            }
        }
    }
    void CommandManager::command_pullall(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (!world)
            return;

        if (world->IsOwner(player) || player->GetRole() > PLAYER_ROLE_MODERATOR) {
            player->SendLog("You pulled all players in `w{}`o.", world->GetName());
        }
        for (auto& playerthing : world->GetPlayers(false)) {
            if (world->HasPlayer(playerthing) and playerthing != player) {
                std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                    ply->SendLog("{} `5pulls`w {}", player->GetDisplayName(world), playerthing->GetDisplayName(world));
                    });
                playerthing->SetPosition(player->GetPosition().m_x, player->GetPosition().m_y);
                world->SendPull(playerthing, player);
            }
        }
    }
    void CommandManager::command_spawnevent(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 2) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        if (!world) {
            return;
        }
        std::string event_name = ctx.m_message.substr(12);
        world->SpawnEvent(event_name);
    }
    void CommandManager::command_kickall(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (!world)
            return;

        if (world->IsOwner(player) || player->GetRole() > PLAYER_ROLE_MODERATOR) {
            player->SendLog("You kicked all players in `w{}`o.", world->GetName());
        }
        for (auto& playerthing : world->GetPlayers(false)) {
            if (world->HasPlayer(playerthing) and playerthing != player && (world->IsOwner(player) || player->GetRole() > PLAYER_ROLE_MODERATOR)) {
                std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                    ply->SendLog("{} `4kicks`w {}", player->GetDisplayName(world), playerthing->GetDisplayName(world));
                    });
                world->SendKick(playerthing, false, 2000);
            }
        }
    }
    void CommandManager::command_kick(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (ctx.m_arguments.empty()) {
            player->SendLog("The second argument should be the player name you want to kick.");
            return;
        }
        std::string new_display_name = ctx.m_arguments[0];
        utils::to_lowercase(new_display_name); // fix lower letter world name
        for (auto& playerthing : ctx.m_servers->GetPlayers()) {
            std::string targetname = playerthing->GetRawName();
            utils::to_lowercase(targetname); // fix lower letter world name
            if (targetname == new_display_name && (world->IsOwner(player) || player->GetRole() >= PLAYER_ROLE_MODERATOR)) {
                if (world->HasPlayer(playerthing)) {
                    world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                        ply->SendLog("{} `4kicks`w {}", player->GetDisplayName(world), playerthing->GetDisplayName(world));
                        });
                    world->SendKick(playerthing, false, 2000);
                }
            }
        }
    }
    void CommandManager::command_warp(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        std::string new_display_name = ctx.m_arguments[0];
        if (ctx.m_arguments.empty()) {
            player->SendLog("The second argument should be the world name you want to warp to.");
            return;
        }
        utils::to_uppercase(new_display_name); // fix lower letter world name
        if ((new_display_name == "QUIZZI" || new_display_name == "SCAMMER" || new_display_name == "KAAN" || new_display_name == "OWNER" || new_display_name == "HARRY" || new_display_name == "BETTERGROWTOPIA") && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("Sorry, that world is reserved.");
            return;
        }
        if (new_display_name.length() <= 2 && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("Sorry, worlds that are 2 letters or less are currently locked right now! ");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (new_display_name.find("BUY", 0) == 0 && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("Sorry, BUY worlds are currently locked right now! ");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (new_display_name.find("SELL", 0) == 0 && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("Sorry, SELL worlds are currently locked right now! ");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        auto world2{ ctx.m_server->GetWorldPool()->GetWorld(new_display_name) };
        if (world2->IsFlagOn(WORLDFLAG_NUKED) && player->GetRole() < PLAYER_ROLE_MODERATOR) {
            player->SendLog("You can't warp there, sorry!");
            return;
        }
        if (player->HasPlaymod(PLAYMOD_TYPE_CURSE)) {
            player->SendLog("You can't warp right now, sorry!");
            return;
        }
        else {
            world->RemovePlayer(player);
            ctx.m_server->GetWorldPool()->OnPlayerLeave(world, player, false);
            // player->SetNetId(world2->AddPlayer(player));
            player->SendLog("You warped to `w{}`o.", new_display_name);
            ctx.m_server->GetWorldPool()->OnPlayerJoin(ctx.m_servers, world2, player, world2->GetTilePos(ITEMTYPE_MAIN_DOOR));
            ctx.m_server->GetWorldPool()->OnPlayerSyncing(world2, player);
            ctx.m_player->PlaySfx("object_spawn", 0);
        }
    }
    void CommandManager::command_summon(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (ctx.m_arguments.empty()) {
            player->SendLog("The second argument should be the player name you want to summon.");
            return;
        }
        std::string new_display_name = ctx.m_arguments[0];
        utils::to_lowercase(new_display_name); // fix lower letter world name
        for (auto& playerthing : ctx.m_servers->GetPlayers()) {
            std::string targetname = playerthing->GetRawName();
            utils::to_lowercase(targetname); // fix lower letter world name
            if (targetname == new_display_name) {
                std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                std::shared_ptr<World> world2{ world_pool->GetWorld(playerthing->GetWorld()) };
                if (world2) {
                    world2->RemovePlayer(playerthing);
                    ctx.m_server->GetWorldPool()->OnPlayerLeave(world2, playerthing, false);
                    ctx.m_player->PlaySfx("object_spawn", 0);
                }
                // player->SetNetId(world2->AddPlayer(player));
                // world2->AddPlayer(player);
                ctx.m_server->GetWorldPool()->OnPlayerJoin(ctx.m_servers, world, playerthing, world->GetTilePos(ITEMTYPE_MAIN_DOOR));
                ctx.m_server->GetWorldPool()->OnPlayerSyncing(world, playerthing);
                if (player->GetRole() > PLAYER_ROLE_MODERATOR) {
                    world->SendPull(playerthing, player);
                }
                return;
            }
        }
        player->SendLog("There is no one online called {}", new_display_name);
    }
    void CommandManager::command_warpto(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (ctx.m_arguments.empty()) {
            player->SendLog("The second argument should be the player name you want to summon.");
            return;
        }
        std::string new_display_name = ctx.m_arguments[0];
        utils::to_lowercase(new_display_name); // fix lower letter world name
        for (auto& playerthing : ctx.m_servers->GetPlayers()) {
            std::string targetname = playerthing->GetRawName();
            utils::to_lowercase(targetname); // fix lower letter world name
            if (targetname == new_display_name) {
                std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                std::shared_ptr<World> world2{ world_pool->GetWorld(playerthing->GetWorld()) };
                if (!world2) {
                    player->SendLog("They are in exit right now!");
                    return;
                }
                world->RemovePlayer(player);
                ctx.m_server->GetWorldPool()->OnPlayerLeave(world, player, false);
                // player->SetNetId(world2->AddPlayer(player));
                // world2->AddPlayer(player);
                ctx.m_server->GetWorldPool()->OnPlayerJoin(ctx.m_servers, world2, player, world2->GetTilePos(ITEMTYPE_MAIN_DOOR));
                ctx.m_server->GetWorldPool()->OnPlayerSyncing(world2, player);
                ctx.m_player->PlaySfx("object_spawn", 0);
                if (player->GetRole() > PLAYER_ROLE_MODERATOR) {
                    world2->SendPull(player, playerthing);
                }
                return;
            }
        }
        player->SendLog("There is no one online called {}", new_display_name);
    }
    void CommandManager::command_nuke(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (!world)
            return;

        if (player->GetRole() >= PLAYER_ROLE_MODERATOR) {
            world->SetFlag(WORLDFLAG_NUKED);
            player->SendLog("You nuked `w{}`o.", world->GetName());
            player->v_sender.OnPlayPositioned("cry", player->GetNetId(), 0);
        }
        for (auto& playerthing : ctx.m_servers->GetPlayers()) {
            playerthing->SendLog("{} has been `4NUKED `ofrom orbit!", world->GetName());
            playerthing->PlaySfx("bigboom", 0);
        }
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::ofstream ban_log_file("nukelogs.txt", std::ios_base::app);
        ban_log_file << world->GetName() << " was nuked by " << player->GetRawName() << " at " << std::put_time(std::localtime(&now), "%c %Z") << std::endl;
        fmt::print("{} was nuked by {}\n", world->GetName(), player->GetRawName());
        for (auto& playerthing : world->GetPlayers(false)) {
            if (playerthing->GetRole() < PLAYER_ROLE_MODERATOR and world->HasPlayer(playerthing)) {
                std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                    ply->SendLog("{} `4world bans`w {}", player->GetDisplayName(world), playerthing->GetDisplayName(world));
                ply->PlaySfx("repair", 0);
                    });
                world_pool->OnPlayerLeave(world, playerthing, true);
            }
        }
    }
    void CommandManager::command_unnuke(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (!world)
            return;
        if (player->GetRole() >= PLAYER_ROLE_MODERATOR) {
            world->RemoveFlag(WORLDFLAG_NUKED);
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::ofstream ban_log_file("unnukelogs.txt", std::ios_base::app);
            ban_log_file << world->GetName() << " was unnuked by " << player->GetRawName() << " at " << std::put_time(std::localtime(&now), "%c %Z") << std::endl;
            fmt::print("{} was unnuked by {}\n", world->GetName(), player->GetRawName());
            player->SendLog("You un-nuked `w{}`o.", world->GetName());
        }
    }
    void CommandManager::command_clearplaymods(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        for (const auto& mod : ctx.m_player->GetPlaymods()) {
            if (mod.m_type != PLAYMOD_TYPE_GHOST_IN_THE_SHELL && mod.m_type != PLAYMOD_TYPE_NICK && mod.m_type != PLAYMOD_TYPE_INVISIBLE) {
                ctx.m_player->RemovePlaymod(mod.m_type);
            }
        }
    }
    void CommandManager::command_gban(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        std::shared_ptr<Player> player = ctx.m_player;
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 2) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        std::shared_ptr<Player> person{ ctx.m_servers->GetPlayerByFormat(ctx.m_arguments[0]) };
        if (!person) {
            ctx.m_player->SendLog("`4Oops``, couldn't find a player with userId/name starting with `w{}``.", ctx.m_arguments[0]);
            return;
        }
        if (person->GetRole() > player->GetRole()) {
            player->SendLog("You can't punish people with a higher role than you!");
            return;
        }
        std::string ban_reason = ctx.m_message.substr(6);
        if (!person->HasPlaymod(PLAYMOD_TYPE_BAN)) {
            std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
            player->v_sender.OnConsoleMessage(fmt::format("You've banned {}.", person->GetDisplayName(world)));
            person->AddPlaymod(PLAYMOD_TYPE_BAN, ITEM_BAN_WAND, steady_clock::now(), std::chrono::seconds(9999999999999));
            for (auto& playerthing2 : ctx.m_servers->GetPlayers()) {
                playerthing2->SendLog("`5** `$The Ancient Ones `ohave `4banned`o {} `5** `w(`4/rules`o to see the rules!)", person->GetDisplayName(world));
            }
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::ofstream ban_log_file_check;
            ban_log_file_check.open(fmt::format("bans/{}.txt", person->GetRawName()), std::ios::trunc | std::ios::out);
            std::ofstream ban_log_file(fmt::format("bans/{}.txt", person->GetRawName()), std::ios_base::app);
            ban_log_file << ban_reason << std::endl;
            std::ofstream ban_log_file_main("banlogs.txt", std::ios_base::app);
            ban_log_file_main << person->GetRawName() << " was banned by " << player->GetRawName() << " at " << std::put_time(std::localtime(&now), "%c %Z") << std::endl;
            fmt::print("{} was banned by {}\n", person->GetDisplayName(world), player->GetRawName());

            PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
            const auto& result{ database->Save(person) };
            if (person->IsFlagOn(PLAYERFLAG_LOGGED_ON)) {
                person->v_sender.OnAddNotification("`wWarning from `4System``: You've been `4BANNED`` from `wBetterGrowtopia! ", "interface/atomic_button.rttex", "audio/hub_open.wav");
                person->PlaySfx("already_used", 0);
                person->PlaySfx("hub_open", 0);
                person->PlaySfx("bgt_ban", 0);
                person->Disconnect(0U);
                person->SendLog("`oWarning from `4System`o: You've been `4BANNED `ofrom `wBetterGrowtopia`o.");
            }
        }
    }
    void CommandManager::command_ban(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        std::shared_ptr<Player> player = ctx.m_player;
        if (ctx.m_arguments.empty()) {
            player->SendLog("The other arguments can not be empty.");
            return;
        }
        std::string new_display_name = ctx.m_arguments[0];
        utils::to_lowercase(new_display_name); // fix lower letter world name
        for (auto& playerthing : ctx.m_servers->GetPlayers()) {
            std::string targetname = playerthing->GetRawName();
            utils::to_lowercase(targetname); // fix lower letter world name
            if (targetname == new_display_name && !world->HasBan(player) && (world->IsOwner(player) || player->GetRole() > PLAYER_ROLE_MODERATOR)) {
                std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                    ply->SendLog("{} `4world bans`w {}", player->GetDisplayName(world), playerthing->GetDisplayName(world));
                    ply->PlaySfx("repair", 0);
                    });
                player->SendLog("You've banned {} from `w{} `ofor 10 minutes! You can also type `5/uba `oto unban him/her early.", playerthing->GetDisplayName(world), world->GetName());
                world->BanPlayer(player->GetUserId());
            }
        }
    }
    void CommandManager::command_uba(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        std::shared_ptr<Player> player = ctx.m_player;
        if (!world->IsOwner(player) && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            return;
        }
        world->ClearBans();
        player->SendLog("You've unbanned everybody from this world!");
    }
    void CommandManager::command_unban(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        std::shared_ptr<Player> player = ctx.m_player;
        if (ctx.m_arguments.empty()) {
            player->SendLog("The other arguments can not be empty.");
            return;
        }
        std::shared_ptr<Player> person{ ctx.m_servers->GetPlayerByFormat(ctx.m_arguments[0]) };
        if (!person) {
            ctx.m_player->SendLog("`4Oops``, couldn't find a player with userId/name starting with `w{}``.", ctx.m_arguments[0]);
            return;
        }
        if (person->HasPlaymod(PLAYMOD_TYPE_BAN)) {
            player->v_sender.OnConsoleMessage(fmt::format("You've unbanned {}.", person->GetDisplayName(world)));
            person->RemovePlaymod(PLAYMOD_TYPE_BAN);
            auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::ofstream ban_log_file("unbanlogs.txt", std::ios_base::app);
            ban_log_file << person->GetRawName() << " was unbanned by " << player->GetRawName() << " at " << std::put_time(std::localtime(&now), "%c %Z") << std::endl;
            fmt::print("{} was unbanned by {}\n", person->GetDisplayName(world), player->GetRawName());
            PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
            const auto& result{ database->Save(person) };
        }
    }
    void CommandManager::command_readyall(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (!world)
            return;
        for (auto& t : world->GetTiles()) {
            if (t.GetBaseItem()->m_item_type == ITEMTYPE_SEED || t.GetBaseItem()->m_item_type == ITEMTYPE_PROVIDER) {
                if (t.GetBaseItem()->m_item_type == ITEMTYPE_PROVIDER) {
                    t.m_planted_date = t.GetPlantedDate() - std::chrono::seconds(t.GetBaseItem()->m_grow_time);
                }
                else {
                    t.m_planted_date = t.GetPlantedDate() - std::chrono::seconds(t.GetBaseItem()->m_grow_time);
                }
            }
        }
        player->v_sender.OnConsoleMessage("Made all trees and providers ready.");
    }
    void CommandManager::command_clearworld(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        std::shared_ptr<Player> player = ctx.m_player;
        if (!world)
            return;
        std::vector<Tile*> valid_tiles{};
        for (auto& t : world->GetTiles()) {
            if (t.GetBaseItem()->m_item_type != ITEMTYPE_MAIN_DOOR && t.GetBaseItem()->m_item_type != ITEMTYPE_BEDROCK && t.GetBaseItem()->m_item_type != ITEMTYPE_LOCK) {
                t.SetForeground(ITEM_BLANK);
                t.SetBackground(ITEM_BLANK);
                valid_tiles.emplace_back(world->GetTile(t.GetPosition()));
            }
        }
        world->SendTileUpdate(valid_tiles);
    }
    void CommandManager::command_1hit(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;

        std::shared_ptr<Player> player = ctx.m_player;

        if (player->HasPlaymod(PLAYMOD_TYPE_1HIT)) {
            player->RemovePlaymod(PLAYMOD_TYPE_1HIT);
        }
        else {
            player->AddPlaymod(PLAYMOD_TYPE_1HIT, ITEM_FIST, std::chrono::steady_clock::now(), std::chrono::seconds(-1));
        }

        world->SyncPlayerData(player);
    }
    void CommandManager::command_spawneffect(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;

        std::shared_ptr<Player> player = ctx.m_player;

        std::string temporary_list{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };

        std::string new_display_name = ctx.m_arguments[0];
        if (ctx.m_arguments.empty() || ctx.m_arguments.size() < 2) {
            ctx.m_player->v_sender.OnConsoleMessage(ctx.m_command->GetDescription());
            return;
        }
        utils::to_lowercase(new_display_name); // fix lower letter world name
        int32_t effect_delay = 300;
        static randutils::pcg_rng gen{ utils::random::get_generator_local() };
        for (const auto& item : temporary_list) {
            GameUpdatePacket update_packet;
            update_packet.m_type = NET_GAME_PACKET_ITEM_EFFECT;
            update_packet.m_animation_type = 0x5;
            update_packet.m_pos_x = ctx.m_player->GetPosition().m_x + gen.uniform(-128, 128);
            update_packet.m_pos_y = ctx.m_player->GetPosition().m_y + gen.uniform(-128, 128);
            update_packet.m_target_net_id = ctx.m_player->GetNetId();
            update_packet.m_delay = effect_delay;
            update_packet.m_particle_size_alt = 90;
            update_packet.m_item_id_alt = 0;
            update_packet.m_item_count = 10;

            world->Broadcast([&](const std::shared_ptr<Player>& player) {
                player->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
            player->v_sender.OnParticleEffect(stoi(new_display_name), CL_Vec2f{update_packet.m_pos_x + 15, update_packet.m_pos_y + 15}, effect_delay - 30);
                });
            effect_delay += 300;
        }
    }
    void CommandManager::command_invis(const CommandContext& ctx) {
        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;

        std::shared_ptr<Player> player = ctx.m_player;

        std::string temporary_list{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

        int32_t effect_delay = 300;
        static randutils::pcg_rng gen{ utils::random::get_generator_local() };
        for (const auto& item : temporary_list) {
            GameUpdatePacket update_packet;
            update_packet.m_type = NET_GAME_PACKET_ITEM_EFFECT;
            update_packet.m_animation_type = 0x5;
            update_packet.m_pos_x = ctx.m_player->GetPosition().m_x + gen.uniform(-128, 128);
            update_packet.m_pos_y = ctx.m_player->GetPosition().m_y + gen.uniform(-128, 128);
            update_packet.m_target_net_id = ctx.m_player->GetNetId();
            update_packet.m_delay = effect_delay;
            update_packet.m_item_id_alt = 0;
            update_packet.m_item_count = 10;

            world->Broadcast([&](const std::shared_ptr<Player>& player) {
                player->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
            player->v_sender.OnParticleEffect(3, CL_Vec2f{ update_packet.m_pos_x + 15, update_packet.m_pos_y + 15 }, effect_delay - 30);
                });
            effect_delay += 300;
        }
        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
            ply->PlaySfx("magic", 0);
            });

        if (player->IsFlagOn(PLAYERFLAG_IS_INVISIBLE)) {
            player->RemoveFlag(PLAYERFLAG_IS_INVISIBLE);
            player->RemovePlaymod(PLAYMOD_TYPE_INVISIBLE);
            player->v_sender.OnInvis(player->GetNetId(), 0, 0);

            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->v_sender.OnInvis(player->GetNetId(), 0, 0);
            ply->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
                });
            world->SyncPlayerData(player);
        }
        else {
            player->SetFlag(PLAYERFLAG_IS_INVISIBLE);
            player->AddPlaymod(PLAYMOD_TYPE_INVISIBLE, ITEM_GHOST_COSTUME, steady_clock::now(), std::chrono::seconds(-1));
            player->v_sender.OnInvis(player->GetNetId(), 0, 1);

            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                if (ply->GetRawName() != player->GetRawName()) {
                    if (ply->GetRole() < player->GetRole()) {
                        ply->v_sender.OnInvis(player->GetNetId(), 0, 1);
                    }
                    else {
                        ply->v_sender.OnInvis(player->GetNetId(), 0, 0);
                        ply->v_sender.OnNameChanged(player->GetNetId(), fmt::format("{} (V)", player->GetDisplayName(world)));
                    }
                }
                });
            world->SyncPlayerData(player);
        }
    }


}
