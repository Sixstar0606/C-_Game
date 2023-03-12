#include <store/store_manager.h>
#include <fmt/core.h>
#include <magic_enum.hpp>
#include <config.h>
#include <player/player.h>
#include <database/item/item_component.h>
#include <proton/utils/dialog_builder.h>

namespace GTServer {
    StoreManager::~StoreManager() {
        m_items.clear();
    }

    bool StoreManager::init__interface() {
        m_items[STOREMENU_TYPE_LIMITED_OFFERS] = {};
        m_items[STOREMENU_TYPE_BASIC_ITEMS] = {};
        m_items[STOREMENU_TYPE_WORLD_BUILDING] = {
            {
                { ITEM_WORLD_LOCK, STOREPURCHASE_TYPE_GEM,
                    "World Lock",
                    "Become the undisputed ruler of your domain with one of these babies.  It works like a normal lock except it locks the `$entire world``!  Won't work on worlds that other people already have locks on. You can even add additional normal locks to give access to certain areas to friends. `5It's a perma-item, is never lost when destroyed.``  `wRecycles for 200 Gems.``",
                    "world_lock",
                    2000,
                    { { ITEM_WORLD_LOCK, 1 } }
                }
            }
        };
        m_items.insert_or_assign(STOREMENU_TYPE_PACK, std::vector<StoreItem> {
            StoreItem{
                ITEM_PET_LEPRECHAUN, STOREPURCHASE_TYPE_GEM,
                "St. Patrick's Pack",
                "Celebrate the greenest of holidays!",
                "stpats_pack",
                6000,
                std::unordered_map<uint32_t, uint8_t> {
                    { ITEM_ORANGE_BEARD, 1 },
                    { ITEM_LEPRECHAUN_SUIT, 1 },
                    { ITEM_LEPRECHAUN_SHOES, 1 },
                    { ITEM_GREEN_PANTS, 1 },
                    { ITEM_LEPRECHAUN_HAT, 1 },
                    { ITEM_POT_O_GOLD, 1 },
                    { ITEM_GREEN_BEER, 6 }
                }
            }
        });
        m_items[STOREMENU_TYPE_BLAST] = {};
        m_items[STOREMENU_TYPE_GROWTOKEN] = {};
        return true;
    }
    void StoreManager::send__interface(std::shared_ptr<Player>& player, const eStoreMenu& type) {
        if (type == STOREMENU_TYPE_NONE) {
            player->v_sender.OnStoreRequest("set_description_text|Welcome to the `2Growtopia Store``! Select the item you'd like more info on.`o `wWant to get `5Supporter`` status? Any Gem purchase (or `520000`` Gems earned with free `5Tapjoy`` offers) will make you one. You'll get new skin colors, the `5Recycle`` tool to convert unwanted items into Gems, and more bonuses!\nenable_tabs|1\nadd_tab_button|main_menu|Home|interface/large/btn_shop2.rttex||1|0|0|0||||-1|-1|||0|0|\nadd_tab_button|locks_menu|Locks And Stuff|interface/large/btn_shop2.rttex||0|1|0|0||||-1|-1|||0|0|\nadd_tab_button|itempack_menu|Item Packs|interface/large/btn_shop2.rttex||0|3|0|0||||-1|-1|||0|0|\nadd_tab_button|bigitems_menu|Awesome Items|interface/large/btn_shop2.rttex||0|4|0|0||||-1|-1|||0|0|\nadd_tab_button|weather_menu|Weather Machines|interface/large/btn_shop2.rttex|Tired of the same sunny sky?  We offer alternatives within...|0|5|0|0||||-1|-1|||0|0|\nadd_tab_button|token_menu|Growtoken Items|interface/large/btn_shop2.rttex||0|2|0|0||||-1|-1|||0|0|\nadd_banner|interface/large/gui_shop_featured_header.rttex|0|1|\nadd_image_button|image_button|interface/large/salesman.rttex|bannerlayout|OPENDIALOG|warp_salesman|\nadd_button|dailyquests_dummy||interface/large/store_buttons/store_gtps.rttex|OPENDIALOG&handle_daily_quest|1|6|0|0|||-1|-1||-1|-1||1|||||`9**`` Claim Reward `9**``|0|0|\nadd_button|9788|Tech Fleece - Gray|interface/large/store_buttons/store_gtps.rttex|`2You Get:`` 1 Tech Fleece Hoodie - Gray, 1 Tech Fleece Joggers - Gray, 1 Air Forces - White <CR><CR>`5Description:`` Awesome Gray - Tech Fleece Pack!``|1|11|50000|0|||-1|-1||-1|-1||1||||||0|\nadd_button|9794|Tech Fleece - Black|interface/large/store_buttons/store_gtps.rttex|`2You Get:`` 1 Tech Fleece Hoodie - Black, 1 Tech Fleece Joggers - Black, 1 Air Forces - Black <CR><CR>`5Description:`` Awesome Black - Tech Fleece Pack!``|1|12|50000|0|||-1|-1||-1|-1||1||||||0|\nadd_button|9808|`oJinx Intergalactic Hair``|interface/large/store_buttons/token_gtps.rttex|`2You Get:`` 1 Jinx Intergalactic Hair.<CR><CR>`5Description:`` A manic and impulsive criminal from Zaun Crest, Jinx Jinx lives to wreak havoc without care for the consequences. With an arsenal of deadly weapons, she unleashes the loudest blasts and brightest explosions to leave a trail of mayhem and panic in her wake. Note: The intergalactic hair is `#UNTRADEABLE``.|0|3|-100|0|||-1|-1||-1|-1||1||||||0|0|\nadd_button|9804|`oNightmare Eyes``|interface/large/store_buttons/token_gtps.rttex|`2You Get:`` 1 Nightmare Eyes.<CR><CR>`5Description:`` This `#Untradeable`` item lets you shoot fire from your eyes! With these Nightmare Eyes, it'll burn everything in its sight.|0|0|-150|0|||-1|-1||-1|-1||1||||||0|0|\nadd_button|9802|`oHell Demonic Scythe``|interface/large/store_buttons/token_gtps.rttex|`2You Get:`` 1 Hell Demonic Scythe.<CR><CR>`5Description:`` This `#Untradeable`` scythe contains the fury of the hell!|0|1|-200|0|||-1|-1||-1|-1||1||||||0|0|\nadd_button|9806|`oDark Chimera Wings``|interface/large/store_buttons/token_gtps.rttex|`2You Get:`` 1 Dark Chimera Wings.<CR><CR>`5Description:`` Forged from bladed feathers of flame and fury, these wings will let you tear through the skies with the might of the chimera! Note: The dark chimera wings are `#UNTRADEABLE``.|0|2|-200|0|||-1|-1||-1|-1||1||||||0|0|\nadd_button|12642|`oGrowtronic Halo``|interface/large/store_buttons/store_buttons37.rttex|`2You Get:`` 1 Growtronic Halo. <CR><CR>`5Description:``  Growtronic!|0|2|750000|0|||-1|-1||-1|-1||1||||||0|\nadd_button|12600|`oUltra World Spray``|interface/large/store_buttons/store_buttons39.rttex|`2You Get:`` 1 Ultra World Spray.<CR><CR>`5Description:`` A shocking display of power!``|0|1|500000|0|||-1|-1|interface/large/gui_shop_buybanner2.rttex|2|5||1||||||0|0|\n\nadd_banner|interface/large/gui_shop_featured_header.rttex|0|0|\nadd_big_banner|interface/large/gui_store_iap_message.rttex|0|0|`0Soon can purchase`` `2Premium BetterGrowtopia Locks`` `0we accept`` `2Real Growtopia`` `0payment, type`` `2/deposit`` `0to see more information or type`` `2/shop`` `0to spend premium locks``|\nadd_button|9999wl|`o9,999 Premium BetterGrowtopia Locks``|interface/large/store_buttons/store_gtps.rttex|https://youtube.com|0|13|0|0|$ USD 59.99||-1|-1||0|0|`4We have automatic delivery, don't need to wait!``<CR><CR>`2You Get:`` `#9,999 Premium BetterGrowtopia Locks``, `#19 Growtoken`` and `#15 Megaphone``.<CR><CR>`5Description:`` Get 9,999 Premium BetterGrowtopia Locks. These premium locks can be spent in the BetterGrowtopia Shop type `9/shop`` to see more!|1||||||0|0|\nadd_button|5000wl|`o5000 Premium BetterGrowtopia Locks``|interface/large/store_buttons/store_gtps.rttex|https://youtube.com|0|6|0|0|$ USD 32.99||-1|-1||0|0|`4We have automatic delivery, don't need to wait!``<CR><CR>`2You Get:`` `#5000 Premium BetterGrowtopia Locks`` and `#10 Growtoken``.<CR><CR>`5Description:`` Get 5000 Premium BetterGrowtopia Locks. These Premium BetterGrowtopia Locks can be spent in GTPS Shop type `9/shop`` to see more!|1||||||0|0|\nadd_button|1500wl|`o1500 Premium BetterGrowtopia Locks``|interface/large/store_buttons/store_gtps.rttex|https://youtube.com|0|5|0|0|$ USD 14.99||-1|-1||0|0|`4We have automatic delivery, don't need to wait!``<CR><CR>`2You Get:`` `#1500 Premium BetterGrowtopia Locks`` and `#3 Growtoken``.<CR><CR>`5Description:`` Get 1500 Premium BetterGrowtopia Locks. These premium world locks can be spent in the BetterGrowtopia Shop type `9/shop`` to see more!|1||||||0|0|\nadd_button|500wl|`o500 Premium World Locks``|interface/large/store_buttons/store_gtps.rttex|https://youtube.com|0|4|0|0|$ USD 4.99||-1|-1||0|0|`4We have automatic delivery, don't need to wait!``<CR><CR>`2You Get:`` `#500 Premium World Locks`` and `#1 Growtoken``.<CR><CR>`5Description:`` Get 500 Premium World Locks. These premium world locks can be spent in the BetterGrowtopia Shop type `9/shop`` to see more!|1||||||0|0|\nadd_button|100wl|`o100 Premium BetterGrowtopia Locks``|interface/large/store_buttons/store_gtps.rttex|https://youtube.com|0|3|0|0|$ USD 1.99||-1|-1||0|0|`4We have automatic delivery, don't need to wait!``<CR><CR>`2You Get:`` `#100 Premium World Locks``.<CR><CR>`5Description:`` Get 100 Premium World Locks. These premium world locks can be spent in the BetterGrowtopia Shop type `9/shop`` to see more!|1||||||0|0|\nadd_button|custom_item|`oCreate Custom Item``|interface/large/store_buttons/store_gtps.rttex|https://youtube.com|0|7|0|0|$ USD 24.99||-1|-1||0|0|`4It takes up to 12 hours to create your custom item!``<CR><CR>`2You Get:`` Your own custom item.<CR><CR>`5Description:`` Get Your own custom item. Item will be created in less then 12 hours.<CR><CR>You can decide `9item name``,`9description``,`9playmods``,`9everything about the item you create you decide!``|1||||||0|0|\nadd_button|rt_grope_battlepass_bundle01|Royal Grow Pass|interface/large/store_buttons/store_buttons37.rttex|https://youtube.com/|4|2|0||||-1|-1||-1|-1|`2You Get:`` 1 Royal Grow Pass Token.<CR><CR>`5Description:`` Play to earn points and level up your Grow Pass to earn rewards. Consume to earn exclusive `5Royal`` rewards as you level up your Grow Pass as well as unlocking all daily bonuses and exclusive `5Royal Perks`` for the entire month.|1||||||0|0|\nend_dialog|store_request|`wCancel||");
            return;
        }
    }
}