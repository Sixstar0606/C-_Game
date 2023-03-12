#pragma once
#include <world/world.h>
#include <world/world_pool.h>

namespace GTServer::events {
    void OnItemActiveRequest(EventContext& ctx) {
        if (!ctx.m_player->IsFlagOn(PLAYERFLAG_IS_IN))
            return;
        std::shared_ptr<WorldPool> world_pool{ ctx.m_server->GetWorldPool() };
        std::shared_ptr<World> world{ world_pool->GetWorld(ctx.m_player->GetWorld()) };
        if (!world)
            return;
        if (!ctx.m_player->m_inventory.Contain(ctx.m_update_packet->m_item_id))
            return;
        ItemInfo* item{ ItemDatabase::GetItem(ctx.m_update_packet->m_item_id) };
        if (!item)
            return;
        switch (item->m_item_type) {
        case ITEMTYPE_ANCES: {
        } break;
        case ITEMTYPE_LOCK: {
            if (item->m_id == ITEM_DIAMOND_LOCK && ctx.m_player->m_inventory.Contain(ITEM_DIAMOND_LOCK) && ctx.m_player->m_inventory.GetItemCount(ITEM_WORLD_LOCK) <= 100) {
                ctx.m_player->m_inventory.Erase(item->m_id, 1, true);
                ctx.m_player->m_inventory.Add(ITEM_WORLD_LOCK, 100, true);
                ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "Yay, I got all my locks back!", true);
                return;
            }
            if (item->m_id == ITEM_BLUE_GEM_LOCK && ctx.m_player->m_inventory.Contain(ITEM_BLUE_GEM_LOCK) && ctx.m_player->m_inventory.GetItemCount(ITEM_DIAMOND_LOCK) <= 100) {
                ctx.m_player->m_inventory.Erase(item->m_id, 1, true);
                ctx.m_player->m_inventory.Add(ITEM_DIAMOND_LOCK, 100, true);
                ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "Yay, I got all my locks back!", true);
                return;
            }
            if (item->m_id == ITEM_WORLD_LOCK && ctx.m_player->m_inventory.GetItemCount(ITEM_WORLD_LOCK) >= 100 && ctx.m_player->m_inventory.GetItemCount(ITEM_DIAMOND_LOCK) < 200) {
                ctx.m_player->m_inventory.Erase(item->m_id, 100, true);
                ctx.m_player->m_inventory.Add(ITEM_DIAMOND_LOCK, 1, true);
                ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "Yay, a diamond lock!", true);
                return;
            }
            if (item->m_id == ITEM_DIAMOND_LOCK && ctx.m_player->m_inventory.GetItemCount(ITEM_DIAMOND_LOCK) >= 100 && ctx.m_player->m_inventory.GetItemCount(ITEM_BLUE_GEM_LOCK) < 200) {
                ctx.m_player->m_inventory.Erase(item->m_id, 100, true);
                ctx.m_player->m_inventory.Add(ITEM_BLUE_GEM_LOCK, 1, true);
                ctx.m_player->v_sender.OnTalkBubble(ctx.m_player->GetNetId(), "Yay, a blue gem lock!", true);
                return;
            }
        } break;
        case ITEMTYPE_CLOTHES: {
            if (item->m_clothing_type < 0 || item->m_clothing_type >= NUM_BODY_PARTS)
                return;
            ItemInfo* base{ ItemDatabase::GetItem(ctx.m_player->GetCloth(item->m_clothing_type)) };
            if (!base)
                return;
            if (base->m_id == static_cast<uint16_t>(item->m_id)) {
                ctx.m_player->SetCloth(item->m_clothing_type, ITEM_BLANK, true);
                break;
            }
            ctx.m_player->SetCloth(item->m_clothing_type, static_cast<uint16_t>(item->m_id), true);
        } break;
        default:
            break;
        }
        world->Broadcast([&](const std::shared_ptr<Player>& player) {
            player->v_sender.OnSetClothing(ctx.m_player->GetClothes(), ctx.m_player->GetSkinColor(), true, ctx.m_player->GetNetId());
            player->SendCharacterState(ctx.m_player);
        });
    }
}