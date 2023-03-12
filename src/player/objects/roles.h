#pragma once
#include <string>
#include <unordered_map>
#include <database/item/item_component.h>

namespace GTServer {
    enum ePlayerRole {
        PLAYER_ROLE_DEFAULT,
        PLAYER_ROLE_VIP,
        PLAYER_ROLE_VIP_PLUS,
        PLAYER_ROLE_MODERATOR,
        PLAYER_ROLE_ADMINISTRATOR,
        PLAYER_ROLE_MANAGER,
        PLAYER_ROLE_DEVELOPER
    };
    struct RoleInfo {
        std::string m_name;
        char m_color;
        uint16_t m_icon_id;
    };

    static std::unordered_map<int32_t, RoleInfo> g_roles_info = {
        { PLAYER_ROLE_DEFAULT, RoleInfo {
            .m_name = "Player",
            .m_color = 'w',
            .m_icon_id = ITEM_FIST
        }},
        { PLAYER_ROLE_VIP, RoleInfo {
            .m_name = "VIP",
            .m_color = '1',
            .m_icon_id = ITEM_HABANERO_CHEESE_BREAD
        }},
        { PLAYER_ROLE_VIP_PLUS, RoleInfo {
            .m_name = "VIP+",
            .m_color = '4',
            .m_icon_id = ITEM_TAMAGO_SUSHI
        }},
        { PLAYER_ROLE_MODERATOR, RoleInfo {
            .m_name = "Moderator",
            .m_color = '#',
            .m_icon_id = ITEM_DIAMOND_FLASHAXE
        }},
        { PLAYER_ROLE_ADMINISTRATOR, RoleInfo {
            .m_name = "Administrator",
            .m_color = '9',
            .m_icon_id = ITEM_GOLDEN_PICKAXE
        }},
        { PLAYER_ROLE_MANAGER, RoleInfo {
            .m_name = "Manager",
            .m_color = '6',
            .m_icon_id = ITEM_SONGPYEON
        }},
        { PLAYER_ROLE_DEVELOPER, RoleInfo {
            .m_name = "Developer",
            .m_color = '6',
            .m_icon_id = ITEM_ELDRITCH_FLAME
        }},
    };
}