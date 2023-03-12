#pragma once
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <database/item/item_info.h>
#include <database/item/item_collision.h>
#include <database/item/item_component.h>
#include <proton/packet.h>

namespace GTServer
{
    enum eRewardType {
        REWARD_TYPE_PROVIDER
    };
    class ItemDatabase
    {
    public:
        ItemDatabase() = default;
        ~ItemDatabase();
        
        bool Serialize();
        void SerializeDetails();
        void Kill();
        void Encode();

        void ModifyIOSSupport();

        static uint32_t GetHash() { return Get().m_hash; }
        static GameUpdatePacket* GetPacket() { return Get().m_update_packet; }
        static std::vector<ItemInfo*> GetItems() { return Get().m_items; }

        static ItemInfo* GetItem(const uint32_t& item) { return Get().get_item__interface(item); }
        static ItemInfo* GetItemByName(std::string name) { return Get().get_item_by_name__interface(name); }

        static std::vector<std::pair<uint32_t, uint8_t>> GetRewards(eRewardType type, const uint32_t& base) {
            switch (type) {
            case REWARD_TYPE_PROVIDER: {
                auto provider_array = Get().m_provider_rewards;
                if (provider_array.find(base) == provider_array.end())
                    return std::vector<std::pair<uint32_t, uint8_t>>{};
                return provider_array[base];
            }
            default:
                return std::vector<std::pair<uint32_t, uint8_t>>{};
            }
        }
    public:
        static ItemDatabase& Get() { static ItemDatabase ret; return ret; }

    public:
        ItemInfo* get_item__interface(const uint32_t& item);
        ItemInfo* get_item_by_name__interface(std::string name);

        void AddReward(eRewardType type, const uint32_t& base, std::vector<std::pair<uint32_t, uint8_t>> rewards) { 
            switch (type) {
            case REWARD_TYPE_PROVIDER: {
                m_provider_rewards.insert_or_assign(base, rewards);
            } break;
            default:
                break;
            }
        }

    private:
        std::size_t m_size;
        uint8_t* m_data;

        uint32_t m_hash;
        uint16_t m_version;
        uint32_t m_item_count;

        GameUpdatePacket* m_update_packet;

    private:

        std::vector<ItemInfo*> m_items;
        std::unordered_map<uint32_t, std::vector<std::pair<uint32_t, uint8_t>>> m_provider_rewards;
    };
}