#include <world/tile.h>
#include <world/tile_extra.h>
#include <player/player.h>
#include <world/world_object.h>
#include <string>
#include <vector>
#include <functional>

namespace GTServer {
    Tile::Tile(const uint16_t& fg, const uint16_t& bg, const uint16_t& lp, const uint16_t& fl) :
        TileExtra{},
        m_position{},
        m_foreground{ fg }, m_background{ bg }, m_parent{ lp }, m_flags{ fl },
        m_last_hitten{},
        m_hit_count{ 0 } {}
    Tile::Tile() : 
        TileExtra{},
        m_position{},
        m_foreground{ 0 }, m_background{ 0 }, m_parent{ 0 }, m_flags{ 0 },
        m_last_hitten{},
        m_hit_count{ 0 } {}

    void Tile::SetPosition(const CL_Vec2i& pos) {
        m_position = pos;
    }
    bool Tile::SetForeground(uint16_t fg) {
        ItemInfo* item{ ItemDatabase::GetItem(fg) };
        if (!item)
            return false;
        if (item->m_has_extra) {
            switch (item->m_item_type) {
            case ITEMTYPE_DOOR:
            case ITEMTYPE_MAIN_DOOR:
            case ITEMTYPE_PORTAL: {
                this->set_door_data("", false);
            } break;
            case ITEMTYPE_SIGN: {
                this->set_sign_data("", -1);
            } break;
            case ITEMTYPE_HEART_MONITOR: {
                this->set_sign_data("", -1);
            } break;
            case ITEMTYPE_LOCK: {
                this->SetExtraType(TILEEXTRA_TYPE_LOCK);
                this->ClearAccess();
                this->m_lock_flags = 0;
                this->m_owner_id = -1;
                if (item->IsWorldLock())
                    this->m_tempo = 100;
            } break;
            case ITEMTYPE_SEED: {
                if (item->m_id == ITEM_MAGIC_EGG) {
                    this->SetExtraType(TILEEXTRA_TYPE_MAGIC_EGG);
                    this->m_eggs_placed = 1;
                    fg = ITEM_BUNNY_EGG;
                    break;
                }
                this->SetFlag(TILEFLAG_SEED);
                this->set_seed_data(fg - 1);
            } break;
            case ITEMTYPE_DICE: {
                this->SetExtraType(TILEEXTRA_TYPE_DICE);
                this->m_random_value = 0;
            } break;
            case ITEMTYPE_PROVIDER: {
                this->SetExtraType(TILEEXTRA_TYPE_PROVIDER);
                m_planted_date = high_resolution_clock::now() - std::chrono::seconds(item->m_grow_time / 2);
            } break;
            case ITEMTYPE_MANNEQUIN: {
                this->SetExtraType(TILEEXTRA_TYPE_MANNEQUIN);
                this->m_label = "";
                for (auto& cloth : this->GetClothes())
                    cloth = ITEM_BLANK;
                this->GetPrimaryColor() = Color{ 0xFF, 0xFF, 0xFF, 0xFF };
            } break;
            case ITEMTYPE_GAME_RESOURCES: {
                this->SetExtraType(TILEEXTRA_TYPE_GAME_RESOURCES);
                this->m_item_id = 4;
            } break;
            case ITEMTYPE_SPOTLIGHT: {
                this->SetExtraType(TILEEXTRA_TYPE_SPOTLIGHT);
                this->m_owner_id = 0;
            } break;
            case ITEMTYPE_DISPLAY_BLOCK: {
                this->SetExtraType(TILEEXTRA_TYPE_DISPLAY_BLOCK);
                this->m_item_id = 0;
            } break;
            case ITEMTYPE_FLAG: {
                this->SetExtraType(TILEEXTRA_TYPE_FLAG);
                this->m_label = "us";
            } break;
            case ITEMTYPE_WEATHER_SPECIAL: {
                this->SetExtraType(TILEEXTRA_TYPE_WEATHER_SPECIAL);
                switch (item->m_id) {
                case ITEM_WEATHER_MACHINE_HEATWAVE: {
                    this->m_primary_color = Color{ 0xFF, 0x80, 0x40 };
                } break;
                case ITEM_WEATHER_MACHINE_BACKGROUND: {
                    this->m_item_id = ITEM_CAVE_BACKGROUND;
                } break;
                default: {
                    this->m_item_id = ITEM_BLANK;
                } break;
                }
            } break;
            case ITEMTYPE_PORTRAIT: {
                this->SetExtraType(TILEEXTRA_TYPE_PORTRAIT); 
                this->m_label = "";
                this->m_expression_id = 0;
                for (auto& cloth : this->GetClothes())
                    cloth = ITEM_BLANK;
                this->GetPrimaryColor() = Color{ 0xFF, 0xFF, 0xFF, 0xFF };
                this->GetSecondaryColor() = Color{ 0xB4, 0x8A, 0x78, 0xFF };
            } break;
            case ITEMTYPE_WEATHER_SPECIAL2: {
                this->SetExtraType(TILEEXTRA_TYPE_WEATHER_SPECIAL2);
                this->m_weather_flags = false | false << 1;
                this->m_item_id = ITEM_DIRT;
                this->m_gravity = 100;
            } break;
            case ITEMTYPE_WEATHER_INFINITY: {
                this->SetExtraType(TILEEXTRA_TYPE_WEATHER_INFINITY);
                this->m_cycle_time = 1;
                this->ClearWeather();
            } break;
            default:
                return false;
            }
            this->SetFlag(TILEFLAG_TILEEXTRA);
        }
        m_foreground = fg;
        return true;
    }
    uint32_t Tile::DevPunchAdd(const std::shared_ptr<Player>& player) {
        m_DevBreak.insert_or_assign(++m_net_id, player);
        return m_net_id;
    }
    void Tile::DevPunchRemove(const std::shared_ptr<Player>& player) {
        auto it = std::find_if(m_DevBreak.begin(), m_DevBreak.end(),
            [&](const auto& p) { return p.second->GetUserId() == player->GetUserId(); });
        if (it != m_DevBreak.end())
            m_DevBreak.erase(it);
    }
    bool Tile::HasDevPunch(const std::shared_ptr<Player>& player) {
        for (const auto& [net_id, ply] : m_DevBreak) {
            if (ply->GetUserId() != player->GetUserId())
                continue;
            return true;
        }
        return false;
    }
    void Tile::SetBackground(const uint16_t& bg) {
        m_background = bg;
    }
    void Tile::SetParent(const uint16_t& parent) {
        m_parent = parent;
    }
        
