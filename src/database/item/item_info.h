#pragma once
#include <string>
#include <fmt/core.h>
#include <database/item/item_component.h>
#include <database/item/item_type.h>
#include <database/item/item_flags.h>
#include <database/item/item_spread_type.h>
#include <utils/binary_reader.h>
#include <utils/binary_writer.h>
#include <utils/text.h>
#include <utils/file_manager.h>
#include <proton/utils/misc_utils.h>

namespace GTServer {
    struct ItemInfo {
        uint32_t m_id;
        uint8_t m_editable_type = 0;
        uint8_t m_item_category = 0;
        uint8_t m_item_type = 0;
        uint8_t m_marterial = 0;

        std::string m_name = "";
        std::string m_texture = "";

        uint32_t m_texture_hash = 0;
        uint8_t m_visual_effect = 0;

        uint32_t m_flags1 = 0;
        uint32_t m_ingredient = 0;

        uint8_t m_texture_x = 0;
        uint8_t m_texture_y = 0;
        uint8_t m_spread_type = 0;
        uint8_t m_is_stripey_wallpaper = 0;
        uint8_t m_collision_type = 0;

        uint8_t m_break_hits = 0;

        uint32_t m_reset_time = 0;

        uint32_t m_grow_time = 0;
        uint32_t m_clothing_type = 0;

        uint16_t m_rarity = 0;
        uint8_t m_max_amount = 0;

        std::string m_extra_file = "";
        uint32_t m_extra_file_hash = 0;

        union {
            uint32_t m_audio_volume = 0;
            uint32_t m_weather_id;
        };

        std::string m_pet_name = "";
        std::string m_pet_prefix = "";
        std::string m_pet_suffix = "";
        std::string m_pet_ability = "";

        uint8_t m_seed_base = 0;
        uint8_t m_seed_overlay = 0;
        uint8_t m_tree_base = 0;
        uint8_t m_tree_leaves = 0;

        uint32_t m_seed_color = 0;
        uint32_t m_seed_overlay_color = 0;

        uint16_t m_flags2 = 0;
        uint16_t m_rayman = 0;

        std::string m_extra_options = "";
        std::string m_texture2 = "";
        std::string m_extra_options2 = "";
        std::string m_punch_options = "";

        uint32_t m_val3 = 0;
        uint32_t m_val4 = 0;
        uint32_t m_val5 = 0;

        uint8_t m_bodypart[9] = { 0 };
        uint8_t m_reserved[80] = { 0 };

        bool m_has_extra = false;
        uint8_t m_default_texture_x = 0;
        uint8_t m_default_texture_y = 0;

        uint32_t m_mods = 0;
        std::string m_description = "This is a seed.";

        static std::string Cypher(const std::string& input, uint32_t item_id) {
            constexpr std::string_view key{ "PBG892FXX982ABC*" };
            std::string ret(input.size(), 0);

            for (uint32_t i = 0; i < input.size(); i++)
                ret[i] = static_cast<char>(input[i] ^ key[(i + item_id) % key.size()]);
            return ret;
        }

        bool IsBackground() const {
            return m_item_type == ITEMTYPE_BACKGROUND ||
                m_item_type == ITEMTYPE_BACK_BOOMBOX ||
                m_item_type == ITEMTYPE_MUSIC_NOTE;
        }
        bool IsWorldLock() const {
            return m_id != ITEM_SMALL_LOCK &&
                m_id != ITEM_BIG_LOCK &&
                m_id != ITEM_HUGE_LOCK &&
                m_id != ITEM_BUILDERS_LOCK;
        }

