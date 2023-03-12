#include <world/world_pool.h>
#include <player/player.h>
#include <server/server_pool.h>
#include <database/database.h>
#include <proton/utils/world_menu.h>
#include <stdlib.h>

namespace GTServer {
    WorldPool::~WorldPool() {
        for (auto& [name, world] : this->m_worlds)
            this->RemoveWorld(name);
        this->m_worlds.clear();
    }

    std::vector<WorldPool::RandomWorld> WorldPool::GetRandomWorlds(const bool& required_players, const bool& jammed) {
        std::vector<RandomWorld> ret{};
        for (const auto& [name, world] : this->m_worlds) {
            if (world->GetPlayers(false).size() < 1 && required_players)
                continue;
            if (world->IsFlagOn(WORLDFLAG_JAMMED) && !jammed)
                continue;
            ret.push_back(RandomWorld{ world->GetPlayers(false).size(), name });
        }
        return ret;
    }
    void WorldPool::SendDefaultOffers(std::shared_ptr<Player> invoker) {
        WorldMenu menu{};
        menu.set_default("START")
            ->add_filter()
            ->set_max_rows(2)
            ->add_heading("Active Worlds")
            ->add_floater("START", this->GetWorld("START")->GetPlayers(false).size(), 0.7, Color{ 0xFF, 0x0, 0xB1 });

        std::vector<RandomWorld> worlds{ this->GetRandomWorlds(true, false) };
        if (!worlds.empty()) {
            std::sort(worlds.begin(), worlds.end(), std::greater<RandomWorld>());
            for (const auto& world : worlds) {
                float size = 0.40 + (0.03 * world.m_players);
                if (size > 0.75) 
                    size = 0.75;
                menu.add_floater(world.m_name, world.m_players, size, Color{ 0xB4, 0xBD, 0xC2 });
            }
        }
        invoker->v_sender.OnRequestWorldSelectMenu(menu.get());
    }
    void WorldPool::SendCategorySelection() {

    }

    std::shared_ptr<World> WorldPool::NewWorld(const std::string& name) {
        std::shared_ptr<World> world{ std::make_shared<World>(name, 100, 60) };
        WorldTable* db{ (WorldTable*)Database::GetTable(Database::DATABASE_WORLD_TABLE) };

        if (db->is_exist(name)) {
            if (!db->load(world)) {
                world.reset();
                return nullptr;
            }
            m_worlds.insert_or_assign(name, std::move(world));
            return m_worlds[name];
        }
        world->Generate(WORLD_TYPE_NORMAL);
        world->SetID(db->insert(world));
        if (world->GetID() == 0) {
            fmt::print("failed to insert database for World -> {}\n", name);
            world.reset();
            return nullptr;
        }
        m_worlds.insert_or_assign(name, std::move(world));
        return m_worlds[name];
    }
    void WorldPool::RemoveWorld(const std::string& name) {
        m_worlds[name].reset();
        m_worlds.erase(name);
    }
    std::shared_ptr<World> WorldPool::GetWorld(const std::string& name) {
        if (name.empty() || name == std::string{ "EXIT" })
            return nullptr;
        for (auto& world : m_worlds) {
            if (world.first != name)   
                continue;
            return world.second;
        }
        return this->NewWorld(name);
    }

