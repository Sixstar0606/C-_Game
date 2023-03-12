#pragma once
#include <functional>
#include <fmt/ranges.h>
#include <fmt/chrono.h>
#include <event/event_context.h>
#include <proton/utils/text_scanner.h>
#include <database/database.h>
#include <discord/discord_bot.h>
#include <store/store_manager.h>
#include <utils/text.h>
#include <utils/random.h>
#include <algorithm/algorithm.h>
#include <sstream>

namespace GTServer::events {
    void dialog_return(EventContext& ctx) {
        if (ctx.m_parser.Get("dialog_name", 1).empty())
            return;
        std::string dialog_name = ctx.m_parser.Get("dialog_name", 1);
        const auto& dialog_hash = utils::quick_hash(dialog_name);
        switch (dialog_hash) {
            case "growid_apply"_qh: { 
                std::string 
                    name{ ctx.m_parser.Get("logon", 1) },
                    password{ ctx.m_parser.Get("password", 1) },
                    verify_password{ ctx.m_parser.Get("verify_password", 1) },
                    verify_email{ ctx.m_parser.Get("verify_email", 1) }
                ;
                PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                const auto& result{ database->RegisterPlayer(name, password, verify_password) };
                if (result.first != PlayerTable::RegistrationResult::SUCCESS) {
                    ctx.m_player->SendDialog(Player::DIALOG_TYPE_REGISTRATION, TextScanner({ 
                        { "logon", name }, 
                        { "password", password },
                        { "verify_password", verify_password },
                        { "verify_email", verify_email },
                        { "extra", result.second }
                    }));
                    break;
                }
                auto lower_name = name;
                std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
                auto login = ctx.m_player->GetLoginDetail();
                login->m_tank_id_name = lower_name;
                login->m_tank_id_pass = password;
                login->m_email = verify_email;
                ctx.m_player->SetRawName(name);
                ctx.m_player->SetDisplayName(name);
                ctx.m_player->m_inventory.Add(ITEM_FIST, 1);
                ctx.m_player->m_inventory.Add(ITEM_WRENCH, 1);
                ctx.m_player->m_inventory.Add(ITEM_MY_FIRST_WORLD_LOCK, 1);
                ctx.m_player->SetUserId(database->Insert(ctx.m_player));

                ctx.m_player->PlaySfx("success", 0);
                ctx.m_player->v_sender.SetHasGrowID(true, name, login->m_tank_id_pass);
                ctx.m_player->v_sender.OnConsoleMessage(fmt::format("`oA `wGrowID`` with the logon of `w{}`` created. Write it and your password down as the will be required to logon!``", ctx.m_player->GetRawName()));
                ctx.m_player->v_sender.OnConsoleMessage("`5Welcome, please press `wBack`` and then press `wConnect``, enjoy `2BetterGrowtopia`` ;)");
            } break;
            case "account_verify"_qh: {
                if (!ctx.m_player->IsFlagOn(PLAYERFLAG_IS_IN) || ctx.m_player->GetDiscord() != 0)
                    return;
                std::string discord_account;
                if (!ctx.m_parser.TryGet("discord_account", discord_account))
                    return;
                ctx.m_servers->AddQueue(QUEUE_TYPE_ACCOUNT_VERIFICATION, ServerQueue{ 
                    .m_keyword = discord_account,
                    .m_player = ctx.m_player
                });
            } break;
            case "search_item"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  

                int count = std::atoi(ctx.m_parser.Get("count", 1).c_str());
                if (count < 1 || count > 200) {
                    ctx.m_player->SendLog("`4Oops!`` the given input is invalid, please enter in range of 1-200");
                    return;
                }
                const auto& data = ctx.m_parser.get_data();
                std::vector<int> items_list, added_items;
                std::vector<std::pair<std::string, uint8_t>> claimed_items;

                for (auto index = 0; index < data.size(); index++) {
                    if (data[index].find("item_") == std::string::npos)
                        continue;
                    if (utils::split(data[index], "|")[1] != "1")
                        continue;
                    items_list.push_back(std::atoi(data[index].substr(5).c_str()));
                }
                if (items_list.empty())
                    return;
                for (const auto& it : items_list) {
                    const auto& item = ItemDatabase::GetItem(it);
                    if (!item)
                        continue;
                    if (item->m_id == ITEM_BLANK)
                        continue;
                    int custom_count = 0;
                    if (ctx.m_player->m_inventory.GetItemCount(item->m_id) + count > item->m_max_amount)
                        custom_count = item->m_max_amount - ((ctx.m_player->m_inventory.GetItemCount(item->m_id) + count) - count);
                    if (ctx.m_player->m_inventory.Add(item->m_id, custom_count != 0 ? custom_count : count, true)) {
                        added_items.push_back(item->m_id);
                        claimed_items.push_back({ item->m_name, (custom_count != 0 ? custom_count : count) });
                    }
                }
                ctx.m_player->SendLog(fmt::format("Claimed `w{}``/`w{}`` Selected Items", added_items.size(), items_list.size()));
                ctx.m_player->SendLog(fmt::format("Items: {}", claimed_items));

                int32_t effect_delay = 300;
                static randutils::pcg_rng gen{ utils::random::get_generator_local() };
                for (const auto& item : added_items) {
                    GameUpdatePacket update_packet;
                    update_packet.m_type = NET_GAME_PACKET_ITEM_EFFECT;
                    update_packet.m_animation_type = 0x5;
                    update_packet.m_pos_x = ctx.m_player->GetPosition().m_x + gen.uniform(-128, 128);
                    update_packet.m_pos_y = ctx.m_player->GetPosition().m_y + gen.uniform(-128, 128);
                    update_packet.m_target_net_id = ctx.m_player->GetNetId();
                    update_packet.m_delay = effect_delay;
                    update_packet.m_item_id_alt = item;
                    update_packet.m_item_count = 10;

                    world->Broadcast([&](const std::shared_ptr<Player>& player) { 
                        player->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket)); 
                        player->v_sender.OnParticleEffect(108, CL_Vec2f{ update_packet.m_pos_x + 15, update_packet.m_pos_y + 15 }, effect_delay - 30);
                    });
                    effect_delay += 300;
                }
                ctx.m_player->PlaySfx("success", 0);
            } break;
            case "drop_item"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  

                uint32_t item_id;
                uint8_t count;
                if (!(ctx.m_parser.TryGet("itemID", item_id) && ctx.m_parser.TryGet("count", count)))
                    return;
                 ItemInfo* item = ItemDatabase::GetItem(item_id);
                if (!item)
                    return;
                if (!ctx.m_player->m_inventory.Contain(item->m_id))
                    return;
                if (count < 1 || count > ctx.m_player->m_inventory.GetItemCount(item->m_id)) {
                    ctx.m_player->SendLog("`oYou cannot do this.``");
                    return;
                } 

                if ((item->m_item_category & ITEMFLAG2_UNTRADABLE) || (item->m_item_category & ITEMFLAG2_MOD)) {
                    ctx.m_player->v_sender.OnTextOverlay("`wYou can't drop that.``");
                    return;
                }
            
                CL_Vec2i position = { ctx.m_player->GetPosition().m_x / 32 + (ctx.m_player->IsFlagOn(PLAYERFLAG_IS_FACING_LEFT) ? -1 : 1), ctx.m_player->GetPosition().m_y / 32 };
                Tile* tile_next = world->GetTile(position);
                if (!tile_next) {
                    ctx.m_player->v_sender.OnTextOverlay("`wYou can't drop that here, face somewhere with open space.``");
                    return;
                } 
                ItemInfo* base = tile_next->GetBaseItem();
                if (base->m_collision_type == ITEMCOLLISION_NORMAL || base->m_collision_type == ITEMCOLLISION_GUILDENTRANCE || base->m_collision_type == ITEMCOLLISION_GATEWAY) {
                    ctx.m_player->v_sender.OnTextOverlay("`wYou can't drop that here, face somewhere with open space.``");
                    return;
                } 
                
                switch (base->m_item_type) {
                case ITEMTYPE_MAIN_DOOR: {
                    ctx.m_player->v_sender.OnTextOverlay(fmt::format("`wYou can't drop items on `2{}``.``", base->m_name));
                    return;
                } break;
                case ITEMTYPE_MAGIC_EGG: {
                    if (item->m_id != ITEM_MAGIC_EGG)
                        break;
                    if (tile_next->GetEggsPlaced() > 2000) {
                        ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "This Magic Egg already at maximum size.", true);
                        return;
                    }
                    if (!ctx.m_player->RemoveItemSafe(item->m_id, count, true))
                        return;
                    tile_next->m_eggs_placed += count;

                    GameUpdatePacket effect_packet {
                        .m_type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
                        .m_particle_alt_id = 66
                    };
                    effect_packet.m_pos_x = (tile_next->GetPosition().m_x * 32) + 15;
                    effect_packet.m_pos_y = (tile_next->GetPosition().m_y * 32) + 15;

                    world->Broadcast([&](const std::shared_ptr<Player>& player) { player->SendPacket(NET_MESSAGE_GAME_PACKET, &effect_packet, sizeof(GameUpdatePacket)); });
                    world->SendTileUpdate(tile_next, 0);
                } return;
                default:
                    break;
                }
                if (!ctx.m_player->RemoveItemSafe(item->m_id, count, true))
                    return;
                WorldObject object {
                    .m_item_id = static_cast<uint16_t>(item->m_id),
                    .m_item_amount = count,
                    .m_pos = { 
                        (float)(ctx.m_player->GetPosition().m_x + (ctx.m_player->IsFlagOn(PLAYERFLAG_IS_FACING_LEFT) ? -25 : 25)),
                        (float)(ctx.m_player->GetPosition().m_y) 
                    }
                };
                world->SyncPlayerData(ctx.m_player);
                world->AddObject(object, true);
            } break;
            case "manage_menu"_qh: {
                if (ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
                    return;
                std::string manage_type = "";
                if (!ctx.m_parser.TryGet("manage_type", manage_type))
                    return;
                switch (utils::quick_hash(manage_type)) {
                case "world"_qh: {
                    std::string world_name, buttonClicked;
                    if (!(ctx.m_parser.TryGet("world_name", world_name) && ctx.m_parser.TryGet("buttonClicked", buttonClicked)))
                        return;
                    auto world{ ctx.m_server->GetWorldPool()->GetWorld(world_name) };
                    if (!world)
                        return; 
                    switch (utils::quick_hash(buttonClicked)) {
                    case "list_players"_qh: {
                        DialogBuilder db{};
                        db.set_default_color('o')
                            ->add_label_with_icon(fmt::format("`wPlayers List`` -> `2{}``", world->GetName()), ITEM_MINI_GROWTOPIAN, DialogBuilder::LEFT, DialogBuilder::BIG)
                            ->add_smalltext("`4INFO``: please don't use this to `4abuse`` in any way, we store every action done by moderators.")
                            ->add_spacer();
                        for (auto& player : world->GetPlayers(true))
                            db.add_button(fmt::format("uid_{}", player->GetUserId()), fmt::format("`5#{} `w-`` {}``", player->GetUserId(), player->GetDisplayName(world)));
                        db.add_quick_exit()
                            ->end_dialog("find_player", "Cancel", "");
                        ctx.m_player->v_sender.OnDialogRequest(db.get());
                    } break;
                    case "toggle_nuke"_qh: {
                        auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                        if (!world)
                            return;
                        if (world->IsFlagOn(WORLDFLAG_NUKED)) {
                            world->RemoveFlag(WORLDFLAG_NUKED);
                            ctx.m_player->SendLog("You un-nuked `w{}`o.", world->GetName());
                        }
                        else {
                            ctx.m_player->SendLog("You nuked `w{}`o.", world->GetName());
                            world->SetFlag(WORLDFLAG_NUKED);
                            for (auto& playerthing : world->GetPlayers(false)) {
                                if (playerthing->GetRole() < PLAYER_ROLE_MODERATOR and world->HasPlayer(playerthing)) {
                                    std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                                    playerthing->SendLog("You have been removed from the world!");
                                    world_pool->OnPlayerLeave(world, playerthing, true);
                                    playerthing->SendLog("{} has been `4NUKED `ofrom orbit!", world->GetName());
                                    playerthing->PlaySfx("bigboom", 0);
                                }
                                else {
                                    playerthing->SendLog("{} has been `4NUKED `ofrom orbit!", world->GetName());
                                    playerthing->PlaySfx("bigboom", 0);
                                }
                            }
                        }
                    } break;
                    }
                } break;
                }
            } break;
            case "punishment_menu"_qh: {
                if (ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
                    return;
                std::string buttonClicked = "";
                if (!ctx.m_parser.TryGet("buttonClicked", buttonClicked))
                    return;
                std::string
                    ban_reason{ ctx.m_parser.Get("ban_reason", 1) },
                    ban_time{ ctx.m_parser.Get("ban_time", 1) },
                    target_player{ ctx.m_parser.Get("offline_name", 1) }
                ;
                switch (utils::quick_hash(buttonClicked)) {
                case "warpto"_qh: {
                    auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                    if (ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
                        return;
                    std::string buttonClicked = "";
                    if (!ctx.m_parser.TryGet("buttonClicked", buttonClicked))
                        return;
                    if (buttonClicked.find("uid_") != std::string::npos) {
                        uint32_t user_id = std::atoi(buttonClicked.substr(4).c_str());
                        std::shared_ptr<Player> target = ctx.m_servers->GetPlayerByUserID(user_id);
                        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                        std::shared_ptr<World> world2{ world_pool->GetWorld(target->GetWorld()) };
                        if (!world2) {
                            ctx.m_player->SendLog("They are in exit right now!");
                            return;
                        }
                        // world->RemovePlayer(player);
                        ctx.m_server->GetWorldPool()->OnPlayerLeave(world, ctx.m_player, false);
                        // player->SetNetId(world2->AddPlayer(player));
                        // world2->AddPlayer(player);
                        ctx.m_server->GetWorldPool()->OnPlayerJoin(ctx.m_servers, world2, ctx.m_player, world2->GetTilePos(ITEMTYPE_MAIN_DOOR));
                        ctx.m_server->GetWorldPool()->OnPlayerSyncing(world2, ctx.m_player);
                        if (ctx.m_player->GetRole() > PLAYER_ROLE_MODERATOR) {
                            ctx.m_player->SetPosition(target->GetPosition().m_x, target->GetPosition().m_y);
                        }
                    }
                } break;
                case "ban_player"_qh: {
                    auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                    std::shared_ptr<Player> player = ctx.m_player;
                    if (ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
                        return;
                    std::shared_ptr<Player> target = ctx.m_servers->GetPlayerByName(target_player);
                    if (target->GetRole() > player->GetRole()) {
                        player->SendLog("You can't punish people with a higher role than you!");
                        return;
                    }
                    if (!target->HasPlaymod(PLAYMOD_TYPE_BAN)) {
                        player->v_sender.OnConsoleMessage(fmt::format("You've banned {}.", target->GetDisplayName(world)));
                        target->AddPlaymod(PLAYMOD_TYPE_BAN, ITEM_BAN_WAND, steady_clock::now(), std::chrono::seconds(stoi(ban_time)));
                        for (auto& playerthing2 : ctx.m_servers->GetPlayers()) {
                            playerthing2->SendLog("`5** `$The Ancient Ones `ohave `4banned`o {} `5** `w(`4/rules`o to see the rules!)", target->GetDisplayName(world));
                        }
                        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                        std::ofstream ban_log_file_check;
                        ban_log_file_check.open(fmt::format("bans/{}.txt", target->GetRawName()), std::ios::trunc | std::ios::out);
                        std::ofstream ban_log_file(fmt::format("bans/{}.txt", target->GetRawName()), std::ios_base::app);
                        ban_log_file << ban_reason << std::endl;
                        std::ofstream ban_log_file_main("banlogs.txt", std::ios_base::app);
                        ban_log_file_main << target->GetRawName() << " was banned by " << player->GetRawName() << " at " << std::put_time(std::localtime(&now), "%c %Z") << std::endl;
                        fmt::print("{} was banned by {}\n", target->GetDisplayName(world), player->GetRawName());

                        PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                        const auto& result{ database->Save(target) };
                        if (target->IsFlagOn(PLAYERFLAG_LOGGED_ON)) {
                            target->v_sender.OnAddNotification("`wWarning from `4System``: You've been `4BANNED`` from `wBetterGrowtopia! ", "interface/atomic_button.rttex", "audio/hub_open.wav");
                            target->PlaySfx("already_used", 0);
                            target->PlaySfx("hub_open", 0);
                            target->PlaySfx("bgt_ban", 0);
                            target->Disconnect(0U);
                            target->SendLog("`oWarning from `4System`o: You've been `4BANNED `ofrom `wBetterGrowtopia`o.");
                        }
                    }
                } break;
                default:
                    break;
                }
            } break;
            case "acceptlock"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;
                if (ctx.m_player->get_access_offer() == -1)
                    return;
                Tile* tile = world->GetTile(ctx.m_player->get_access_offer().m_x, ctx.m_player->get_access_offer().m_y);
                if (!tile)
                    return;
                if (tile->GetBaseItem()->m_item_type != ITEMTYPE_LOCK)
                    return;
                tile->AddAccess(ctx.m_player->GetUserId());
                ctx.m_player->SendLog("You accepted access to the lock!");
                world->SyncPlayerData(ctx.m_player);
                return;
            } break;
            case "wrench_menu"_qh: {
                if (ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
                    return;
                std::string buttonClicked = "";
                if (!ctx.m_parser.TryGet("buttonClicked", buttonClicked))
                    return;
                switch (utils::quick_hash(buttonClicked)) {
                case "wrench_kick"_qh: {
                    auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                    if (!world)
                        return;
                    uint32_t target_net_id;
                    if (ctx.m_parser.TryGet("target", target_net_id)) {
                        for (auto& player : world->GetPlayers(true)) {
                            if (target_net_id == player->GetUserId()) {
                                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                                    ply->SendLog("{} `4kicks`w {}", ctx.m_player->GetDisplayName(world), player->GetDisplayName(world));
                                    });
                                world->SendKick(player, false, 2000);
                                return;
                            }
                        }
                    }
                } break;
                case "wrench_pull"_qh: {
                    auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                    if (!world)
                        return;
                    uint32_t target_net_id;
                    if (ctx.m_parser.TryGet("target", target_net_id)) {
                        for (auto& player : world->GetPlayers(true)) {
                            if (target_net_id == player->GetUserId()) {
                                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                                    ply->SendLog("{} `5pulls`w {}", ctx.m_player->GetDisplayName(world), player->GetDisplayName(world));
                                    });
                                world->SendPull(player, ctx.m_player);
                                int32_t effect_delay = 300;
                                GameUpdatePacket update_packet;
                                update_packet.m_type = NET_GAME_PACKET_ITEM_EFFECT;
                                update_packet.m_animation_type = 0x5;
                                update_packet.m_pos_x = player->GetPosition().m_x;
                                update_packet.m_pos_y = player->GetPosition().m_y;
                                update_packet.m_target_net_id = player->GetNetId();
                                update_packet.m_delay = effect_delay;
                                update_packet.m_particle_size_alt = 90;
                                update_packet.m_item_id_alt = 0;
                                update_packet.m_item_count = 10;

                                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                                    ply->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
                                ply->v_sender.OnParticleEffect(20, CL_Vec2f{ update_packet.m_pos_x + 15, update_packet.m_pos_y + 15 }, effect_delay - 30);
                                    });
                                effect_delay += 300;
                                return;
                            }
                        }
                    }
                } break;
                case "wrench_ban"_qh: {
                    auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                    if (!world)
                        return;
                    uint32_t target_net_id;
                    if (ctx.m_parser.TryGet("target", target_net_id)) {
                        for (auto& player : world->GetPlayers(true)) {
                            if (target_net_id == player->GetUserId()) {
                                std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
                                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                                    ply->SendLog("{} `4world bans`w {}", ctx.m_player->GetDisplayName(world), player->GetDisplayName(world));
                                ply->PlaySfx("repair", 0);
                                    });
                                ctx.m_player->SendLog("You've banned {} from `w{} `ofor 10 minutes! You can also type `5/uba `oto unban him/her early.", player->GetDisplayName(world), world->GetName());
                                world->BanPlayer(player->GetUserId());
                                world->SyncPlayerData(player);
                                world_pool->OnPlayerLeave(world, player, true);
                                return;
                            }
                        }
                    }
                } break;
                case "wrench_punish"_qh: {
                    auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                    if (!world)
                        return;
                    uint32_t target_net_id;
                    if (ctx.m_parser.TryGet("target", target_net_id)) {
                        for (auto& player : world->GetPlayers(true)) {
                            if (target_net_id == player->GetUserId()) {
                                ctx.m_player->SendDialog(Player::DIALOG_TYPE_PUNISHMENT, TextScanner{}, nullptr, player);
                                return;
                            }
                        }
                    }
                } break;
                default:
                    break;
                }
            } break;
            case "find_player"_qh: {
                if (ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
                    return;
                std::string buttonClicked = "";
                if (!ctx.m_parser.TryGet("buttonClicked", buttonClicked))
                    return;
                if (buttonClicked.find("uid_") != std::string::npos) {
                    uint32_t user_id = std::atoi(buttonClicked.substr(4).c_str());
                    std::shared_ptr<Player> target = ctx.m_servers->GetPlayerByUserID(user_id);
                    ctx.m_player->SendDialog(Player::DIALOG_TYPE_PUNISHMENT, TextScanner{}, nullptr, target);
                    if (!ctx.m_servers->HasPlayer(user_id))
                        target.reset();
                }
            } break;
            case "socialportal"_qh: {
                if (ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
                    return;
                std::string buttonClicked = "";
                if (!ctx.m_parser.TryGet("buttonClicked", buttonClicked))
                    return;
                switch (utils::quick_hash(buttonClicked)) {
                case "modstuff"_qh: {
                    DialogBuilder db{};
                    db.set_default_color('o')
                        ->add_label_with_icon("`##Moderation Panel``", ITEM_SONGPYEON, DialogBuilder::LEFT, DialogBuilder::BIG)
                        ->add_spacer()
                        ->add_button("ply_lookup", "`#Players Lookup``")
                        ->add_button("wrd_lookup", "`#Worlds Lookup``")
                        ->add_button("gld_lookup", "`#Guilds Lookup``")
                        ->add_spacer()
                        ->add_smalltext("`4Warning``: please don't abuse this, we are logging every action you did.")
                        ->add_button("srv_tools", "`9Server Tools``")
                        ->add_button("psn_settings", "`9Personal Settings``")
                        ->add_quick_exit()
                        ->end_dialog("moderator_stuff", "`wCancel``", "`wBack``");
                    ctx.m_player->v_sender.OnDialogRequest(db.get());
                } break;
                case "showfriend"_qh: {
                    ctx.m_servers->AddQueue(QUEUE_TYPE_GET_FRIENDS, ServerQueue{
                        .m_player = ctx.m_player
                        });
                } break;
                default:
                    break;
                }
            } break; 
            case "moderator_stuff"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;
                if (ctx.m_player->GetRole() < PLAYER_ROLE_MODERATOR)
                    return;
                std::string buttonClicked = "";
                if (!ctx.m_parser.TryGet("buttonClicked", buttonClicked))
                    return;
                switch (utils::quick_hash(buttonClicked)) {
                case "psn_settings"_qh: {
                    RoleInfo role_info = g_roles_info[ctx.m_player->GetRole()];
                    DialogBuilder db{};
                    db.set_default_color('o')
                        ->add_label_with_icon("`##Moderation Panel -> Personal Settings``", ITEM_SONGPYEON, DialogBuilder::LEFT, DialogBuilder::BIG)
                        ->add_spacer()
                        ->add_label_with_icon(fmt::format("``{}`` ({}``)``", ctx.m_player->GetDisplayName(world), ctx.m_player->GetRawName()), ITEM_XENONITE_CRYSTAL)
                        ->add_smalltext(fmt::format("`9Rank: `w{}``", role_info.m_name))
                        ->add_spacer()
                        ->add_label_with_icon("`wSettings Available for your Role.``", ITEM_GROWMOJI_FIREWORKS)
                        ->add_spacer()
                        ->add_label("`9Basic Stuff``")
                        ->add_checkbox("showalerts", "`oShow Moderation Alert``", true)
                        ->add_checkbox("showreports", "`oShow `3Live Reports``", false)
                        ->add_label("`9Spawn Settings``")
                        ->add_smalltext("`3Spawn Effect Code as number by (0-255)``")
                        ->add_text_input("spawn_code", "Spawn Effect: ", "0", 3)
                        ->add_checkbox("spawn_invisible", "`oAlways `2Invisble`` when joining worlds", false)
                        ->add_quick_exit()
                        ->end_dialog("personal_settings", "`wCancel``", "`wApply``");
                    ctx.m_player->v_sender.OnDialogRequest(db.get());
                } break;
                default:
                    break;
                }
            } break;
            case "door_edit"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  
                std::string label = ctx.m_parser.Get("door_name", 1),
                    destination = ctx.m_parser.Get("door_target", 1),
                    id = ctx.m_parser.Get("door_id", 1);
                CL_Vec2i position;
                if (!(ctx.m_parser.TryGet("tilex", position.m_x) && ctx.m_parser.TryGet("tiley", position.m_y)))
                    return;
                Tile* tile = world->GetTile(position);
                if (!tile)
                    return;
                if ((world->IsOwned() && !world->IsOwner(ctx.m_player) && !ctx.m_player->HasAccess(world, position, ITEM_WRENCH) && !tile->HasAccess(ctx.m_player->GetUserId()) && (ctx.m_player->GetRole() != PLAYER_ROLE_DEVELOPER)))
                    return;
                if (id.length() < 3 && id.length() > 0) {
                    ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "`4Warning:`` That doorID is easy to guess.  People can use a doorID to warp directly in to this point from another world!", true);
                }
                destination.erase(std::remove_if(destination.begin(), destination.end(), [&](char c) {
                    if (c == ':')
                        return false;
                    return std::isalnum(c) == 0;
                }), destination.end());
                id.erase(std::remove_if(id.begin(), id.end(), [&](char c) {
                    return std::isalnum(c) == 0;
                }), id.end());
                utils::uppercase(destination);
                utils::uppercase(id);

                switch (tile->GetForeground()) {
                case ITEM_PASSWORD_DOOR: {
                    
                } break;
                default:
                    break;
                }
                tile->set_door_data(label, false, destination, id);
                world->SendTileUpdate(tile, 0);
            } break;
            case "lock_edit"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  
                CL_Vec2i position;
                bool public_lock;
                bool remove;
                if (!(ctx.m_parser.TryGet("tilex", position.m_x)) || !(ctx.m_parser.TryGet("tiley", position.m_y)))
                    return;
                Tile* tile = world->GetTile(position);
                if (!tile)
                    return;
                if (tile->GetBaseItem()->m_item_type != ITEMTYPE_LOCK || tile->GetOwnerId() != ctx.m_player->GetUserId() && (ctx.m_player->GetRole() != PLAYER_ROLE_DEVELOPER))
                    return;
                if (!ctx.m_parser.TryGet("public_lock", public_lock))
                    return;
                for (auto& user_id : tile->GetAccessList()) {
                    if (ctx.m_parser.TryGet(fmt::format("remove_{}", user_id), remove) && !remove) {
                        tile->RemoveAccess(user_id);
                        for (auto& player : world->GetPlayers(true)) {
                            if (player->GetUserId() == user_id) {
                                world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                                    ply->v_sender.OnTalkBubble(player->GetNetId(), fmt::format("{} has `4removed `wyour access from a lock on world {}.", ctx.m_player->GetDisplayName(world), world->GetName()), true);
                                ply->SendLog(fmt::format("{} `owas removed from a {}.", player->GetDisplayName(world), tile->GetBaseItem()->m_name));
                                if (!player->HasPlaymod(PLAYMOD_TYPE_INVISIBLE)) {
                                    ply->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
                                }
                                else {
                                    ply->v_sender.OnNameChanged(player->GetNetId(), fmt::format("{} (V)", player->GetDisplayName(world)));
                                }
                                    });
                            }
                        }
                        for (auto& t : world->GetTiles()) {
                            if (t.HasAccess(user_id) && tile->GetParent() != tile->GetPosition().m_x + tile->GetPosition().m_y * world->GetSize().m_x) {
                                t.RemoveAccess(user_id);
                            }
                        }
                    }
                }

                if (!tile->IsFlagOn(TILEFLAG_PUBLIC) && public_lock)
                    tile->SetFlag(TILEFLAG_PUBLIC);
                else if (tile->IsFlagOn(TILEFLAG_PUBLIC) && !public_lock)
                    tile->RemoveFlag(TILEFLAG_PUBLIC);

                uint32_t target_net_id;
                if (ctx.m_parser.TryGet("playerNetID", target_net_id)) {
                    for (auto& player : world->GetPlayers(true)) {
                        if (target_net_id != player->GetNetId())
                            continue;
                        if (player->GetUserId() == tile->GetOwnerId()) {
                            ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "`wI already have access!``", true);
                            break;
                        } else if (tile->HasAccess(player->GetUserId())) {
                            ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "`wThis player already have access!``", true);
                            break;
                        }
                        tile->AddAccess(player->GetUserId());
                        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                            if (!player->HasPlaymod(PLAYMOD_TYPE_INVISIBLE)) {
                                ply->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
                            }
                            else {
                                ply->v_sender.OnNameChanged(player->GetNetId(), fmt::format("{} (V)", player->GetDisplayName(world)));
                            }
                            });
                        for (auto& t : world->GetTiles()) {
                            if (t.GetBaseItem()->m_item_type != ITEMTYPE_LOCK && t.GetParent() == tile->GetPosition().m_x + tile->GetPosition().m_y * world->GetSize().m_x) {
                                t.AddAccess(player->GetUserId());
                            }
                        }
                        world->SyncPlayerData(player);
                        // ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), fmt::format("`wOffered {} `waccess to the lock.``", player->GetDisplayName(world)), true);
                        // player->v_sender.OnConsoleMessage(fmt::format("{}`` `owants to add you to a {}. Wrench yourself to accept.", ctx.m_player->GetDisplayName(world), tile->GetBaseItem()->m_name));
                        player->v_sender.OnConsoleMessage(fmt::format("{}`` `ohas given you access to a {}.", ctx.m_player->GetDisplayName(world), tile->GetBaseItem()->m_name));
                        player->PlaySfx("secret", 0);
                        player->set_access_offer({ position.m_x, position.m_y });
                        break;
                    }
                }

                if (tile->GetBaseItem()->IsWorldLock()) {
                    bool disable_music, invisible_music;
                    if (!(ctx.m_parser.TryGet("disable_music", disable_music) && ctx.m_parser.TryGet("invisible_music", invisible_music)))
                        return;
                    if (!tile->IsLockFlagOn(LOCKFLAG_DISABLE_MUSIC_NOTE) && disable_music) tile->SetLockFlag(LOCKFLAG_DISABLE_MUSIC_NOTE);
                    else if (tile->IsLockFlagOn(LOCKFLAG_DISABLE_MUSIC_NOTE) && !disable_music) tile->RemoveLockFlag(LOCKFLAG_DISABLE_MUSIC_NOTE);
                    
                    if (!tile->IsLockFlagOn(LOCKFLAG_INVISIBLE_MUSIC_NOTE) && invisible_music) tile->SetLockFlag(LOCKFLAG_INVISIBLE_MUSIC_NOTE);
                    else if (tile->IsLockFlagOn(LOCKFLAG_INVISIBLE_MUSIC_NOTE) && !invisible_music) tile->RemoveLockFlag(LOCKFLAG_INVISIBLE_MUSIC_NOTE);
                } else {
                    bool ignore_air;
                    if (!ctx.m_parser.TryGet("ignore_air", ignore_air))
                        return;
                    if (!tile->IsLockFlagOn(LOCKFLAG_IGNORE_EMPTY_AIR) && ignore_air)
                        tile->SetLockFlag(LOCKFLAG_IGNORE_EMPTY_AIR);
                    else if (tile->IsLockFlagOn(LOCKFLAG_IGNORE_EMPTY_AIR) && !ignore_air)
                        tile->RemoveLockFlag(LOCKFLAG_IGNORE_EMPTY_AIR);      
                }

                switch (tile->GetBaseItem()->m_id) {
                case ITEM_ROYAL_LOCK: {
                    bool royal_silence, royal_rainbows;
                    if (!(ctx.m_parser.TryGet("royal_silence", royal_silence) && ctx.m_parser.TryGet("royal_rainbows", royal_rainbows)))
                        break;
                    if (!tile->IsLockFlagOn(LOCKFLAG_SILENCE_GUEST) && royal_silence) tile->SetLockFlag(LOCKFLAG_SILENCE_GUEST);
                    else if (tile->IsLockFlagOn(LOCKFLAG_SILENCE_GUEST) && !royal_silence) tile->RemoveLockFlag(LOCKFLAG_SILENCE_GUEST);
                    
                    if (!tile->IsLockFlagOn(LOCKFLAG_RAINBOW_TRAIL) && royal_rainbows) tile->SetLockFlag(LOCKFLAG_RAINBOW_TRAIL);
                    else if (tile->IsLockFlagOn(LOCKFLAG_RAINBOW_TRAIL) && !royal_rainbows) tile->RemoveLockFlag(LOCKFLAG_RAINBOW_TRAIL);
                } break;
                default:
                    break;
                }

                std::string buttonClicked = ctx.m_parser.Get("buttonClicked", 1);
                switch (utils::quick_hash(buttonClicked)) {
                case "recalcLock"_qh: {
                    GameUpdatePacket visual_packet {
                        .m_int_x = static_cast<uint32_t>(tile->GetPosition().m_x),
                        .m_int_y = static_cast<uint32_t>(tile->GetPosition().m_y)
                    };
                    visual_packet.m_item_id = tile->GetForeground();
                    Algorithm::OnLockReApply(ctx.m_player, world, &visual_packet);
                } break;
                default:
                    break;
                }
                world->SendTileUpdate(tile, 0);
            } break;
            case "sign_edit"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  
                CL_Vec2i position;
                std::string sign_text;
                if (!(ctx.m_parser.TryGet("tilex", position.m_x)) || !(ctx.m_parser.TryGet("tiley", position.m_y)))
                    return;
                Tile* tile = world->GetTile(position);
                if (!tile)
                    return;
                if (!(tile->GetBaseItem()->m_item_type != ITEMTYPE_SIGN || tile->GetBaseItem()->m_item_type != ITEMTYPE_MANNEQUIN))
                    return;
                if (world->IsOwned() && !world->IsOwner(ctx.m_player) || world->IsTileOwned(tile) && !world->IsTileOwner(tile, ctx.m_player) || !ctx.m_player->HasAccess(world, tile->GetPosition(), ITEM_WRENCH) && (ctx.m_player->GetRole() != PLAYER_ROLE_DEVELOPER))
                    return;
                if (!ctx.m_parser.TryGet("sign_text", sign_text))
                    return;
                if (sign_text.length() > 128)
                    return;
                auto* base = tile->GetBaseItem();
                if (base->m_id == ITEM_PATH_MARKER || base->m_id == ITEM_OBJECTIVE_MARKER || base->m_id == ITEM_CARNIVAL_LANDING)
                    sign_text.erase(std::remove_if(sign_text.begin(), sign_text.end(), (int(*)(int))std::isalnum), sign_text.end());
                tile->m_label = sign_text;
                world->SendTileUpdate(tile);
            } break;
            case "mannequin_edit"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  
                CL_Vec2i position;
                if (!(ctx.m_parser.TryGet("tilex", position.m_x)) || !(ctx.m_parser.TryGet("tiley", position.m_y)))
                    return;
                Tile* tile = world->GetTile(position);
                if (!tile)
                    return;
                if (tile->GetBaseItem()->m_item_type != ITEMTYPE_MANNEQUIN)
                    return;
                if ((world->IsOwned() && !world->IsOwner(ctx.m_player) && !ctx.m_player->HasAccess(world, position, ITEM_WRENCH) && !tile->HasAccess(ctx.m_player->GetUserId()) && (ctx.m_player->GetRole() != PLAYER_ROLE_DEVELOPER)))
                    return;
                uint32_t item_id;
                if (ctx.m_parser.TryGet("itemId", item_id)) {
                    auto* item = ItemDatabase::GetItem(item_id);
                    if (!item)
                        return;
                    if (item->m_item_type != ITEMTYPE_CLOTHES || (item->m_item_category & ITEMFLAG2_UNTRADABLE) || tile->GetCloth(item->m_clothing_type) == item->m_id || tile->GetBackground() == ITEM_DARK_CAVE_BACKGROUND)
                        return;
                    if (tile->GetCloth(item->m_clothing_type) != ITEM_BLANK)
                        return; //PlayerInventory::ValidateMannequin
                    if (!ctx.m_player->RemoveItemSafe(item->m_id, 1, true))
                        return;
                    tile->SetCloth(item->m_clothing_type, static_cast<uint16_t>(item->m_id));
                    if (item->m_clothing_type == CLOTHTYPE_HAIR)
                        tile->GetPrimaryColor() = Color{ ctx.m_player->GetHairColor().GetInt() };
                    world->SendTileUpdate(tile);
                }
            } break;
            case "team_edit"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  
                CL_Vec2i position;
                std::string buttonClicked;
                if (!(ctx.m_parser.TryGet("tilex", position.m_x)) || !(ctx.m_parser.TryGet("tiley", position.m_y)))
                    return;
                Tile* tile = world->GetTile(position);
                if (!tile)
                    return;
                if (tile->GetBaseItem()->m_item_type != ITEMTYPE_GAME_RESOURCES)
                    return;
                if (world->IsOwned() && !world->IsOwner(ctx.m_player) || world->IsTileOwned(tile) && !world->IsTileOwner(tile, ctx.m_player) || !ctx.m_player->HasAccess(world, tile->GetPosition(), ITEM_WRENCH))
                    return;
                if (!ctx.m_parser.TryGet("buttonClicked", buttonClicked))
                    return;
                if (buttonClicked.find("team") == std::string::npos)
                    return;
                int teamId = std::atoi(buttonClicked.substr(4).c_str());
                if (!(teamId == 0 || teamId == 1 || teamId == 2 || teamId == 3 || teamId == 4))
                    return;
                tile->m_item_id = teamId;
                world->SendTileUpdate(tile, 0);
            } break;
            case "trade"_qh: {
            } break;
            case "spotlight"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  
                CL_Vec2i position;
                if (!(ctx.m_parser.TryGet("tilex", position.m_x)) || !(ctx.m_parser.TryGet("tiley", position.m_y)))
                    return;
                Tile* tile = world->GetTile(position);
                if (!tile)
                    return;
                if (tile->GetBaseItem()->m_item_type != ITEMTYPE_SPOTLIGHT)
                    return;
                if ((world->IsOwned() && !world->IsOwner(ctx.m_player) && !ctx.m_player->HasAccess(world, position, ITEM_WRENCH) && !tile->HasAccess(ctx.m_player->GetUserId()) && (ctx.m_player->GetRole() != PLAYER_ROLE_DEVELOPER)))
                    return;

                uint32_t target_net_id;
                if (ctx.m_parser.TryGet("playerNetID", target_net_id)) {
                    for (auto& player : world->GetPlayers(true)) {
                        if (target_net_id != player->GetNetId())
                            continue;
                        if (tile->GetOwnerId() != 0) {
                            for (auto& current_star : world->GetPlayers(true)) {
                                if (current_star->GetNetId() != tile->GetOwnerId())
                                    continue;
                                if (current_star->HasPlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT))
                                    current_star->RemovePlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT);
                                tile->m_owner_id = 0;
                                world->SyncPlayerData(current_star);
                                break;
                            }
                        }
                        ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), fmt::format("You shine the light on {}!", target_net_id == ctx.m_player->GetNetId() ? "yourself" : player->GetDisplayName(world)), true);
                        player->AddPlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT, ITEM_SPOTLIGHT, steady_clock::now(), std::chrono::seconds(-1));
                        tile->m_owner_id = player->GetNetId();
                        world->SyncPlayerData(player);
                        break;
                    }
                }
            
                std::string buttonClicked = ctx.m_parser.Get("buttonClicked", 1);
                switch (utils::quick_hash(buttonClicked)) {
                case "off"_qh: {
                    for (auto& current_star : world->GetPlayers(true)) {
                        if (current_star->GetNetId() != tile->GetOwnerId())
                            continue;
                        if (current_star->HasPlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT))
                            current_star->RemovePlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT);
                        tile->m_owner_id = 0;
                        world->SyncPlayerData(current_star);
                        break;
                    }
                } break;
                }
            } break;
            case "weatherspcl"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  
                CL_Vec2i position;
                if (!(ctx.m_parser.TryGet("tilex", position.m_x)) || !(ctx.m_parser.TryGet("tiley", position.m_y)))
                    return;
                Tile* tile = world->GetTile(position);
                if (!tile)
                    return;
                if (tile->GetBaseItem()->m_item_type != ITEMTYPE_WEATHER_SPECIAL)
                    return;
                if ((world->IsOwned() && !world->IsOwner(ctx.m_player) && !ctx.m_player->HasAccess(world, position, ITEM_WRENCH) && !tile->HasAccess(ctx.m_player->GetUserId()) && (ctx.m_player->GetRole() != PLAYER_ROLE_DEVELOPER)))
                    return;
                switch (tile->GetBaseItem()->m_id) {
                case ITEM_WEATHER_MACHINE_HEATWAVE: {
                    uint8_t red, green, blue;
                    if (!(ctx.m_parser.TryGet("red", red) && ctx.m_parser.TryGet("green", green) && ctx.m_parser.TryGet("blue", blue)))
                        return;
                    if ((red > 0xFF || red < 0) || (green > 0xFF || green < 0) || (blue > 0xFF || blue < 0))
                        return;
                    tile->m_primary_color = Color{ red, green, blue, 0xFF };
                } break;
                case ITEM_WEATHER_MACHINE_BACKGROUND: {
                    int item_id;
                    if (!ctx.m_parser.TryGet("choose", item_id))
                        return;
                    auto* item = ItemDatabase::GetItem(item_id);
                    if (!item)
                        return;
                    if (!ctx.m_player->m_inventory.Contain(item->m_id))
                        return;
                    if (!item->IsBackground()) {
                        ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "That's not a background!", true);
                        return;
                    }
                    tile->m_item_id = item->m_id;
                } break;
                case ITEM_WEATHER_MACHINE_DIGITAL_RAIN: {
                    int activeBGM;
                    if (!ctx.m_parser.TryGet("activeBGM", activeBGM))
                        return;
                    tile->m_item_id = activeBGM == 1 ? 1 : 0;
                } break;
                }
                world->SendTileUpdate(tile);
                if (tile->GetBaseItem()->m_weather_id == world->GetWeatherId())
                    world->Broadcast([&](const std::shared_ptr<Player>& player) { player->v_sender.OnSetCurrentWeather(world->GetWeatherId()); });
            } break;
            case "portrait"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;
                CL_Vec2i position;
                if (!(ctx.m_parser.TryGet("tilex", position.m_x)) || !(ctx.m_parser.TryGet("tiley", position.m_y)))
                    return;
                Tile* tile = world->GetTile(position);
                if (!tile)
                    return;
                if (tile->GetBaseItem()->m_item_type != ITEMTYPE_PORTRAIT)
                    return;
                if ((world->IsOwned() && !world->IsOwner(ctx.m_player) && !ctx.m_player->HasAccess(world, position, ITEM_WRENCH) && !tile->HasAccess(ctx.m_player->GetUserId()) && (ctx.m_player->GetRole() != PLAYER_ROLE_DEVELOPER)))
                    return;
                if (tile->GetExpressionId() != 0) {
                    if (ctx.m_parser.Get("buttonClicked") == "erase") {
                        if (!ctx.m_player->m_inventory.Erase(ITEM_PAINT_BUCKET_VARNISH, 4, true)) {
                            ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), fmt::format("`wYou'll need 4 {} to erase this {}.``", ItemDatabase::GetItem(ITEM_PAINT_BUCKET_VARNISH)->m_name, tile->GetBaseItem()->m_name), true);
                            return;
                        }
                        tile->m_label = "";
                        tile->m_expression_id = 0;
                        for (auto& cloth : tile->GetClothes())
                            cloth = ITEM_BLANK;
                        tile->GetPrimaryColor() = Color{ 0xFF, 0xFF, 0xFF, 0xFF };
                        tile->GetSecondaryColor() = Color{ 0xB4, 0x8A, 0x78, 0xFF };
                        world->SendTileUpdate(tile);
                        return;
                    }
                    std::string label{};
                    if (!ctx.m_parser.TryGet("artname", label))
                        break;
                    tile->m_label = label;
                    
                    auto data = ctx.m_parser.get_data();
                    for (auto index = 0; index < data.size(); index++) {
                        if (data[index].find("chk") == std::string::npos)
                            continue;
                        if (utils::split(data[index], "|")[1] != "1")
                            continue;
                        tile->m_expression_id = std::atoi(data[index].substr(3).c_str());
                        break;
                    }
                    world->SendTileUpdate(tile);
                }
                else {
                    if (!ctx.m_player->m_inventory.ContainAllBuckets())
                        break;
                    int32_t target_net_id;
                    if (!ctx.m_parser.TryGet("playerNetID", target_net_id))
                        break;
                    for (auto& player : world->GetPlayers(true)) {
                        if (target_net_id != player->GetNetId())
                            continue;
                        if (!ctx.m_player->m_inventory.EraseAllBuckets(2))
                            return;
                        tile->m_label = player->GetDisplayName(world);
                        tile->m_expression_id = 1;
                        tile->SetCloth(CLOTHTYPE_FACE, player->GetCloth(CLOTHTYPE_FACE));
                        tile->SetCloth(CLOTHTYPE_HAIR, player->GetCloth(CLOTHTYPE_HAIR));
                        tile->SetCloth(CLOTHTYPE_MASK, player->GetCloth(CLOTHTYPE_MASK));
                        tile->m_primary_color = player->GetHairColor();
                        tile->m_secondary_color = player->GetSkinColor();
                        world->SendTileUpdate(tile, 0);
                        break;
                    }
                }
            } break;
            case "weatherspcl2"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;  
                CL_Vec2i position;
                if (!(ctx.m_parser.TryGet("tilex", position.m_x)) || !(ctx.m_parser.TryGet("tiley", position.m_y)))
                    return;
                Tile* tile = world->GetTile(position);
                if (!tile)
                    return;
                if (tile->GetBaseItem()->m_item_type != ITEMTYPE_WEATHER_SPECIAL2)
                    return;
                if ((world->IsOwned() && !world->IsOwner(ctx.m_player) && !ctx.m_player->HasAccess(world, position, ITEM_WRENCH) && !tile->HasAccess(ctx.m_player->GetUserId()) && (ctx.m_player->GetRole() != PLAYER_ROLE_DEVELOPER)))
                    return;
                switch (tile->GetBaseItem()->m_id) {
                case ITEM_WEATHER_MACHINE_STUFF: {
                    int gravity;
                    bool spin, invert;
                    if (!(ctx.m_parser.TryGet("gravity", gravity) && ctx.m_parser.TryGet("spin", spin) && ctx.m_parser.TryGet("invert", invert)))
                        return;
                    if (gravity > 100 || gravity < -100)
                        return;
                    uint8_t spin_val = static_cast<uint8_t>(spin),
                            invert_val = static_cast<uint8_t>(invert);
                    tile->m_gravity = gravity;
                    tile->m_weather_flags = spin_val | invert_val << 1;

                    int item_id;
                    if (ctx.m_parser.TryGet("choose", item_id)) {
                        if (!ctx.m_player->m_inventory.Contain(item_id))
                            return;
                        tile->m_item_id = item_id;
                    }
                } break;
                }
                world->SendTileUpdate(tile);
                if (tile->GetBaseItem()->m_weather_id == world->GetWeatherId())
                    world->Broadcast([&](const std::shared_ptr<Player>& player) { player->v_sender.OnSetCurrentWeather(world->GetWeatherId()); });
            } break;
            case "store_request"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;
                std::string buttonClicked;
                uint8_t category;

                if (!ctx.m_parser.TryGet("buttonClicked", buttonClicked))
                    return;
            } break;
            case "popup"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;
                std::string buttonClicked;
                int32_t net_id;
                if (!(ctx.m_parser.TryGet("buttonClicked", buttonClicked) && ctx.m_parser.TryGet("netID", net_id)))
                    return;

                if (ctx.m_player->GetNetId() == net_id) {
                    auto button_hash = utils::quick_hash(buttonClicked);
                    switch (button_hash) {
                    case "discord"_qh: {
                        if (ctx.m_player->GetDiscord() != 0) {
                            auto* cluster = (dpp::cluster*)DiscordBot::GetBot(DiscordBot::BOT_TYPE_VANGUARD);
                            auto* user = dpp::find_user(dpp::snowflake{ ctx.m_player->GetDiscord() });
                            if (!user) break;
                            DialogBuilder db{};
                            db.set_default_color('o')
                                ->add_label_with_icon("`wDiscord Account``", ITEM_MAILBOX, DialogBuilder::LEFT, DialogBuilder::BIG)
                                ->add_spacer()
                                ->add_label("Linked Discord Account:")
                                ->add_textbox(fmt::format(" - ClientID: {}", static_cast<uint64_t>(user->id)))
                                ->add_textbox(fmt::format(" - Username: {}", user->format_username()))
                                ->add_spacer()
                                ->add_textbox("Wait for more updates....")
                                ->add_quick_exit()
                                ->end_dialog("popup", "Okay", "");
                            ctx.m_player->v_sender.OnDialogRequest(db.get());
                            break;
                        }
                        ctx.m_player->SendDialog(Player::DIALOG_TYPE_ACCOUNT_VERIFY, TextScanner{});
                    } break;
                    default:
                        break;
                    }
                }
            } break;
            case "renderworld"_qh: {
                auto world{ ctx.m_server->GetWorldPool()->GetWorld(ctx.m_player->GetWorld()) };
                if (!world)
                    return;
                ctx.m_servers->AddQueue(QUEUE_TYPE_RENDER_WORLD, ServerQueue {
                    .m_keyword = world->GetName(),
                    .m_player = ctx.m_player,
                    .m_world = world
                });
            } break;
            default: {
                ctx.m_player->SendLog("unhandled events::dialog_return: `wdialog_name`` -> `w{}``", dialog_name);
                break;
            }
        }
    }
}