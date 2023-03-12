#pragma once
#include <fmt/core.h>
#include <proton/utils/dialog_builder.h>

namespace GTServer::events {
    void wrench(EventContext& ctx) { 
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_IS_IN)) {
            ctx.m_player->Disconnect(0U);
            return;
        }
        if (ctx.m_player->GetWorld().empty() || ctx.m_player->GetWorld() == std::string{ "EXIT" })
            return;

        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        auto splited_text = utils::split(ctx.m_parser.get_data()[1], "|netid|");
        if (splited_text.empty() || splited_text.size() < 2)
            return;
        int net_id{ std::atoi(splited_text[1].c_str()) };

        if (ctx.m_player->GetNetId() == net_id) {
            std::string note{ "B" };
            switch ((ctx.m_player->GetPosition().m_y / 32) % 14){
                case 0: { note = "B"; break; }
                case 1: { note = "A"; break; }
                case 2: { note = "G"; break; }
                case 3: { note = "F"; break; }
                case 4: { note = "E"; break; }
                case 5: { note = "D"; break; }
                case 6: { note = "C"; break; }
                case 7: { note = "B"; break; }
                case 8: { note = "A"; break; }
                case 9: { note = "G"; break; }
                case 10: { note = "F"; break; }
                case 11: { note = "E"; break; }
                case 12: { note = "D"; break; }
                case 13: { note = "C"; break; }      
                default: {note = "???"; break; }         
            }
            TextScanner parser{};
            parser.add("standing_note", note);
            ctx.m_player->SendDialog(Player::DIALOG_TYPE_POPUP, parser, world);
        }
        else {
            for (auto& target : world->GetPlayers(false)) {
                if (target->GetNetId() != net_id)
                    continue;
                if (world->IsOwner(ctx.m_player)) {
                    if (ctx.m_player->GetRole() >= PLAYER_ROLE_MODERATOR) {
                        DialogBuilder db;
                        db.embed_data<int>("netID", net_id)
                            ->add_label_with_icon(fmt::format("`w{} (`2{}``)``", target->GetDisplayName(world), target->get_experience().second), ITEM_FIST, DialogBuilder::LEFT, DialogBuilder::BIG)
                            ->add_spacer();
                        db.embed_data<int>("target", target->GetUserId());
                        db.add_button("trade", "`wTrade``");
                        db.add_button("add_friend", "`wAdd as friend``");
                        db.add_button("wrench_punish", "`3Punish/View``");
                        db.add_button("wrench_kick", "`4Kick``");
                        db.add_button("wrench_pull", "`5Pull``");
                        db.add_button("wrench_ban", "`4World Ban``");
                        db.add_quick_exit();
                        db.add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)->end_dialog("wrench_menu", "`wContinue", "");
                        ctx.m_player->v_sender.OnDialogRequest(db.get(), target->GetUserId());
                    }
                    else {
                        DialogBuilder db;
                        db.embed_data<int>("netID", net_id)
                            ->add_label_with_icon(fmt::format("`w{} (`2{}``)``", target->GetDisplayName(world), target->get_experience().second), ITEM_FIST, DialogBuilder::LEFT, DialogBuilder::BIG)
                            ->add_spacer();
                        db.embed_data<int>("target", target->GetUserId());
                        db.add_button("trade", "`wTrade``");
                        db.add_button("add_friend", "`wAdd as friend``");
                        db.add_button("wrench_kick", "`4Kick``");
                        db.add_button("wrench_pull", "`5Pull``");
                        db.add_button("wrench_ban", "`4World Ban``");
                        db.add_quick_exit();
                        db.add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)->end_dialog("wrench_menu", "`wContinue", "");
                        ctx.m_player->v_sender.OnDialogRequest(db.get(), target->GetUserId());
                    }
                }
                else {
                    if (ctx.m_player->GetRole() > PLAYER_ROLE_VIP_PLUS) {
                        if (ctx.m_player->GetRole() > PLAYER_ROLE_MODERATOR) {
                            DialogBuilder db;
                            db.embed_data<int>("netID", net_id)
                                ->add_label_with_icon(fmt::format("`w{} (`2{}``)``", target->GetDisplayName(world), target->get_experience().second), ITEM_FIST, DialogBuilder::LEFT, DialogBuilder::BIG)
                                ->add_spacer();
                            db.embed_data<int>("target", target->GetUserId());
                            db.add_button("trade", "`wTrade``");
                            db.add_button("add_friend", "`wAdd as friend``");
                            db.add_button("wrench_punish", "`3Punish/View``");
                            db.add_button("wrench_kick", "`4Kick``");
                            db.add_button("wrench_pull", "`5Pull``");
                            db.add_button("wrench_ban", "`4World Ban``");
                            db.add_quick_exit();
                            db.add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)->end_dialog("wrench_menu", "`wContinue", "");
                            ctx.m_player->v_sender.OnDialogRequest(db.get(), target->GetUserId());
                        }
                        else {
                            DialogBuilder db;
                            db.embed_data<int>("netID", net_id)
                                ->add_label_with_icon(fmt::format("`w{} (`2{}``)``", target->GetDisplayName(world), target->get_experience().second), ITEM_FIST, DialogBuilder::LEFT, DialogBuilder::BIG)
                                ->add_spacer();
                            db.embed_data<int>("target", target->GetUserId());
                            db.add_button("trade", "`wTrade``");
                            db.add_button("add_friend", "`wAdd as friend``");
                            db.add_button("wrench_punish", "`3Punish/View``");
                            db.add_quick_exit();
                            db.add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)->end_dialog("wrench_menu", "`wContinue", "");
                            ctx.m_player->v_sender.OnDialogRequest(db.get(), target->GetUserId());
                        }
                    }
                    else {
                        DialogBuilder db;
                        db.embed_data<int>("netID", net_id)
                            ->add_label_with_icon(fmt::format("`w{} (`2{}``)``", target->GetDisplayName(world), target->get_experience().second), ITEM_FIST, DialogBuilder::LEFT, DialogBuilder::BIG)
                            ->add_spacer();
                        db.embed_data<int>("target", target->GetUserId());
                        db.add_button("trade", "`wTrade``");
                        db.add_button("add_friend", "`wAdd as friend``");
                        db.add_quick_exit();
                        db.add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)->end_dialog("wrench_menu", "`wContinue", "");
                        ctx.m_player->v_sender.OnDialogRequest(db.get(), target->GetUserId());
                    }
                }
            }
        }
    }
}