    ItemInfo* Tile::GetBaseItem() {
        return ItemDatabase::GetItem(m_foreground != 0 ? m_foreground : m_background);
    }
    void Tile::RemoveBase() {
        uint16_t& base{ m_foreground != 0 ? m_foreground : m_background };
        this->ResetHits();

        uint16_t flags = 0;
        if (this->IsFlagOn(TILEFLAG_LOCKED))
            flags |= TILEFLAG_LOCKED;
        if (this->IsFlagOn(TILEFLAG_WATER))
            flags |= TILEFLAG_WATER;
        if (this->IsFlagOn(TILEFLAG_GLUE))
            flags |= TILEFLAG_GLUE;
        if (this->IsFlagOn(TILEFLAG_FIRE))
            flags |= TILEFLAG_FIRE;

        m_flags = flags;
        base = 0;
    }

    bool Tile::IsFlagOn(const eTileFlags& flag) const {
        if (m_flags & static_cast<uint16_t>(flag))
            return true;
        return false;
    }
    void Tile::SetFlag(const eTileFlags& flag) {
        m_flags |= flag;
    }
    void Tile::RemoveFlag(const eTileFlags& flag) {
        m_flags &= ~flag;
    }

    std::size_t Tile::GetMemoryUsage(const bool& to_database) {
        ItemInfo* item = this->GetBaseItem();
        if (!item)
            return 0;
        std::size_t ret{ 8 }; 
        if (to_database)
            ret += sizeof(CL_Vec2i);

        if (this->m_parent != 0)
            ret += sizeof(uint16_t);
        if (this->IsFlagOn(TILEFLAG_TILEEXTRA)) {
            ret += sizeof(uint8_t);

            switch (this->GetExtraType()) {
            case TILEEXTRA_TYPE_DOOR: {
                ret += sizeof(uint16_t) + this->GetLabel().size();
                ret += sizeof(bool);
                if (to_database) {
                    ret += sizeof(uint16_t) + this->GetDestination().size();
                    ret += sizeof(uint16_t) + this->GetDoorUniqueId().size();
                    ret += sizeof(uint16_t) + this->GetPassword().size();
                }
            } break;
            case TILEEXTRA_TYPE_SIGN: {
                ret += sizeof(uint16_t) + this->GetLabel().size();
                ret += sizeof(int32_t);
            } break;
            case TILEEXTRA_TYPE_LOCK: {
                ret += 21 + (this->GetAccessList().size() * 4);
                if (item->m_id == ITEM_GUILD_LOCK)
                    ret += 16;
                return ret;
            } break;
            case TILEEXTRA_TYPE_SEED: {
                if (to_database) {
                    ret += sizeof(high_resolution_clock::time_point);
                    ret += sizeof(bool);
                } else {
                    ret += sizeof(uint32_t);
                }
                ret += sizeof(uint8_t);
            } break;
            case TILEEXTRA_TYPE_DICE: {
                ret += sizeof(uint8_t);
            } break;
            case TILEEXTRA_TYPE_PROVIDER: {
                if (to_database) {
                    ret += sizeof(high_resolution_clock::time_point);
                } else {
                    ret += sizeof(uint32_t);
                }
            } break;
            case TILEEXTRA_TYPE_MANNEQUIN: {
                ret += sizeof(uint16_t) + this->GetLabel().size();
                ret += 23;
            } break;
            case TILEEXTRA_TYPE_MAGIC_EGG: {
                ret += sizeof(uint32_t);
            } break;
            case TILEEXTRA_TYPE_GAME_RESOURCES: {
                ret += sizeof(uint8_t);
            } break;
            case TILEEXTRA_TYPE_SPOTLIGHT: {
            } break;
            case TILEEXTRA_TYPE_DISPLAY_BLOCK: {
                ret += sizeof(uint32_t);
            } break;
            case TILEEXTRA_TYPE_FLAG: {
                ret += sizeof(uint16_t) + this->GetLabel().size();
            } break;
            case TILEEXTRA_TYPE_WEATHER_SPECIAL: {
                ret += sizeof(uint32_t);
            } break;
            case TILEEXTRA_TYPE_PORTRAIT: {
                ret += sizeof(uint16_t) + this->GetLabel().size();
                ret += sizeof(uint32_t) + sizeof(uint32_t);
                ret += sizeof(Color) * 2;
                ret += sizeof(uint16_t) * 3;
                ret += sizeof(uint32_t);
            } break;
            case TILEEXTRA_TYPE_WEATHER_SPECIAL2: {
                ret += sizeof(uint32_t) + sizeof(int32_t) + sizeof(uint8_t);
            } break;
            case TILEEXTRA_TYPE_WEATHER_INFINITY: {
                ret += sizeof(uint32_t) + sizeof(uint32_t);
                ret += this->m_uint_array.size() * 4;
            } break;
            default:
                break;
            }
        }
        return ret;
    }
    void Tile::Pack(BinaryWriter& buffer, const bool& to_database) {
        ItemInfo* item = this->GetBaseItem();
        if (!item)
            return;
        if (to_database) 
            buffer.write<CL_Vec2i>(m_position);
        buffer.write<uint16_t>(m_foreground);
        buffer.write<uint16_t>(m_background);
        buffer.write<uint16_t>(m_parent);
        buffer.write<uint16_t>(m_flags);
        
        if (this->m_parent != 0)
            buffer.write<uint16_t>(m_parent);
        if (this->IsFlagOn(TILEFLAG_TILEEXTRA)) {
            buffer.write<uint8_t>(this->GetExtraType());

            switch (this->GetExtraType()) {
            case TILEEXTRA_TYPE_DOOR: {
                buffer.write(this->GetLabel(), sizeof(uint16_t));
                buffer.write<bool>(this->IsLocked());
                if (to_database) {
                    buffer.write(this->GetDestination(), sizeof(uint16_t));
                    buffer.write(this->GetDoorUniqueId(), sizeof(uint16_t));
                    buffer.write(this->GetPassword(), sizeof(uint16_t));
                }
            } break;
            case TILEEXTRA_TYPE_SIGN: {
                buffer.write(this->GetLabel(), sizeof(uint16_t));
                buffer.write<int32_t>(this->GetEndMarker());
            } break; 
            case TILEEXTRA_TYPE_LOCK: {
                buffer.write<uint8_t>(this->GetLockFlags());
                buffer.write<uint32_t>(this->GetOwnerId());

                auto access_list = this->GetAccessList();
                buffer.write<uint32_t>(static_cast<uint32_t>(access_list.size() + 1));
                for (auto& uid : access_list)
                    buffer.write<uint32_t>(uid);
                buffer.write<int32_t>(this->GetTempo() * -1);
                buffer.write<uint32_t>(0x1);
                buffer.write<uint32_t>(0x0);
            } break;
            case TILEEXTRA_TYPE_SEED: {
                if (to_database) {
                    buffer.write<uint64_t>(this->GetPlantedDate().time_since_epoch().count());
                    buffer.write<bool>(this->IsSpliced());
                } else {
                    buffer.write<uint32_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(high_resolution_clock::now() - this->GetPlantedDate()).count()
                    );
                }
                buffer.write<uint8_t>(this->GetFruitCount());
            } break;
            case TILEEXTRA_TYPE_DICE: {
                buffer.write<uint8_t>(this->GetDiceResult());
            } break;
            case TILEEXTRA_TYPE_PROVIDER: {   
                if (to_database) {
                    buffer.write<uint64_t>(this->GetPlantedDate().time_since_epoch().count());
                } else {
                    buffer.write<uint32_t>(
                        std::chrono::duration_cast<std::chrono::seconds>(high_resolution_clock::now() - this->GetPlantedDate()).count()
                    );
                }
            } break;
            case TILEEXTRA_TYPE_MANNEQUIN: {
                buffer.write(this->GetLabel(), sizeof(uint16_t));
                buffer.write<uint32_t>(this->GetPrimaryColor().GetInt());
                buffer.write<uint8_t>(0);
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_MASK));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_SHIRT));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_PANTS));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_FEET));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_FACE));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_HAND));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_BACK));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_HAIR));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_NECKLACE));
            } break;
            case TILEEXTRA_TYPE_MAGIC_EGG: {
                buffer.write<uint32_t>(this->GetEggsPlaced());
            } break;
            case TILEEXTRA_TYPE_GAME_RESOURCES: {
                buffer.write<uint8_t>(static_cast<uint8_t>(this->GetItemId()));
            } break;
            case TILEEXTRA_TYPE_SPOTLIGHT: {
            } break;
            case TILEEXTRA_TYPE_DISPLAY_BLOCK: {
                buffer.write<uint32_t>(this->GetItemId());
            } break;
            case TILEEXTRA_TYPE_FLAG: {
                buffer.write(this->GetLabel(), sizeof(uint16_t));
            } break;
            case TILEEXTRA_TYPE_WEATHER_SPECIAL: {
                if (item->m_id == ITEM_WEATHER_MACHINE_HEATWAVE) {
                    buffer.write<uint32_t>(this->GetPrimaryColor().GetInt());
                } else {
                    buffer.write<uint32_t>(this->GetItemId());
                }
            } break;
            case TILEEXTRA_TYPE_PORTRAIT: {
                buffer.write(this->GetLabel(), sizeof(uint16_t));
                buffer.write<uint32_t>(this->GetExpressionId());
                buffer.write<uint32_t>(2);
                buffer.write<uint32_t>(this->GetPrimaryColor().GetInt());
                buffer.write<uint32_t>(this->GetSecondaryColor().GetInt());
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_FACE));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_HAIR));
                buffer.write<uint16_t>(this->GetCloth(CLOTHTYPE_MASK));
                buffer.write<uint32_t>(6);
            } break;
            case TILEEXTRA_TYPE_WEATHER_SPECIAL2: {
                buffer.write<uint32_t>(this->GetItemId());
                buffer.write<int32_t>(this->GetGravity());
                buffer.write<uint8_t>(this->GetWeatherFlags());
            } break;
            case TILEEXTRA_TYPE_WEATHER_INFINITY: {
                buffer.write<uint32_t>(this->GetCycleTime());
                buffer.write<uint32_t>(static_cast<uint32_t>(this->GetWeatherList().size()));
                for (const auto& item_id : this->GetWeatherList())
                    buffer.write<uint32_t>(item_id);
            }
            default:
                break;
            }
        }
    }
    void Tile::Serialize(BinaryReader& br) {
        this->SetPosition(br.read<CL_Vec2i>());
        this->SetForeground(br.read<uint16_t>());
        this->SetBackground(br.read<uint16_t>());
        this->SetParent(br.read<uint16_t>());
        this->SetFlags(br.read<uint16_t>()); 

        if (m_parent != 0)
            br.skip(sizeof(uint16_t));
        if (this->IsFlagOn(TILEFLAG_TILEEXTRA)) {
            ItemInfo* item = ItemDatabase::GetItem(this->GetForeground());
            if (!item)
                return;
            this->SetExtraType(br.read<uint8_t>());

            switch (this->GetExtraType()) {
            case TILEEXTRA_TYPE_DOOR: {
                this->m_label = br.read_string();
                this->m_locked = br.read<bool>();
                
                this->m_destination = br.read_string();
                this->m_door_unique_id = br.read_string();
                this->m_password = br.read_string();
            } break;
            case TILEEXTRA_TYPE_SIGN: {
                this->m_label = br.read_string();
                this->m_end_marker = br.read<int32_t>();
            } break;
            case TILEEXTRA_TYPE_LOCK: {
                this->m_lock_flags = br.read<uint8_t>();
                this->m_owner_id = br.read<uint32_t>();
                
                auto access_size = br.read<uint32_t>() - 1;
                for (auto index = 0; index < access_size; index++)
                    this->m_uint_array.push_back(br.read<uint32_t>());
                this->m_tempo = std::abs(br.read<int32_t>());
                br.skip(8);
            } break;
            case TILEEXTRA_TYPE_SEED: {
                this->m_planted_date = high_resolution_clock::time_point{ std::chrono::nanoseconds(br.read<uint64_t>()) };
                this->m_spliced = br.read<bool>();
                this->m_fruit_count = br.read<uint8_t>();
            } break;
            case TILEEXTRA_TYPE_DICE: {
                this->m_random_value = br.read<uint8_t>();
            } break;
            case TILEEXTRA_TYPE_PROVIDER: {
                this->m_planted_date = high_resolution_clock::time_point{ std::chrono::nanoseconds(br.read<uint64_t>()) };
            } break;
            case TILEEXTRA_TYPE_MANNEQUIN: {
                this->m_label = br.read_string();
                this->GetPrimaryColor() = Color{ br.read<uint32_t>() };
                br.skip(1);
                this->SetCloth(CLOTHTYPE_MASK, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_SHIRT, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_PANTS, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_FEET, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_FACE, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_HAND, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_BACK, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_HAIR, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_NECKLACE, br.read<uint16_t>());
            } break;
            case TILEEXTRA_TYPE_MAGIC_EGG: {
                this->m_eggs_placed = br.read<uint32_t>();
            } break; 
            case TILEEXTRA_TYPE_GAME_RESOURCES: {
                this->m_item_id = br.read<uint8_t>();
            } break;
            case TILEEXTRA_TYPE_SPOTLIGHT: {
            } break;
            case TILEEXTRA_TYPE_DISPLAY_BLOCK: {
                this->m_item_id = br.read<uint32_t>();
            } break;
            case TILEEXTRA_TYPE_FLAG: {
                this->m_label = br.read_string();
            } break;
            case TILEEXTRA_TYPE_WEATHER_SPECIAL: {
                if (item->m_id == ITEM_WEATHER_MACHINE_HEATWAVE) {
                    this->m_primary_color = Color{ br.read<uint32_t>() };
                    break;
                }
                this->m_item_id = br.read<uint32_t>();
            } break;
            case TILEEXTRA_TYPE_PORTRAIT: {
                this->m_label = br.read_string();
                this->m_expression_id = br.read<uint32_t>();
                br.skip(sizeof(uint32_t));
                this->m_primary_color = Color{ br.read<uint32_t>() };
                this->m_secondary_color = Color{ br.read<uint32_t>() };
                this->SetCloth(CLOTHTYPE_FACE, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_HAIR, br.read<uint16_t>());
                this->SetCloth(CLOTHTYPE_MASK, br.read<uint16_t>());
                br.skip(sizeof(uint32_t));
            } break;
            case TILEEXTRA_TYPE_WEATHER_SPECIAL2: {
                this->m_item_id = br.read<uint32_t>();
                this->m_gravity = br.read<int32_t>();
                this->m_weather_flags = br.read<uint8_t>();
            } break;
            case TILEEXTRA_TYPE_WEATHER_INFINITY: {
                this->m_cycle_time = br.read<uint32_t>();
                uint32_t weathers_count = br.read<uint32_t>();
                for (auto index = 0; index < weathers_count; ++index)
                    this->AddWeather(br.read<uint32_t>());
            } break;
            default:
                break;
            }
        }
    }

    void Tile::ResetHits() {
        m_hit_count = 0;
        m_last_hitten = TimingClock{};
    }
    uint8_t Tile::IndicateHit() {
        m_last_hitten.UpdateTime();
        return ++m_hit_count;
    }

    void Tile::RemoveLock() {
        this->SetParent(0);
        this->RemoveFlag(TILEFLAG_LOCKED);
        this->m_lock_flags = 0;
    }
    void Tile::ApplyLockOwner(const uint32_t& uid) {
        this->m_owner_id = uid;

        if (this->GetParent() != 0) {
            this->SetParent(0);
            this->RemoveFlag(TILEFLAG_LOCKED);
        }
    }
}