#include <world/tile_extra.h>

namespace GTServer {
    bool TileExtra::IsLockFlagOn(const eLockFlags& flag) const {
        if (m_lock_flags & static_cast<uint8_t>(flag))
            return true;
        return false;
    }
    void TileExtra::SetLockFlag(const eLockFlags& flag) {
        m_lock_flags |= flag;
    }
    void TileExtra::RemoveLockFlag(const eLockFlags& flag) {
        m_lock_flags &= ~flag;
    }

    std::vector<uint32_t>& TileExtra::GetAccessList() { 
        return m_uint_array; 
    }
    bool TileExtra::HasAccess(const uint32_t& uid) const {
        return std::find(m_uint_array.begin(), m_uint_array.end(), uid) != m_uint_array.end();
    }
    bool TileExtra::AddAccess(const uint32_t& uid) {
        if (std::find(m_uint_array.begin(), m_uint_array.end(), uid) != m_uint_array.end())
            return false;
        m_uint_array.push_back(uid);
        return true;
    }
    bool TileExtra::RemoveAccess(const uint32_t& uid) {
        auto iterator = std::find(m_uint_array.begin(), m_uint_array.end(), uid);
        m_uint_array.erase(iterator);
        return true;
    }
    void TileExtra::ClearAccess() {
        m_uint_array.clear();
    }

    std::vector<uint32_t>& TileExtra::GetWeatherList() {
        return m_uint_array;
    }
    bool TileExtra::HasWeather(uint32_t item_id) {
        return std::find(m_uint_array.begin(), m_uint_array.end(), item_id) != m_uint_array.end();
    }
    bool TileExtra::AddWeather(uint32_t item_id) {
        if (std::find(m_uint_array.begin(), m_uint_array.end(), item_id) != m_uint_array.end())
            return false;
        m_uint_array.push_back(item_id);
        return true;
    }
    bool TileExtra::EraseWeather(uint32_t item_id) {
        auto iterator = std::find(m_uint_array.begin(), m_uint_array.end(), item_id);
        if (iterator == m_uint_array.end())
            return false;
        iterator = m_uint_array.erase(iterator);
        return true;
    }
    void TileExtra::ClearWeather() {
        m_uint_array.clear();
    }
}