        std::size_t GetMemoryUsage() const {
            std::size_t ret{ 8 };
            ret += (uint16_t) + m_name.size();
            ret += (uint16_t) + m_texture.size();
            ret += 34;
            ret += (uint16_t) + m_extra_file.size();
            ret += 12;
            ret += (uint16_t) + m_pet_name.size();
            ret += (uint16_t) + m_pet_prefix.size();
            ret += (uint16_t) + m_pet_suffix.size();
            ret += (uint16_t) + m_pet_ability.size();
            ret += 16;
            ret += (uint16_t) + m_extra_options.size();
            ret += (uint16_t) + m_texture2.size();
            ret += (uint16_t) + m_extra_options2.size();
            ret += 80;
            ret += (uint16_t) + m_punch_options.size();
            ret += 21;
            return ret;
        }
        void Pack(BinaryWriter& buffer) {
            buffer.write<uint32_t>(m_id);
            buffer.write<uint8_t>(m_editable_type);
            buffer.write<uint8_t>(m_item_category);
            buffer.write<uint8_t>(m_item_type);
            buffer.write<uint8_t>(m_marterial);

            buffer.write(this->Cypher(m_name, m_id));
            buffer.write(m_texture);
            buffer.write<uint32_t>(m_texture_hash);

            buffer.write<uint8_t>(m_visual_effect);
            buffer.write<uint32_t>(m_flags1);
            buffer.write<uint8_t>(m_texture_x);
            buffer.write<uint8_t>(m_texture_y);

            buffer.write<uint8_t>(m_spread_type);
            switch(m_spread_type) {
                case TILESPREAD_DIRT:
                case TILESPREAD_PLATFORM: {
                    m_default_texture_x = m_texture_x + 4;
                    m_default_texture_y = m_texture_y + 1;
                    break;
                }
                case TILESPREAD_LAVA:
                case TILESPREAD_PILLAR: {
                    m_default_texture_x = m_texture_x + 3;
                    m_default_texture_y = m_texture_y;
                    break;
                }
                default: {
                    m_default_texture_x = m_texture_x;
                    m_default_texture_y = m_texture_y;
                    break;
                }
            }

            buffer.write<uint8_t>(m_is_stripey_wallpaper);
            buffer.write<uint8_t>(m_collision_type);
            buffer.write<uint8_t>(m_break_hits * 6);
            buffer.write<uint32_t>(m_reset_time);
            buffer.write<uint8_t>(m_clothing_type);
            buffer.write<uint16_t>(m_rarity);
            buffer.write<uint8_t>(m_max_amount);

            buffer.write(m_extra_file);
            buffer.write<uint32_t>(m_extra_file_hash);
            buffer.write<uint32_t>(m_audio_volume);

            buffer.write(m_pet_name);
            buffer.write(m_pet_prefix);
            buffer.write(m_pet_suffix);
            buffer.write(m_pet_ability);
            
            buffer.write<uint8_t>(m_seed_base);
            buffer.write<uint8_t>(m_seed_overlay);
            buffer.write<uint8_t>(m_tree_base);
            buffer.write<uint8_t>(m_tree_leaves);
            buffer.write<uint32_t>(m_seed_color);
            buffer.write<uint32_t>(m_seed_overlay_color);
            buffer.write<uint32_t>(m_ingredient);
            buffer.write<uint32_t>(m_grow_time);
            buffer.write<uint16_t>(m_flags2);
            buffer.write<uint16_t>(m_rayman);

            buffer.write(m_extra_options);
            buffer.write(m_texture2);
            buffer.write(m_extra_options2);

            for (auto index = 0; index < 80; index++)
                buffer.write<uint8_t>(m_reserved[index]);

            buffer.write(m_punch_options);
            buffer.write<uint32_t>(m_val3);

            for (auto index = 0; index < 9; index++)
                buffer.write<uint8_t>(m_bodypart[index]);
            buffer.write<uint32_t>(m_val4);
            buffer.write<uint32_t>(m_val5);
        }
        void Serialize(BinaryReader& br) {
            m_id = br.read<uint32_t>();
            m_editable_type = br.read<uint8_t>();
            m_item_category = br.read<uint8_t>();
            m_item_type = br.read<uint8_t>();
            m_marterial = br.read<uint8_t>();

            m_name = this->Cypher(br.read_string(), m_id);
            m_texture = br.read_string();

            m_texture_hash = br.read<uint32_t>();
            m_visual_effect = br.read<uint8_t>();
            m_flags1 = br.read<uint32_t>();
            m_texture_x = br.read<uint8_t>();
            m_texture_y = br.read<uint8_t>();
            m_spread_type = br.read<uint8_t>();
            m_is_stripey_wallpaper = br.read<uint8_t>();
            m_collision_type = br.read<uint8_t>();
            m_break_hits = br.read<uint8_t>() / 6;
            m_reset_time = br.read<uint32_t>();
            m_clothing_type = br.read<uint8_t>();
            m_rarity = br.read<uint16_t>();
            m_max_amount = br.read<uint8_t>();

            m_extra_file = br.read_string();

            m_extra_file_hash = br.read<uint32_t>();
            m_audio_volume = br.read<uint32_t>();

            m_pet_name = br.read_string();
            m_pet_prefix = br.read_string();
            m_pet_suffix = br.read_string();
            m_pet_ability = br.read_string();

            m_seed_base = br.read<uint8_t>();
            m_seed_overlay = br.read<uint8_t>();
            m_tree_base = br.read<uint8_t>();
            m_tree_leaves = br.read<uint8_t>();
            m_seed_color = br.read<uint32_t>();
            m_seed_overlay_color = br.read<uint32_t>();
            m_ingredient = br.read<uint32_t>();
            m_grow_time = br.read<uint32_t>();
            m_flags2 = br.read<uint16_t>();
            m_rayman = br.read<uint16_t>();

            m_extra_options = br.read_string();
            m_texture2 = br.read_string();
            m_extra_options2 = br.read_string();
            for (auto index = 0; index < 80; index++)
                m_reserved[index] = br.read<uint8_t>();
            
            m_punch_options = br.read_string();
            m_val3 = br.read<uint32_t>();
            for (auto index = 0; index < 9; index++)
                m_bodypart[index] = br.read<uint8_t>();
            m_val4 = br.read<uint32_t>();
            m_val5 = br.read<uint32_t>();

            switch (m_item_type)
            {
            case ITEMTYPE_DOOR:
            case ITEMTYPE_LOCK:
            case ITEMTYPE_SIGN:
            case ITEMTYPE_MAIN_DOOR:
            case ITEMTYPE_SEED:
            case ITEMTYPE_PORTAL:
            case ITEMTYPE_MAILBOX:
            case ITEMTYPE_BULLETIN:
            case ITEMTYPE_DICE:
            case ITEMTYPE_PROVIDER:
            case ITEMTYPE_ACHIEVEMENT:
            case ITEMTYPE_SUNGATE:
            case ITEMTYPE_HEART_MONITOR:
            case ITEMTYPE_DONATION_BOX:
            case ITEMTYPE_TOYBOX:
            case ITEMTYPE_MANNEQUIN:
            case ITEMTYPE_SECURITY_CAMERA:
            case ITEMTYPE_MAGIC_EGG:
            case ITEMTYPE_GAME_RESOURCES:
            case ITEMTYPE_GAME_GENERATOR:
            case ITEMTYPE_XENONITE:
            case ITEMTYPE_DRESSUP:
            case ITEMTYPE_CRYSTAL:
            case ITEMTYPE_BURGLAR:
            case ITEMTYPE_SPOTLIGHT:
            case ITEMTYPE_DISPLAY_BLOCK:
            case ITEMTYPE_VENDING_MACHINE:
            case ITEMTYPE_FISHTANK:
            case ITEMTYPE_SOLAR:
            case ITEMTYPE_FORGE:
            case ITEMTYPE_GIVING_TREE:
            case ITEMTYPE_GIVING_TREE_STUMP:
            case ITEMTYPE_STEAM_ORGAN:
            case ITEMTYPE_TAMAGOTCHI:
            case ITEMTYPE_SWING:
            case ITEMTYPE_FLAG:
            case ITEMTYPE_LOBSTER_TRAP:
            case ITEMTYPE_ART_CANVAS:
            case ITEMTYPE_BATTLE_CAGE:
            case ITEMTYPE_PET_TRAINER:
            case ITEMTYPE_STEAM_ENGINE:
            case ITEMTYPE_LOCKBOT:
            case ITEMTYPE_WEATHER_SPECIAL:
            case ITEMTYPE_SPIRIT_STORAGE:
            case ITEMTYPE_DISPLAY_SHELF:
            case ITEMTYPE_VIP_ENTRANCE:
            case ITEMTYPE_CHALLENGE_TIMER:
            case ITEMTYPE_CHALLENGE_FLAG:
            case ITEMTYPE_FISH_MOUNT:
            case ITEMTYPE_PORTRAIT:
            case ITEMTYPE_WEATHER_SPECIAL2:
            case ITEMTYPE_FOSSIL_PREP:
            case ITEMTYPE_DNA_MACHINE:
            case ITEMTYPE_BLASTER:
            case ITEMTYPE_CHEMTANK:
            case ITEMTYPE_STORAGE:
            case ITEMTYPE_OVEN:
            case ITEMTYPE_SUPER_MUSIC:
            case ITEMTYPE_GEIGER_CHARGER:
            case ITEMTYPE_ADVENTURE_RESET:
            case ITEMTYPE_TOMB_ROBBER:
            case ITEMTYPE_FACTION:
            case ITEMTYPE_RED_FACTION:
            case ITEMTYPE_GREEN_FACTION:
            case ITEMTYPE_BLUE_FACTION:
            case ITEMTYPE_FISHGOTCHI_TANK:
            case ITEMTYPE_ITEM_SUCKER:
            case ITEMTYPE_ROBOT:
            case ITEMTYPE_TICKET:
            case ITEMTYPE_STATS_BLOCK:
            case ITEMTYPE_FIELD_NODE:
            case ITEMTYPE_OUIJA_BOARD:
            case ITEMTYPE_AUTO_ACTION_BREAK:
            case ITEMTYPE_AUTO_ACTION_HARVEST:
            case ITEMTYPE_AUTO_ACTION_HARVEST_SUCK:
            case ITEMTYPE_LIGHTNING_IF_ON:
            case ITEMTYPE_PHASED_BLOCK:
            case ITEMTYPE_PASSWORD_STORAGE:
            case ITEMTYPE_PHASED_BLOCK_2:
            case ITEMTYPE_WEATHER_INFINITY:
            case ITEMTYPE_KRANKENS_BLOCK:
            case ITEMTYPE_FRIENDS_ENTRANCE:
            case 30:
            case 133: {
                m_has_extra = true;
                break;
            }
            }

            /*if (!m_texture.empty()) {
                uintmax_t file_size = 0;
                void* data = nullptr;
                if (std::filesystem::exists(fmt::format("cache/game/{}", m_texture)))
                    data = FileManager::read_all_bytes(fmt::format("cache/game/{}", m_texture), file_size);

                if (!data) 
                    fmt::print("ItemInfo::Serialize -> failed to load texture for item {}, (texture: {})\n", m_name, m_texture);
                m_texture_hash = proton::utils::hash(data, file_size);
                std::free(data);
            }
            if (!m_extra_file.empty()) {
                uintmax_t file_size = 0;
                void* data = nullptr;

                if (std::filesystem::exists(fmt::format("cache/{}", m_extra_file)))
                    data = FileManager::read_all_bytes(fmt::format("cache/{}", m_extra_file), file_size);

                // if (!data) 
                    // fmt::print("ItemInfo::Serialize -> failed to load extra_file for item {}, (extra_file: {})\n", m_name, m_extra_file);
                m_extra_file_hash = proton::utils::hash(data, file_size);
                std::free(data);
            }*/
            /*std::string name = m_name;
            std::transform(name.begin(), name.end(), name.begin(), ::toupper);
            utils::replace(name, " ", "_");
            utils::replace(name, "_-_", "_");
            utils::replace(name, "-", "_");
            utils::replace(name, ":", "");
            utils::replace(name, "'", "");
            utils::replace(name, "!", "");
            utils::replace(name, "#", "");
            utils::replace(name, ".", "");
            utils::replace(name, "(", "");
            utils::replace(name, ")", "");
            if (m_id > 12653)
                fmt::print("ITEM_{} = {},\n", name, m_id);*/
        }
    };
}