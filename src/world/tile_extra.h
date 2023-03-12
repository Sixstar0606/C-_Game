#pragma once
#include <string>
#include <vector>
#include <world/objects/enums.h>
#include <database/item/item_database.h>
#include <utils/random.h>
#include <utils/color.h>
#include <utils/timing_clock.h>

namespace GTServer {
    class TileExtra {
    public:
        TileExtra() : m_type{ TILEEXTRA_TYPE_NONE } {}
        ~TileExtra() = default;

    public:
        uint8_t m_type;

        std::string m_label;
        std::string m_destination;
        std::string m_door_unique_id;
        std::string m_password;
        union {
            bool m_locked;
            bool m_spliced;
        };
        union {
            uint8_t m_lock_flags;
            uint8_t m_weather_flags;
            uint8_t m_fruit_count;
            uint8_t m_random_value;
        };

        union {
            int32_t m_end_marker;
        };
        union {
            uint32_t m_owner_id;
            uint32_t m_item_id;
            uint32_t m_expression_id;
            uint32_t m_eggs_placed;
            uint32_t m_cycle_time;
        };
        union {
            int32_t m_tempo;
            int32_t m_gravity;
        };
        Color m_primary_color{ 0xFF, 0xFF, 0xFF, 0xFF };
        Color m_secondary_color{ 0xFF, 0xFF, 0xFF, 0xFF };

        std::vector<uint32_t> m_uint_array{};
        std::array<uint16_t, NUM_BODY_PARTS> m_clothes{};

        union {
            high_resolution_clock::time_point m_planted_date{};
        };

    public:
        void SetExtraType(const uint8_t& type) { m_type = type; }
        uint8_t GetExtraType() const { return m_type; }

    public:
        std::string GetLabel() const { return m_label; }
        std::string GetDestination() const { return m_destination; }
        std::string GetDoorUniqueId() const { return m_door_unique_id; }
        std::string GetPassword() const { return m_password; }
        bool IsLocked() const { return m_locked; }
        bool IsSpliced() const { return m_spliced; }
        
        uint8_t GetFruitCount() const { return m_fruit_count; }
        uint8_t GetDiceResult() const { return m_random_value; }

        int32_t GetEndMarker() const { return m_end_marker; }
        uint32_t GetOwnerId() const { return m_owner_id; }
        uint32_t GetItemId() const { return m_item_id; }
        uint32_t GetExpressionId() const { return m_expression_id; }
        uint32_t GetEggsPlaced() const { return m_eggs_placed; }
        uint32_t GetCycleTime() const { return m_cycle_time; }

        Color& GetPrimaryColor() { return m_primary_color; }
        Color& GetSecondaryColor() { return m_secondary_color; }

        std::vector<uint32_t>& GetAccessList();
        bool HasAccess(const uint32_t& uid) const;
        bool AddAccess(const uint32_t& uid);
        bool RemoveAccess(const uint32_t& uid);
        void ClearAccess();

        std::vector<uint32_t>& GetWeatherList();
        bool HasWeather(uint32_t item_id);
        bool AddWeather(uint32_t item_id);
        bool EraseWeather(uint32_t item_id);
        void ClearWeather();

        void SetCloth(const uint8_t& body_part, const uint16_t& id) {
            if (body_part < 0 || body_part > NUM_BODY_PARTS)
                return;
            this->m_clothes[body_part] = id;
        }
        uint16_t& GetCloth(const uint8_t& body_part) { return this->m_clothes[body_part]; }
        std::array<uint16_t, NUM_BODY_PARTS>& GetClothes() { return this->m_clothes; }

        int32_t GetTempo() const { return m_tempo; }
        int32_t GetGravity() const { return m_gravity; }

        high_resolution_clock::time_point GetPlantedDate() const { return m_planted_date; }

    public:
        uint8_t GetLockFlags() const { return m_lock_flags; }
        bool IsLockFlagOn(const eLockFlags& flag) const;
        void SetLockFlag(const eLockFlags& flag);
        void RemoveLockFlag(const eLockFlags& flag);

        uint8_t GetWeatherFlags() const { return m_weather_flags; }
        
        void set_door_data(std::string label, bool locked, std::string dest = "", std::string id = "", std::string pass = "") {
            if (this->GetExtraType() != TILEEXTRA_TYPE_DOOR)
                this->SetExtraType(TILEEXTRA_TYPE_DOOR);
            if (label.empty())
                label = dest;
            if (dest.find(":") != std::string::npos) {
                if (label.empty())
                    label = "...";
            }
            
            this->m_label = label;
            this->m_locked = locked;

            this->m_destination = dest;
            this->m_door_unique_id = id;
            this->m_password = pass;
        }
        void set_sign_data(const std::string& label, const int32_t& end_marker) {
            if (this->GetExtraType() != TILEEXTRA_TYPE_SIGN)
                this->SetExtraType(TILEEXTRA_TYPE_SIGN);
            this->m_label = label;
            this->m_end_marker =  end_marker;
        }
        void set_seed_data(const uint16_t& seed_id) {
            if (this->GetExtraType() != TILEEXTRA_TYPE_SEED)
                this->SetExtraType(TILEEXTRA_TYPE_SEED);
            ItemInfo* item = ItemDatabase::GetItem(seed_id);
            uint64_t grow_time = ItemDatabase::GetItem(seed_id + 1)->m_grow_time / 2;
            static randutils::pcg_rng gen{ utils::random::get_generator_local() };

            if (item->m_id == ITEM_LEGENDARY_WIZARD)
                m_fruit_count = 1;
            else
                m_fruit_count = gen.uniform(1, 4);
            m_spliced = false;
            m_planted_date = high_resolution_clock::now() - std::chrono::seconds(grow_time);
        }
    };
}