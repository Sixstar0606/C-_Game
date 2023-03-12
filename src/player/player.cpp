#include <player/player.h>
#include <world/world.h>
#include <world/world_pool.h>
#include <render/world_render.h>
#include <database/database.h>
#include <magic_enum.hpp>
#include <database/item/item_component.h>
#include <proton/utils/dialog_builder.h>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <iostream>
#include <fstream>

namespace GTServer {
    Player::Player(ENetPeer* peer) : 
        m_peer(peer),
        m_login_info{ std::make_shared<LoginInformation>() },
        m_inventory{ peer },
        v_sender{ peer },
        PacketSender{ peer },
        PlayerComponent{},
        CharacterState{}
    {
        if(!m_peer)
            return;
        m_peer->data = std::malloc(sizeof(uint32_t));
        std::memcpy(m_peer->data, &m_peer->connectID, sizeof(uint32_t));

        m_ip_address.reserve(16);
        enet_address_get_host_ip(&m_peer->address, m_ip_address.data(), 16);
        this->SetConnectID(m_peer->connectID);

        for (uint8_t index = 0; index < NUM_BODY_PARTS; index++)
            this->SetCloth(index, (int32_t)ITEM_BLANK);
    }
    Player::~Player() {
        m_login_info.reset();
    }

    bool Player::IsFlagOn(const ePlayerFlags& flag) const {
        if (m_flags & flag)
            return true;
        return false;
    }
    void Player::SetFlag(const ePlayerFlags& flag) {
        m_flags |= flag;
    }
    void Player::RemoveFlag(const ePlayerFlags& flag) {
        m_flags &= ~flag;
    }
    
    std::string Player::GetDisplayName(std::shared_ptr<World> world) const { 
        RoleInfo role_info = g_roles_info[this->GetRole()];
        char tag{}, color = role_info.m_color;
        if (m_role >= PLAYER_ROLE_MODERATOR && m_display_name == m_raw_name) {
            tag = char{ '@' };
        }
        else {
            color = char{ 'w' };
        }
        if (world && m_display_name != m_raw_name &&
        (this->GetWorld() != std::string{ "EXIT" } || this->GetWorld() != std::string{ "" })) {
            if (world->IsOwned()) {
                Tile* main_lock = world->GetTile(world->GetMainLock());
                if (main_lock->GetOwnerId() == this->GetUserId())
                    color = char{ '2' };
                else if (main_lock->HasAccess(this->GetUserId()))
                    color = char{ '^' };
                else
                    color = char{ 'w' };
            }
        }
        return fmt::format("`{}{}{}``", color, tag == char{} ? "" : std::string{1, tag} , m_display_name);
    }

    void Player::SetCloth(const uint8_t& body_part, const uint16_t& item_id, const bool& add_playmod) {
        ItemInfo* base = ItemDatabase::GetItem(this->GetCloth(body_part));
        ItemInfo* item = ItemDatabase::GetItem(item_id);
        if (!base || !item)
            return;
        this->GetClothes()[body_part] = item_id;

        if (item->m_id == ITEM_LEGENDBOT_009) {
            this->SetPunchID(20);
        }
        if (!add_playmod)
            return;
        if (item->m_id == ITEM_BLANK) {
            if (base->m_mods & ITEMMOD_TANK_DRIVER)
                this->RemovePlaymod(PLAYMOD_TYPE_TANK_DRIVER);
            if (base->m_mods & ITEMMOD_SPEEDY)
                this->RemovePlaymod(PLAYMOD_TYPE_SPEEDY);
            if (base->m_mods & ITEMMOD_ARMED_AND_DANGEROUS)
                this->RemovePlaymod(PLAYMOD_TYPE_ARMED_AND_DANGEROUS);
            if (base->m_mods & ITEMMOD_HIGH_JUMP)
                this->RemovePlaymod(PLAYMOD_TYPE_FISTS_O_FURY);
            if (base->m_mods & ITEMMOD_HIGH_JUMP)
                this->RemovePlaymod(PLAYMOD_TYPE_HIGH_JUMP);
            if (base->m_mods & ITEMMOD_DOUBLE_JUMP)
                this->RemovePlaymod(PLAYMOD_TYPE_DOUBLE_JUMP);
            return;
        }
        if (item->m_mods & ITEMMOD_TANK_DRIVER)
            this->AddPlaymod(PLAYMOD_TYPE_TANK_DRIVER, item->m_id, steady_clock::now(), std::chrono::seconds(-1));
        if (item->m_mods & ITEMMOD_SPEEDY)
            this->AddPlaymod(PLAYMOD_TYPE_SPEEDY, item->m_id, steady_clock::now(), std::chrono::seconds(-1));
        if (item->m_mods & ITEMMOD_ARMED_AND_DANGEROUS)
            this->AddPlaymod(PLAYMOD_TYPE_ARMED_AND_DANGEROUS, item->m_id, steady_clock::now(), std::chrono::seconds(-1));
        if (item->m_mods & ITEMMOD_FISTS_O_FURY)
            this->AddPlaymod(PLAYMOD_TYPE_FISTS_O_FURY, item->m_id, steady_clock::now(), std::chrono::seconds(-1));
        if (item->m_mods & ITEMMOD_HIGH_JUMP)
            this->AddPlaymod(PLAYMOD_TYPE_HIGH_JUMP, item->m_id, steady_clock::now(), std::chrono::seconds(-1));
        if (item->m_mods & ITEMMOD_DOUBLE_JUMP)
            this->AddPlaymod(PLAYMOD_TYPE_DOUBLE_JUMP, item->m_id, steady_clock::now(), std::chrono::seconds(-1));
    }

