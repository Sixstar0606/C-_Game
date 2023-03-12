#include <world/world.h>
#include <cstdlib>
#include <vector>
#include <algorithm/algorithm.h>
#include <database/item/item_component.h>
#include <database/item/item_database.h>
#include <utils/binary_writer.h>
#include <utils/random.h>

namespace GTServer {
    World::World(const std::string& name, const uint32_t& width, const uint32_t& height) :
        m_flags{ 0 },
        m_name{ name },
        m_width(width),
        m_height(height),
        m_net_id{ 0 } {

    }
    World::~World() {
        m_tiles.clear();
    }

    uint32_t World::AddPlayer(const std::shared_ptr<Player>& player) {
        m_players.insert_or_assign(++m_net_id, player);
        return m_net_id;
    }
    void World::RemovePlayer(const std::shared_ptr<Player>& player) {
        auto it = std::find_if(m_players.begin(), m_players.end(),
            [&](const auto& p) { return p.second->GetUserId() == player->GetUserId(); });
        if (it != m_players.end())
            m_players.erase(it);
    }
    uint32_t World::DevPunchAdd(const std::shared_ptr<Player>& player) {
        m_DevBreak.insert_or_assign(++m_net_id, player);
        return m_net_id;
    }
    void World::DevPunchRemove(const std::shared_ptr<Player>& player) {
        auto it = std::find_if(m_DevBreak.begin(), m_DevBreak.end(),
            [&](const auto& p) { return p.second->GetUserId() == player->GetUserId(); });
        if (it != m_DevBreak.end())
            m_DevBreak.erase(it);
    }
    bool World::HasDevPunch(const std::shared_ptr<Player>& player) {
        for (const auto& [net_id, ply] : m_DevBreak) {
            if (ply->GetUserId() != player->GetUserId())
                continue;
            return true;
        }
        return false;
    }
    bool World::HasPlayer(const std::shared_ptr<Player>& player) {
        for (const auto& [net_id, ply] : m_players) {
            if (ply->GetUserId() != player->GetUserId())
                continue;
            return true;
        }
        return false;
    }
    bool World::BanPlayer(const int32_t& player_id) {
        // Add the player to the banned players map with the current time plus 10 minutes as the expiration time
        m_banned_players[player_id] = std::chrono::system_clock::now() + std::chrono::minutes(10);
        return true;
    }
    bool World::HasBan(const std::shared_ptr<Player>& player) {
        // Check if the player is in the banned players map
        if (m_banned_players.count(player->GetUserId()) > 0) {
            // Check if the ban has expired
            if (m_banned_players[player->GetUserId()] > std::chrono::system_clock::now()) {
                // The ban has not expired, so return false to indicate that the player cannot join the world
                return true;
            }
            else {
                // The ban has expired, so remove the player from the banned players map
                m_banned_players.erase(player->GetUserId());
            }
        }
        return false;
    }
    bool World::ClearBans() {
        m_banned_players.clear();
        return true;
    }
    std::vector<std::shared_ptr<Player>> World::GetPlayers(const bool& invis) {
        std::vector<std::shared_ptr<Player>> ret{};
        for (const auto& [net_id, player] : m_players) {
            if (!invis && player->IsFlagOn(PLAYERFLAG_IS_INVISIBLE))
                continue;
            ret.push_back(player);
        }
        return ret;
    }
    void World::Broadcast(const std::function<void(const std::shared_ptr<Player>&)>& func) {
        for (const auto& [net_id, player] : m_players)
            func(player);
    }

