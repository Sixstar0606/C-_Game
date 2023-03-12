#pragma once
#include <player/player.h>
#include <world/world.h>
#include <world/tile_extra.h>
#include <database/database.h>
#include <database/item/item_database.h>
#include <database/item/item_type.h>
#include <proton/utils/dialog_builder.h>

namespace GTServer::events::tile_change_req {
    void OnWrench(EventContext& ctx, std::shared_ptr<World> world, CL_Vec2i position) {
        std::shared_ptr<Player> player{ ctx.m_player };
        GameUpdatePacket* packet{ ctx.m_update_packet };
        Tile* tile{ world->GetTile(position.m_x, position.m_y) };
        if (!tile)
            return;
        ItemInfo* base{ tile->GetBaseItem() };
        if (!base)
            return;
        PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);

        switch (base->m_item_type) {
        case ITEMTYPE_DOOR:
        case ITEMTYPE_PORTAL: {
            DialogBuilder db{};
            db.set_default_color('o')
                ->add_label_with_icon(fmt::format("`wEdit {}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                ->add_spacer()
                ->embed_data<int32_t>("tilex", position.m_x)
                ->embed_data<int32_t>("tiley", position.m_y)
                ->add_text_input("door_name", "Label", tile->GetLabel(), 100)
                ->add_text_input("door_target", "Destination", tile->GetDestination(), 24)
                ->add_smalltext("Enter a Destination in this format: `2WORLDNAME:ID``")
                ->add_smalltext("Leave `2WORLDNAME`` blank (:ID) to go to the door with `2ID`` in the `2Current World``.")
                ->add_text_input("door_id", "ID", tile->GetDoorUniqueId(), 11)
                ->add_smalltext("Set a unique `2ID`` to target this door as a Destination from another!")
                ->end_dialog("door_edit", "Cancel", "OK");
            ctx.m_player->v_sender.OnDialogRequest(db.get());
        } break;
        case ITEMTYPE_LOCK: {
            if (world->IsOwner(player) || player->HasAccess(world, position, ITEM_FIST) || tile->HasAccess(player->GetUserId()) || player->GetRole() == PLAYER_ROLE_DEVELOPER) {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon(fmt::format("`wEdit {}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->add_label("`wAccess list:``")
                    ->embed_data<int32_t>("tilex", position.m_x)
                    ->embed_data<int32_t>("tiley", position.m_y)
                    ->add_spacer();
                if (tile->GetAccessList().empty())
                    db.add_label("Currently, you're the only one with access.");
                else {
                    for (auto& user_id : tile->GetAccessList())
                        db.add_checkbox(fmt::format("remove_{}", user_id), database->GetRowVarchar(user_id, 5), true);
                }
                db.add_spacer()
                    ->add_player_picker("playerNetID", "`wAdd``");
                switch (base->m_id) {
                case ITEM_GUILD_LOCK:
                    break;
                default:
                    break;
                }
                db.add_checkbox("public_lock", "`oAllow anyone to Build or Break``", tile->IsFlagOn(TILEFLAG_PUBLIC));
                if (base->IsWorldLock()) {
                     db.add_checkbox("disable_music", "`oDisable Custom Music Blocks``", tile->IsLockFlagOn(LOCKFLAG_DISABLE_MUSIC_NOTE))
                        ->add_checkbox("invisible_music", "`oMake Custom Music Blocks invisible``", tile->IsLockFlagOn(LOCKFLAG_INVISIBLE_MUSIC_NOTE));
                } else {
                    db.add_checkbox("ignore_air", "Ignore empty air", tile->IsLockFlagOn(LOCKFLAG_IGNORE_EMPTY_AIR))
                        ->add_button("recalcLock", "`wRe-apply lock``");
                }
                switch (base->m_id) {
                case ITEM_ROYAL_LOCK: {
                    db.add_textbox("`oYe Royal Options``")
                        ->add_checkbox("royal_silence", "`oSilenced, Peasants!``", tile->IsLockFlagOn(LOCKFLAG_SILENCE_GUEST))
                        ->add_checkbox("royal_rainbows", "`oRainbows For The King!``", tile->IsLockFlagOn(LOCKFLAG_RAINBOW_TRAIL));
                } break;
                case ITEM_BUILDERS_LOCK: {
                    db.add_textbox("This lock allows Building or Breaking.")
                        ->add_textbox("(ONLY if \"Allow anyone to Build or Break\" is checked above)!")
                        ->add_spacer()
                        ->add_textbox("Leaving this box unchecked only allows Breaking.")
                        ->add_checkbox("checkbox_buildonly", "`oOnly Allow Building!``", tile->IsLockFlagOn(LOCKFLAG_ONLY_BUILDING))
                        ->add_spacer()
                        ->add_textbox("People with lock access can both build and break unless you check below. The lock owner can always build and break.")
                        ->add_checkbox("checkbox_admins_limited", "`oAdmins Are Limited``", tile->IsLockFlagOn(LOCKFLAG_RESTRICT_ADMIN));
                } break;
                case ITEM_GUILD_LOCK: {
                    db.add_button("get_key", "`wGet Guild Key``")
                        ->add_button("abondonguildconfirm", "`wAbandon Guild``");
                } break;
                default:
                    break;
                }
                if (base->IsWorldLock()) {
                    if (!player->m_inventory.Contain(ITEM_WORLD_KEY))
                        db.add_button("get_key", "Get World Key");
                }
                db.end_dialog("lock_edit", "Cancel", "OK");
                player->v_sender.OnDialogRequest(db.get());
                break;
            }
        } break;
        case ITEMTYPE_SIGN: {
            DialogBuilder db{};
            db.set_default_color('o')
                ->add_label_with_icon(fmt::format("`wEdit {}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                ->add_spacer()
                ->embed_data<int32_t>("tilex", position.m_x)
                ->embed_data<int32_t>("tiley", position.m_y)
                ->add_textbox("What would you like to write on this sign?")
                ->add_text_input("sign_text", "", tile->GetLabel(), 128)
                ->end_dialog("sign_edit", "Cancel", "OK");
            player->v_sender.OnDialogRequest(db.get());
        } break;
        case ITEMTYPE_MANNEQUIN: {
            DialogBuilder db{};
            db.set_default_color('o')
                ->add_label_with_icon(fmt::format("`wEdit {}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                ->add_spacer()
                ->embed_data<int32_t>("tilex", position.m_x)
                ->embed_data<int32_t>("tiley", position.m_y)
                ->add_spacer()
                ->add_textbox("To dress, select a clothing item then use on the mannequin. To remove clothes, punch it to remove.")
                ->add_spacer()
                ->add_textbox(fmt::format("What would you like to write on this {}?``", base->m_name))
                ->add_text_input("sign_text", "", tile->GetLabel(), 128)
                ->end_dialog("sign_edit", "Cancel", "OK");
            player->v_sender.OnDialogRequest(db.get());
        } break;
        case ITEMTYPE_GAME_RESOURCES: {
            DialogBuilder db{};
            db.set_default_color('o')
                ->add_label_with_icon(fmt::format("`w{}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                ->add_spacer()
                ->embed_data<int32_t>("tilex", position.m_x)
                ->embed_data<int32_t>("tiley", position.m_y)
                ->add_textbox("Choose a team:")
                ->text_scaling_string("Penguins")
                ->add_button_with_icon("team0", "`$Rabbits``", -10)
                ->add_button_with_icon("team1", "`$Bombers``", -11)
                ->add_button_with_icon("team2", "`$Yaks``", -12)
                ->add_button_with_icon("team3", "`$Penguins``", -13)
                ->add_button_with_icon("team4", "`$None``", -14)
                ->add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)
                ->end_dialog("team_edit", "Cancel", "");
            player->v_sender.OnDialogRequest(db.get());
        } break;
        case ITEMTYPE_SPOTLIGHT: {
            DialogBuilder db{};
            db.set_default_color('o')
                ->add_label_with_icon("`wShine the Spotlight!``", base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                ->add_spacer()
                ->embed_data<int32_t>("tilex", position.m_x)
                ->embed_data<int32_t>("tiley", position.m_y);
            if (tile->GetOwnerId() != 0) {
                bool found_light = false;
                for (auto& current_star : world->GetPlayers(true)) {
                    if (!(current_star->HasPlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT) && current_star->GetNetId() == tile->GetOwnerId()))
                        continue;
                    found_light = true;
                    db.add_textbox(fmt::format("The light is shining on `w{}``.", current_star->GetDisplayName(world)))
                        ->add_spacer()
                        ->add_button("off", "Turn it off");
                    break;
                }
                if (!found_light) {
                    tile->m_owner_id = 0;
                    db.add_textbox("The light is currently off.")
                        ->add_spacer();
                }
            }
            else {
                db.add_textbox("The light is currently off.")
                    ->add_spacer();
            }
            db.add_player_picker("playerNetID", "`wChoose a superstar``")
                ->end_dialog("spotlight", "Nevermind", "");
            player->v_sender.OnDialogRequest(db.get());
        } break;
        case ITEMTYPE_WEATHER_SPECIAL: {
            switch (base->m_id) {
            case ITEM_WEATHER_MACHINE_HEATWAVE: {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon(fmt::format("`w{}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->add_spacer()
                    ->embed_data<int32_t>("tilex", position.m_x)
                    ->embed_data<int32_t>("tiley", position.m_y)
                    ->add_textbox("`oAdjust the color of your heatwave here, by including 0-255 of Red, Green, and Blue.")
                    ->add_spacer()
                    ->add_text_input("red", "Red", fmt::format("{}", tile->GetPrimaryColor().GetRed()), 3)
                    ->add_text_input("green", "Green", fmt::format("{}", tile->GetPrimaryColor().GetGreen()), 3)
                    ->add_text_input("blue", "Blue", fmt::format("{}", tile->GetPrimaryColor().GetBlue()), 3)
                    ->end_dialog("weatherspcl", "Cancel", "Okay");
                player->v_sender.OnDialogRequest(db.get());
            } break;
            case ITEM_WEATHER_MACHINE_BACKGROUND: {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon(fmt::format("`w{}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->add_spacer()
                    ->embed_data<int32_t>("tilex", position.m_x)
                    ->embed_data<int32_t>("tiley", position.m_y)
                    ->add_textbox("You can scan any Background Block to set it up in your weather machine.")
                    ->add_item_picker("choose", fmt::format("Item: `2{}``", ItemDatabase::GetItem(tile->GetItemId())->m_name), "Select any Background Block")
                    ->end_dialog("weatherspcl", "Cancel", "Okay");
                player->v_sender.OnDialogRequest(db.get());
            } break;
            case ITEM_WEATHER_MACHINE_DIGITAL_RAIN: {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon(fmt::format("`w{}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->add_spacer()
                    ->embed_data<int32_t>("tilex", position.m_x)
                    ->embed_data<int32_t>("tiley", position.m_y)
                    ->add_checkbox("activeBGM", "Activate Background Music", tile->GetItemId() == 1 ? true : false)
                    ->end_dialog("weatherspcl", "Cancel", "Okay");
                player->v_sender.OnDialogRequest(db.get());
            } break;
            }
        } break;
        case ITEMTYPE_PORTRAIT: {
            DialogBuilder db{};
            db.set_default_color('o')
                ->add_label_with_icon(fmt::format("`wEdit {}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                ->add_spacer()
                ->embed_data<int32_t>("tilex", position.m_x)
                ->embed_data<int32_t>("tiley", position.m_y);
            if (tile->GetExpressionId() != 0) {
                db.add_textbox("This is a lovely portrait of a Growtopian.")
                    ->add_button("erase", "Erase Painting")
                    ->add_smalltext("`5(Erasing costs 4 Paint Bucket - Varnish)``")
                    ->add_text_input("artname", "Title:", tile->GetLabel(), 60)
                    ->add_smalltext("If you'd like to touch up the painting slightly, you could change the expression:");
                db.add_checkbox("chk1", "Unconcerned", (tile->GetExpressionId() == 1));
                db.add_checkbox("chk2", "Happy", (tile->GetExpressionId() == 2));
                db.add_checkbox("chk3", "Sad", (tile->GetExpressionId() == 3));
                db.add_checkbox("chk4", "Tongue Out", (tile->GetExpressionId() == 4));
                db.add_checkbox("chk5", "Surprised", (tile->GetExpressionId() == 5));
                db.add_checkbox("chk6", "Angry", (tile->GetExpressionId() == 6));
                db.add_checkbox("chk7", "Talking", (tile->GetExpressionId() == 7));
                db.add_checkbox("chk8", "Dissatisfied", (tile->GetExpressionId() == 8));
                db.add_checkbox("chk9", "Ecstatic", (tile->GetExpressionId() == 9));
                db.add_checkbox("chk11", "Wry", (tile->GetExpressionId() == 11));
                db.add_checkbox("chk12", "Sleeping", (tile->GetExpressionId() == 12));
                db.add_checkbox("chk13", "Whistling", (tile->GetExpressionId() == 13));
                db.add_checkbox("chk14", "Winking", (tile->GetExpressionId() == 14));
                db.add_checkbox("chk16", "Trolling", (tile->GetExpressionId() == 16));
                db.add_checkbox("chk17", "Vampire Fangs", (tile->GetExpressionId() == 17));
                db.add_checkbox("chk18", "Vampire", (tile->GetExpressionId() == 18));
                db.add_checkbox("chk22", "Underwater", (tile->GetExpressionId() == 22));
            } else {
                db.add_textbox("The canvas is blank.");
                if (!ctx.m_player->m_inventory.ContainAllBuckets()) {
                    db.add_textbox("You'll need 2 of each color of Paint Bucket to paint someone.");
                }
                else {
                    db.add_player_picker("playerNetID", "`wPaint Someone``")
                        ->add_smalltext("`5(Painting costs 2 Paint Bucket of each color)``");
                }
            }
            db.end_dialog("portrait", "Cancel", "Update");
            ctx.m_player->v_sender.OnDialogRequest(db.get());
        } break;
        case ITEMTYPE_WEATHER_SPECIAL2: {
            switch (base->m_id) {
            case ITEM_WEATHER_MACHINE_STUFF: {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon(fmt::format("`w{}``", base->m_name), base->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->add_spacer()
                    ->embed_data<int32_t>("tilex", position.m_x)
                    ->embed_data<int32_t>("tiley", position.m_y)
                    ->add_item_picker("choose", fmt::format("Item: `2{}``", ItemDatabase::GetItem(tile->GetItemId())->m_name), "Select any item to rain down")
                    ->add_text_input("gravity", "Gravity:", fmt::format("{}", tile->GetGravity()), 5)
                    ->add_checkbox("spin", "Spin Items", tile->GetWeatherFlags() & 1)
                    ->add_checkbox("invert", "Invert Sky Colors", tile->GetWeatherFlags() & ~1)
                    ->end_dialog("weatherspcl2", "Cancel", "Okay");
                player->v_sender.OnDialogRequest(db.get());
            } break;
            }
        } break;
        default:
            break;
        }
    }
}