    std::vector<uint8_t> Player::Pack(const ePlayerData& type) const {
        switch (type) {
        case PLAYER_DATA_CLOTHES: { 
            std::vector<uint8_t> ret{};
            ret.resize(sizeof(uint16_t) * NUM_BODY_PARTS);
            
            BinaryWriter buffer{ ret.data() };
            for (uint8_t index = 0; index < NUM_BODY_PARTS; index++) 
                buffer.write<uint16_t>(this->m_clothes[index]);
            
            return ret;
        };
        case PLAYER_DATA_INVENTORY:
            return m_inventory.Pack();
        case PLAYER_DATA_PLAYMODS: {
            std::vector<uint8_t> ret{};
            ret.resize(sizeof(uint32_t) + (sizeof(Playmod) * m_playmods.size()));
            
            BinaryWriter buffer{ ret.data() };
            buffer.write<uint32_t>(static_cast<uint32_t>(m_playmods.size()));
            for (auto& mod : this->m_playmods) {
                buffer.write<uint16_t>(static_cast<uint16_t>(mod.m_type));
                buffer.write<uint16_t>(mod.m_icon_id);
                buffer.write<uint64_t>(mod.m_time.GetTime().time_since_epoch().count());
                buffer.write<uint64_t>(mod.m_time.GetTimeout().count());
            }
            
            return ret;
        }
        case PLAYER_DATA_CHARACTER_STATE:
            return CharacterState::Pack();
        default:
            return std::vector<uint8_t>{};
        }
    }
    void Player::Serialize(const ePlayerData& type, const std::vector<uint8_t>& data) {
        switch (type) {
        case PLAYER_DATA_CLOTHES: {
            BinaryReader br{ data };
            for (uint8_t index = 0; index < NUM_BODY_PARTS; index++)
                this->SetCloth(index, br.read<uint16_t>());
        } break;
        case PLAYER_DATA_INVENTORY: {
            m_inventory.Serialize(data);
        } break;
        case PLAYER_DATA_PLAYMODS: {
            BinaryReader br{ data };
            uint32_t playmods_count = br.read<uint32_t>();
            for (auto index = 0; index < playmods_count; index++) {
                Playmod val{};
                val.m_type = static_cast<ePlaymodType>(br.read<uint16_t>());
                val.m_icon_id = br.read<uint16_t>();
                val.m_time = TimingClock{ steady_clock::time_point{ std::chrono::nanoseconds(br.read<uint64_t>()) }, std::chrono::seconds{ br.read<std::chrono::seconds>() } };
                this->m_playmods.push_back(val);
            }
        } break; 
        case PLAYER_DATA_CHARACTER_STATE: {
            CharacterState::Serialize(data);
        } break;
        default:
            break;
        }
    }

