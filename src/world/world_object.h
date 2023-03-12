#pragma once
#include <proton/utils/common.h>

namespace GTServer {
    enum eObjectChangeType {
        OBJECT_CHANGE_TYPE_COLLECT = 0,
        OBJECT_CHANGE_TYPE_ADD = -1,
        OBJECT_CHANGE_TYPE_REMOVE = -2,
        OBJECT_CHANGE_TYPE_MODIFY = -3,
    };

    struct WorldObject {
        uint16_t m_item_id = 0;
        uint8_t m_flags = 0;
        uint8_t m_item_amount = 0;
        
        CL_Vec2f m_pos = { 0, 0 };
    };
}