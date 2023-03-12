#pragma once
#include <world/world.h>
#include <world/world_pool.h>
#include <database/item/item_database.h>
#include <proton/utils/dialog_builder.h>

namespace GTServer::events {
    void drop(EventContext& ctx) {
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        auto splited_text = utils::split(ctx.m_parser.get_data()[1], "|itemID|");
        if (splited_text.empty() || splited_text.size() < 2)
            return;
        ItemInfo* item = ItemDatabase::GetItem(std::atoi(splited_text[1].c_str()));
        if (!item)
            return;
        if (!ctx.m_player->m_inventory.Contain(item->m_id))
            return;

        if ((item->m_item_category & ITEMFLAG2_UNTRADABLE) || (item->m_item_category & ITEMFLAG2_MOD)) {
            ctx.m_player->v_sender.OnTextOverlay("`wYou can't drop that.``");
            return;
        }
    
        Tile* tile_next = world->GetTile(ctx.m_player->GetPosition().m_x / 32 + (ctx.m_player->IsFlagOn(PLAYERFLAG_IS_FACING_LEFT) ? -1 : 1), ctx.m_player->GetPosition().m_y / 32);
        if (!tile_next) {
            ctx.m_player->v_sender.OnTextOverlay("`wYou can't drop that here, face somewhere with open space.``");
            return;
        } 
        ItemInfo* base = tile_next->GetBaseItem();
        if (base->m_collision_type == ITEMCOLLISION_NORMAL || base->m_collision_type == ITEMCOLLISION_GUILDENTRANCE || base->m_collision_type == ITEMCOLLISION_GATEWAY) {
            ctx.m_player->v_sender.OnTextOverlay("`wYou can't drop that here, face somewhere with open space.``");
            return;
        } else if (base->m_item_type == ITEMTYPE_MAIN_DOOR) {
            ctx.m_player->v_sender.OnTextOverlay(fmt::format("`wYou can't drop items on `2{}``.``", base->m_name));
            return;
        }

        uint8_t item_amount = ctx.m_player->m_inventory.GetItemCount(item->m_id);
        static std::vector<std::string> messages = {
            "`4Warning:`` Once you drop an item, it is no longer yours. Anyone can take it from you and we CANNOT return it!",
            "`4Warning:`` If someone is asking you to drop items, they are DEFINITELY trying to scam you. Do not drop items in other players' worlds.",
            "`4Warning:`` We cannot restore items you lose because you dropped them. Do not drop items you want to keep!",
            "If you are trying to trade an item with another player, use your wrench on them instead to use our Trade System! `4Dropping items is not safe!``",
            "`4Warning:`` Any player who asks you to drop items is scamming you. We cannot restore scammed items."
        };
        DialogBuilder db{};
        db.set_default_color('o')
            ->add_label_with_icon(fmt::format("`wDrop {}``", item->m_name), item->m_id, DialogBuilder::LEFT, DialogBuilder::BIG)
            ->add_textbox("`oHow many to drop?``")
            ->add_text_input("count", "", "0", 5)
            ->embed_data("itemID", item->m_id)
            ->add_textbox(messages[std::rand() % messages.size()])
            ->end_dialog("drop_item", "`wCancel``", "`wOK``");
        ctx.m_player->v_sender.OnDialogRequest(db.get());
    }
}