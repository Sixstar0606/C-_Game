#include <server/server_pool.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>
#include <enet/enet.h>
#include <event/event_pool.h>
#include <player/player_pool.h>
#include <world/world_pool.h>
#include <render/world_render.h>
#include <database/database.h>
#include <utils/text.h>
#include <proton/packet.h>

namespace GTServer {
    ServerPool::ServerPool(std::shared_ptr<EventPool> events) :
        PacketDecoder{ },
        m_events{ events } {
        fmt::print("Initializing ServerPool\n");
    }
    ServerPool::~ServerPool() {
        //TODO: delete servers
        enet_deinitialize();
    }

    bool ServerPool::InitializeENet() {
        if (enet_initialize() != 0) {
            fmt::print("Failed to Initialize ENet Library\n");
            return false;
        }
        return true;
    }
    void ServerPool::DeInitializeENet() {
        enet_deinitialize();
    }

    std::shared_ptr<Server> ServerPool::StartInstance() {
        uint8_t instance_id = static_cast<uint8_t>(m_servers.size());
        auto server{ std::make_shared<Server>(instance_id, m_address, m_port++, m_max_peers) };
        if (!server->Start()) {
            fmt::print("failed to start enet server -> {}:{}", server->GetAddress(), server->GetPort());
            return nullptr;
        }
        fmt::print("starting instance_id: {}, {}:{} - {}\n", server->GetInstanceId(), server->GetAddress(), server->GetPort(), std::chrono::system_clock::now());
        m_servers.push_back(std::move(server));
        return server;
    }
    void ServerPool::StopInstance(std::shared_ptr<Server> server) {
        if (!server)
            return;

        fmt::print("shutting down instance_id: {} - {}\n", server->GetInstanceId(), std::chrono::system_clock::now());
        if (auto it = std::find(m_servers.begin(), m_servers.end(), server); it != m_servers.end()) {
            server->Stop();
            server.reset();
            m_servers.erase(it);
            return;
        }
        fmt::print("failed to shutdown instance_id: {}\n", server->GetInstanceId());
    }

