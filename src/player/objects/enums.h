#pragma once

namespace GTServer {
    enum ePlatformType {
        PLATFORM_ID_UNKNOWN = -1,
        PLATFORM_ID_WINDOWS,
        PLATFORM_ID_IOS,
        PLATFORM_ID_OSX,
        PLATFORM_ID_LINUX,
        PLATFORM_ID_ANDROID,
        PLATFORM_ID_WINDOWS_MOBILE,
        PLATFORM_ID_WEBOS,
        PLATFORM_ID_BBX,
        PLATFORM_ID_FLASH,
        PLATFORM_ID_HTML5,
        PLATFORM_MAXVAL
    };
    enum ePlayerFlags {
        PLAYERFLAG_LOGGED_ON = (1 << 0),
        PLAYERFLAG_IS_IN = (1 << 1),
        PLAYERFLAG_UPDATING_ITEMS = (1 << 2),
        PLAYERFLAG_UPDATING_TRIBUTE = (1 << 3),
        PLAYERFLAG_IS_FACING_LEFT = (1 << 4),
        PLAYERFLAG_IS_INVISIBLE = (1 << 5),
        PLAYERFLAG_IS_GHOST = (1 << 8),
        PLAYERFLAG_IS_NICK = (1 << 9),
        PLAYERFLAG_IS_BAN = (1 << 10),
        PLAYERFLAG_IS_MOD = (1 << 6),
        PLAYERFLAG_IS_SUPER_MOD = (1 << 7)
    };
    enum ePlayerData {
        PLAYER_DATA_INVENTORY,
        PLAYER_DATA_CLOTHES,
        PLAYER_DATA_PLAYMODS,
        PLAYER_DATA_CHARACTER_STATE
    };
}