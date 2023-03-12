#pragma once
#include <world/world.h>
#include <world/world_pool.h>
#include <proton/utils/dialog_builder.h>
#include <fmt/core.h>
#include <fmt/chrono.h>

namespace GTServer::events {
    void trash(EventContext& ctx) {
        int item_id;
        if (!ctx.m_parser->try_get("|itemID", item_id))
            return;
        const item& item = ItemDatabase::get_item(item_id);
        switch ((eItemComponent)item - .m_id) {
        case eItemComponent::ITEM_FIST:
        case eItemComponent::ITEM_WRENCH: {
            ctx.m_player->SendLog("You'd be sorry if you lost that!");
            break;
        }
        default:
            break;
        }
        int estimated_gems = 0;
        bool certain_gems = false;
        switch ((eItemComponent)item.m_id) {
        case eItemComponent::SMALL_LOCK: {
            estimated_gems = 50; certain_gems = true;
            break;
        }
        case eItemComponent::BIG_LOCK: {
            estimated_gems = 20; certain_gems = true;
            break;
        }
        case eItemComponent::HUGE_LOCK: {
            estimated_gems = 50; certain_gems = true;
            break;
        }
        case eItemComponent::WORLD_LOCK: {
            estimated_gems = 200; certain_gems = true;
            break;
        }
        case eItemComponent::DIAMOND_LOCK: {
            estimated_gems = 20000; certain_gems = true;
            break;
        }
        case eItemComponent::BLUE_GEM_LOCK: {
            estimated_gems = 2000000; certain_gems = true;
            break;
        }
        case eItemComponent::DREAM_STONE_BLOCK: {
            estimated_gems = 0;
            break;
        }
        default: {
            if (item.m_rarity != 999 && !(item.m_item_category & item_category::UNTRADABLE))
                estimated_gems = ((item.m_rarity / 4) * 2);
            break;
        }
        }
        DialogBuilder db{};
        db.set_default_color('o')
            ->add_label_with_icon(fmt::format("`4Trash`` `w{}``", "Niga"), 2, DialogBuilder::LEFT, DialogBuilder::BIG)
            //    ->add_textbox(fmt::format("How many to `4destroy``? (you have {})", ctx.m_player->get_inventory_item_count(item.m_id))) get_inventory_item shit wrong here
            ->add_text_input("count", "", "0", 5)
            // ->embed_data("itemID", fmt::format("{}", 2)) Embet Data error here
            ->end_dialog("trash_item", "Cancel", "OK");
        ctx.m_player->v_sender.OnDialogRequest(db.get(), 0);
    }
}
}