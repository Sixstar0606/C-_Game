#pragma once

namespace GTServer {
    enum eItemFlag1 {
        ITEMFLAG1_FLIPPED = 1 << 0,
        ITEMFLAG1_EDITABLE = 1 << 1,
        ITEMFLAG1_SEEDLESS = 1 << 2,
        ITEMFLAG1_PERMANENT = 1 << 3,
        ITEMFLAG1_DROPLESS = 1 << 4,
        ITEMFLAG1_NOSELF = 1 << 5,
        ITEMFLAG1_NOSHADOW = 1 << 6,
        ITEMFLAG1_WORLD_LOCK = 1 << 7
    };

    enum eItemFlag2 {
        ITEMFLAG2_BETA = 1 << 0,
        ITEMFLAG2_AUTOPICKUP = 1 << 1,
        ITEMFLAG2_MOD = 1 << 2,
        ITEMFLAG2_RANDGROW = 1 << 3,
        ITEMFLAG2_PUBLIC = 1 << 4,
        ITEMFLAG2_FOREGROUND = 1 << 5,
        ITEMFLAG2_HOLIDAY = 1 << 6,
        ITEMFLAG2_UNTRADABLE = 1 << 7,
    };

    enum eItemMods {
        ITEMMOD_DOUBLE_JUMP = (1 << 0),
        ITEMMOD_HIGH_JUMP = (1 << 1),
        ITEMMOD_FISTS_O_FURY = (1 << 2),
        ITEMMOD_ARMED_AND_DANGEROUS = (1 << 3),
        ITEMMOD_SPEEDY = (1 << 4),
        ITEMMOD_TANK_DRIVER = (1 << 5),
        ITEMMOD_ENCHANCED_DIGGING = (1 << 6),
        ITEMMOD_FIREPROOF = (1 << 7),
        ITEMMOD_SLOWFALL = (1 << 8),
        ITEMMOD_XP_BUFF = (1 << 9)
    };

}