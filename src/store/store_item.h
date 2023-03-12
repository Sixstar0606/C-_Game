#pragma once
#include <unordered_map>
#include <utils/text.h>

enum eStoreMenu {
    STOREMENU_TYPE_NONE,
    STOREMENU_TYPE_LIMITED_OFFERS,
    STOREMENU_TYPE_BASIC_ITEMS,
    STOREMENU_TYPE_WORLD_BUILDING,
    STOREMENU_TYPE_PACK,
    STOREMENU_TYPE_BLAST,
    STOREMENU_TYPE_GROWTOKEN
};

enum eStorePurchase {
    STOREPURCHASE_TYPE_GEM,
    STOREPURCHASE_TYPE_TOKEN
};

struct StoreItem {
    uint32_t m_icon_id;
    eStorePurchase m_buy_type;

    std::string m_name;
    std::string m_description;
    std::string m_button;

    int32_t m_price;
    std::unordered_map<uint32_t, uint8_t> m_items;
};