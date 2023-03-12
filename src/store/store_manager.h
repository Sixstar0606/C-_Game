#pragma once
#include <vector>
#include <store/store_item.h>

namespace GTServer {
    class Player;
    class StoreManager {
    public:
        StoreManager() = default;
        ~StoreManager();
        
        static bool Init() { return Get().init__interface(); }
        static void Send(std::shared_ptr<Player>& player, const eStoreMenu& type) { Get().send__interface(player, type); }

        static std::unordered_map<eStoreMenu, std::vector<StoreItem>> GetItems() { return Get().m_items; }
        static std::string GetCategoryAsString(const eStoreMenu& type) {
            switch (type) {
            case STOREMENU_TYPE_LIMITED_OFFERS: return "`5Limited Offers!``";
            case STOREMENU_TYPE_BASIC_ITEMS: return "`oBasic Items``";
            case STOREMENU_TYPE_WORLD_BUILDING: return "`oWorld Building``";
            case STOREMENU_TYPE_PACK: return "`oItem Packs``";
            case STOREMENU_TYPE_BLAST: return "`oBlasts``";
            case STOREMENU_TYPE_GROWTOKEN: return "`oGrowtoken``";
            default:
                return "";
            }
        }
    public:
        static StoreManager& Get() { static StoreManager ret; return ret; }

    private:
        bool init__interface();
        void send__interface(std::shared_ptr<Player>& player, const eStoreMenu& type);

    private:
        std::unordered_map<eStoreMenu, std::vector<StoreItem>> m_items{};
    };
}