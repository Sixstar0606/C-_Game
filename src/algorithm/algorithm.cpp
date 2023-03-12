#include <algorithm/algorithm.h>
#include <deque>
#include <list>
#include <random>
#include <utils/binary_writer.h>

namespace GTServer {
    bool Algorithm::IsLockNeighbour(std::shared_ptr<World> world, CL_Vec2i tile, CL_Vec2i lock, bool ignoreAir) {
        if (tile.m_x == lock.m_x && tile.m_y == lock.m_y)
            return false;
        auto* base = world->GetTile(tile)->GetBaseItem();
        if (!base)
            return false;
        if (base->m_item_type == ITEMTYPE_MAIN_DOOR
            || base->m_item_type == ITEMTYPE_BEDROCK
            || base->m_item_type == ITEMTYPE_LOCK)
            return false;
        if (ignoreAir && (world->GetTile(tile)->GetForeground() == ITEM_BLANK && world->GetTile(tile)->GetBackground() == ITEM_BLANK))
            return false;

        uint16_t lock_index = lock.m_x + lock.m_y * world->GetSize().m_x;
        if (world->GetTile(tile.m_x + 1, tile.m_y)->GetParent() == lock_index || (tile.m_x + 1 == lock.m_x && tile.m_y == lock.m_y))
            return true;
        if (world->GetTile(tile.m_x, tile.m_y + 1)->GetParent() == lock_index || (tile.m_x == lock.m_x && tile.m_y + 1 == lock.m_y))
            return true;
        if (world->GetTile(tile.m_x - 1, tile.m_y)->GetParent() == lock_index || (tile.m_x - 1 == lock.m_x && tile.m_y == lock.m_y))
            return true;
        if (world->GetTile(tile.m_x, tile.m_y - 1)->GetParent() == lock_index || (tile.m_x == lock.m_x && tile.m_y - 1 == lock.m_y))
            return true;
        return false;
    }