    void WorldPool::OnPlayerJoin(ServerPool* pool, std::shared_ptr<World> world, std::shared_ptr<Player> player, const CL_Vec2i& pos) {
        if (world->IsFlagOn(WORLDFLAG_NUKED) && player->GetRole() < PLAYER_ROLE_MODERATOR) {
            player->v_sender.OnConsoleMessage("Sorry, that world has been removed from BetterGrowtopia to keep our players safe.");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (player->HasPlaymod(PLAYMOD_TYPE_BAN) && player->GetRole() < PLAYER_ROLE_DEVELOPER) {
            player->v_sender.OnConsoleMessage("`oOops, you are currently `4BANNED `ofrom BetterGrowtopia, join when you have been unbanned.");
            player->v_sender.OnFailedToEnterWorld(true);
            return;
        }
        if (player->HasPlaymod(PLAYMOD_TYPE_CURSE)) {
            player->CharacterState::SetFlag(STATEFLAG_NO_EYE);
            Color darkGrey = { 0x80, 0x80, 0x80, 0xFF }; // fully opaque dark grey
            player->SetSkinColor(darkGrey);
            player->v_sender.OnSetClothing(player->GetClothes(), player->GetSkinColor(), true, player->GetNetId());
            player->SendCharacterState(player);
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->v_sender.OnSetClothing(player->GetClothes(), player->GetSkinColor(), false, player->GetNetId());
                ply->SendCharacterState(player);
                player->v_sender.OnSetClothing(ply->GetClothes(), player->GetSkinColor(), false, player->GetNetId());
                player->SendCharacterState(player);
                });
        }
        player->SetWorld(world->GetName());
        player->SetNetId(world->AddPlayer(player));
        player->SetPosition(pos.m_x * 32, pos.m_y * 32);
        player->set_respawn_pos({ pos.m_x, pos.m_y });
        player->m_inventory.Send();

        auto data{ world->Pack() };
        GameUpdatePacket* update_packet{ static_cast<GameUpdatePacket*>(std::malloc(sizeof(GameUpdatePacket) + data.size())) };
        update_packet->m_type = NET_GAME_PACKET_SEND_MAP_DATA;
        update_packet->m_net_id = -1;
        update_packet->m_flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
        update_packet->m_data_size = static_cast<uint32_t>(data.size());
        std::memcpy(&update_packet->m_data, data.data(), data.size());
        player->SendPacket(NET_MESSAGE_GAME_PACKET, update_packet, sizeof(GameUpdatePacket) + data.size());
        
        data.clear();
        std::free(update_packet);

        std::vector<std::string> active_bits;
        if (world->IsFlagOn(WORLDFLAG_PUNCH_JAMMER))
            active_bits.push_back("`2NOPUNCH");
        if (world->IsFlagOn(WORLDFLAG_ZOMBIE_JAMMER))
            active_bits.push_back("`2IMMUNE");
        if (world->IsFlagOn(WORLDFLAG_ANTI_GRAIVTY))
            active_bits.push_back("`2ANTIGRAVITY");
        if (world->IsFlagOn(WORLDFLAG_JAMMED))
            active_bits.push_back("`4JAMMED");
        if (world->IsFlagOn(WORLDFLAG_BALLOON_JAMMED))
            active_bits.push_back("`2NOWAR");
        if (world->IsFlagOn(WORLDFLAG_NUKED))
            active_bits.push_back("`4NUKED");
 
        std::string jammer_list = fmt::format(" `0[{}`0]", fmt::join(active_bits, "`0, "));
        if (jammer_list == " `0[`0]") jammer_list = "";

