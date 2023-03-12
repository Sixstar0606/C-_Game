#include <database/item/item_database.h>
#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <config.h>
#include <utils/binary_reader.h>
#include <utils/binary_writer.h>
#include <utils/file_manager.h>
#include <utils/text.h>
#include <proton/utils/misc_utils.h>

namespace GTServer {
    ItemDatabase::~ItemDatabase() {
        this->Kill();
    }

    bool ItemDatabase::Serialize() {
        if (!std::filesystem::exists("cache/items.dat") 
        || !std::filesystem::exists("utils/punch_data.dat")
        || !std::filesystem::exists("utils/items_detail.json"))
            return false;
        m_data = FileManager::read_all_bytes("cache/items.dat", m_size);
        if (!m_data)
            return false;
        m_hash = proton::utils::RTHash(m_data, m_size);

        BinaryReader br{ m_data };
        m_version = br.read<uint16_t>();
        m_item_count = br.read<uint32_t>();

        m_items.reserve(m_item_count);
        for (uint32_t i = 0; i < m_item_count; i++) {
            m_items.push_back(new ItemInfo{});
            m_items[i]->Serialize(br);

            if (i != m_items[i]->m_id) {
                fmt::print(" - unsupported items.dat version -> {}\n", m_version);
                break;
            }
        }

        m_update_packet = (GameUpdatePacket*)std::malloc(sizeof(GameUpdatePacket) + m_size);
        std::memset(m_update_packet, 0, sizeof(GameUpdatePacket) + m_size);
        m_update_packet->m_type = NET_GAME_PACKET_SEND_ITEM_DATABASE_DATA;
        m_update_packet->m_net_id = -1;
        m_update_packet->m_flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
        m_update_packet->m_data_size = static_cast<uint32_t>(m_size);
        std::memcpy(&m_update_packet->m_data, m_data, m_size);
        return true;
    }
    void ItemDatabase::SerializeDetails() {
        {
            std::ifstream file("utils/items_detail.json");
            if (!file.is_open())
                return;
            nlohmann::json j{};
            file >> j;
            file.close();

            nlohmann::json array = j["items"];
            for (auto index = 0; index < array.size(); index++) {
                ItemInfo* item = this->GetItem(array[index]["itemID"].get<uint32_t>());
                if (!item)
                    continue;
                item->m_mods = array[index]["mods"].get<uint32_t>();
                item->m_description = array[index]["description"].get<std::string>();
            }
            j.clear();
        } {
            this->AddReward(REWARD_TYPE_PROVIDER, ITEM_ATM_MACHINE, {
                { ITEM_GEMS, 200 }
            });
            this->AddReward(REWARD_TYPE_PROVIDER, ITEM_AWKWARD_FRIENDLY_UNICORN, {
                { ITEM_CLOUDS, 6 },
                { ITEM_DREAMSTONE_BLOCK, 6 },
                { ITEM_RAINBOW_BLOCK, 6 },
                { ITEM_RAINBOW_WIG, 6 },
                { ITEM_ENCHANTED_SPATULA, 6 },
                { ITEM_HAPPY_UNICORN_BLOCK, 15 },
                { ITEM_ANGRY_UNICORN_BLOCK, 15 },
                { ITEM_UNICORN_JUMPER, 30 },
                { ITEM_HORSE_MASK, 30 },
                { ITEM_SCROLL_BULLETIN, 60 },
                { ITEM_VERY_BAD_UNICORN, 1 },
                { ITEM_TEDDY_BEAR, 1 },
                { ITEM_GIFT_OF_THE_UNICORN, 1 },
                { ITEM_RETRO_LEG_WARMERS, 1 },
                { ITEM_RAINBOW_SCARF, 1 },
                { ITEM_TWINTAIL_HAIR, 1},
                { ITEM_GROWMOJI_COOL_SHADES_MASK, 1 },
                { ITEM_CARTOON_GLOVE_HAT_RED, 1 },
                { ITEM_CARTOON_GLOVE_HAT_BLUE, 1 },
                { ITEM_THAT_90S_HAIR, 1 },
                { ITEM_FREAKY_FRIED_EGG_EYES, 1 },
                { ITEM_PEAS_IN_A_POD_HAT, 1 },
                { ITEM_PET_PINK_CROCODILE, 1 }
            });
            this->AddReward(REWARD_TYPE_PROVIDER, ITEM_BENBARRAGES_AWESOME_ITEM_O_MATIC, {
                { ITEM_BENBARRAGES_RED_BLOCK, 10 },
                { ITEM_BENBARRAGES_GREEN_BLOCK, 10 },
                { ITEM_BENBARRAGES_YELLOW_BLOCK, 10 },
                { ITEM_BENBARRAGES_BLUE_BLOCK, 10 },
                { ITEM_BENBARRAGES_RUBY_BLOCK, 10 }
            });
            this->AddReward(REWARD_TYPE_PROVIDER, ITEM_BUFFALO, {
                { ITEM_MILK, 2 }
            });
            this->AddReward(REWARD_TYPE_PROVIDER, ITEM_CHICKEN, {
                { ITEM_EGG, 2 }
            });
            this->AddReward(REWARD_TYPE_PROVIDER, ITEM_COFFEE_MAKER, {
                { ITEM_COFFEE, 1 }
            });
        }
    }
    void ItemDatabase::Kill() {
        for (auto item : m_items) {
            if (!item)
                continue;
            delete item;
            item = nullptr;
        }
        m_items.clear();
        m_provider_rewards.clear();
        // std::free(m_data);
    }
    void ItemDatabase::Encode() {
        std::size_t alloc = 6;
        for (ItemInfo* item : m_items)
            alloc += item->GetMemoryUsage() + 20;

        BinaryWriter buffer{ alloc };
        buffer.write<uint16_t>(this->m_version);
        buffer.write<uint32_t>(this->m_item_count);
        for (ItemInfo* item : m_items)
            item->Pack(buffer);

        FileManager::write_all_bytes("cache/items.dat", reinterpret_cast<char*>(buffer.get()), buffer.get_pos());
    }

    void ItemDatabase::ModifyIOSSupport() {
        for (ItemInfo* item : m_items) {
            if (item->m_extra_file.find(".mp3") == std::string::npos || item->m_extra_file.find("audio/mp3/") == std::string::npos)
                continue;
            { 
                std::size_t start_pos = item->m_extra_file.find("audio/mp3/");
                if (start_pos != std::string::npos)
                    item->m_extra_file.replace(start_pos, 10, "audio/ogg/");
            } {   
                std::size_t start_pos = item->m_extra_file.find(".mp3");
                if (start_pos != std::string::npos)
                    item->m_extra_file.replace(start_pos, 4, ".ogg");
            }
        }
    }

    ItemInfo* ItemDatabase::get_item__interface(const uint32_t& item) {
        if (item < ITEM_BLANK || item > m_items.size())
            return nullptr;
        return m_items[item];
    }
    ItemInfo* ItemDatabase::get_item_by_name__interface(std::string name) {
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });

        for (auto item : this->GetItems()) {
            std::string item_name = item->m_name;
            std::transform(item_name.begin(), item_name.end(), item_name.begin(), [](unsigned char c) { return std::tolower(c); });
            if (item_name != name)
                continue;
            return item;
        }
        return nullptr;
    }
}