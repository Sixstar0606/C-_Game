#pragma once
#include <chrono>
#include <fmt/core.h>

namespace GTServer {
    using high_resolution_clock = std::chrono::high_resolution_clock;
    using system_clock = std::chrono::system_clock;
    using steady_clock = std::chrono::steady_clock;
    using time_point =  system_clock::time_point;

    class XYZClock {
    public:
        XYZClock() : m_time(system_clock::now()) {}
        XYZClock(const system_clock::time_point& tp) : m_time(tp) {}

        system_clock::time_point& get_time_point() { return m_time; }
        long get_days_passed() noexcept {
            return std::chrono::duration_cast<std::chrono::hours>(system_clock::now() - m_time).count() / 24;
        }
        int get_hours_passed() noexcept {
           return std::chrono::duration_cast<std::chrono::hours>(system_clock::now() - m_time).count();
        }

    public:
        static std::string to_string(const std::chrono::seconds& time) noexcept {
            static int day_count = 24 * 3600;
            static int hour_count = 3600;
            static int min_count = 60;

            int n = time.count();

            int day = n / day_count;
            n %= day_count;
            int hour = n / hour_count;
            n %= hour_count;
            int min = n / min_count;
            n %= min_count;
            int sec = n;

            std::string result;
            if (day != 0)
                result.append(fmt::format("{} days, ", day));
            if (hour != 0)
                result.append(fmt::format("{} hours, ", hour));
            if (min != 0)
                result.append(fmt::format("{} mins", min));
            return result;
        }
    private:
        system_clock::time_point m_time;
    };

    class TimingClock {
    public:
        TimingClock() : m_time{ steady_clock::now() }, m_timeout{ std::chrono::seconds(0) } {}
        TimingClock(const int32_t& timeout) : m_time{ steady_clock::now() }, m_timeout{ std::chrono::seconds(timeout) } {}
        TimingClock(const std::chrono::seconds& timeout) : m_time{ steady_clock::now() }, m_timeout{ timeout } {}
        TimingClock(const steady_clock::time_point& time, const std::chrono::seconds& timeout) : m_time{ time }, m_timeout{ timeout } {}

        void UpdateTime() { m_time = steady_clock::now(); }
        void UpdateTime(const steady_clock::time_point& time) { m_time = time; }
        void UpdateTimeout(const int32_t& timeout) { m_timeout = std::chrono::seconds{ timeout }; }
        void UpdateTimeout(const std::chrono::seconds& timeout) { m_timeout = timeout; }

        [[nodiscard]] steady_clock::time_point GetTime() const { return m_time; }
        [[nodiscard]] std::chrono::seconds GetTimeout() const { return m_timeout; }
        [[nodiscard]] std::chrono::seconds GetPassedTime() const { return std::chrono::duration_cast<std::chrono::seconds>(steady_clock::now() - m_time); }
        
    private:
        steady_clock::time_point m_time{ steady_clock::now() };
        std::chrono::seconds m_timeout{};
    };
}