    bool Algorithm::IsSteamPowered(const uint16_t& foreground) {
        switch (foreground) {
        case ITEM_STEAM_VENT: return true;
        case ITEM_STEAM_DOOR: return true;
        case ITEM_STEAM_LAUNCHER: return true;
        case ITEM_STEAM_LAMP: return true;
        case ITEM_STEAM_SPIKES: return true;
        default: return false;
        }
    }
    void Algorithm::OnSteamActive(std::shared_ptr<World> world, Tile* tile, int x, int y, int delay) {
        switch (tile->GetForeground()) {
        case ITEM_STEAM_DOOR:
        case ITEM_STEAM_LAUNCHER:
        case ITEM_STEAM_SPIKES: {
            if (tile->IsFlagOn(TILEFLAG_OPEN))
                tile->RemoveFlag(TILEFLAG_OPEN);
            else
                tile->SetFlag(TILEFLAG_OPEN);
        } break;
        default:
            break;
        }
        GameUpdatePacket packet {
            .m_type = NET_GAME_PACKET_STEAM,
            .m_net_id = -1
        };
        packet.m_int_x = x;
        packet.m_int_y = y;
        packet.m_delay = delay;
        uint8_t steam_effect = STEAM_EFFECT_NONE;
                
        switch (tile->GetForeground()) {
            case ITEM_STEAM_VENT: {
                steam_effect = STEAM_EFFECT_ACTIVATE_VENT;
            } break;
            case ITEM_STEAM_DOOR: {
                steam_effect = tile->IsFlagOn(TILEFLAG_OPEN) ? STEAM_EFFECT_OPEN_DOOR : STEAM_EFFECT_CLOSE_DOOR;
            } break;
            case ITEM_STEAM_LAUNCHER: {
                steam_effect = tile->IsFlagOn(TILEFLAG_OPEN) ? STEAM_EFFECT_OPEN_LAUNCHER : STEAM_EFFECT_CLOSE_LAUNCHER;
            } break;
            case ITEM_STEAM_LAMP: {
                steam_effect = STEAM_EFFECT_ACTIVATE_LAMP;
            } break;
            case ITEM_STEAM_SPIKES: {
                steam_effect = tile->IsFlagOn(TILEFLAG_OPEN) ? STEAM_EFFECT_OPEN_SPIKE: STEAM_EFFECT_CLOSE_SPIKE;
            } break;
        }
        packet.m_steam_effect = steam_effect;

        world->Broadcast([&](std::shared_ptr<Player> player) { player->SendPacket(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket)); });
    }

    void Algorithm::OnLockReApply(std::shared_ptr<Player> player, std::shared_ptr<World> world, GameUpdatePacket* update_packet) {
        ItemInfo* item = ItemDatabase::GetItem(update_packet->m_item_id);
        if (!item)
            return;
        std::vector<CL_Vec2i> total_tiles;
        uint8_t lock_size = 0;

        switch (item->m_id) {
        case ITEM_SMALL_LOCK: {
            lock_size = SMALL_LOCK_SIZE;
        } break;
        case ITEM_BIG_LOCK: {
            lock_size = BIG_LOCK_SIZE;
        } break;
        default: {
            lock_size = HUGE_LOCK_SIZE;
        } break;
        }
		CL_Vec2i start_pos = CL_Vec2i{ static_cast<int>(update_packet->m_int_x), static_cast<int>(update_packet->m_int_y) };
		std::deque<CL_Vec2i> nodes{ start_pos };

        Tile* start_tile = world->GetTile(start_pos);

        for (auto& tile : world->GetTiles()) {
            if (tile.GetParent() != start_pos.m_x + start_pos.m_y * world->GetSize().m_x)
                continue;
            tile.SetParent(0);
            tile.RemoveFlag(TILEFLAG_LOCKED);
            tile.ClearAccess();
        }

        while (nodes.size() < lock_size) {	
			std::deque<CL_Vec2i> temporary_node;
			for (const auto& node : nodes) {
				std::vector<CL_Vec2i> neighbours;  

				if (node.m_y + 1 < world->GetSize().m_y)
					neighbours.push_back({ node.m_x, node.m_y + 1 });   
				if (node.m_x + 1 < world->GetSize().m_x)
					neighbours.push_back({ node.m_x + 1, node.m_y }); 
				if (node.m_y - 1 >= 0)
					neighbours.push_back({ node.m_x, node.m_y - 1 });
				if (node.m_x - 1 >= 0)
					neighbours.push_back({ node.m_x - 1, node.m_y }); 
			
				if (lock_size != SMALL_LOCK_SIZE) {
					if (nodes.size() > 6 || lock_size == BIG_LOCK_SIZE) {
						if (node.m_x - 1 >= 0 && node.m_y - 1 >= 0)
						    neighbours.push_back({ node.m_x - 1, node.m_y - 1 });				
						if (node.m_x - 1 >= 0 && node.m_y + 1 < world->GetSize().m_y)
							neighbours.push_back({ node.m_x - 1, node.m_y + 1 });
						if (node.m_x + 1 < world->GetSize().m_x && node.m_y - 1 >= 0)
							neighbours.push_back({ node.m_x + 1, node.m_y - 1 });
						if (node.m_x + 1 < world->GetSize().m_x && node.m_y + 1 < world->GetSize().m_y)
							neighbours.push_back({ node.m_x + 1, node.m_y + 1 });
					}		
				}

				for (const auto& neighbour : neighbours) {
					Tile* tile = world->GetTile(neighbour);
					if (!tile)
						continue;
                    ItemInfo* item = tile->GetBaseItem();

					if (std::find(total_tiles.begin(), total_tiles.end(), neighbour) != total_tiles.end())
						continue;
					else if (tile->IsFlagOn(TILEFLAG_LOCKED) && tile->GetParent() != start_pos.m_x + start_pos.m_y * world->GetSize().m_x)  
						continue;
					else if (item->m_item_type == ITEMTYPE_LOCK ||
							 item->m_item_type == ITEMTYPE_MAIN_DOOR || 
							 item->m_item_type == ITEMTYPE_BEDROCK)
						continue;
                    else if (tile->GetBaseItem()->m_id == ITEM_BLANK && start_tile->IsLockFlagOn(LOCKFLAG_IGNORE_EMPTY_AIR))
                        continue;

					temporary_node.emplace_back(neighbour); 
					total_tiles.emplace_back(neighbour);
					if (total_tiles.size() > lock_size)
						goto done;
				}
			}
            if (nodes.empty())
				goto done;
			nodes.pop_front();

			if (!temporary_node.empty()) {
                for (auto p = temporary_node.end() - 1; p != temporary_node.begin(); --p)
                    nodes.emplace_back(*p); 
			}
		}
done:;
        if (total_tiles.size() > lock_size)
            total_tiles.resize(lock_size);

        GameUpdatePacket* visual_packet = (GameUpdatePacket*)std::malloc(sizeof(GameUpdatePacket) + total_tiles.size() * 2);
        std::memset(visual_packet, 0, sizeof(GameUpdatePacket) + total_tiles.size() * 2);

        visual_packet->m_type = NET_GAME_PACKET_SEND_LOCK;
        visual_packet->m_flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
        visual_packet->m_int_x = update_packet->m_int_x;
        visual_packet->m_int_y = update_packet->m_int_y;
        visual_packet->m_item_id = update_packet->m_item_id;
        visual_packet->m_owner_id = player->GetUserId();
        visual_packet->m_data_size = total_tiles.size() * 2;
        visual_packet->m_tiles_length = total_tiles.size();

        BinaryWriter buffer{ total_tiles.size() * 2 };
        for (const auto& pos : total_tiles) {
            Tile* tile = world->GetTile(pos);
            if (!tile)
                continue;
            if (pos.m_x == update_packet->m_int_x && pos.m_y == update_packet->m_int_y)
                continue;
            tile->SetParent(start_pos.m_x + start_pos.m_y * world->GetSize().m_x);
            tile->SetFlag(TILEFLAG_LOCKED);
            if ((world->IsOwned()) && (tile->GetBaseItem()->m_id == ITEM_SMALL_LOCK || tile->GetBaseItem()->m_id == ITEM_BIG_LOCK || tile->GetBaseItem()->m_id == ITEM_HUGE_LOCK || tile->GetBaseItem()->m_id == ITEM_BUILDERS_LOCK)) {
                tile->AddAccess(world->GetOwnerId());
            }
            for (auto& user_id : start_tile->GetAccessList()) {
                tile->AddAccess(user_id);
            }

            buffer.write<uint16_t>(pos.m_x + pos.m_y * world->GetSize().m_x);
        }
        std::memcpy(&visual_packet->m_data, buffer.get(), buffer.get_pos());

        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
            ply->v_sender.OnPlayPositioned("use_lock", player->GetNetId());
            ply->SendPacket(NET_MESSAGE_GAME_PACKET, visual_packet, sizeof(GameUpdatePacket) + visual_packet->m_data_size);
        });
        std::free(visual_packet);
    }
    void Algorithm::OnLockApply(std::shared_ptr<Player> player, std::shared_ptr<World> world, GameUpdatePacket* update_packet) {
        ItemInfo* item = ItemDatabase::GetItem(update_packet->m_item_id);
        if (!item)
            return;
        if (item->IsWorldLock()) {
            world->SetOwnerId(player->GetUserId());
            world->SetMainLock(update_packet->m_int_x + update_packet->m_int_y * world->GetSize().m_x);

            GameUpdatePacket visual_packet{ NET_GAME_PACKET_SEND_LOCK };
            visual_packet.m_int_x = update_packet->m_int_x;
            visual_packet.m_int_y = update_packet->m_int_y;
            visual_packet.m_item_id = update_packet->m_item_id;
            visual_packet.m_owner_id = player->GetUserId();
            world->Broadcast([&](const std::shared_ptr<Player>& ply) {
                ply->SendPacket(NET_MESSAGE_GAME_PACKET, &visual_packet, sizeof(GameUpdatePacket));
                ply->v_sender.OnNameChanged(player->GetNetId(), player->GetDisplayName(world));
                ply->v_sender.OnPlayPositioned("use_lock", player->GetNetId());
                ply->v_sender.OnConsoleMessage(fmt::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", world->GetName(), player->GetDisplayName(world)));
                ply->v_sender.OnTalkBubble(player->GetNetId(), fmt::format("`5[```w{}`` has been `$World Locked`` by {}`5]``", world->GetName(), player->GetDisplayName(world)), true);
            });
            return;
        }
        
        uint32_t ranges = 1, locked_tiles = 0;
        uint8_t lock_size = 0;
        CL_Vec2i lock_pos = CL_Vec2i{ update_packet->m_int_x, update_packet->m_int_y };
        std::vector<CL_Vec2i> total_tiles;

        switch (update_packet->m_item_id) {
        case ITEM_SMALL_LOCK: {
            lock_size = SMALL_LOCK_SIZE;
        } break;
        case ITEM_BIG_LOCK: {
            lock_size = BIG_LOCK_SIZE;
        } break;
        default: {
            lock_size = HUGE_LOCK_SIZE;
        } break;
        }
        
        while (locked_tiles < lock_size) {
            if (lock_pos.m_x - ranges > lock_pos.m_x + ranges || lock_pos.m_y - ranges > lock_pos.m_y + ranges) 
                break;
            bool found_block = false;

            while (true) {
                int32_t min_tile_dist = 99999, selected_tileX = -1, selected_tileY = -1;
                for (auto tileX = lock_pos.m_x - ranges; tileX < lock_pos.m_x + ranges; tileX++) {
                    for (auto tileY = lock_pos.m_y - ranges; tileY < lock_pos.m_y + ranges; tileY++) {
                        auto* tile = world->GetTile(tileX, tileY);
                        if (!tile)
                            continue;
                        if (!Algorithm::IsLockNeighbour(world, CL_Vec2i{ tileX, tileY }, lock_pos, false))
                            continue;
                        int32_t tile_dist = std::abs(static_cast<int>(tileY - lock_pos.m_y)) + std::abs(static_cast<int>(tileX - lock_pos.m_x));
                        if (tile_dist >= min_tile_dist)
                            continue;

                        tile_dist = min_tile_dist;
                        selected_tileX = tileX;
                        selected_tileY = tileY;
                        break;
                    }
                }

                if (selected_tileX == -1 && selected_tileY == -1)
                    break;
                found_block = true;

                total_tiles.push_back(CL_Vec2i{ selected_tileX, selected_tileY });
                locked_tiles = locked_tiles + 1;

                if (locked_tiles >= lock_size)
                    break;
            }

            if (!found_block)
                return;
            ranges = ranges + 1;
        }

        /*GameUpdatePacket* visual_packet = (GameUpdatePacket*)std::malloc(sizeof(GameUpdatePacket) + total_tiles.size() * 2);
        std::memset(visual_packet, 0, sizeof(GameUpdatePacket) + total_tiles.size() * 2);

        visual_packet->m_type = NET_GAME_PACKET_SEND_LOCK;
        visual_packet->m_flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
        visual_packet->m_int_x = update_packet->m_int_x;
        visual_packet->m_int_y = update_packet->m_int_y;
        visual_packet->m_item_id = update_packet->m_item_id;
        visual_packet->m_owner_id = player->GetUserId();
        visual_packet->m_data_size = total_tiles.size() * 2;
        visual_packet->m_tiles_length = total_tiles.size();

        BinaryWriter buffer{ total_tiles.size() * 2 };
        for (const auto& pos : total_tiles) {
            Tile* tile = world->GetTile(pos);
            if (!tile)
                continue;
            if (pos.m_x == update_packet->m_int_x && pos.m_y == update_packet->m_int_y)
                continue;
            tile->SetParent(start_pos.m_x + start_pos.m_y * world->GetSize().m_x);
            tile->SetFlag(TILEFLAG_LOCKED);

            buffer.write<uint16_t>(pos.m_x + pos.m_y * world->GetSize().m_x);
        }
        std::memcpy(&visual_packet->m_data, buffer.get(), buffer.get_pos());

        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
            ply->v_sender.OnPlayPositioned("use_lock", player->GetNetId());
            ply->SendPacket(NET_MESSAGE_GAME_PACKET, visual_packet, sizeof(GameUpdatePacket) + visual_packet->m_data_size);
        });
        std::free(visual_packet);*/
    }
    void Algorithm::OnSteamPulse(std::shared_ptr<Player> player, std::shared_ptr<World> world, GameUpdatePacket* update_packet, eSteamDirection direction) {
        Tile* tile = world->GetTile(update_packet->m_int_x, update_packet->m_int_y);
        if (!tile)
            return;
        GameUpdatePacket packet {
            .m_type = NET_GAME_PACKET_STEAM,
            .m_net_id = -1,
            .m_int_x = update_packet->m_int_x,
            .m_int_y = update_packet->m_int_y
        };
        world->Broadcast([&](const std::shared_ptr<Player>& ply) {
            ply->SendPacket(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket));
        });

        auto steam_packet = [&](int x, int y, uint8_t effect_type = STEAM_EFFECT_NONE) {
            GameUpdatePacket packet {
                .m_type = NET_GAME_PACKET_STEAM,
                .m_net_id = -1
            };
            packet.m_int_x = x;
            packet.m_int_y = y;
            packet.m_particle_id = effect_type;
            
            world->Broadcast([&](std::shared_ptr<Player> player) {
                player->SendPacket(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket));
            });
        };
    
        int steam_power = STEAM_POWER_LENGTH;
        int x = update_packet->m_int_x,
            y = update_packet->m_int_y;
        int x_pos = x,
            y_pos = y;
        int delay = 0;

        while (steam_power > 0)
        {
            if (direction == STEAM_DIRECTION_DOWN) 
            {
                if (y + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(x, y + 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK) 
                    y += 1;
                else {  
                    if (x - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(x - 1, y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK) {
                        x -= 1;
                        direction = STEAM_DIRECTION_LEFT;
                    } else if (x + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(x + 1, y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK) {
                        x += 1;     
                        direction = STEAM_DIRECTION_RIGHT;
                    }
                }
            } else if (direction == STEAM_DIRECTION_LEFT) {
                if (x - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(x - 1, y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK)
                    x -= 1;
                else {   
                    if (y + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(x, y + 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK) {
                        y += 1;
                        direction = STEAM_DIRECTION_DOWN;
                    } else if (y - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(x, y - 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK) {
                        y -= 1;
                        direction = STEAM_DIRECTION_UP;
                    }
                }
            } else if (direction == STEAM_DIRECTION_RIGHT) {
                if (x + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(x + 1, y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK)
                    x += 1; 
                else {   
                    if (y + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(x, y + 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK) {
                        y += 1;
                        direction = STEAM_DIRECTION_DOWN;
                    } else if (y - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(x, y - 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK) {
                        y -= 1;
                        direction = STEAM_DIRECTION_UP;
                    }
                }
            } else if (direction == STEAM_DIRECTION_UP) { //here to fix steam funnel up
                if (y - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(x, y - 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK)
                    y -= 1; 
                else {   
                    if (x - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(x - 1, y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK) {
                        x -= 1;
                        direction = STEAM_DIRECTION_LEFT;
                    } else if (x + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(x + 1, y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK) {
                        x += 1;     
                        direction = STEAM_DIRECTION_RIGHT;
                    }
                }
            } 
        
            if (x == x_pos && y == y_pos) {
                if (y + 1 < world->GetSize().m_y && Algorithm::IsSteamPowered(world->GetTile(x, y + 1)->GetForeground()))
                    Algorithm::OnSteamActive(world, world->GetTile(x, y + 1), x, y + 1, delay);
                else if (x - 1 >= 0 && Algorithm::IsSteamPowered(world->GetTile(x - 1, y)->GetForeground()))
                    Algorithm::OnSteamActive(world, world->GetTile(x - 1, y), x - 1, y, delay);
                else if (x + 1 < world->GetSize().m_x && Algorithm::IsSteamPowered(world->GetTile(x + 1, y)->GetForeground()))
                    Algorithm::OnSteamActive(world, world->GetTile(x + 1, y), x + 1, y, delay);
                else if (y - 1 >= 0 && Algorithm::IsSteamPowered(world->GetTile(x, y - 1)->GetForeground()))
                    Algorithm::OnSteamActive(world, world->GetTile(x, y - 1), x, y - 1, delay);
                goto done;
            }
            else {
                Tile* conductor_tile = world->GetTile(x, y);
                if (!conductor_tile)
                    break;
                ItemInfo* conductor = conductor_tile->GetBaseItem();

                steam_power -= 1;
                delay += 230;
                switch(conductor->m_id) {       
                case ITEM_STEAM_PIPE:
                case ITEM_STEAM_TUBES: {
                    x_pos = x;
                    y_pos = y;
                } break;
                case ITEM_STEAM_FUNNEL_DOWN: {
                    direction = STEAM_DIRECTION_DOWN;
                    x_pos = x;
                    y_pos = y;
                } break;
                case ITEM_STEAM_FUNNEL_UP: {
                    direction = STEAM_DIRECTION_UP;
                    x_pos = x;
                    y_pos = y;
                } break;
                case ITEM_STEAM_FUNNEL: {
                    direction = conductor_tile->IsFlagOn(TILEFLAG_FLIPPED) ? STEAM_DIRECTION_LEFT : STEAM_DIRECTION_RIGHT;  
                    x_pos = x;
                    y_pos = y;
                } break;
                case ITEM_STEAM_CROSSOVER: {
                    x_pos = x;
                    y_pos = y;

                    if (conductor_tile->IsFlagOn(TILEFLAG_FLIPPED)) {
                        if (direction == STEAM_DIRECTION_RIGHT) direction = STEAM_DIRECTION_UP;
                        else if (direction == STEAM_DIRECTION_DOWN)  direction = STEAM_DIRECTION_LEFT;
                        else if (direction == STEAM_DIRECTION_UP) direction = STEAM_DIRECTION_RIGHT;
                        else if (direction == STEAM_DIRECTION_LEFT) direction = STEAM_DIRECTION_DOWN;
                    }
                    else {
                        if (direction == STEAM_DIRECTION_RIGHT) direction = STEAM_DIRECTION_DOWN;
                        else if (direction == STEAM_DIRECTION_DOWN) direction = STEAM_DIRECTION_RIGHT;
                        else if (direction == STEAM_DIRECTION_UP) direction = STEAM_DIRECTION_LEFT;
                        else if (direction == STEAM_DIRECTION_LEFT) direction = STEAM_DIRECTION_UP;
                    }
                } break;
                case ITEM_STEAM_SCRAMBLER: {
                    Tile* neighbour = world->GetTile(x, y);
                    if (neighbour)
                        steam_packet(x, y, 0);

                    std::vector<int> neighbour_direction;
                    neighbour_direction.reserve(4);

                    if (y + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(x, y + 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK && direction != STEAM_DIRECTION_UP) 
                        neighbour_direction.emplace_back(STEAM_DIRECTION_DOWN);
                    if (x - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(x - 1, y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK && direction != STEAM_DIRECTION_RIGHT)
                        neighbour_direction.emplace_back(STEAM_DIRECTION_LEFT);
                    if (x + 1 < world->GetSize().m_x && ItemDatabase::GetItem(world->GetTile(x + 1, y)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK && direction != STEAM_DIRECTION_LEFT)
                        neighbour_direction.emplace_back(STEAM_DIRECTION_RIGHT);
                    if (y - 1 >= 0 && ItemDatabase::GetItem(world->GetTile(x, y - 1)->GetForeground())->m_item_type == ITEMTYPE_STEAMPUNK && direction != STEAM_DIRECTION_DOWN)    
                        neighbour_direction.emplace_back(STEAM_DIRECTION_UP);

                    if (neighbour_direction.empty())
                        goto done;

                    if (neighbour_direction.size() > 1) {
                        auto rd = std::random_device {}; 
                        auto rng = std::default_random_engine { rd() };
                        std::shuffle(std::begin(neighbour_direction), std::end(neighbour_direction), rng);
                    } 
                    int front = neighbour_direction.front();
                    int steam_x = x;
                    int steam_y = y;

                    if (front == STEAM_DIRECTION_DOWN)
                        steam_y += 1;
                    else if (front == STEAM_DIRECTION_LEFT)
                        steam_x -= 1;
                    else if (front == STEAM_DIRECTION_RIGHT)
                        steam_x += 1;
                    else if (front == STEAM_DIRECTION_UP)
                        steam_y -= 1;                            

                    Tile* steam = world->GetTile(steam_x, steam_y);
                    if (!steam)
                        goto done;
                    steam_packet(steam_x, steam_y, 0);
                    x_pos = x;
                    y_pos = y;
                    direction = (eSteamDirection)front;
                    break;
                }
                default:
                    goto done;
                } 
            }         
        } 
        done:;
    }
    
    bool Algorithm::OnFindPath(std::shared_ptr<Player> player, std::shared_ptr<World> world, const CL_Vec2i& current_pos, const CL_Vec2i& future_pos) {
        int start = current_pos.m_x + current_pos.m_y * world->GetSize().m_x;
        int end = future_pos.m_x + future_pos.m_y * world->GetSize().m_x;

        if (!player->CharacterState::IsFlagOn(STATEFLAG_NOCLIP)) {
            if (current_pos.m_x < 0 || current_pos.m_x >= world->GetSize().m_x)
                return false;
            if (future_pos.m_x < 0 || future_pos.m_x >= world->GetSize().m_x)
                return false;

            if (current_pos.m_y < 0 || current_pos.m_y >= world->GetSize().m_y)
                return false;
            if (future_pos.m_y < 0 || future_pos.m_y >= world->GetSize().m_y)
                return false;
        }
        if (world->IsObstacle(player, current_pos) || world->IsObstacle(player, future_pos))
            return false;
        if (start == end)
            return true;

        for (auto& tile : world->GetTiles()) {
            tile.m_path_parent = -1;
            tile.m_visited = false;
            tile.m_local = INFINITY;
            tile.m_global = INFINITY;
        }
        auto get_distance = [&](int32_t a, int32_t b) {
            return sqrtf(
                (world->GetTile(a)->GetPosition().m_x - world->GetTile(b)->GetPosition().m_x) * (world->GetTile(a)->GetPosition().m_x - world->GetTile(b)->GetPosition().m_x) +
                (world->GetTile(a)->GetPosition().m_y - world->GetTile(b)->GetPosition().m_y) * (world->GetTile(a)->GetPosition().m_y - world->GetTile(b)->GetPosition().m_y)
            );
        };
        
        int current = start;
        world->GetTile(start)->m_local = 0.0f;
        world->GetTile(start)->m_global = get_distance(start, end);

        std::list<int> not_tested;
        not_tested.emplace_back(start);
        while(!not_tested.empty() && current != end) {
            not_tested.sort([&](const int lhs, const int rhs) {
                return world->GetTile(lhs)->m_global < world->GetTile(rhs)->m_global;
            });

            while(!not_tested.empty() && world->GetTile(not_tested.front())->m_visited)
                not_tested.pop_front();
            if (not_tested.empty())
                break;

            current = not_tested.front();
            world->GetTile(current)->m_visited = true;

            int x = world->GetTile(current)->GetPosition().m_x, y = world->GetTile(current)->GetPosition().m_y;
            if (y + 1 < world->GetSize().m_y) {
                int index = world->GetTile(x + (y + 1) * world->GetSize().m_x)->GetPosition().m_x + world->GetTile(x + (y + 1) * world->GetSize().m_x)->GetPosition().m_y * world->GetSize().m_x;

                if (!world->GetTile(index)->m_visited && world->IsObstacle(player, { world->GetTile(index)->GetPosition().m_x, world->GetTile(index)->GetPosition().m_y }) == false)
                    not_tested.emplace_back(index);

                float distance = world->GetTile(current)->m_local + get_distance(current, index);
                if (distance < world->GetTile(index)->m_local) {
                    world->GetTile(index)->m_path_parent = current;
                    world->GetTile(index)->m_local = distance;
                    world->GetTile(index)->m_global = world->GetTile(index)->m_local + get_distance(index, end);
                }
            }
            if (x + 1 < world->GetSize().m_x)  {
                int index = world->GetTile((x + 1) + y * world->GetSize().m_x)->GetPosition().m_x + world->GetTile((x + 1) + y * world->GetSize().m_x)->GetPosition().m_y * world->GetSize().m_x;
                if (!world->GetTile(index)->m_visited && world->IsObstacle(player, { world->GetTile(index)->GetPosition().m_x, world->GetTile(index)->GetPosition().m_y }) == false)
                    not_tested.emplace_back(index);

                float distance = world->GetTile(current)->m_local + get_distance(current, index);
                if (distance < world->GetTile(index)->m_local) {
                    world->GetTile(index)->m_path_parent = current;
                    world->GetTile(index)->m_local = distance;
                    world->GetTile(index)->m_global = world->GetTile(index)->m_local + get_distance(index, end);
                }
            }

            if (y - 1 >= 0) {
                int index = world->GetTile(x + (y - 1) * world->GetSize().m_x)->GetPosition().m_x + world->GetTile(x + (y - 1) * world->GetSize().m_x)->GetPosition().m_y * world->GetSize().m_x;
                if (!world->GetTile(index)->m_visited && world->IsObstacle(player, { world->GetTile(index)->GetPosition().m_x, world->GetTile(index)->GetPosition().m_y }) == false)
                    not_tested.emplace_back(index);

                float distance = world->GetTile(current)->m_local + get_distance(current, index);
                if (distance < world->GetTile(index)->m_local) {
                    world->GetTile(index)->m_path_parent = current;
                    world->GetTile(index)->m_local = distance;
                    world->GetTile(index)->m_global = world->GetTile(index)->m_local + get_distance(index, end);
                }
            }
            if (x - 1 >= 0) {
                int index = world->GetTile((x - 1) + y * world->GetSize().m_x)->GetPosition().m_x + world->GetTile((x - 1) + y * world->GetSize().m_x)->GetPosition().m_y * world->GetSize().m_x;
                if (!world->GetTile(index)->m_visited && world->IsObstacle(player, { world->GetTile(index)->GetPosition().m_x, world->GetTile(index)->GetPosition().m_y }) == false)
                    not_tested.emplace_back(index);

                float distance = world->GetTile(current)->m_local + get_distance(current, index);
                if (distance < world->GetTile(index)->m_local) {
                    world->GetTile(index)->m_path_parent = current;
                    world->GetTile(index)->m_local = distance;
                    world->GetTile(index)->m_global = world->GetTile(index)->m_local + get_distance(index, end);
                }
            }       
        }
    
        if (end != -1) {
            int p = end;
            if (p < 0 || p >= world->GetSize().m_x * world->GetSize().m_y)
                return false;    

            while (world->GetTile(p)->m_path_parent != -1) {
                if (world->GetTile(p)->m_path_parent == p)
                    return false;
                if (p < 0 || p >= world->GetSize().m_x * world->GetSize().m_y)
                    return false;

                p = world->GetTile(p)->m_path_parent;
            }
            if (p == start)
               return true;
        }
        return false;
    }
}