    bool World::IsFlagOn(const eWorldFlags& flag) const {
        if (m_flags & flag)
            return true;
        return false;
    }
    void World::SetFlag(const eWorldFlags& flag) {
        m_flags |= flag;
    }
    void World::RemoveFlag(const eWorldFlags& flag) {
        m_flags &= ~flag;
    }
    void World::SpawnEvent(const std::string& eventname) {
        std::random_device rd;
        std::mt19937 rng(rd());
        std::vector<Tile*> tiles;
        for (auto& t : this->GetTiles()) {
            if (t.GetBaseItem()->m_id == ITEM_BLANK) {
                tiles.push_back(&t);
            }
        }

        if (tiles.empty()) {
            // No tiles found
            return;
        }

        std::uniform_int_distribution<int> dist(0, tiles.size() - 1);
        int index = dist(rng);

        if (eventname == "beautiful_crystal") {
            this->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->v_sender.OnAddNotification(fmt::format("`4{}: `oYou have `w30 `oseconds to find and grab the `#Crystal Block Seed`o.", eventname), "interface/large/special_event.rttex", "audio/cumbia_horns.wav");
                });
            WorldObject obj{
                .m_item_id = static_cast<uint16_t>(ITEM_CRYSTAL_BLOCK_SEED),
                .m_item_amount = 1
            };
            obj.m_pos = CL_Vec2f{ static_cast<float>((tiles[index]->GetPosition().m_x * 32) + 8), static_cast<float>((tiles[index]->GetPosition().m_y * 32) + 8) };
            this->AddObject(obj, false, false);
            std::this_thread::sleep_for(std::chrono::seconds(30));
            this->RemoveObject(obj.m_item_id);
            return;
        }
    }

    void World::Generate(const eWorldType& type) {
        static randutils::pcg_rng gen{ utils::random::get_generator_local() };
        const auto& world_size = this->GetSize();

        switch (type) {
        case WORLD_TYPE_NORMAL: {
            int main_door_x = (std::rand() % 97) + 1;
            int height = 2500;

            this->SetWeatherId(WORLD_WEATHER_SUNNY);
            this->SetBaseWeatherId(WORLD_WEATHER_SUNNY);

            m_tiles.reserve(world_size.m_x * world_size.m_y);
            for (int i = 0; i < world_size.m_x * world_size.m_y; i++) {
                Tile& tile = m_tiles.emplace_back(Tile{});
                CL_Vec2i pos{ i % world_size.m_x, i / world_size.m_x };
                tile.SetPosition(pos);

                if (i == (height - 100) + main_door_x) {
                    tile.SetForeground(ITEM_MAIN_DOOR);
                    tile.set_door_data("EXIT", false);
                } else if (i == height + main_door_x) {
                    tile.SetForeground(ITEM_BEDROCK);
                } else if (i >= (height + 100) && i < 5400 && !(std::rand() % 25)) {
                    tile.SetForeground(ITEM_ROCK);
                } else if (i >= height && i < 5400) {
                    if (i > 5000 && !(std::rand() % 2)) {
                        tile.SetForeground(ITEM_LAVA);
                    } else {
                        tile.SetForeground(ITEM_DIRT);
                    }
                } else if (i >= 5400) {
                    tile.SetForeground(ITEM_BEDROCK);
                }

                if (i >= height) {
                    tile.SetBackground(ITEM_CAVE_BACKGROUND);
                }
            }
        } break;
        case WORLD_TYPE_BEACH: {
            this->SetWeatherId(WORLD_WEATHER_SUNSET);
            this->SetBaseWeatherId(WORLD_WEATHER_SUNSET);

            m_tiles.reserve(world_size.m_x * world_size.m_y);
            for (int i = 0; i < world_size.m_x * world_size.m_y; ++i) {
                CL_Vec2i pos{ i % world_size.m_x, i / world_size.m_y };

                Tile tile{ ITEM_BLANK, ITEM_BLANK, uint16_t{ 0 }, uint16_t{ 0 } };
                tile.SetPosition(pos);
                m_tiles.push_back(std::move(tile));
            }

            const auto offset = new int[m_width];
            offset[0] = 6;

            CL_Vec2i height = CL_Vec2i{ 19, 45 };
            int initial_height = height.m_y;
            int start_offset_x = 20;

            for (auto i = 1; i < world_size.m_x; i++) {
                offset[i] = 0x6;

                if ((gen.uniform(0, 30) - 10) > 13 && i >= start_offset_x)
                    offset[i] = offset[i - 1] + 1;
                else
                    offset[i] = offset[i - 1]; 
            }

            int main_door_x = (gen.uniform(0, start_offset_x - 2)) + 1;
            int umbrella_x = (gen.uniform(0, start_offset_x - 2)) + 1;
            bool door_done = false;
            bool umbrella_done = false;

            if (main_door_x == umbrella_x)
                umbrella_x = (gen.uniform(0, start_offset_x - 2)) + 1;

            for (auto horizontal = 0; horizontal < world_size.m_x; horizontal++) {
                for (auto vertical = initial_height; vertical < world_size.m_y; vertical++) {
                    int horizontal_pos = horizontal;
                    int vertical_pos = vertical + 6;
                    int index = horizontal_pos + vertical_pos * world_size.m_x;

                    if (index > world_size.m_x * world_size.m_y || index < 0)
                        continue;
                    if (horizontal_pos >= world_size.m_x || vertical_pos >= world_size.m_y)
                        continue;

                    m_tiles[index].SetFlag(TILEFLAG_WATER);
                    m_tiles[index].SetBackground(ITEM_OCEAN_ROCK); 
                }
            }
            for (auto horizontal = 0; horizontal < world_size.m_x; horizontal++) {
                for (auto vertical = initial_height; vertical < world_size.m_y; vertical++) {
                    int horizontal_pos = horizontal;
                    int vertical_pos = vertical + offset[horizontal];
                    int index = horizontal_pos + vertical_pos * world_size.m_x;

                    if (index > world_size.m_x * world_size.m_y || index < 0)
                        continue;
                    if (horizontal_pos >= world_size.m_x || vertical_pos >= world_size.m_y)
                        continue;

                    if (main_door_x == horizontal_pos && !door_done) {
                        door_done = true;
                        m_tiles[index - world_size.m_x].SetForeground(ITEM_MAIN_DOOR);
                        m_tiles[index - world_size.m_x].set_door_data("Beach `2TEST``", false);
                        m_tiles[index].SetForeground(ITEM_BEDROCK);
                        m_tiles[index].RemoveFlag(TILEFLAG_WATER);
                        continue;
                    }

                    if (m_tiles[index].GetForeground() == ITEM_BEDROCK || m_tiles[index].GetForeground() == ITEM_MAIN_DOOR)
                        continue;

                    if (vertical_pos == initial_height + offset[horizontal]) {
                        if (horizontal_pos > start_offset_x + 4) {
                            if (!(rand() % 10))
                                m_tiles[index - world_size.m_x].SetForeground(ITEM_SEAWEED);
                        }
                        else if (!(rand() % 9))
                            m_tiles[index - world_size.m_x].SetForeground(ITEM_PALM_TREE);
                    } 
                    
                    if (umbrella_x == horizontal_pos && !umbrella_done) {
                        umbrella_done = true;
                        m_tiles[index - world_size.m_x].SetForeground(ITEM_BEACH_UMBRELLA);
                    }

                    m_tiles[index].SetForeground(ITEM_SAND);
                    m_tiles[index].RemoveFlag(TILEFLAG_WATER);
        
                    if (vertical > initial_height) {
                        if (!(rand() % 280))
                            m_tiles[index].SetForeground(ITEM_ROCK); 

                        if (!(rand() % 590))
                            m_tiles[index].SetForeground(ITEM_TREASURE_CHEST);  
                    }
                } 
            }
        
            for (auto index = 0; index < world_size.m_x * world_size.m_y; index++) {
                if (index >= 5400)
                    m_tiles[index].SetForeground(ITEM_BEDROCK);
            }

            delete[] offset;
        } break;
        default:
            break;
        }
    }

    Tile* World::GetTile(uint16_t x, uint16_t y) {
        if (x < 0 || y < 0 || x > m_width || y > m_height || x > 99 and y > 58) {
        std::cout << "get_tile return nullptr" << std::endl;
            return nullptr;
        }
        return &m_tiles[x + y * m_width];
    }
    CL_Vec2i World::GetTilePos(const uint16_t& id) const {
        for (int i = 0; i < m_tiles.size(); i++) {
            if (m_tiles[i].GetForeground() != id)
                continue;
            return { i % m_width, i / m_width };
        }
        return { 0, 0 };
    }
    CL_Vec2i World::GetTilePos(const eItemTypes& type) const {
        for (int i = 0; i < m_tiles.size(); i++) {
            const auto& item = ItemDatabase::GetItem(m_tiles[i].GetForeground());
            if (item->m_item_type != type)
                continue;
            return { i % m_width, i / m_width };
        }
        return { 0, 0 };
    }

    bool World::IsObstacle(std::shared_ptr<Player> player, CL_Vec2i position) {
        if (player->CharacterState::IsFlagOn(STATEFLAG_NOCLIP))
            return false;
        if (position.m_x >= this->GetSize().m_x || position.m_y >= this->GetSize().m_y) return false;
        if (position.m_x < 0 || position.m_y < 0) return false;

        Tile* tile = this->GetTile(position.m_x, position.m_y);
        if (!tile) return true;
        ItemInfo* base = tile->GetBaseItem();

        switch(base->m_collision_type) {
        case ITEMCOLLISION_NONE:
            return false;
        case ITEMCOLLISION_NORMAL: {
            if (base->m_id == ITEM_BLANK)
                return false;
        } return true;
        case ITEMCOLLISION_GATEWAY: {
            if (player->GetRole() >= PLAYER_ROLE_DEVELOPER) {
                return false;
            }
            if (tile->IsFlagOn(TILEFLAG_LOCKED)) {
                Tile* parent = this->GetParentTile(tile);
                if (!parent)
                    return true;
                auto access_list = parent->GetAccessList();
                if (std::find(access_list.begin(), access_list.end(), player->GetUserId()) != access_list.end()
                    || parent->IsFlagOn(TILEFLAG_PUBLIC) || parent->GetOwnerId() == player->GetUserId()
                    || tile->IsFlagOn(TILEFLAG_PUBLIC))
                    return false;
            }
            else
            {
                if (!this->IsOwned() || this->IsOwner(player) || player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                    return false;
                Tile* main_lock = this->GetTile(this->GetMainLock());
                if (!main_lock)
                    return true;
                auto access_list = main_lock->GetAccessList();
                if (std::find(access_list.begin(), access_list.end(), player->GetUserId()) != access_list.end()
                    || main_lock->IsFlagOn(TILEFLAG_PUBLIC) || tile->IsFlagOn(TILEFLAG_PUBLIC) || player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                    return false;  
            }
        } return true;
        case ITEMCOLLISION_GUILDENTRANCE: {
            if (player->GetRole() >= PLAYER_ROLE_DEVELOPER) {
                return false;
            }
            if (tile->IsFlagOn(TILEFLAG_LOCKED)) {
                Tile* parent = this->GetParentTile(tile);
                if (!parent)
                    return true;
                auto access_list = parent->GetAccessList();
                if (std::find(access_list.begin(), access_list.end(), player->GetUserId()) != access_list.end()
                    || parent->IsFlagOn(TILEFLAG_PUBLIC) || parent->GetOwnerId() == player->GetUserId()
                    || tile->IsFlagOn(TILEFLAG_PUBLIC))
                    return false;
            }
            else
            {
                if (!this->IsOwned() || this->IsOwner(player) || player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                    return false;
                Tile* main_lock = this->GetTile(this->GetMainLock());
                if (!main_lock)
                    return true;
                auto access_list = main_lock->GetAccessList();
                if (std::find(access_list.begin(), access_list.end(), player->GetUserId()) != access_list.end()
                    || main_lock->IsFlagOn(TILEFLAG_PUBLIC) || tile->IsFlagOn(TILEFLAG_PUBLIC) || player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                    return false;
            }
        } return true;
        case ITEMCOLLISION_SWITCHEROO: {
            if (player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                return false;
            if (tile->IsFlagOn(TILEFLAG_OPEN))
                return false;
        } return true;
        case ITEMCOLLISION_VIP_GATEWAY: {
            if (player->GetRole() > PLAYER_ROLE_ADMINISTRATOR) {
                return false;
            }
            auto access_list = tile->GetAccessList();
            if (std::find(access_list.begin(), access_list.end(), player->GetUserId()) != access_list.end()
                || tile->IsFlagOn(TILEFLAG_PUBLIC) || tile->GetOwnerId() == player->GetUserId() || player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                return false;
        } return true;
        case ITEMCOLLISION_ADVENTURE_DOOR: {
            if (player->GetRole() > PLAYER_ROLE_ADMINISTRATOR) {
                return false;
            }
            auto access_list = tile->GetAccessList();
            if (std::find(access_list.begin(), access_list.end(), player->GetUserId()) != access_list.end()
                || tile->IsFlagOn(TILEFLAG_PUBLIC) || tile->GetOwnerId() == player->GetUserId() || player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                return false;
        } return true;
        case ITEMCOLLISION_CLOUD: {
            if (player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                return false;
        } return true;
        case ITEMCOLLISION_GORILLA: {
            if (player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                return false;
        } return true;
        case ITEMCOLLISION_ONE_WAY_LEFT_RIGHT: {
            if (player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                return false;
        } return true;
        case ITEMCOLLISION_WATERFALL: {
            if (player->GetRole() >= PLAYER_ROLE_DEVELOPER)
                return false;
        } return true;
        default:
            return false;
        }
        return true;
    }

    std::size_t World::GetMemoryUsage() {
        std::size_t size{ sizeof(uint16_t) }; // version
        size += sizeof(uint32_t); // flags
        size += sizeof(uint16_t); // name length
        size += m_name.length(); // name
        size += sizeof(uint32_t); // width
        size += sizeof(uint32_t); // height
        size += this->GetTilesMemoryUsage(false); // tiles
        size += this->GetObjectsMemoryUsage(); // objects
        size += sizeof(uint32_t); // weather
        size += sizeof(uint32_t); // base weather
        return size;
    }
    std::size_t World::GetTilesMemoryUsage(const bool& to_database) {
        std::size_t size{};
        size += sizeof(uint32_t); // tiles count
        for (auto& tile : m_tiles)
            size += tile.GetMemoryUsage(to_database);
        return size;
    }
    std::size_t World::GetObjectsMemoryUsage() {
        std::size_t size{
            sizeof(uint32_t) +  // object count
            sizeof(uint32_t)    // object last id
        };
        size += (m_objects.size() * (sizeof(uint32_t) + sizeof(WorldObject)));
        return size;
    }
    std::vector<uint8_t> World::Pack() {
        const auto& alloc = this->GetMemoryUsage();
        std::vector<uint8_t> ret{};
        ret.resize(alloc);

        BinaryWriter buffer{ ret.data() };
        buffer.write<uint16_t>(m_version);
        buffer.write<uint32_t>(m_flags);
        buffer.write(m_name, sizeof(uint16_t));
        buffer.write<uint32_t>(m_width);
        buffer.write<uint32_t>(m_height);
 
        auto tiles{ this->PackTiles(false) };
        buffer.write(tiles.data(), tiles.size());
        auto objects{ this->PackObjects(false) };
        buffer.write(objects.data(), objects.size());

        buffer.write<uint32_t>(this->GetBaseWeatherId());
        buffer.write<uint32_t>(this->GetWeatherId());
        return ret;
    }
    std::vector<uint8_t> World::PackTiles(const bool& to_database) {
        std::vector<uint8_t> ret{};
        ret.resize(this->GetTilesMemoryUsage(to_database));

        BinaryWriter buffer{ ret.data() };
        buffer.write<uint32_t>(static_cast<uint32_t>(m_tiles.size()));
        for (auto& tile : m_tiles)
            tile.Pack(buffer, to_database);

        return ret;
    }
    std::vector<uint8_t> World::PackObjects(const bool& to_database) {
        std::vector<uint8_t> ret{};
        ret.resize(this->GetObjectsMemoryUsage());

        BinaryWriter buffer{ ret.data() };
        buffer.write<uint32_t>(static_cast<uint32_t>(m_objects.size()));
        buffer.write<uint32_t>(m_object_id - (to_database ? 0 : 1));
        for (auto& [id, object] : m_objects) {
            buffer.write<uint16_t>(object.m_item_id);
            buffer.write<CL_Vec2f>(object.m_pos);
            buffer.write<uint8_t>(object.m_item_amount);
            buffer.write<uint8_t>(object.m_flags);
            buffer.write<uint32_t>(id);
        }

        return ret;
    }
    
    void World::SyncPlayerData(std::shared_ptr<Player> player) {
        this->Broadcast([&](const std::shared_ptr<Player>& ply) { 
            player->SendCharacterState(ply);
            player->v_sender.OnSetClothing(ply->GetClothes(), ply->GetSkinColor(), false, ply->GetNetId());
      
            ply->SendCharacterState(player);
            ply->v_sender.OnSetClothing(player->GetClothes(), player->GetSkinColor(), false, player->GetNetId());
        });
    }
    void World::SendTileUpdate(Tile* tile, const int32_t& delay) {
        std::size_t alloc = tile->GetMemoryUsage(false);
        GameUpdatePacket* update_packet = (GameUpdatePacket*)std::malloc(sizeof(GameUpdatePacket) + alloc);
        if (!update_packet)
            return;
        std::memset(update_packet, 0, sizeof(GameUpdatePacket) + alloc);

        update_packet->m_type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA;
        update_packet->m_int_x = tile->GetPosition().m_x;
        update_packet->m_int_y = tile->GetPosition().m_y;
        update_packet->m_net_id = -1;
        update_packet->m_delay = delay;
        update_packet->m_flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
        update_packet->m_data_size = alloc;

        BinaryWriter buffer{ alloc };
        tile->Pack(buffer, false);
        std::memcpy(&update_packet->m_data, buffer.get(), buffer.get_pos());

        this->Broadcast([&](const std::shared_ptr<Player>& player) { player->SendPacket(NET_MESSAGE_GAME_PACKET, update_packet, sizeof(GameUpdatePacket) + alloc); });
        std::free(update_packet);
    }
    void World::SendTileUpdate(std::vector<Tile*> tiles) {
        std::size_t alloc{ sizeof(int32_t) };
        for (auto& tile : tiles)
            alloc += sizeof(uint64_t) + tile->GetMemoryUsage(false);
        GameUpdatePacket* update_packet = (GameUpdatePacket*)std::malloc(sizeof(GameUpdatePacket) + alloc);
        if (!update_packet)
            return;
        std::memset(update_packet, 0, sizeof(GameUpdatePacket) + alloc);

        update_packet->m_type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE;
        update_packet->m_flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
        update_packet->m_int_x = -1, update_packet->m_int_y = -1;
        
        BinaryWriter buffer{ alloc };
        for (auto& tile : tiles) {
            buffer.write<int>(tile->GetPosition().m_x);
            buffer.write<int>(tile->GetPosition().m_y);
            tile->Pack(buffer, false);
        }
        buffer.write<int32_t>(-1);
        std::memcpy(&update_packet->m_data, buffer.get(), buffer.get_pos());

        this->Broadcast([&](const std::shared_ptr<Player>& player) { player->SendPacket(NET_MESSAGE_GAME_PACKET, update_packet, sizeof(GameUpdatePacket) + alloc); });
        std::free(update_packet);
    }

    void World::SendWho(std::shared_ptr<Player> player, bool show_self) {
        this->Broadcast([&](const std::shared_ptr<Player>& ply) {
            if (ply->GetUserId() == player->GetUserId() && !show_self)
                return;
            if (!ply->HasPlaymod(PLAYMOD_TYPE_INVISIBILITY) && !ply->IsFlagOn(PLAYERFLAG_IS_INVISIBLE))
                player->v_sender.OnTalkBubble(ply->GetNetId(), fmt::format("`w{}``", ply->GetDisplayName(nullptr)), true);
        });
    }
    void World::SendPull(std::shared_ptr<Player> player, std::shared_ptr<Player> target) {
        this->Broadcast([&](const std::shared_ptr<Player>& ply) {
            ply->PlaySfx("object_spawn", 0);
            });
        if (target->GetRole() >= PLAYER_ROLE_MODERATOR) {
            player->v_sender.OnTextOverlay("`wYou've been summoned by a mod!``");
        }
        else {
            player->v_sender.OnTextOverlay(fmt::format("`wYou've been pulled by {}``", target->GetDisplayName(nullptr)));
        }
        player->SetPosition(target->GetPosition().m_x, target->GetPosition().m_y);
            this->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->v_sender.OnSetFreezeState(player->GetNetId(), 2, 0);
            player->v_sender.OnSetPos(player->GetNetId(), { (float)(target->GetPosition().m_x), (float)(target->GetPosition().m_y ) }, 1);
            });
            player->SetPosition(target->GetPosition().m_x, target->GetPosition().m_y);
            this->Broadcast([&](const std::shared_ptr<Player>& ply) {
                player->v_sender.OnSetPos(player->GetNetId(), { (float)(target->GetPosition().m_x), (float)(target->GetPosition().m_y) }, 1);
                ply->v_sender.OnSetFreezeState(player->GetNetId(), 0, 0);
                player->v_sender.OnSetPos(player->GetNetId(), { (float)(target->GetPosition().m_x), (float)(target->GetPosition().m_y) }, 1);
                });
            player->SetPosition(target->GetPosition().m_x, target->GetPosition().m_y);
    }
    void World::SendKick(std::shared_ptr<Player> player, bool killed, int delay) {
        player->SetPosition(player->get_respawn_pos().m_x * 32, player->get_respawn_pos().m_y * 32);
        if (!killed) {
            this->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->v_sender.OnKilled(player->GetNetId(), 0);
                ply->v_sender.OnSetFreezeState(player->GetNetId(), 2, delay);
            });
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

            this->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
            ply->v_sender.OnParticleEffect(3, CL_Vec2f{ update_packet.m_pos_x + 15, update_packet.m_pos_y + 15 }, effect_delay - 30);
                });
            effect_delay += 300;
        }
        delay = delay == -1 ? 2000 : delay;
        this->Broadcast([&](const std::shared_ptr<Player>& ply) {
            ply->v_sender.OnSetPos(player->GetNetId(), { (float)(player->get_respawn_pos().m_x * 32), (float)(player->get_respawn_pos().m_y * 32) }, delay);
            ply->v_sender.OnSetFreezeState(player->GetNetId(), 0, delay);
            ply->v_sender.OnPlayPositioned("teleport", player->GetNetId(), delay);
        });
    }

    bool World::EditTile(std::string action, CL_Vec2i center, float radius, ItemInfo* item, bool ignore_areas) {
        auto inside_circle = [&](CL_Vec2i center, CL_Vec2i tile, float radius) -> bool {
            float dx = center.m_x - tile.m_x,
                dy = center.m_y - tile.m_y;
            float distance = sqrt(dx * dx + dy * dy);
            return distance <= radius;
        };
        
        switch (utils::quick_hash(action)) {
        case "add"_qh: {
            std::vector<Tile*> valid_tiles{};
            for (auto& tile : this->GetTiles()) {
                if (!inside_circle(center, tile.GetPosition(), radius))
                    continue;
                if (tile.GetPosition() == center)
                    continue;
                else if ((this->IsTileOwned(&tile) || tile.GetBaseItem()->m_item_type == ITEMTYPE_LOCK))
                    continue;

                if ((ignore_areas && tile.GetBaseItem()->m_id != ITEM_BLANK) || (!ignore_areas && tile.GetBaseItem()->m_id != ITEM_BLANK)) {
                    if (item->IsBackground()) {
                        tile.SetBackground(item->m_id);
                    }
                    else {
                        if (tile.IsFlagOn(TILEFLAG_TILEEXTRA))
                            continue;
                        tile.SetForeground(item->m_id);
                    }
                    valid_tiles.emplace_back(this->GetTile(tile.GetPosition()));
                }

            }
            this->SendTileUpdate(valid_tiles);
        } return true;
        case "erase"_qh: {
            std::vector<Tile*> valid_tiles{};
            for (auto& tile : this->GetTiles()) {
                if (!inside_circle(center, tile.GetPosition(), radius))
                    continue;
                if (tile.GetBaseItem()->m_id != item->m_id)
                    continue;
                else if (!ignore_areas && (this->IsTileOwned(&tile) || tile.GetBaseItem()->m_item_type == ITEMTYPE_LOCK))
                    continue;
                else if (tile.IsFlagOn(TILEFLAG_TILEEXTRA))
                    continue;

                tile.RemoveBase();
                valid_tiles.emplace_back(this->GetTile(tile.GetPosition()));
            }
            this->SendTileUpdate(valid_tiles);
        } return true;
        case "addobject"_qh: {
            for (auto& tile : this->GetTiles()) {
                if (!inside_circle(center, tile.GetPosition(), radius))
                    continue;
                if (tile.GetPosition() == center)
                    continue;
                else if (!ignore_areas && (this->IsTileOwned(&tile) || tile.GetBaseItem()->m_item_type == ITEMTYPE_LOCK))
                    continue;
                WorldObject obj{
                    .m_item_id = static_cast<uint16_t>(item->m_id),
                    .m_item_amount = 1
                };
                obj.m_pos = CL_Vec2f{ static_cast<float>((tile.GetPosition().m_x * 32) + 8), static_cast<float>((tile.GetPosition().m_y * 32) + 8) };
                this->AddObject(obj, false, false);
            }
        } return true;
        case "clearobject"_qh: {
            for (auto& tile : this->GetTiles()) {
                if (!inside_circle(center, tile.GetPosition(), radius))
                    continue;
                if (tile.GetPosition() == center)
                    continue;
                else if (!ignore_areas && (this->IsTileOwned(&tile) || tile.GetBaseItem()->m_item_type == ITEMTYPE_LOCK))
                    continue;
                auto objects = this->GetObjectsOnPos(CL_Vec2f{ static_cast<float>(tile.GetPosition().m_x), static_cast<float>(tile.GetPosition().m_y) });
                if (objects.empty())
                    continue;

                for (auto& [obj_id, obj] : objects) {
                    if (obj.m_item_id != item->m_id)
                        continue;
                    if (!this->m_objects.erase(obj_id))
                        continue;
                    GameUpdatePacket packet{ 
                        .m_type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT,
                        .m_object_change_type = OBJECT_CHANGE_TYPE_REMOVE,
                        .m_item_net_id = -1,
                        .m_object_id = obj_id
                    };
                    this->Broadcast([&](const std::shared_ptr<Player>& player) { player->SendPacket(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket)); });
                }
            }
        } return true;
        default:
            return false;
        }
        return false;
    }

    Tile* World::GetParentTile(Tile* neighbour) {
        if (neighbour->GetParent() < 0)
            return nullptr;
        CL_Vec2i parent = { 
            (int)(neighbour->GetParent() % this->m_width), (int)(neighbour->GetParent() / this->m_width)
        };
        return this->GetTile(parent.m_x, parent.m_y);
    }
    bool World::HasTileAccess(Tile* neighbour, const std::shared_ptr<Player>& player) {
        if (!neighbour->IsFlagOn(TILEFLAG_LOCKED))
            return false;
        Tile* parent = this->GetParentTile(neighbour);
        if (!parent)
            return false;
        auto access_list = parent->GetAccessList();
        return std::find(access_list.begin(), access_list.end(), player->GetUserId()) != access_list.end();
    }
    bool World::IsTileOwner(Tile* neighbour, const std::shared_ptr<Player>& player) {
        Tile* parent = this->GetParentTile(neighbour);
        if (!parent)
            return false;
        return parent->GetOwnerId() == player->GetUserId();
    }
    bool World::IsTileOwned(Tile* neighbour) {
        Tile* parent = this->GetParentTile(neighbour);
        if (!parent)
            return false;
        if (parent->GetBaseItem()->m_item_type == ITEMTYPE_LOCK && parent->GetOwnerId() > 0)
            return true;
        return false;
    }

    std::unordered_map<int32_t, WorldObject> World::GetObjectsOnPos(const CL_Vec2f& pos) {
        std::unordered_map<int32_t, WorldObject> objects;
        for (auto &[id, object] : m_objects)
        {
            if (object.m_pos.m_x < ((pos.m_x * 32) + 20) && object.m_pos.m_x > ((pos.m_x * 32) - 12) && object.m_pos.m_y < ((pos.m_y * 32) + 20) && object.m_pos.m_y > ((pos.m_y * 32) - 12))
                objects.insert_or_assign(id, object);
        }   
        return objects;     
    }
    std::unordered_map<int32_t, WorldObject> World::GetLastObjectById(std::unordered_map<int32_t, WorldObject> &obj_list, uint32_t item_id) {
        std::unordered_map<int32_t, WorldObject> objects;
        int highest = -1;

        for (auto &[id, object] : obj_list) {
            if (id > highest && item_id == object.m_item_id)
                highest = id;
        }   
        if (auto it = obj_list.find(highest); it != obj_list.end())
            objects.insert_or_assign(it->first, it->second);

        return objects;
    }
    void World::AddObject(uint32_t item_id, uint8_t amount, CL_Vec2f position) {
        WorldObject object{
            .m_item_id = static_cast<uint16_t>(item_id),
            .m_item_amount = amount
        };
        object.m_pos = position;
        this->AddObject(object, true, true, true);
    }
    void World::AddObject(WorldObject& object, bool check, bool randomize_pos, bool magplant) {
        if (object.m_item_id < 1 || object.m_item_amount < 1 || object.m_item_amount > 200)
            return;
        if (magplant) {} // World::AddObjectToMagplant
    
        if (check && object.m_item_id != ITEM_FOSSIL) {
            std::unordered_map<int32_t, WorldObject> wrapped_gems;

            auto wrap_gems = [&] (uint8_t amount, uint8_t target, uint8_t target_amount) -> bool {
                if (object.m_item_amount != amount)
                    return false;
                std::unordered_map<int32_t, WorldObject> gem_objects;

                for (auto& [id, obj] : wrapped_gems) {
                    if (obj.m_item_amount != amount)
                        continue;
                    gem_objects.insert_or_assign(id, obj);
                }

                while (gem_objects.size() >= target) {
                    int success = 0;
                    for (auto it = gem_objects.cbegin(); it != gem_objects.cend();) {
                        success += 1;
                        if (success != target + 1) {
                            this->RemoveObject(it->first);
                            it = gem_objects.erase(it);
                        }
                        else
                            break;
                    }

                    WorldObject new_object{};
                    new_object.m_pos = object.m_pos;
                    new_object.m_item_id = ITEM_GEMS;
                    new_object.m_item_amount = target_amount;
                    this->AddObject(new_object, true, false);
                }
                return true;
            };

            for (auto& [obj_id, obj] : m_objects) {
                  if (obj.m_item_id == object.m_item_id &&
                   obj.m_pos.m_x < (object.m_pos.m_x + 20) && obj.m_pos.m_x > (object.m_pos.m_x - 12)
                && obj.m_pos.m_y < (object.m_pos.m_y + 20) && obj.m_pos.m_y > (object.m_pos.m_y - 12)) {
                    if (object.m_item_id == ITEM_GEMS) {
                        wrapped_gems.insert_or_assign(obj_id, obj);
                    }
                    else {
                        if (obj.m_item_amount + object.m_item_amount > 200) {
                            object.m_item_amount = static_cast<uint8_t>(obj.m_item_amount + object.m_item_amount - 200);
                            obj.m_item_amount = 200;
                            this->ModifyObject({ obj_id, obj });
                            break;
                        } else {
                            obj.m_item_amount += object.m_item_amount;
                            this->ModifyObject({ obj_id, obj });
                            return;
                        }
                    }
                }
            }

            if (object.m_item_id == ITEM_GEMS) {
                if (wrap_gems(1, 5, 5))
					return;
				if (wrap_gems(5, 2, 10))
					return;
				if (wrap_gems(10, 5, 50))
					return;
				if (wrap_gems(50, 2, 100))
					return;
            }
        }

        float x = object.m_pos.m_x;
        float y = object.m_pos.m_y;

        if (randomize_pos) {
            auto rand_generator = utils::random::get_generator_static();
            x = (rand() % 2) == 0 ?
                object.m_pos.m_x + rand_generator.uniform(0, 6) :
                object.m_pos.m_x - rand_generator.uniform(0, 6);
            y = (rand() % 2) == 0 ?
                object.m_pos.m_y + rand_generator.uniform(0, 6) :
                object.m_pos.m_y - rand_generator.uniform(0, 6);
        }

        if (x < 0)
            x = 12;
        if (y < 0)
            y = 12;
        if (x >= this->GetSize().m_x * 32)
            x = (this->GetSize().m_x * 32) - 12;
        if (y >= this->GetSize().m_y * 32)
            y = (this->GetSize().m_y * 32) - 12;

        object.m_pos = { x, y };
        m_objects.insert_or_assign(m_object_id++, std::move(object));

        GameUpdatePacket packet{ NET_GAME_PACKET_ITEM_CHANGE_OBJECT };
        packet.m_object_change_type = OBJECT_CHANGE_TYPE_ADD;
        packet.m_pos_x = object.m_pos.m_x;
        packet.m_pos_y = object.m_pos.m_y;
        packet.m_item_id = object.m_item_id;
        packet.m_object_count = static_cast<float>(object.m_item_amount);

        this->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->SendPacket(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket));
        });
    }
    void World::AddGemsObject(int32_t gems, const CL_Vec2f& position) {
        if (gems < 1)
            return;
        WorldObject object{};
        object.m_item_id = ITEM_GEMS;
        object.m_pos = position;

        while (gems != 0) {
            object.m_item_amount = object.m_item_amount + 1;
            gems -= 1;
        }
        this->AddObject(object, true);
    }
    void World::ModifyObject(const std::pair<int32_t, WorldObject>& object) {
        GameUpdatePacket packet{ NET_GAME_PACKET_ITEM_CHANGE_OBJECT };
        packet.m_object_change_type = OBJECT_CHANGE_TYPE_MODIFY;
        packet.m_pos_x = object.second.m_pos.m_x;
        packet.m_pos_y = object.second.m_pos.m_y;
        packet.m_item_id = object.second.m_item_id;
        packet.m_item_net_id = object.first;
        packet.m_object_count = static_cast<float>(object.second.m_item_amount);

        this->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->SendPacket(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket));
        });
        m_objects[object.first] = object.second;
    }
    void World::CollectObject(std::shared_ptr<Player> player, const int32_t& obj_id, const CL_Vec2f& position) {
        auto it = this->m_objects.find(obj_id); 
        if (it == this->m_objects.end())
            return;
        WorldObject& object = it->second;

        CL_Vec2f distance = { std::abs(player->GetPosition().m_x - position.m_x), std::abs(player->GetPosition().m_y - position.m_y) };
        CL_Vec2i limit_distance = { object.m_item_id == ITEM_GEMS ? 128 : 96, object.m_item_id == ITEM_GEMS ? 96 : 64 };
        if (distance.m_x > limit_distance.m_x || distance.m_y > limit_distance.m_y) {
            player->v_sender.OnTalkBubble(player->GetNetId(), "`w(Too far away)``", true);
            return;
        }

        Tile* tile = this->GetTile(static_cast<int>(object.m_pos.m_x) / 32, static_cast<int>(object.m_pos.m_y) / 32);
        if (!tile)
            return;
        bool has_access = false,
            collected = false;
        if (this->GetOwnerId() == player->GetUserId() || this->GetOwnerId() < 1 || player->GetRole() >= PLAYER_ROLE_MODERATOR ||
            tile->IsFlagOn(TILEFLAG_LOCKED) && std::find(this->GetParentTile(tile)->GetAccessList().begin(), this->GetParentTile(tile)->GetAccessList().end(), player->GetUserId()) != this->GetParentTile(tile)->GetAccessList().end() || 
            std::find(tile->GetAccessList().begin(), tile->GetAccessList().end(), player->GetUserId()) != tile->GetAccessList().end()) {
            has_access = true;
        }
        ItemInfo* base = tile->GetBaseItem();
        if (!base)
            return;
        if (base->m_collision_type == ITEMCOLLISION_NORMAL)
            return;
        if ((base->m_id == ITEM_DISPLAY_BOX || base->m_id == ITEM_TRANSMATTER_FIELD) && !has_access)
            return;

        if (object.m_item_id == ITEM_GEMS) {
            if (player->GetGems() >= INT_MAX - 200)
                return;
            player->SetGems(player->GetGems() + object.m_item_amount);
            collected = true;
        } else {
            if (player->m_inventory.IsMaxed() && !player->m_inventory.Contain(object.m_item_id))
                return;
            ItemInfo* obj_item = ItemDatabase::GetItem(object.m_item_id);
            bool overload = false;
            
            if (player->m_inventory.Contain(object.m_item_id)) {
                uint8_t item_amount = player->m_inventory.GetItemCount(object.m_item_id);
                if (item_amount + object.m_item_amount > 200) {
                    overload = true;
                    uint8_t new_amount = static_cast<uint8_t>(200 - item_amount);
                    uint8_t new_object_amount = static_cast<uint8_t>(object.m_item_amount - new_amount);
                    if (new_amount > 200) new_amount = 200;
                    if (player->m_inventory.Add(object.m_item_id, new_amount)) {
                        collected = true;
                        WorldObject new_object;
                        new_object.m_pos = object.m_pos;
                        new_object.m_item_id = object.m_item_id;
                        new_object.m_item_amount = new_object_amount;
                        this->AddObject(new_object, false, false);

                        std::string msg = fmt::format("Collected `w{} {}``.", new_amount, obj_item->m_name);
                        if (obj_item->m_rarity != 999)
                            msg.append(fmt::format(" Rarity: `w{}``", obj_item->m_rarity));
                        player->v_sender.OnConsoleMessage(msg);
                    }
                }
            }

            if (!overload) {
                if (player->m_inventory.Add(object.m_item_id, object.m_item_amount)) {
                    collected = true;
                    std::string msg = fmt::format("Collected `w{} {}``.", object.m_item_amount, obj_item->m_name);
                    if (obj_item->m_rarity != 999)
                        msg.append(fmt::format(" Rarity: `w{}``", obj_item->m_rarity));
                    player->v_sender.OnConsoleMessage(msg);
                }
            }
        }
    
        if (!collected)
            return;
        m_objects.erase(it);

        GameUpdatePacket packet{ NET_GAME_PACKET_ITEM_CHANGE_OBJECT };
        packet.m_object_change_type = OBJECT_CHANGE_TYPE_COLLECT;
        packet.m_net_id = player->GetNetId();
        packet.m_object_id = obj_id;

        this->Broadcast([&](const std::shared_ptr<Player>& ply) {
            ply->SendPacket(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket));
        }); 
    }
    void World::RemoveObject(const int32_t& id) {
        if (!m_objects.erase(id))
            return;
        GameUpdatePacket packet{ NET_GAME_PACKET_ITEM_CHANGE_OBJECT };
        packet.m_object_change_type = OBJECT_CHANGE_TYPE_REMOVE;
        packet.m_item_net_id = -1;
        packet.m_object_id = id;

        this->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->SendPacket(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket));
        });
    }
}