    TextScanner Player::GetSpawnData(const bool& local) const {
        TextScanner text_parse{};
        text_parse.add("spawn", "avatar")
            ->add<uint32_t>("netID", GetNetId())
            ->add<uint32_t>("userID", GetUserId())
            ->add("colrect", CL_Recti{ 0, 0, 20, 30 })
            ->add("posXY", CL_Vec2i{ GetPosition().m_x, GetPosition().m_y })
            ->add("name", this->GetDisplayName(nullptr))
            ->add("country", m_login_info->m_country)
            ->add<uint8_t>("invis", IsFlagOn(PLAYERFLAG_IS_INVISIBLE))
            ->add<uint8_t>("ghost", IsFlagOn(PLAYERFLAG_IS_GHOST))
            ->add<uint8_t>("mstate", IsFlagOn(PLAYERFLAG_IS_MOD))
            ->add<uint8_t>("smstate", IsFlagOn(PLAYERFLAG_IS_SUPER_MOD))
            ->add("onlineID", "");
        if (local)
            text_parse.add("type", "local");
        return text_parse;
    }

    void Player::SendDialog(const eDialogType& type, TextScanner parser, std::shared_ptr<World> world, std::shared_ptr<Player> target) {
        switch (type) {
            case DIALOG_TYPE_REGISTRATION: {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon("`wGet a GrowID!``", 206, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->add_spacer();
                if (parser.contain("extra"))
                    db.add_textbox(parser.Get("extra"))->add_spacer();
                    db.add_textbox("By choosing a `wGrowID``, you can use a name and password to logon from any device. Your `wname `owill be shown to other players, so choose carefully!")
                    ->add_text_input("logon", "Name ", parser.Get("name"), 18)
                    ->add_spacer()
                    ->add_textbox("Your `wpassword`` must contain`` 8 to 18 characters, 1 letter, 1 number`` and`` 1 special character: @#!$^&*.,``")
                    ->add_text_input_password("password", "Password ", parser.Get("password"), 18)
                    ->add_text_input_password("verify_password", "Password Verify", parser.Get("verify_password"), 18)
                    ->add_textbox("Your `wpassword`` must contain`` 8 to 18 characters, 1 letter, 1 number`` and`` 1 special character: @#!$^&*.,``")
                    ->add_spacer()
                    ->add_textbox("`oYour `wemail address`o will only be used for account verification purposes and won't be spammed or shared. If you use a fake email, you'll never be able to recover or change your password.")
                    ->add_text_input("verify_email", "Email", parser.Get("verify_email"), 32)
                    ->end_dialog("growid_apply", "Cancel", "`wGet a GrowID!``");
                v_sender.OnDialogRequest(db.get(), 0);
            } break;
            case DIALOG_TYPE_ACCOUNT_VERIFY: {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon("`wAccount Verification``", ITEM_WARNING_BLOCK, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->add_spacer()
                    ->add_textbox("Would you like to link your `wDiscord Account`` to this `wGrowID``?")
                    ->add_textbox("For example if you lost your account credentials you can recover them with this.")
                    ->add_smalltext("If you dont want to link you can leave the below input box empty, you also can link your `wGrowID`` with `wDiscord Account`` anytime as you want.")
                    ->add_spacer()
                    ->add_textbox("Example: `wPizza#1234`` or `wName#Tag``")
                    ->add_text_input("discord_account", "Discord Account: ", "", 30)
                    ->end_dialog("account_verify", "`wPlay `2Better Growtopia``", "`wLink Discord``");
                v_sender.OnDialogRequest(db.get(), 0);
            } break;
            case DIALOG_TYPE_NEWS: {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon("`wBetter Growtopia``", ITEM_SNOWY_TREE_BLOCK, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->add_spacer()
                    ->add_textbox("Pre-Release Testing")
                    ->add_textbox("Better Growtopia - Play with friends and make some amazing stuff, don't be afraid to report bugs on the Discord server!")
                    ->add_smalltext("Welcome to Better Growtopia!")
                    ->add_spacer()
                    ->add_textbox("`2asd")
                    ->end_dialog("popup", "", "`wContinue``");
                v_sender.OnDialogRequest(db.get(), 0);
            } break;
            case DIALOG_TYPE_POPUP: {
                DialogBuilder db{};
                db.embed_data("netID", fmt::format("{}", GetNetId()))
                    ->add_player_info(this->GetDisplayName(world), this->get_experience().second, this->get_experience().first, this->get_experience().second * 1500);
                if (this->get_access_offer() != -1)
                    db.add_spacer()
                        ->add_button("acceptlock", fmt::format("`2Accept Access on {}``", world->GetTile(this->get_access_offer())->GetBaseItem()->m_name));
                if (false) {} // Player::IsHasGuild
                db.add_spacer()->set_custom_spacing(5, 10);
                if (this->get_experience().second >= 10)
                    db.add_custom_button("billboard_edit", "interface/large/gui_wrench_edit_billboard.rttex", 400, 260, 0.19);
                db.add_custom_button("title_edit", "interface/large/gui_wrench_title.rttex", 400, 260, 0.19)
                    ->add_custom_button("set_online_status", "interface/large/gui_wrench_online_status_1green.rttex", 400, 260, 0.19)
                    ->add_custom_button("notebook_edit", "interface/large/gui_wrench_notebook.rttex", 400, 260, 0.19);
                if (false) // Player::IsHasGuild
                    db.add_custom_button("guild_notebook_edit", "interface/large/gui_wrench_guild_notebook.rttex", 400, 260, 0.19);
                db.add_custom_button("goals", "interface/large/gui_wrench_goals_quests.rttex", 400, 260, 0.19)
                    ->add_custom_button("bonus_inactive", "interface/large/gui_wrench_daily_bonus_inactive.rttex", 400, 260, 0.19)
                    ->add_custom_button("my_worlds", "interface/large/gui_wrench_my_worlds.rttex", 400, 260, 0.19)
                    ->add_custom_button("alist", "interface/large/gui_wrench_achievements.rttex", 400, 260, 0.19)
                    ->add_custom_label("alist", "(0/169)", 0.72, 0.50, DialogBuilder::SMALL)
                    ->add_custom_button("emojis", "interface/large/gui_wrench_growmojis.rttex", 400, 260, 0.19)
                    ->add_custom_button("marvelous_missions", "interface/large/gui_wrench_marvelous_missions.rttex", 400, 260, 0.19);
                if (false) // Player::HasExtendedPlaymod(GUILD_BADOLOER, PLAYMOD_TYPE_RAISE_THE_FLAG)
                    db.add_custom_button("banner_bandolier_edit", "interface/large/gui_wrench_banner_bandolier.rttex", 400, 260, 0.19);
                db.add_custom_break()
                    ->set_custom_spacing(0, 0)
                    ->add_spacer();
                if (this->GetDiscord() != 0) {
                    db.add_button("discord", "View Discord Account");
                } else {
                    db.add_button("discord", "Link your account with `2Discord``");
                }
                db.add_spacer()
                    ->add_textbox("`wActive effects:``");
                for (const auto& mod : this->GetPlaymods()) {
                    PlaymodData mod_data = PlaymodManager::Get(mod.m_type);
                    db.add_label_with_icon(fmt::format("`w{}``{}``", mod_data.m_name, mod.m_time.GetTimeout() != std::chrono::seconds(-1) ? fmt::format(" (`w{}`` left)", mod.GetTabString()) : ""), mod.m_icon_id);
                }
                db.add_spacer()
                    ->add_textbox(fmt::format("`oYou have `w{}`` backpack slots``", m_inventory.GetSize()))
                    ->add_textbox(fmt::format("`oCurrent world: `w{}`` (`w{}``, `w{}``) (`w{}`` person)````", world->GetName(), GetPosition().m_x / 32, GetPosition().m_y / 32, world->GetPlayers(false).size()))
                    ->add_textbox(fmt::format("`oYou are standing on the note \"{}\".``", parser.Get("standing_note", 1)));
                switch (this->GetRole()) {
                case PLAYER_ROLE_VIP: {
                    db.add_textbox("`oYou are a `5VIP`` and have the `wRecycler``.``");
                } break;
                case PLAYER_ROLE_VIP_PLUS: {
                    db.add_textbox("`oYou are a `5VIP+`` unlocked the `wRecycler`` and `wMore skin colors``.``");
                } break;
                default: {
                    db.add_textbox("`oYou are not yet a `2Supporter`` or `5Super Supporter``.``");
                } break;
                }
                db.add_spacer()
                    ->add_button("emojis", "`$Growmojis``")
                    ->add_spacer()
                    ->add_quick_exit()
                    ->end_dialog("popup", "", "Continue");
                this->v_sender.OnDialogRequest(db.get());
            } break;
            case DIALOG_TYPE_PUNISHMENT: {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon(fmt::format("{}`o's Punishment Menu", target->GetRawName()), ITEM_BAN_WAND, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->embed_data("offline_name", target->GetRawName())
                    ->add_smalltext(fmt::format("Real Name: {}", target->GetRawName()))
                    ->add_smalltext(fmt::format("Display Name: {}", target->GetDisplayName(nullptr)))
                    ->add_smalltext(fmt::format("User ID: {}", target->GetUserId()))
                    ->add_smalltext(fmt::format("Relative Identifier: {}", target->GetLoginDetail()->m_rid))
                    ->add_spacer()
                    ->add_textbox("Alternative Accounts:")
                    ->add_smalltext(" - # NONE")
                    ->add_text_input("ban_time", "Seconds", parser.Get("ban_time"), 9)
                    ->add_text_input("ban_reason", "Reason", parser.Get("ban_reason"), 32)
                    ->add_spacer();
                if (target->IsFlagOn(PLAYERFLAG_LOGGED_ON))
                    db.add_button("warpto", fmt::format("`oWarp To User `o(in `5{}`o)", target->GetWorld()))
                        ->add_spacer();
                db.add_textbox("`oActive Effects:``")
                    ->add_label_with_icon("`wNone!``", ITEM_WARNING_BLOCK);
                db.add_button_with_icon("view_inventory", "View Inventory", ITEM_BACKPACK, DialogBuilder::LEFT)
                    ->add_button_with_icon("view_owned_worlds", "View Owned Worlds", ITEM_GLOBE, DialogBuilder::LEFT)
                    ->add_button_with_icon("ban_player", "Ban", ITEM_BAN_WAND, DialogBuilder::LEFT)
                    ->add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE);
                db.add_quick_exit()
                    ->end_dialog("punishment_menu", "Cancel", "");
                this->v_sender.OnDialogRequest(db.get());
            } break;
            case DIALOG_TYPE_SOCIAL_PORTAL: {
                DialogBuilder db{};
                db.set_default_color('o')
                    ->add_label_with_icon("`wSocial Portal``", ITEM_DUMB_FRIEND, DialogBuilder::LEFT, DialogBuilder::BIG)
                    ->add_spacer()
                    ->add_button("showfriend", "`wShow Friends``");
                if (false) { // Player::IsHasGuild
                    db.add_button("showguild", "`wShow Guild Members``");
                } else {
                    db.add_button("showguild", "`wCreate Guild``");
                }
                if (this->GetRole() > PLAYER_ROLE_MODERATOR)
                    db.add_button("modstuff", "`## Moderation Stuffs``");
                db.add_quick_exit()
                    ->end_dialog("socialportal", "", "`wBack``");
                this->v_sender.OnDialogRequest(db.get());
            } break;
            default:
                break;
        }
    }

    void Player::SendCharacterState(std::shared_ptr<Player> player) {
        GameUpdatePacket packet{};
        packet.m_type = NET_GAME_PACKET_SET_CHARACTER_STATE;
        packet.m_punch_id = GetPunchID();
        packet.m_build_range = GetBuildRange();
        packet.m_punch_range = GetPunchRange();
        packet.m_net_id = this->GetNetId();
        packet.m_flags = NET_GAME_PACKET_FLAGS_NONE;
        packet.m_water_speed = GetWaterSpeed();
        packet.m_effect_flags = GetFlags();
        packet.m_acceleration = GetAcceleration();
        packet.m_punch_strength = GetPunchStrength();
        packet.m_speed_out = GetSpeed();
        packet.m_gravity_out = GetGravity();
        packet.m_pupil_color = GetPupilColor().GetInt();
        packet.m_hair_color = GetHairColor().GetInt();
        packet.m_eyes_color = GetEyesColor().GetInt();

        player->SendPacket(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket));
    }
    void Player::SendExperience(std::shared_ptr<World> world, const int32_t& amount) {
        auto& exp = this->get_experience();
        if (exp.second >= 125)
            return;
        exp.first += amount;
        if (exp.first >= (exp.second * 1500)) {
            exp.first = 0;
            exp.second += 1;

            GameUpdatePacket update_packet {
                .m_type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
                .m_particle_emitter_id = -1,
                .m_particle_alt_id = 46
            };
            update_packet.m_particle_pos_x = this->GetPosition().m_x + 12;
            update_packet.m_particle_pos_y = this->GetPosition().m_y + 15;
            
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
                ply->v_sender.OnTalkBubble(this->GetNetId(), fmt::format("`o{} `wis now level {}!", this->GetDisplayName(world), exp.second), true);                           
                ply->SendLog("`o{} `wis now level {}!", this->GetDisplayName(world), exp.second);
            });
        }
    }

