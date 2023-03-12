#pragma once
#include <unordered_map>
#include <player/objects/packet_sender.h>
#include <database/item/item_database.h>
#include <utils/binary_reader.h>
#include <utils/binary_writer.h>

#define MAX_INVENTORY_SLOTS 396

namespace GTServer {
    enum eInventoryItemFlags {
        INVENTORY_ITEM_FLAGS_NONE = 0,
        INVENTORY_ITEM_FLAGS_ACTIVATED = (1 << 0)
    };

    class Inventory : public PacketSender {
    public:
        Inventory(ENetPeer* peer) : PacketSender{ peer } {}
        ~Inventory() = default;

        [[nodiscard]] uint32_t GetSize() const { return m_size; }
        [[nodiscard]] std::unordered_map<uint16_t, uint8_t> GetItems() const { return m_items; }
        
        bool IsMaxed() const { return m_items.size() + 1 > m_size; }
        bool Contain(const uint16_t& item_id) {
            return m_items.find(item_id) != m_items.end();
        }
        bool Add(const uint16_t& item_id, const uint8_t& count, const bool& send_packet = false) {
            if (count == 0)
                return false;
            if (auto it = m_items.find(item_id); it != m_items.end()) {
                const auto& item = ItemDatabase::GetItem(item_id);
                if (!item)
                    return false;
                if ((it->second + count) > item->m_max_amount)
                    return false;
                it->second += count;
            }
            else {
                if (m_items.size() >= m_size)
                    return false;
                m_items.insert_or_assign(item_id, count);
            }
            if (send_packet)
                this->Update(item_id, count, true);
            return true;
        }
        bool Erase(const uint16_t& item_id, const uint8_t& count, const bool& send_packet = false ){
            auto it = m_items.find(item_id);
            if (it == m_items.end())
                return false;
            ItemInfo* item{ ItemDatabase::GetItem(item_id) };
            if (!item)
                return false;
            if (count > item->m_max_amount || (it->second - count) <= 0)
                m_items.erase(it);
            else
                it->second -= count;
            if (send_packet)
                this->Update(item_id, count, false);
            return true;
        }
        uint8_t GetItemCount(uint16_t item) {
            if (auto it = m_items.find(item); it != m_items.end())
                return m_items[item];
            return 0;
        }

        std::vector<uint8_t> Pack() const {
            std::size_t alloc{ 8 + (4 * m_items.size()) };
            std::vector<uint8_t> ret{};
            ret.resize(alloc);
            
            BinaryWriter buffer{ ret.data() };
            buffer.write<uint8_t>(0x1); // inventory version??
            buffer.write<uint32_t>(m_size);
            buffer.write<uint16_t>(static_cast<uint16_t>(m_items.size()));

            for (const auto& [item_id, count] : m_items) {
                uint8_t flags = INVENTORY_ITEM_FLAGS_NONE; // irrelevant
                buffer.write<uint16_t>(item_id);
                buffer.write<uint8_t>(count);
                buffer.write<uint8_t>(flags);
            }
            return ret;
        }
        void Serialize(const std::vector<uint8_t>& data) {
            BinaryReader br{ data };
            br.skip(1); // inventory version??
            m_size = br.read<uint32_t>();
            const uint16_t& items{ br.read<uint16_t>() };

            for (uint16_t i = 0; i < items; i++) {
                const uint16_t item_id{ br.read<uint16_t>() };
                const uint8_t count{ br.read<uint8_t>() };
                br.skip(1); // flags irrelevant
                m_items.insert_or_assign(item_id, count);
            }
        }

        void Send() {
            auto data{ this->Pack() };
            GameUpdatePacket* update_packet{ static_cast<GameUpdatePacket*>(std::malloc(sizeof(GameUpdatePacket) + data.size())) };
            std::memset(update_packet, 0, data.size());

            update_packet->m_type = NET_GAME_PACKET_SEND_INVENTORY_STATE;
            update_packet->m_flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
            update_packet->m_data_size = static_cast<uint32_t>(data.size());
            std::memcpy(&update_packet->m_data, data.data(), data.size());
            this->SendPacket(NET_MESSAGE_GAME_PACKET, update_packet, sizeof(GameUpdatePacket) + data.size());

            data.clear();
            std::free(update_packet);
        }

    public:
        bool UpgradeBackpack() {
            if (this->GetSize() >= MAX_INVENTORY_SLOTS)
                return false;
            this->m_size += 10;
            return true;
        }
        
        bool ContainAllBuckets() {
            return (
                this->Contain(ITEM_PAINT_BUCKET_RED)
                && this->Contain(ITEM_PAINT_BUCKET_YELLOW)
                && this->Contain(ITEM_PAINT_BUCKET_GREEN)
                && this->Contain(ITEM_PAINT_BUCKET_AQUA)
                && this->Contain(ITEM_PAINT_BUCKET_BLUE)
                && this->Contain(ITEM_PAINT_BUCKET_PURPLE)
                && this->Contain(ITEM_PAINT_BUCKET_CHARCOAL)
                && this->Contain(ITEM_PAINT_BUCKET_VARNISH)
                );
        }
        bool EraseAllBuckets(uint8_t count) {
            return (
                this->Erase(ITEM_PAINT_BUCKET_RED, count, true)
                && this->Erase(ITEM_PAINT_BUCKET_YELLOW, count, true)
                && this->Erase(ITEM_PAINT_BUCKET_GREEN, count, true)
                && this->Erase(ITEM_PAINT_BUCKET_AQUA, count, true)
                && this->Erase(ITEM_PAINT_BUCKET_BLUE, count, true)
                && this->Erase(ITEM_PAINT_BUCKET_PURPLE, count, true)
                && this->Erase(ITEM_PAINT_BUCKET_CHARCOAL, count, true)
                && this->Erase(ITEM_PAINT_BUCKET_VARNISH, count, true)
                );
        }
    private:
        void Update(const uint16_t& item_id, const uint8_t& quantity, const bool& adding) {
            GameUpdatePacket update_packet{
                .m_type = NET_GAME_PACKET_MODIFY_ITEM_INVENTORY,
                .m_item_id = item_id
            };
            if (adding)
                update_packet.m_gained_item_count = quantity;
            else
                update_packet.m_lost_item_count = quantity;
            this->SendPacket(NET_MESSAGE_GAME_PACKET, &update_packet, sizeof(GameUpdatePacket));
        }
    private:
        uint32_t m_size{ 200 };
        std::unordered_map<uint16_t, uint8_t> m_items{};
    };
}
