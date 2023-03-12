#pragma once
#include <database/item/item_info.h>

namespace GTServer {
    class Player;
    class World;
    enum eQueueType {
        QUEUE_TYPE_NONE,
        QUEUE_TYPE_FINDING_ITEMS,
        QUEUE_TYPE_GET_FRIENDS,
        QUEUE_TYPE_FINDING_PLAYERS,
        QUEUE_TYPE_RENDER_WORLD,
        QUEUE_TYPE_ACCOUNT_VERIFICATION,
        SERVERQUEUE_TYPE_LOGIN,
        NUM_QUEUE_TYPES
    };
    
    struct ServerQueue {
        eQueueType m_queue_type{ QUEUE_TYPE_NONE };
       
        std::string m_keyword{};
        std::shared_ptr<Player> m_player = nullptr;
        std::shared_ptr<World> m_world = nullptr;
    };
}