    bool Player::HasAccess(std::shared_ptr<World> world, const CL_Vec2i position, uint32_t item) {
        Tile* tile = world->GetTile(position);
        if (!tile)
            return false;
        if (world->IsTileOwned(tile)) {
            Tile* parent = world->GetParentTile(tile);
            if (!parent)
                return false;
            if (this->GetRole() >= PLAYER_ROLE_DEVELOPER)
                return true;
            if (parent->GetOwnerId() == this->GetUserId())
                return true;
            if (parent->GetBaseItem()->m_id == ITEM_BUILDERS_LOCK) {
                if (parent->HasAccess(this->GetUserId())) {
                    if (!parent->IsLockFlagOn(LOCKFLAG_RESTRICT_ADMIN))
                        return true;
                    if (parent->IsLockFlagOn(LOCKFLAG_ONLY_BUILDING)) {
                        if (item == ITEM_FIST && !(tile->GetBaseItem()->m_item_category & ITEMFLAG2_PUBLIC)) {
                            this->v_sender.OnTalkBubble(this->GetNetId(), "`wThat area allows only Building.``", true);
                            return false;
                        }
                    } else if (item != ITEM_FIST && item != ITEM_WRENCH) {
                        this->v_sender.OnTalkBubble(this->GetNetId(), "`wThat area allows only Breaking.``", true);
                        return false;
                    }
                    return true;
                } else {
                    if (parent->IsFlagOn(TILEFLAG_PUBLIC)) {
                        if (parent->IsLockFlagOn(LOCKFLAG_ONLY_BUILDING)) {
                            if (item == ITEM_FIST && !(tile->GetBaseItem()->m_item_category & ITEMFLAG2_PUBLIC)) {
                                this->v_sender.OnTalkBubble(this->GetNetId(), "`wThat area allows only Building.``", true);
                                return false;
                            }
                        } else if (item != ITEM_FIST && item != ITEM_WRENCH) {
                            this->v_sender.OnTalkBubble(this->GetNetId(), "`wThat area allows only Breaking.``", true);
                            return false;
                        }
                    }
                    return true;
                }
            }
            if (this->GetRole() >= PLAYER_ROLE_DEVELOPER)
                return true;
            if (parent->IsFlagOn(TILEFLAG_PUBLIC) || parent->HasAccess(this->GetNetId()) || (tile->GetBaseItem()->m_item_category & ITEMFLAG2_PUBLIC))
                return true;
            return false;
        }
        if (world->IsOwned()) {
            Tile* main_lock = world->GetTile(world->GetMainLock());
            if (!main_lock)
                return false;
            if (main_lock->GetOwnerId() == this->GetUserId())
                return true;
            if (main_lock->HasAccess(this->GetUserId()) || main_lock->IsFlagOn(TILEFLAG_PUBLIC) || (tile->GetBaseItem()->m_item_category & ITEMFLAG2_PUBLIC))
                return true;
            return false;
        }
        return true;
    }
    bool Player::RemoveItemSafe(const uint16_t& item_id, const uint8_t& count, const bool& send_packet) {
        auto* item = ItemDatabase::GetItem(item_id);
        if (!item)
            return false;
        if (!this->m_inventory.Erase(item->m_id, count, send_packet))
            return false;
        if (!this->m_inventory.Contain(item->m_id)) {
            if (item->m_item_type == ITEMTYPE_CLOTHES || item->m_item_type == ITEMTYPE_ANCES)
                this->SetCloth(item->m_clothing_type, ITEM_BLANK, true);
        }
        return true;
    }