    void ServerPool::StartService() {
        if (m_running.load())
            return;
        if (m_servers.empty())
            this->StartInstance();
        m_running.store(true);

        m_threads.push_back(std::thread{ &ServerPool::ServicePoll, this });
        m_threads.push_back(std::thread{ [&]() {
            while (true) {
                if (this->m_queue_worker.empty())
                    continue;
                auto now = high_resolution_clock::now();
                ServerQueue& ctx = this->m_queue_worker.front();

                switch (ctx.m_queue_type) {
                case QUEUE_TYPE_FINDING_ITEMS: {
                    std::string keyword = ctx.m_keyword;
                    std::transform(keyword.begin(), keyword.end(), keyword.begin(), [](unsigned char c) { return std::tolower(c); });
                    std::vector<ItemInfo*> items;

                    for (const auto& item : ItemDatabase::GetItems()) {
                        std::string item_name = item->m_name;
                        std::transform(item_name.begin(), item_name.end(), item_name.begin(), [](unsigned char c) { return std::tolower(c); });
                        if (item_name.find(keyword) == std::string::npos)
                            continue;
                        if (item->m_id % 2 == 1)
                            continue;
                        items.push_back(item);
                    }

                    if (items.size() < 1) {
                        ctx.m_player->SendLog("`4Oops! `ocould not find the following item. (`w{}`o)``", ctx.m_keyword);
                        break;
                    }
                    DialogBuilder dialog{};
                    dialog.add_label_with_icon("`wItems Finding``", ITEM_GROWSCAN_9000, DialogBuilder::LEFT, DialogBuilder::BIG)
                        ->add_spacer()
                        ->add_label(fmt::format("found `2{}`` results for keyword `2{}``", items.size(), ctx.m_keyword));
                    if (items.size() > 50) {
                        dialog.embed_data<uint8_t>("page", 1)
                            ->add_button("next_page", "Next Page `7>>``");
                    }
                    dialog.add_spacer()
                        ->add_textbox("`wResults:``")
                        ->text_scaling_string("idkman");
                    for (uint16_t index = 0; index < items.size(); index++) {
                        if (index >= 50)
                            break;
                        ItemInfo* item = items[index];
                        dialog.text_scaling_string("|")
                            ->add_checkicon(fmt::format("item_{}", item->m_id), fmt::format("`w{}``", item->m_name), item->m_id, fmt::format("{}", item->m_id), false);
                    }
                    dialog.add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)
                        ->add_text_input("count", "Count:", "", 5)
                        ->add_quick_exit()
                        ->end_dialog("search_item", "Cancel", "Claim Items!");
                    ctx.m_player->v_sender.OnDialogRequest(dialog.get());
                } break;
                case QUEUE_TYPE_GET_FRIENDS: {
                    std::string friends_file_path = std::format("PlayerData/{}/friends.txt", ctx.m_player->GetUserId());

                    // Check if the file exists and create it if it doesn't
                    std::ifstream friends_file(friends_file_path);
                    if (!friends_file.good()) {
                        std::ofstream new_file(friends_file_path);
                        new_file.close();
                    }

                    // Re-open the file for reading
                    friends_file.open(friends_file_path);

                    // Create a dialog builder object to build the dialog
                    DialogBuilder dialog;

                    // Add a label with the title of the dialog
                    dialog.add_label_with_icon("`wFriends``", ITEM_FRIENDLY_COCONUT, DialogBuilder::LEFT, DialogBuilder::BIG);

                    // Add a spacer between the title and the list of friends
                    dialog.add_spacer();

                    // Create a loop to read each line of the friends.txt file
                    std::string line;
                    while (std::getline(friends_file, line)) {
                        // Trim any whitespace from the beginning and end of the line
                        line = std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");

                        // Add a button for the user ID on this line
                        dialog.embed_data<uint32_t>("friend_id", std::stoul(line))
                            ->add_button("view_friend", line);
                    }

                    // Add a spacer at the end of the list of friends
                    dialog.add_spacer();

                    // Add a button to close the dialog
                    dialog.add_button_with_icon("", "END_LIST", 0, DialogBuilder::NONE)
                        ->add_quick_exit()
                        ->add_button("close", "`wClose");
                    ctx.m_player->v_sender.OnDialogRequest(dialog.get());
                } break;
                case QUEUE_TYPE_FINDING_PLAYERS: {
                    std::string keyword = ctx.m_keyword;
                    PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                    auto players = database->GetPlayersMatchingName(keyword);
                    if (players.size() < 1) {
                        ctx.m_player->SendLog("`4Oops! `ocould not find the following player's name. (`w{}`o)``", ctx.m_keyword);
                        break;
                    }
                    int32_t ordered_id = 1;
                    DialogBuilder db{};
                    db.set_default_color('o')
                        ->add_label_with_icon(fmt::format("`wFinding Player: `2{}``", ctx.m_keyword), ITEM_GROWSCAN_9000, DialogBuilder::LEFT, DialogBuilder::BIG)
                        ->add_smalltext("`4INFO``: please don't `4abuse`` on this command or will be result in ban, we store every your action.")
                        ->add_spacer();
                    if (players.size() > 20)
                        db.add_button("next_page", "Next Page >>")
                            ->add_spacer();
                    for (auto& [user_id, name] : players) {
                        if (ordered_id > 20)
                            break;
                        bool online = this->HasPlayer(user_id);
                        db.add_friend_image_label_button(
                            fmt::format("uid_{}", user_id), 
                            fmt::format("`#{}- {}{}", ordered_id, name,
                            online == false ? 
                                fmt::format(" ({})", XYZClock::to_string(std::chrono::duration_cast<std::chrono::seconds>(system_clock::now() - database->GetRowTimestamp(user_id, 1)))) : 
                                ""
                            ), 
                            "game/tiles_page14.rttex", 1.3, { online ? 28 : 31, 23 });
                        ordered_id++;
                    }
                    db.add_quick_exit()
                        ->end_dialog("find_player", "Cancel", "");
                    ctx.m_player->v_sender.OnDialogRequest(db.get());
                } break;
                case QUEUE_TYPE_RENDER_WORLD: {
                    if (!ctx.m_world)
                        break;
                    auto result = WorldRender::render(this, ctx.m_world);
                    switch (result) {
                    case WorldRender::RENDER_RESULT_SUCCESS: {
                        PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                        auto vanguard = (dpp::cluster*)DiscordBot::GetBot(DiscordBot::BOT_TYPE_VANGUARD);
                        
                        dpp::message message(dpp::snowflake{ 1020562097853190154 }, 
                        dpp::embed().
                            set_color(0x00FFFF). 
                            add_field(
                                "World:", ctx.m_world->GetName(), true
                            ).
                            set_image(fmt::format("attachment://{}.png", ctx.m_world->GetName())).
                            set_footer(dpp::embed_footer().set_text("BetterGrowtopia")).
                            set_timestamp(std::time(0))
                        );
                        message.set_file_content(dpp::utility::read_file(fmt::format("renders/{}.png", ctx.m_world->GetName())));
                        message.set_filename(fmt::format("{}.png", ctx.m_world->GetName()));
                        vanguard->message_create(message);
                    } break;
                    case WorldRender::RENDER_RESULT_FAILED: {
                        fmt::print("WorldRender::Render -> failed to render world {}\n", ctx.m_world->GetName());
                    } break;
                    }
                } break;
                case QUEUE_TYPE_ACCOUNT_VERIFICATION: {
                    bool found = false;
                    auto* cluster = (dpp::cluster*)DiscordBot::GetBot(DiscordBot::BOT_TYPE_VANGUARD);
                    auto* guild = dpp::find_guild(dpp::snowflake(948423022744850443));
                    for (auto& [snowflake_id, guild_member] : guild->members) {
                        auto* user = dpp::find_user(snowflake_id);
                        if (user->format_username() != ctx.m_keyword)
                            continue;
                        if (m_account_verify.find(user->id) != m_account_verify.end()) {
                            auto& [user_id, time] = m_account_verify[user->id];
                            if (time.GetPassedTime() < time.GetTimeout()) {
                                DialogBuilder db{};
                                db.set_default_color('o')
                                    ->add_label_with_icon("`wAccount Verification``", ITEM_WARNING_BLOCK, DialogBuilder::LEFT, DialogBuilder::BIG)
                                    ->add_spacer()
                                    ->add_textbox(fmt::format("Sorry, seem like you've requested a account verification for Discord Account: `w{}``", ctx.m_keyword))
                                    ->add_textbox(fmt::format("You've {:%M:%S} minutes left before its being expired.", std::chrono::floor<std::chrono::seconds>(time.GetTimeout() - time.GetPassedTime())))
                                    ->end_dialog("account_verify", "`wOkay``", "");
                                ctx.m_player->v_sender.OnDialogRequest(db.get(), 0); 
                            }
                            if (time.GetPassedTime() > time.GetTimeout())
                                this->m_account_verify.erase(user->id);
                            break;
                        }
                        dpp::message message{};
                        message.add_embed(dpp::embed()
                            .set_color(0)
                            .set_footer(fmt::format("BetterGrowtopia - {}", system_clock::now()), cluster->me.get_avatar_url())
                            .set_title("BetterGrowtopia Discord Verification")
                            .set_description(fmt::format("You've recieve an account verification message for **GrowID**: **{}** in BetterGrowtopia, do not click any button if you're not sending this request. To verify your account click **\"Verify!\"** button below, you have 10 minutes before it expired.", ctx.m_player->GetRawName()))
                            .set_thumbnail("https://i.imgur.com/EQdgGwV.jpg"));
                        message.add_component(dpp::component()
                            .add_component(dpp::component()
                                .set_label("Verify!")
                                .set_type(dpp::cot_button)
                                .set_id(fmt::format("verify_account_{}", static_cast<uint64_t>(user->id)))
                            )
                        );
                        cluster->direct_message_create(snowflake_id, message);
                        m_account_verify.insert_or_assign(user->id, std::pair<uint32_t, TimingClock>{ ctx.m_player->GetUserId(), TimingClock{ 10 * 60 } });
                        
                        DialogBuilder db{};
                        db.set_default_color('o')
                            ->add_label_with_icon("`wAccount Verification``", ITEM_WARNING_BLOCK, DialogBuilder::LEFT, DialogBuilder::BIG)
                            ->add_spacer()
                            ->add_textbox(fmt::format("Your `wVerification Message`` has been sent to `w{}``", user->format_username()))
                            ->end_dialog("account_verify", "`wOkay``", "");
                        ctx.m_player->v_sender.OnDialogRequest(db.get(), 0);
                        found = true;
                        break;
                    }
                    if (!found) {
                        auto* cluster = (dpp::cluster*)DiscordBot::GetBot(DiscordBot::BOT_TYPE_VANGUARD);
                        cluster->message_create(dpp::message(dpp::snowflake(1022078096964341821), fmt::format("We cannot find **{}** on this server, report date - {}", ctx.m_keyword, system_clock::now())));
                    }
                } break;
                default:
                    break;
                }

                auto time_taken = high_resolution_clock::now() - now;
                ctx.m_player->SendLog("[DEBUG]: handled {} for `w{}``, took {}ms - {}us", magic_enum::enum_name(ctx.m_queue_type), ctx.m_player->GetDisplayName(ctx.m_world != nullptr ? ctx.m_world : nullptr),
                    std::chrono::duration_cast<std::chrono::milliseconds>(time_taken).count(), std::chrono::duration_cast<std::chrono::microseconds>(time_taken).count());
                this->m_queue_worker.pop_front();
            }
        }});
        m_threads.push_back(std::thread{ [&]() {
            while (m_running.load())  {
            for (auto& server : m_servers) {
                auto& queues = server->m_queue_worker;
                if (queues.empty())
                    continue;
                ServerQueue& ctx = queues.front();
                if (!ctx.m_player)
                    continue;

                switch (ctx.m_queue_type) {
                case SERVERQUEUE_TYPE_LOGIN: {
                    PlayerTable* table = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
                    if (!table->Load(ctx.m_player)) {
                        ctx.m_player->SendLog("`4Unable to log on: `oThat `wGrowID `odoesn't seem valid, or the password is wrong. If you don't have one, press `wCancel`o, un-check `w'I have a GrowID'`o, then click `wConnect`o.``");
                        ctx.m_player->SendSetURL(config::server::discord, "`eBetterGrowtopia Discord``");
                        ctx.m_player->Disconnect(0U);
                        break;
                    }
                    bool found_session{ false };
                    for (auto& player : this->GetPlayers()) {
                        if (!(ctx.m_player->GetPeer() != player->GetPeer() && ctx.m_player->GetUserId() == player->GetUserId()))
                            continue;
                        ctx.m_player->v_sender.OnConsoleMessage("`4OOPS, `oSomeone else was logged into this account! He was kicked out now.``");
                        player->v_sender.OnConsoleMessage("`4OOPS, `oSomeone else logged into this account!``");
                        player->Disconnect(0U);
                        found_session = true;
                        break;
                    }
                    if (found_session) {       
                        if (!table->Load(ctx.m_player)) {
                            ctx.m_player->SendLog("`4Unable to log on: `oThat `wGrowID `odoesn't seem valid, or the password is wrong. If you don't have one, press `wCancel`o, un-check `w'I have a GrowID'`o, then click `wConnect`o.``");
                            ctx.m_player->SendSetURL(config::server::discord, "`eBetterGrowtopia Discord``");
                            ctx.m_player->Disconnect(0U);
                            break;
                        }
                    }
                    // TODO: Player::IsPlaymodActive(PLAYMOD_TYPE_BAN)

                    ctx.m_player->SetFlag(PLAYERFLAG_LOGGED_ON);
                    ctx.m_player->v_sender.OnSuperMainStart(
                        ItemDatabase::Get().GetHash(),
                        config::server::cache_server, 
                        config::server::cache_path, 
                        "cc.cz.madkite.freedom org.aqua.gg idv.aqua.bulldog com.cih.gamecih2 com.cih.gamecih com.cih.game_cih cn.maocai.gamekiller com.gmd.speedtime org.dax.attack com.x0.strai.frep com.x0.strai.free org.cheatengine.cegui org.sbtools.gamehack com.skgames.traffikrider org.sbtoods.gamehaca com.skype.ralder org.cheatengine.cegui.xx.multi1458919170111 com.prohiro.macro me.autotouch.autotouch com.cygery.repetitouch.free com.cygery.repetitouch.pro com.proziro.zacro com.slash.gamebuster",
                        "proto=175|choosemusic=audio/mp3/about_theme.mp3|active_holiday=0|wing_week_day=0|ubi_week_day=0|server_tick=263203319|clash_active=0|drop_lavacheck_faster=1|isPayingUser=0|usingStoreNavigation=1|enableInventoryTab=1|bigBackpack=1|",
                        PlayerTribute::get().get_hash()
                    );
                    fmt::print("[LoginQueue]: A player {} has logged on, userId: {} - {}\n", ctx.m_player->GetRawName(), ctx.m_player->GetUserId(), system_clock::now());
                } break;
                default:
                    break;
                }
                queues.pop_front();
            }}
        }});
        for (auto& thread : m_threads)
            thread.detach();
    }
    void ServerPool::StopService() {
        if (!m_running.load())
            return;
        m_running.store(false);
    }
    void ServerPool::ServicePoll() {
        try {
        ENetEvent event{};
        while (m_running.load()) {
            for (auto& server : m_servers) {
                while (enet_host_check_events(server->GetHost(), &event)) {
                    if (!event.peer)
                        break;

                    switch(event.type) {
                    case ENET_EVENT_TYPE_CONNECT: {
                        std::shared_ptr<Player> player{ server->GetPlayerPool()->NewPlayer(event.peer) };
                        player->SendPacket({ NET_MESSAGE_SERVER_HELLO }, sizeof(TankUpdatePacket));
                        break;
                    }
                    case ENET_EVENT_TYPE_DISCONNECT: {
                        if (!event.peer->data)
                            break;
                        std::uint32_t connect_id{};
                        std::memcpy(&connect_id, event.peer->data, sizeof(std::uint32_t));
                        std::free(event.peer->data);
                        event.peer->data = NULL;

                        std::shared_ptr<Player> player{ server->GetPlayerPool()->GetPlayer(connect_id) };
                        if (!player)
                            return;
                        if (player->IsFlagOn(PLAYERFLAG_LOGGED_ON)) {
                            player->set_last_active(system_clock::now());
                            PlayerTable* db{ (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE) };
                            if (!db->Save(player))
                                fmt::print("PlayerTable::save, Failed to save {} - {}\n", player->GetRawName(), system_clock::now());
                        }
                        if (!player->GetWorld().empty() || player->GetWorld() != std::string{ "EXIT" }) {
                            std::shared_ptr<WorldPool> world_pool{ server->GetWorldPool() };
                            std::shared_ptr<World> world{ world_pool->GetWorld(player->GetWorld()) };
                            if (world)
                                world_pool->OnPlayerLeave(world, player, false);
                        }
                        server->GetPlayerPool()->RemovePlayer(connect_id);
                        break;
                    }
                    case ENET_EVENT_TYPE_RECEIVE: {
                        if (event.packet->dataLength < sizeof(TankUpdatePacket::m_type) + 1 || event.packet->dataLength > 0x400) {
                            enet_packet_destroy(event.packet);
                            break;
                        }
                        std::shared_ptr<Player> player{ server->GetPlayerPool()->GetPlayer(event.peer->connectID) };
                        if (!player) {
                            enet_packet_destroy(event.packet);
                            break;
                        }
                        switch (*((int32_t*)event.packet->data)) {
                        case NET_MESSAGE_GENERIC_TEXT:
                        case NET_MESSAGE_GAME_MESSAGE: {
                            const auto& str = PacketDecoder::DataToString(event.packet->data + 4, event.packet->dataLength - 4);
                            EventContext ctx { 
                                .m_player = player,
                                .m_events = this->GetEvents(),
                                .m_server = server,
                                .m_servers = this,
                                .m_parser = TextScanner{ str }, 
                                .m_update_packet = nullptr 
                            };
                            std::string event_data = str.substr(0, str.find('|'));
                            if (!m_events->execute(EVENT_TYPE_GENERIC_TEXT, event_data, ctx))
                                break;
                            break;
                        }
                        case NET_MESSAGE_GAME_PACKET: {
                            GameUpdatePacket* update_packet = this->DataToUpdatePacket(event.packet);
                            if (!update_packet)
                                break;
                            if (player->m_packet_sec + 1000 > std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() && update_packet->m_type != NET_GAME_PACKET_STATE && update_packet->m_type != NET_GAME_PACKET_ITEM_ACTIVATE_OBJECT_REQUEST) {
                                if (player->m_packet_in_sec >= 85) {
                                    player->SendLog("`4Warning: `oYou are sending too many packets!");
                                    break;
                                }
                                player->m_packet_in_sec++;
                            } else {
                                player->m_packet_sec = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                                player->m_packet_in_sec = 0;
                            }
                    
                            EventContext ctx { 
                                .m_player = player,
                                .m_events = this->GetEvents(),
                                .m_server = server,
                                .m_servers = this,
                                .m_parser = TextScanner{}, 
                                .m_update_packet = update_packet 
                            };

                            if (!m_events->execute(EVENT_TYPE_GAME_PACKET, "gup_" + std::to_string(update_packet->m_type), ctx)) {
                                player->SendLog("unhandled EVENT_TYPE_GAME_PACKET -> `w{}`o", magic_enum::enum_name(static_cast<eNetPacketType>(update_packet->m_type)));
                                break;
                            }
                            break;
                        }
                        }
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    case ENET_EVENT_TYPE_NONE:
                    default:
                        break;
                    }
                }

                enet_host_service(server->GetHost(), nullptr, 0);
            } 
        }
        } catch (std::exception& e) {
            fmt::print("ServerPool >> {}\n", e.what());
        }
    }

    bool ServerPool::HasPlayer(const uint32_t& user_id) const {
        for (auto& server : m_servers) {
            auto pool = server->GetPlayerPool();
            if (pool->HasPlayer(user_id))
                return true;
        }
        return false;
    }
    std::shared_ptr<Player> ServerPool::GetPlayerByUserID(const uint32_t& user_id) {
        for (auto& server : m_servers) {
            auto pool = server->GetPlayerPool();
            if (!pool->HasPlayer(user_id))
                continue;
            return pool->GetPlayerByUserId(user_id);
        }
        PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
        std::shared_ptr<Player> ret = std::make_shared<Player>(nullptr);
        if (!database->SerializeByUserID(ret, user_id))
            return nullptr;
        return ret;
    }
    std::shared_ptr<Player> ServerPool::GetPlayerByName(const std::string& name) {
        for (auto& server : m_servers) {
            auto pool = server->GetPlayerPool();
            if (!pool->HasPlayer(name))
                continue;
            return pool->GetPlayerByName(name);
        }
        PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
        std::shared_ptr<Player> ret = std::make_shared<Player>(nullptr);
        if (!database->SerializeByName(ret, name))
            return nullptr;
        return ret;
    }
    std::shared_ptr<Player> ServerPool::GetPlayerByFormat(const std::string& data) {
        if (utils::IsUserIdFormat(data))
            return this->GetPlayerByUserID(std::atoi(data.substr(1).c_str()));
        else if (utils::IsForceUserFormat(data))
            return this->GetPlayerByName(data.substr(1));
        return this->GetPlayerByName(data);
    }
}