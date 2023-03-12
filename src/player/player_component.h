#pragma once
#include <string>
#include <proton/utils/common.h>
#include <utils/timing_clock.h>

#define MAX_PLAYER_OUTFITS 5

namespace GTServer {
    class PlayerComponent {
    public:
        PlayerComponent() = default;
        ~PlayerComponent() = default;

        CL_Vec2i get_access_offer() const { return m_access_offer; }
        void set_access_offer(const CL_Vec2i& tile) { m_access_offer = { tile.m_x, tile.m_y }; }
        CL_Vec2i get_respawn_pos() const { return m_respawn_pos; }
        void set_respawn_pos(const CL_Vec2i& pos) { m_respawn_pos = { pos.m_x, pos.m_y }; }

        void set_experience(const int32_t& exp, const int32_t& level) { m_experience = { exp, level }; }
        [[nodiscard]] std::pair<int32_t, int32_t>& get_experience() { return m_experience; }
        
        void set_last_active(const system_clock::time_point& time) { m_last_active = time; }
        system_clock::time_point get_last_active() { return m_last_active; }
        
    private:
        CL_Vec2i m_access_offer = CL_Vec2i{ -1, -1 };
        CL_Vec2i m_respawn_pos = CL_Vec2i{ -1, -1 };

        std::pair<int32_t, int32_t> m_experience = std::pair<int32_t, int32_t>{ 0, 1 };

    public:
        system_clock::time_point m_last_active;
        TimingClock m_respawn_time = TimingClock{ std::chrono::seconds(2) };

        long long int m_packet_sec = 0;
        int m_packet_in_sec = 0;

    public:
        struct ReceiveMessage {
            int32_t m_user_id   { -1 };
            std::string m_world { "" };

            int32_t GetUserId() const { return m_user_id; }
            void SetUserId(int32_t id) { m_user_id = id;  }

            std::string GetWorld() const { return m_world; }
            void SetWorld(std::string world) { m_world = world; }
        } m_receive_message;
        ReceiveMessage& GetReceiveMessage() { return m_receive_message; }
    };
}