    void Player::AddPlaymod(ePlaymodType type, uint16_t icon_id, steady_clock::time_point apply_mod, std::chrono::seconds time) {  
        if (this->HasPlaymod(type)) {
            this->GetPlaymods()[static_cast<std::size_t>(type)].m_icon_id = icon_id;
            return;
        }
        Playmod mod{};
        mod.m_type = type;
        mod.m_icon_id = icon_id;
        mod.m_time = TimingClock{ apply_mod, time };
        m_playmods.push_back(mod);
        if (!this->GetPeer())
            return;

        switch (type) {
        case PLAYMOD_TYPE_DOUBLE_JUMP: { CharacterState::SetFlag(STATEFLAG_DOUBLE_JUMP); } break;
        case PLAYMOD_TYPE_HIGH_JUMP: { CharacterState::SetGravity(this->GetGravity() - 300.0f); } break;
        case PLAYMOD_TYPE_FISTS_O_FURY: { CharacterState::SetPunchStrength(this->GetPunchStrength() + 120.0f); } break;
        case PLAYMOD_TYPE_ARMED_AND_DANGEROUS: { CharacterState::SetPunchID(PUNCH_EFFECT_TOMMYGUN); } break;
        case PLAYMOD_TYPE_SPEEDY: { 
            if (!this->HasPlaymod(PLAYMOD_TYPE_TANK_DRIVER))
                CharacterState::SetSpeed(this->GetSpeed() + 60.0f); 
        } break;
        case PLAYMOD_TYPE_GHOST_IN_THE_SHELL: {
            if (!this->HasPlaymod(PLAYMOD_TYPE_GHOST_IN_THE_SHELL))
                CharacterState::SetSpeed(this->GetSpeed() + 90.0f);
                CharacterState::SetFlag(STATEFLAG_NOCLIP);
        } break;
        case PLAYMOD_TYPE_TANK_DRIVER: {
            CharacterState::SetPunchID(PUNCH_EFFECT_TINY_TANK);
            if (!this->HasPlaymod(PLAYMOD_TYPE_SPEEDY))
                CharacterState::SetSpeed(260.0f + 60.0f);
        } break;
        case PLAYMOD_TYPE_IN_THE_SPOTLIGHT: {
            CharacterState::SetFlag(STATEFLAG_SPOTLIGHT);
        } break;
        case PLAYMOD_TYPE_FROZEN: {
            Color blue = { 0x00, 0xC0, 0xFF, 0x7F }; // transparent blue
            this->SetSkinColor(blue);
            CharacterState::SetFlag(STATEFLAG_FROZEN);
            this->v_sender.OnSetClothing(this->GetClothes(), this->GetSkinColor(), true, this->GetNetId());
        } break;
        case PLAYMOD_TYPE_CURSE: {
            Color darkGrey = { 0x80, 0x80, 0x80, 0xFF }; // fully opaque dark grey
            this->SetSkinColor(darkGrey);
            this->v_sender.OnSetClothing(this->GetClothes(), this->GetSkinColor(), true, this->GetNetId());
            CharacterState::SetFlag(STATEFLAG_NO_EYE);
        } break;
        default:
            break;
        }
 
        PlaymodData mod_data = PlaymodManager::Get(type);
        this->SendLog("`o{}`` (`${} `omod added.)``", mod_data.m_adding, mod_data.m_name);
    }
    void Player::RemovePlaymod(ePlaymodType type) {
        auto& playmods = this->GetPlaymods();
        auto iterator = std::find_if(playmods.begin(), playmods.end(), 
            [&type](const Playmod& mod) { return mod.m_type == type; });
        if (iterator == playmods.end())
            return;
        iterator = playmods.erase(iterator);
        if (!this->GetPeer())
            return;

        switch (type) {
        case PLAYMOD_TYPE_DOUBLE_JUMP: {
            if (!this->HasPlaymod(PLAYMOD_TYPE_HELICOPTER_HAIR))
                CharacterState::RemoveFlag(STATEFLAG_DOUBLE_JUMP);
        } break;
        case PLAYMOD_TYPE_HIGH_JUMP: { CharacterState::SetGravity(this->GetGravity() + 300.0f); } break;
        case PLAYMOD_TYPE_FISTS_O_FURY: { CharacterState::SetPunchStrength(this->GetPunchStrength() - 120.0f); } break;
        case PLAYMOD_TYPE_ARMED_AND_DANGEROUS: { CharacterState::SetPunchID(this->GetActivePunchID()); } break;
        case PLAYMOD_TYPE_DUCT_TAPE: {
            if (this->HasPlaymod(PLAYMOD_TYPE_DUCT_TAPE))
                break;
            this->SetCloth(CLOTHTYPE_FACE, ITEM_BLANK, false);
        } break;
        case PLAYMOD_TYPE_SPEEDY: { 
            if (this->HasPlaymod(PLAYMOD_TYPE_TANK_DRIVER))
                break;
            CharacterState::SetSpeed(this->GetSpeed() - 60.0f);
        } break;
        case PLAYMOD_TYPE_GHOST_IN_THE_SHELL: {
            if (!this->HasPlaymod(PLAYMOD_TYPE_GHOST_IN_THE_SHELL))
                CharacterState::SetSpeed(260.0f);
                CharacterState::RemoveFlag(STATEFLAG_NOCLIP);
        } break;
        case PLAYMOD_TYPE_TANK_DRIVER: {
            CharacterState::SetPunchID(this->GetActivePunchID());
            if (!this->HasPlaymod(PLAYMOD_TYPE_SPEEDY))
                CharacterState::SetSpeed(this->GetSpeed() - 60.0f);
        } break;
        case PLAYMOD_TYPE_IN_THE_SPOTLIGHT: {
            CharacterState::RemoveFlag(STATEFLAG_SPOTLIGHT);
        } break;
        case PLAYMOD_TYPE_FROZEN: {
            Color firstcolor = { 0xB4, 0x8A, 0x78, 0xFF };
            this->SetSkinColor(firstcolor);
            CharacterState::RemoveFlag(STATEFLAG_FROZEN);
            this->v_sender.OnSetClothing(this->GetClothes(), this->GetSkinColor(), true, this->GetNetId());
        } break;
        case PLAYMOD_TYPE_CURSE: {
            Color firstcolor = { 0xB4, 0x8A, 0x78, 0xFF };
            this->SetSkinColor(firstcolor);
            this->v_sender.OnSetClothing(this->GetClothes(), this->GetSkinColor(), true, this->GetNetId());
            CharacterState::RemoveFlag(STATEFLAG_NO_EYE);
        } break;
        default:
            break;
        }
        
        PlaymodData mod_data = PlaymodManager::Get(type);
        this->SendLog("`o{}`` (`${} `omod removed.)``", mod_data.m_removing, mod_data.m_name);
        this->PlaySfx("dialog_confirm", 0);
    }
    bool Player::HasPlaymod(ePlaymodType type) {
        for (auto& mod : this->GetPlaymods()) {
            if (mod.m_type == type)
                return true;
        }
        return false;
    }
    bool Player::GetPlaymod(ePlaymodType type, Playmod& val) {
        for (auto& mod : this->GetPlaymods()) {
            if (mod.m_type == type) {
                val = mod;
                return true;
            }
        }
        return false;
    }
    uint8_t Player::GetActivePunchID() {
        if (this->HasPlaymod(PLAYMOD_TYPE_ARMED_AND_DANGEROUS))
            return PUNCH_EFFECT_TOMMYGUN;
        if (this->HasPlaymod(PLAYMOD_TYPE_TANK_DRIVER))
            return PUNCH_EFFECT_TINY_TANK;
        return PUNCH_EFFECT_NONE;
    }
}