        PlayerTable* db{ (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE) };
        player->PlaySfx("door_open", 0);
        if (player->CharacterState::IsFlagOn(STATEFLAG_NOCLIP)) {
            player->SetSkinColor(-0x7B); // change skin to 0x7B 
            player->SetPunchRange(500);
            player->SetBuildRange(500);
        }
        player->SendLog("`oWorld `w{}``{}`` `oentered. There are `w{}`o other people here, `w{}`o online.``", world->GetName(), jammer_list, world->GetPlayers(false).size() - 1, pool->GetActivePlayers());
        if (world->IsOwned()) {
            Tile* main_lock = world->GetTile(world->GetMainLock());
            if (!main_lock)
                return;
            bool has_access = true;
            if (!world->IsOwner(player) && !main_lock->HasAccess(player->GetUserId()))
                has_access = false;
            player->SendLog("`5[`w{}`` `$World Locked by {}{}`5]", world->GetName(), db->GetName(world->GetOwnerId()), (has_access ? " `$(`2ACCESS GRANTED`$)" : ""));
        }
        if (player->GetRole() >= PLAYER_ROLE_MODERATOR) {
            player->SetPunchRange(500);
            player->SetBuildRange(500);
        }
        world->SendWho(player, false);
    }
    void WorldPool::OnPlayerSyncing(std::shared_ptr<World> world, std::shared_ptr<Player> player) {
        player->v_sender.OnSpawn(player->GetSpawnData(true));
        player->v_sender.OnSetClothing(player->GetClothes(), player->GetSkinColor(), false, player->GetNetId());
        player->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
        player->SendCharacterState(player);

        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
            if (ply->GetUserId() == player->GetUserId())
                return;
            player->v_sender.OnSpawn(ply->GetSpawnData());
            player->v_sender.OnSetClothing(ply->GetClothes(), ply->GetSkinColor(), false, ply->GetNetId());
            player->v_sender.OnNameChanged(ply->GetNetId(), ply->GetDisplayName(world));
            player->v_sender.SetRespawnPos(ply->GetNetId(), ply->get_respawn_pos().m_x + ply->get_respawn_pos().m_y * world->GetSize().m_x);
            player->SendCharacterState(ply);

            ply->v_sender.OnSpawn(player->GetSpawnData());  
            ply->v_sender.OnSetClothing(player->GetClothes(), player->GetSkinColor(), false, player->GetNetId());
            ply->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
            ply->v_sender.SetRespawnPos(player->GetNetId(), player->get_respawn_pos().m_x + player->get_respawn_pos().m_y * world->GetSize().m_x);
            ply->SendCharacterState(player);

            if (!player->IsFlagOn(PLAYERFLAG_IS_INVISIBLE)) {
                ply->PlaySfx("door_open", 0);
                ply->SendLog("`5<`w{} `5entered, `w{}`` `5others here>```w", player->GetDisplayName(world), world->GetPlayers(false).size() - 1);
                ply->v_sender.OnTalkBubble(player->GetNetId(), fmt::format("`5<`w{} `5entered, `w{}`` `5others here>```w", player->GetDisplayName(world), world->GetPlayers(false).size() - 1));
            }
        });
    }
    void WorldPool::OnPlayerLeave(std::shared_ptr<World> world, std::shared_ptr<Player> player, const bool& send_offers) {
        TextScanner parser{};
        PlayerTable* database = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
        const auto& result{ database->Save(player)};
        parser.add<uint32_t>("netID", player->GetNetId());
        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
            ply->v_sender.OnRemove(parser);
            if (ply->IsFlagOn(PLAYERFLAG_IS_INVISIBLE))
                return;
            ply->v_sender.OnTalkBubble(player->GetNetId(), fmt::format(
                "`5<{} `5left, `w{}`` `5others here>``", player->GetDisplayName(world), world->GetPlayers(false).size() - 1
            ));
            ply->v_sender.OnConsoleMessage(fmt::format(
                "`5<{} `5left, `w{}`` `5others here>``", player->GetDisplayName(world), world->GetPlayers(false).size() - 1
            ));
            ply->v_sender.OnPlayPositioned("door_shut", player->GetNetId());
        });
        player->SetWorld("EXIT");
        player->set_access_offer({ -1, -1 });
        
        if (player->HasPlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT)) {
            player->RemovePlaymod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT);
            for (auto& tile : world->GetTiles()) {
                for (auto& current_star : world->GetPlayers(true)) {
                    if (!(current_star->GetNetId() == tile.GetOwnerId() && current_star->GetNetId() == player->GetNetId()))
                        continue;
                    tile.m_owner_id = 0;
                    break;
                }
            }
        }

        world->RemovePlayer(player);
        if (world->GetPlayers(false).size() < 1) {
            WorldTable* db{ (WorldTable*)Database::GetTable(Database::DATABASE_WORLD_TABLE) };
            if (!db->save(world))
                fmt::print("WorldTable::save, Failed to save {}\n", world->GetName());
        }
        if (send_offers)
            this->SendDefaultOffers(player);
    }
}