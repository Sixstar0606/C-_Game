#include <algorithm>
#include <event/event_pool.h>
#include <fmt/core.h>
#include <utils/text.h>

#include <event/tank_events/OnItemActiveObjectRequest.h>
#include <event/tank_events/OnItemActiveRequest.h>
#include <event/tank_events/OnMovement.h>
#include <event/tank_events/OnSetIconState.h>
#include <event/tank_events/OnTileActiveRequest.h>
#include <event/tank_events/OnTileChangeRequest.h>

#include <event/text_events/action.h>
#include <event/text_events/requested_name.h>
#include <event/text_events/tank_id_name.h>

#include <event/text_events/action/dialog_return.h>
#include <event/text_events/action/drop.h>
#include <event/text_events/action/enter_game.h>
#include <event/text_events/action/friends.h>
#include <event/text_events/action/get_dr_animations.h>
#include <event/text_events/action/growid.h>
#include <event/text_events/action/input.h>
#include <event/text_events/action/join_request.h>
#include <event/text_events/action/quit_to_exit.h>
#include <event/text_events/action/quit.h>
#include <event/text_events/action/refresh_item_data.h>
#include <event/text_events/action/refresh_player_tribute_data.h>
#include <event/text_events/action/respawn_spike.h>
#include <event/text_events/action/respawn.h>
#include <event/text_events/action/store.h>
#include <event/text_events/action/wrench.h>

namespace GTServer {
    EventPool::EventPool() {
        fmt::print("Initializing EventPool\n");
    }
    EventPool::~EventPool() {
        this->unload_events();
    }

    void EventPool::load_events() {
        reg_generic("requestedName", events::requested_name);
        reg_generic("tankIDName", events::tank_id_name);
        reg_generic("action", events::action);

        reg_action("dialog_return", events::dialog_return);
        reg_action("drop", events::drop);
        reg_action("enter_game", events::enter_game);
        reg_action("friends", events::friends);
        reg_action("getDRAnimations", events::get_dr_animations);
        reg_action("growid", events::growid);
        reg_action("input", events::input);
        reg_action("join_request", events::join_request);
        reg_action("quit_to_exit", events::quit_to_exit);
        reg_action("quit", events::quit);
        reg_action("refresh_item_data", events::refresh_item_data);
        reg_action("refresh_player_tribute_data", events::refresh_player_tribute_data);
        reg_action("respawn_spike", events::respawn_spike);
        reg_action("respawn_lava", events::respawn_lava);
        reg_action("respawn", events::respawn);
        reg_action("store", events::store);
        reg_action("wrench", events::wrench);

        reg_packet(NET_GAME_PACKET_STATE, events::OnMovement);
        reg_packet(NET_GAME_PACKET_TILE_CHANGE_REQUEST, events::OnTileChangeRequest);
        reg_packet(NET_GAME_PACKET_TILE_ACTIVATE_REQUEST, events::OnTileActiveRequest);
        reg_packet(NET_GAME_PACKET_ITEM_ACTIVATE_REQUEST, events::OnItemActiveRequest);
        reg_packet(NET_GAME_PACKET_ITEM_ACTIVATE_OBJECT_REQUEST, events::OnItemActiveObjectRequest);
        reg_packet(NET_GAME_PACKET_SET_ICON_STATE, events::OnSetIconState);
        
        fmt::print(" - registered events, {} genetric texts {} actions {} game packets\n",
            this->get_registered_event(EVENT_TYPE_GENERIC_TEXT),
            this->get_registered_event(EVENT_TYPE_ACTION),
            this->get_registered_event(EVENT_TYPE_GAME_PACKET));
    }
    void EventPool::unload_events() {
        m_events.clear();
    }

    void EventPool::reg_generic(const std::string& ev, std::function<void(EventContext&)> fn) {
        m_events[EVENT_TYPE_GENERIC_TEXT].push_back({ utils::quick_hash(ev), fn });
    }
    void EventPool::reg_action(const std::string& ev, std::function<void(EventContext&)> fn) {
         m_events[EVENT_TYPE_ACTION].push_back({ utils::quick_hash(ev), fn });
    }
    void EventPool::reg_packet(const uint8_t& ev, std::function<void(EventContext&)> fn) {
         m_events[EVENT_TYPE_GAME_PACKET].push_back({ utils::quick_hash("gup_" + std::to_string(ev)), fn });
    }

    std::size_t EventPool::get_registered_event(const eEventType& type) const {
        if (auto it = m_events.find(type); it == m_events.end())
            return std::size_t{ 0 };
        return m_events.at(type).size();
    }
}