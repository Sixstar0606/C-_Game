#include <render/world_render.h>
#include <filesystem>
#include <vector>
#include <fmt/core.h>
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Text.hpp>
#include <extra_dependencies/FText.h>
#include <utils/text.h>
#include <world/tile.h>
#include <world/world.h>
#include <server/server_pool.h>
#include <database/database.h>

namespace GTServer {
    WorldRender::~WorldRender() {

    }

    void WorldRender::load_caches() {
        if (sf::Texture::getMaximumSize() < 4096) {
            fmt::print("WorldRender::OnCacheInit -> failed load caches due low-end graphic drivers couldn't render at 4K\n");
            fmt::print(" |-> currently maximum size for rendering is {}px.\n", sf::Texture::getMaximumSize());
            return;
        }
        std::vector<std::string> textures_to_cache;
        std::vector<std::string> borders_to_cache;
        std::vector<std::string> weathers_to_cache;

        for (const auto& entry : std::filesystem::directory_iterator("cache/sprites")) {
            if (entry.path().extension() != ".png")
                continue;
            textures_to_cache.push_back(entry.path().string());
        }
        for (const auto& entry : std::filesystem::directory_iterator("cache/locale")) {
            if (entry.path().extension() != ".png")
                continue;
            textures_to_cache.push_back(entry.path().string());
        }
        for (const auto& entry : std::filesystem::directory_iterator("cache/borders")) {
            if (entry.path().extension() != ".png")
                continue;
            borders_to_cache.push_back(entry.path().string());
        }

        weathers_to_cache.push_back("cache/weathers/Apocalypse.png");
        weathers_to_cache.push_back("cache/weathers/Arid.png");
        weathers_to_cache.push_back("cache/weathers/Ascended Ship.png");
        weathers_to_cache.push_back("cache/weathers/Autumn.png");
        weathers_to_cache.push_back("cache/weathers/Balloon.png");
        weathers_to_cache.push_back("cache/weathers/Beach.png");
        weathers_to_cache.push_back("cache/weathers/Bountiful.png");
        weathers_to_cache.push_back("cache/weathers/Celebrity Hills.png");
        weathers_to_cache.push_back("cache/weathers/Comet.png");
        weathers_to_cache.push_back("cache/weathers/Descended Ship.png");
        weathers_to_cache.push_back("cache/weathers/Digital Rain.png");
        weathers_to_cache.push_back("cache/weathers/Epoch - Iceberg.png");
        weathers_to_cache.push_back("cache/weathers/Epoch - Lava.png");
        weathers_to_cache.push_back("cache/weathers/Epoch - Skylands.png");
        weathers_to_cache.push_back("cache/weathers/Frozen Cliffs.png");
        weathers_to_cache.push_back("cache/weathers/Harvest.png");
        weathers_to_cache.push_back("cache/weathers/Hospital.png");
        weathers_to_cache.push_back("cache/weathers/Howling Sky.png");
        weathers_to_cache.push_back("cache/weathers/Jungle.png");
        weathers_to_cache.push_back("cache/weathers/Legendary City.png");
        weathers_to_cache.push_back("cache/weathers/Mars.png");
        weathers_to_cache.push_back("cache/weathers/Meteor Shower.png");
        weathers_to_cache.push_back("cache/weathers/Monochrome.png");
        weathers_to_cache.push_back("cache/weathers/Night.png");
        weathers_to_cache.push_back("cache/weathers/Nothing.png");
        weathers_to_cache.push_back("cache/weathers/Pagoda.png");
        weathers_to_cache.push_back("cache/weathers/Party.png");
        weathers_to_cache.push_back("cache/weathers/Pineapples.png");
        weathers_to_cache.push_back("cache/weathers/Rainy.png");
        weathers_to_cache.push_back("cache/weathers/Snowy Night.png");
        weathers_to_cache.push_back("cache/weathers/Snowy.png");
        weathers_to_cache.push_back("cache/weathers/Spooky.png");
        weathers_to_cache.push_back("cache/weathers/Spring.png");
        weathers_to_cache.push_back("cache/weathers/St Patricks.png");
        weathers_to_cache.push_back("cache/weathers/Sunny.png");
        weathers_to_cache.push_back("cache/weathers/Undersea.png");
        weathers_to_cache.push_back("cache/weathers/Valentines.png");
        weathers_to_cache.push_back("cache/weathers/Warp.png");
        weathers_to_cache.push_back("cache/weathers/princepersia.png");
        weathers_to_cache.push_back("cache/weathers/vapor.png");

        for (const auto& texture_path : textures_to_cache) {
            if (!std::filesystem::exists(texture_path))
                continue;
            sf::Texture* draw = new sf::Texture();
            draw->loadFromFile(texture_path);
            t_cache.insert_or_assign(utils::replace_text(texture_path, ".png", ".rttex"), std::move(draw));
        }
        for (const auto& texture_path : weathers_to_cache) {
            if (!std::filesystem::exists(texture_path))
                continue;
            sf::Texture* draw = new sf::Texture();
            draw->loadFromFile(texture_path);
            t_weather_cache.insert_or_assign(texture_path, std::move(draw));
        }
        for (const auto& texture_path : borders_to_cache) {
            if (!std::filesystem::exists(texture_path))
                continue;
            sf::Texture* draw = new sf::Texture();
            draw->loadFromFile(texture_path);
            t_border_cache.insert_or_assign(texture_path, std::move(draw));
        }

        sf_century = new sf::Font();
        if (!sf_century->loadFromFile("cache/fonts/century_gothic_bold.ttf"))
            fmt::print("WorldRender::OnCacheInit -> can't load font Century Gothic Bold.\n");
        sf_gothic_regular = new sf::Font();
        if (!sf_gothic_regular->loadFromFile("cache/fonts/gothic_regular.ttf"))
            fmt::print("WorldRender::OnCacheInit -> can't load font Gothic Regular.\n");

        lut_8bit[2] = 11;
        lut_8bit[8] = 30;
        lut_8bit[10] = 44;
        lut_8bit[11] = 8;
        lut_8bit[16] = 29;
        lut_8bit[18] = 43;
        lut_8bit[22] = 7;
        lut_8bit[24] = 28;
        lut_8bit[26] = 42;
        lut_8bit[27] = 41;
        lut_8bit[30] = 40;
        lut_8bit[31] = 2;
        lut_8bit[64] = 10;
        lut_8bit[66] = 9;
        lut_8bit[72] = 46;
        lut_8bit[74] = 36;
        lut_8bit[75] = 35;
        lut_8bit[80] = 45;
        lut_8bit[82] = 33;
        lut_8bit[86] = 32;
        lut_8bit[88] = 39;
        lut_8bit[90] = 27;
        lut_8bit[91] = 23;
        lut_8bit[94] = 24;
        lut_8bit[95] = 18;
        lut_8bit[104] = 6;
        lut_8bit[106] = 34;
        lut_8bit[107] = 4;
        lut_8bit[120] = 38;
        lut_8bit[122] = 25;
        lut_8bit[123] = 20;
        lut_8bit[126] = 21;
        lut_8bit[127] = 16;
        lut_8bit[208] = 5;
        lut_8bit[210] = 31;
        lut_8bit[214] = 3;
        lut_8bit[216] = 37;
        lut_8bit[218] = 26;
        lut_8bit[219] = 22;
        lut_8bit[222] = 19;
        lut_8bit[223] = 15;
        lut_8bit[248] = 1;
        lut_8bit[250] = 17;
        lut_8bit[251] = 14;
        lut_8bit[254] = 13;
        lut_8bit[0] = 12;

        fmt::print("WorldRender initialized,\n"
            " |-> {} sprite caches, {} weather caches and {} border caches are loaded.\n", t_cache.size(), t_weather_cache.size(), t_border_cache.size());
    }

    sf::Texture* WorldRender::get_texture_from_cache__interface(const std::string& file) {
        if (auto it = t_cache.find(fmt::format("cache/sprites/{}", file)); it != t_cache.end())
            return it->second;
        if (auto it = t_cache.find(fmt::format("cache/locale/{}", file)); it != t_cache.end())
            return it->second;
        return nullptr;
    }
    WorldRender::eRenderResult WorldRender::render__interface(ServerPool* server_pool, const std::shared_ptr<World>& world) {
        auto remove_gt_color = [&]( std::string str, std::string from) {
            std::size_t start_pos = 0;
            bool found = false;
            while(((start_pos = from.find(str)) != std::string::npos)) 
                from.erase(start_pos, str.length() + 1);
            return from;
        };
        try {
        PlayerTable* player_db = (PlayerTable*)Database::GetTable(Database::DATABASE_PLAYER_TABLE);
        int lut_4bit[] = { 12, 11, 15, 8, 14, 7, 13, 2, 10, 9, 6, 4, 5, 3, 1, 0 };
        sf::RenderTexture r_render_texture;
        sf::VertexArray v_background_array;
        int d = std::rand() % 5, c = std::rand() % 8;

        r_render_texture.create(world->GetSize().m_x * 32, world->GetSize().m_y * 32);
        v_background_array.setPrimitiveType(sf::Quads);

        size_t size = 0;     
        sf::Texture* world_background = nullptr;
        if (auto it = t_weather_cache.find(fmt::format("cache/weathers/{}", v_background_path[world->GetWeatherId()])); it != t_weather_cache.end())
            world_background = it->second;

        if (!world_background)
            return RENDER_RESULT_FAILED;
        size = 4;
        v_background_array.resize(size);

        sf::Vertex* bg_vertex = &v_background_array[0];
        float pixel_left = 0;
        float pixel_right = 3200;
        float pixel_top = 0;
        float pixel_bottom = 1920;

        bg_vertex[0].position = sf::Vector2f(pixel_left, pixel_bottom);
        bg_vertex[1].position = sf::Vector2f(pixel_left, pixel_top);
        bg_vertex[2].position = sf::Vector2f(pixel_right, pixel_top);
        bg_vertex[3].position = sf::Vector2f(pixel_right, pixel_bottom);

        float texture_left = 0;
        float texture_right = 1920;
        float texture_top = 0;
        float texture_bottom = 1080;

        std::swap(texture_top, texture_bottom);

        bg_vertex[0].texCoords = sf::Vector2f(texture_left, texture_bottom);
        bg_vertex[1].texCoords = sf::Vector2f(texture_left, texture_top);
        bg_vertex[2].texCoords = sf::Vector2f(texture_right, texture_top);
        bg_vertex[3].texCoords = sf::Vector2f(texture_right, texture_bottom);

        r_render_texture.draw(v_background_array, world_background);
        v_background_array.clear();
        size = 0;

        for (std::size_t index = 0; index < world->GetTiles().size(); ++index)
        {
            int x = static_cast<int>(index) % world->GetSize().m_x, y = static_cast<int>(index) / world->GetSize().m_x;
            ItemInfo* background = ItemDatabase::GetItem(world->GetTile(index)->GetBackground());
            ItemInfo* foreground = ItemDatabase::GetItem(world->GetTile(index)->GetForeground());

            if (world->IsOwned() && background->m_item_type == ITEMTYPE_MUSIC_NOTE) {
                Tile* main = world->GetTile(world->GetMainLock());
                if (!main)
                    continue;
                if (main->IsLockFlagOn(LOCKFLAG_INVISIBLE_MUSIC_NOTE))
                    continue;
            }
            float left = x * 32;
            float right = (x * 32) + 32;
            float top = (world->GetSize().m_y - y) * 32;
            float bottom = ((world->GetSize().m_y - y) * 32) - 32;

            if (background->m_id != ITEM_BLANK) {
                int offset_x = 0;
                int offset_y = 0;
                sf::Texture* bg_texture = get_texture_from_cache(background->m_texture);
                if (!bg_texture)
                    continue;
                switch (background->m_spread_type) {
                case 3: {
                    if (foreground->m_item_type == ITEMTYPE_SEED)
                        break;
                    bool left_2 = ItemDatabase::GetItem(world->GetTile(index - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                    bool right_2 = ItemDatabase::GetItem(world->GetTile(index + 1)->GetForeground())->m_id == foreground->m_id ? true : false;

                    if (!left_2 && !right_2)
                        offset_x = 3;
                    else if (!left_2 && right_2)
                        offset_x = 0;
                    else if (left_2 && !right_2)
                        offset_x = 2;
                    else if (left_2 && right_2)
                        offset_x = 1;
                    break;
                }
                case TILESPREAD_DIRT: {
                    if (foreground->m_item_type == ITEMTYPE_SEED)
                        break;
                    
                    bool top_left_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) - 1)->GetBackground())->m_id == background->m_id ? true : false;
                    bool top_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x))->GetBackground())->m_id == background->m_id ? true : false;
                    bool top_right_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) + 1)->GetBackground())->m_id == background->m_id ? true : false;

                    bool left_2 = ItemDatabase::GetItem(world->GetTile(index - 1)->GetBackground())->m_id == background->m_id ? true : false;
                    bool right_2 = ItemDatabase::GetItem(world->GetTile(index + 1)->GetBackground())->m_id == background->m_id ? true : false;

                    bool bottom_left_2 = false;
                    bool bottom_2 = false;
                    bool bottom_right_2 = false;

                    if (index < 5900) {
                        bottom_left_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) - 1)->GetBackground())->m_id == background->m_id ? true : false;
                        bottom_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x))->GetBackground())->m_id == background->m_id ? true : false;
                        bottom_right_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) + 1)->GetBackground())->m_id == background->m_id ? true : false;
                    }

                    if (!left_2 || !top_2)
                        top_left_2 = false;
                    if (!left_2 || !bottom_2)
                        bottom_left_2 = false;
                    if (!right_2 || !top_2)
                        top_right_2 = false;
                    if (!right_2 || !bottom_2)
                        bottom_right_2 = false;

                    int bit = 1 * top_left_2 + 2 * top_2 + 4 * top_right_2 + 8 * left_2 + 16 * right_2 + 32 * bottom_left_2 + 64 * bottom_2 + 128 * bottom_right_2;

                    offset_x = lut_8bit[bit] % 8;
                    offset_y = lut_8bit[bit] / 8;
                    break;
                }
                case TILESPREAD_PLATFORM: {
                    if (foreground->m_item_type == ITEMTYPE_SEED)
                        break;
                    bool top_pos = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x))->GetBackground())->m_id == background->m_id ? true : false;
                    bool left_pos = ItemDatabase::GetItem(world->GetTile(index - 1)->GetBackground())->m_id == background->m_id ? true : false;
                    bool right_pos = ItemDatabase::GetItem(world->GetTile(index + 1)->GetBackground())->m_id == background->m_id ? true : false;
                    bool bottom_pos = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x))->GetBackground())->m_id == background->m_id ? true : false;

                    int bit = 1 * top_pos + 2 * left_pos + 4 * right_pos + 8 * bottom_pos;

                    offset_x = lut_4bit[bit] % 8;
                    offset_y = lut_4bit[bit] / 8;
                    break;
                }
                case TILESPREAD_PILLAR: {
                        bool up = ItemDatabase::GetItem(world->GetTile(index - world->GetSize().m_x)->GetBackground())->m_id == background->m_id ? true : false;
                        bool down = ItemDatabase::GetItem(world->GetTile(index + world->GetSize().m_x)->GetBackground())->m_id == background->m_id ? true : false;

                        if (!up && !down)
                            offset_x = 3;
                        else if (!down && up)
                            offset_x = 0;
                        else if (down && !up)
                            offset_x = 2;
                        else if (up && down)
                            offset_x = 1;
                        break;
                    }
                }

                float _left = (offset_x + background->m_texture_x) * 32;
                float _right = ((offset_x + background->m_texture_x) * 32) + 32;
                float _top = (offset_y + background->m_texture_y) * 32;
                float _bottom = ((offset_y + background->m_texture_y) * 32) + 32;

                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                uint8_t t_red = world->GetTile(index)->IsFlagOn(TILEFLAG_RED) ? 0xFF : 70;
                uint8_t t_green = world->GetTile(index)->IsFlagOn(TILEFLAG_GREEN) ? 0xFF : 70;
                uint8_t t_blue = world->GetTile(index)->IsFlagOn(TILEFLAG_BLUE) ? 0xFF : 70;
                
                if (t_red == 0xFF || t_green == 0xFF || t_blue == 0xFF) {
                    if (t_red == 0xFF && t_green == 0xFF && t_blue == 0xFF) {
                        t_red = 70;
                        t_green = 70;
                        t_blue = 70; 
                    }                     
                    quad[0].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                    quad[1].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                    quad[2].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                    quad[3].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                }
                quad[0].position = sf::Vector2f(left, bottom);
                quad[1].position = sf::Vector2f(left, top);
                quad[2].position = sf::Vector2f(right, top);
                quad[3].position = sf::Vector2f(right, bottom);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                r_render_texture.draw(v_background_array, bg_texture);
                v_background_array.clear();
                size = 0;
            }
        }

        sf::RenderTexture r_block_shadows;
        sf::VertexArray v_block_shadows;
        r_block_shadows.create(world->GetSize().m_x * 32, world->GetSize().m_y * 32);
        v_block_shadows.setPrimitiveType(sf::Quads);

        for (auto it = world->GetObjects().cbegin(); it != world->GetObjects().cend();)
        {
            int x = static_cast<int>(it->second.m_pos.m_x), y = static_cast<int>(it->second.m_pos.m_y);
            ItemInfo* object = ItemDatabase::GetItem(it->second.m_item_id);

            if (object->m_item_type != ITEMTYPE_SEED) {
                float left = x + 2;
                float right = x + 18;
                float top = (world->GetSize().m_y * 32) - (y + 2);
                float bottom = ((world->GetSize().m_y * 32) - y) - 18;

                int offset_x = 0;
                int offset_y = 0;

                /*if (object->m_spread_type == 5 || object->m_spread_type == 2)
                    offset_x = 12 % 8;*/

                if (object->m_id == ITEM_GEMS) {
                    if (it->second.m_item_amount == 5)
                        offset_x = 1;
                    else if (it->second.m_item_amount == 10)
                        offset_x = 2;
                    else if (it->second.m_item_amount == 50)
                        offset_x = 3;
                    else if (it->second.m_item_amount == 100)
                        offset_x = 4;
                }

                float _left = (offset_x + object->m_default_texture_x) * 32;
                float _right = ((offset_x + object->m_default_texture_x) * 32) + 32;
                float _top = (offset_y + object->m_default_texture_y) * 32;
                float _bottom = ((offset_y + object->m_default_texture_y) * 32) + 32;

                sf::Texture* object_texture = get_texture_from_cache(object->m_texture);
                if (!object_texture)
                    continue;
                size += 4;
                v_block_shadows.resize(size);
                sf::Vertex* quad = &v_block_shadows[size - 4];

                quad[0].position = sf::Vector2f(left - 4, bottom - 4);
                quad[1].position = sf::Vector2f(left - 4, top - 4);
                quad[2].position = sf::Vector2f(right - 4, top - 4);
                quad[3].position = sf::Vector2f(right - 4, bottom - 4);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                quad[0].color = sf::Color(0, 0, 0, 0xFF);
                quad[1].color = sf::Color(0, 0, 0, 0xFF);
                quad[2].color = sf::Color(0, 0, 0, 0xFF);
                quad[3].color = sf::Color(0, 0, 0, 0xFF); // 145

                r_block_shadows.draw(v_block_shadows, object_texture);
                v_block_shadows.clear();
                size = 0;
            } else {
                float left = x + 2;
                float right = x + 18;
                float top = (world->GetSize().m_y * 32) - (y + 2);
                float bottom = ((world->GetSize().m_y * 32) - y) - 18;

                int offset_x = object->m_seed_base;
                int offset_y = 0;          

                sf::Texture* seed_background = get_texture_from_cache("seed.rttex");
                if (!seed_background)
                    continue;
                float _left = offset_x * 16;
                float _right = (offset_x * 16) + 16;
                float _top = offset_y * 16;
                float _bottom = (offset_y * 16) + 16;

                size += 4;
                v_block_shadows.resize(size);
                sf::Vertex* quad = &v_block_shadows[size - 4];

                quad[0].position = sf::Vector2f(left - 4, bottom - 4);
                quad[1].position = sf::Vector2f(left - 4, top - 4);
                quad[2].position = sf::Vector2f(right - 4, top - 4);
                quad[3].position = sf::Vector2f(right - 4, bottom - 4);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                quad[0].color = sf::Color(0, 0, 0, 0xFF);
                quad[1].color = sf::Color(0, 0, 0, 0xFF);
                quad[2].color = sf::Color(0, 0, 0, 0xFF);
                quad[3].color = sf::Color(0, 0, 0, 0xFF); // 145

                r_block_shadows.draw(v_block_shadows, seed_background);
                v_block_shadows.clear();
                size = 0;
            }

            if (object->m_id != ITEM_GEMS && object->m_item_type != ITEMTYPE_SEED) {
                float left2 = x;
                float right2 = x + 20;
                float top2 = (world->GetSize().m_y * 32) - y;
                float bottom2 = ((world->GetSize().m_y * 32) - y) - 20;

                int offset_x2 = 0;
                int offset_y2 = 0;

                if (object->m_item_type == ITEMTYPE_LOCK)
                    offset_x2 = 6;
                else if (object->m_item_category & ITEMFLAG2_UNTRADABLE)
                    offset_x2 = 8;
                else if (object->m_item_type == ITEMTYPE_CONSUMABLE)
                    offset_x2 = 5;

                float _left2 = offset_x2 * 20;
                float _right2 = (offset_x2 * 20) + 20;
                float _top2 = offset_y2 * 32;
                float _bottom2 = (offset_y2 * 20) + 20;

                sf::Texture* frame_texture = get_texture_from_cache("pickup_box.rttex");
                if (!frame_texture)
                    continue;
                size += 4;
                v_block_shadows.resize(size);
                sf::Vertex* quad = &v_block_shadows[size - 4];

                quad[0].position = sf::Vector2f(left2 - 4, bottom2 - 4);
                quad[1].position = sf::Vector2f(left2 - 4, top2 - 4);
                quad[2].position = sf::Vector2f(right2 - 4, top2 - 4);
                quad[3].position = sf::Vector2f(right2 - 4, bottom2 - 4);

                quad[0].texCoords = sf::Vector2f(_left2, _bottom2);
                quad[1].texCoords = sf::Vector2f(_left2, _top2);
                quad[2].texCoords = sf::Vector2f(_right2, _top2);
                quad[3].texCoords = sf::Vector2f(_right2, _bottom2);

                quad[0].color = sf::Color(0, 0, 0, 0xFF);
                quad[1].color = sf::Color(0, 0, 0, 0xFF);
                quad[2].color = sf::Color(0, 0, 0, 0xFF);
                quad[3].color = sf::Color(0, 0, 0, 0xFF); // 145

                r_block_shadows.draw(v_block_shadows, frame_texture);
                v_block_shadows.clear();
                size = 0;
            }
            ++it;
        }

        for (std::size_t index = 0; index < world->GetTiles().size(); ++index) {
            int x = static_cast<int>(index) % world->GetSize().m_x, y = static_cast<int>(index) / world->GetSize().m_x;
            ItemInfo* foreground = ItemDatabase::GetItem(world->GetTile(index)->GetForeground());
            int scalePX = 0;                        

            float xoff = 0;
            float yoff = 0;

            if (foreground->m_item_type == ITEMTYPE_SEED && 
                high_resolution_clock::now() - world->GetTile(index)->GetPlantedDate() < std::chrono::seconds(foreground->m_grow_time)) {
                auto remaining_seconds = high_resolution_clock::now() - world->GetTile(index)->GetPlantedDate();
                int min_size = 13;

                min_size -= (remaining_seconds / (std::chrono::seconds(foreground->m_grow_time) * 1.0f)) * min_size;

                scalePX = utils::always_negative(min_size);
                yoff = min_size;
            }

            float left = ((x * 32) - scalePX) + xoff;
            float right = (((x * 32) + 32) + scalePX) + xoff;
            float top = (((world->GetSize().m_y - y) * 32) + scalePX) - yoff;
            float bottom = ((((world->GetSize().m_y - y) * 32) - 32) - scalePX) - yoff;

            int offset_x = 0;
            int offset_y = 0;

            if (foreground->m_id == ITEM_BLANK)
                continue;
            if (foreground->m_editable_type & ITEMFLAG1_NOSHADOW)
                continue;

            if (foreground->m_item_type != ITEMTYPE_SEED) {
                sf::Texture* fg_texture = get_texture_from_cache(foreground->m_texture);
                if (!fg_texture)
                    continue;
                switch (foreground->m_spread_type) {
                    case TILESPREAD_DIRT: {
                        bool top_left_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                        bool top_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x))->GetForeground())->m_id == foreground->m_id ? true : false;
                        bool top_right_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) + 1)->GetForeground())->m_id == foreground->m_id ? true : false;

                        bool left_2 = ItemDatabase::GetItem(world->GetTile(index - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                        bool right_2 = ItemDatabase::GetItem(world->GetTile(index + 1)->GetForeground())->m_id == foreground->m_id ? true : false;

                        bool bottom_left_2 = false;
                        bool bottom_2 = false;
                        bool bottom_right_2 = false;

                        if (index < 5900) {
                            bottom_left_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                            bottom_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x))->GetForeground())->m_id == foreground->m_id ? true : false;
                            bottom_right_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) + 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                        }

                        if (!left_2 || !top_2)
                            top_left_2 = false;
                        if (!left_2 || !bottom_2)
                            bottom_left_2 = false;
                        if (!right_2 || !top_2)
                            top_right_2 = false;
                        if (!right_2 || !bottom_2)
                            bottom_right_2 = false;

                        int bit = 1 * top_left_2 + 2 * top_2 + 4 * top_right_2 + 8 * left_2 + 16 * right_2 + 32 * bottom_left_2 + 64 * bottom_2 + 128 * bottom_right_2;

                        offset_x = lut_8bit[bit] % 8;
                        offset_y = lut_8bit[bit] / 8;
                    } break;
                    case TILESPREAD_LAVA: {
                        bool left_2 = ItemDatabase::GetItem(world->GetTile(index - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                        bool right_2 = ItemDatabase::GetItem(world->GetTile(index + 1)->GetForeground())->m_id == foreground->m_id ? true : false;

                        if (!left_2 && !right_2)
                            offset_x = 3;
                        else if (!left_2 && right_2)
                            offset_x = 0;
                        else if (left_2 && !right_2)
                            offset_x = 2;
                        else if (left_2 && right_2)
                            offset_x = 1;
                    } break;
                    case TILESPREAD_PLATFORM: {
                        bool top_pos = ItemDatabase::GetItem(world->GetTile(index - world->GetSize().m_x)->GetForeground())->m_id == foreground->m_id ? true : false;
                        bool left_pos = ItemDatabase::GetItem(world->GetTile(index - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                        bool right_pos = ItemDatabase::GetItem(world->GetTile(index + 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                        bool bottom_pos = ItemDatabase::GetItem(world->GetTile(index + world->GetSize().m_x)->GetForeground())->m_id == foreground->m_id ? true : false;
                        int bit = 1 * top_pos + 2 * left_pos + 4 * right_pos + 8 * bottom_pos;
                        offset_x = lut_4bit[bit] % 8;
                        offset_y = lut_4bit[bit] / 8;
                    } break;
                    case TILESPREAD_PILLAR: {
                        bool up = ItemDatabase::GetItem(world->GetTile(index - world->GetSize().m_x)->GetForeground())->m_id == foreground->m_id ? true : false;
                        bool down = ItemDatabase::GetItem(world->GetTile(index + world->GetSize().m_x)->GetForeground())->m_id == foreground->m_id ? true : false;

                        if (!up && !down)
                            offset_x = 3;
                        else if (!down && up)
                            offset_x = 0;
                        else if (down && !up)
                            offset_x = 2;
                        else if (up && down)
                            offset_x = 1;
                    } break;
                }

                float _left = (offset_x + foreground->m_texture_x) * 32;
                float _right = ((offset_x + foreground->m_texture_x) * 32) + 32;
                float _top = (offset_y + foreground->m_texture_y) * 32;
                float _bottom = ((offset_y + foreground->m_texture_y) * 32) + 32;

                if (world->GetTile(index)->IsFlagOn(TILEFLAG_FLIPPED))
                    std::swap(_left, _right);

                size += 4;
                v_block_shadows.resize(size);
                sf::Vertex* quad = &v_block_shadows[size - 4];

                quad[0].position = sf::Vector2f(left - 4, bottom - 4);
                quad[1].position = sf::Vector2f(left - 4, top - 4);
                quad[2].position = sf::Vector2f(right - 4, top - 4);
                quad[3].position = sf::Vector2f(right - 4, bottom - 4);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                quad[0].color = sf::Color(0, 0, 0, 0xFF);
                quad[1].color = sf::Color(0, 0, 0, 0xFF);
                quad[2].color = sf::Color(0, 0, 0, 0xFF);
                quad[3].color = sf::Color(0, 0, 0, 0xFF);

                r_block_shadows.draw(v_block_shadows, fg_texture);
                v_block_shadows.clear();
                size = 0;

                switch(foreground->m_item_type) {
                    case ITEMTYPE_STEAMPUNK: {
                        sf::Texture* steam_outline = get_texture_from_cache("tiles_page5.rttex");
                        if (!steam_outline)
                            continue;
                        int offset_x = 0;
                        int offset_y = 0;

                        bool top_left_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) - 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                        bool top_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x))->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                        bool top_right_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) + 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;

                        bool left_2 = ItemDatabase::GetItem(world->GetTile(index - 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                        bool right_2 = ItemDatabase::GetItem(world->GetTile(index + 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;

                        bool bottom_left_2 = false;
                        bool bottom_2 = false;
                        bool bottom_right_2 = false;

                        if (index < 5900) {
                            bottom_left_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) - 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                            bottom_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x))->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                            bottom_right_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) + 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                        }

                        if (!left_2 || !top_2)
                            top_left_2 = false;
                        if (!left_2 || !bottom_2)
                            bottom_left_2 = false;
                        if (!right_2 || !top_2)
                            top_right_2 = false;
                        if (!right_2 || !bottom_2)
                            bottom_right_2 = false;

                        int bit = 1 * top_left_2 + 2 * top_2 + 4 * top_right_2 + 8 * left_2 + 16 * right_2 + 32 * bottom_left_2 + 64 * bottom_2 + 128 * bottom_right_2;

                        offset_x = lut_8bit[bit] % 8;
                        offset_y = lut_8bit[bit] / 8;

                        float _left = (offset_x + 8) * 32;
                        float _right = ((offset_x + 8) * 32) + 32;
                        float _top = (offset_y + 0) * 32;
                        float _bottom = ((offset_y + 0) * 32) + 32;

                        if (world->GetTile(index)->IsFlagOn(TILEFLAG_FLIPPED))
                            std::swap(_left, _right);

                        size += 4;
                        v_background_array.resize(size);
                        sf::Vertex* quad = &v_background_array[size - 4];

                        quad[0].position = sf::Vector2f(left - 4, bottom - 4);
                        quad[1].position = sf::Vector2f(left - 4, top - 4);
                        quad[2].position = sf::Vector2f(right - 4, top - 4);
                        quad[3].position = sf::Vector2f(right - 4, bottom - 4);

                        quad[0].texCoords = sf::Vector2f(_left, _bottom);
                        quad[1].texCoords = sf::Vector2f(_left, _top);
                        quad[2].texCoords = sf::Vector2f(_right, _top);
                        quad[3].texCoords = sf::Vector2f(_right, _bottom);

                        quad[0].color = sf::Color(0, 0, 0, 0xFF);
                        quad[1].color = sf::Color(0, 0, 0, 0xFF);
                        quad[2].color = sf::Color(0, 0, 0, 0xFF);
                        quad[3].color = sf::Color(0, 0, 0, 0xFF);

                        r_block_shadows.draw(v_block_shadows, fg_texture);
                        v_block_shadows.clear();
                        size = 0;

                        offset_x = 0;
                        offset_y = 0;
                    } break;
                }
            } else {
                sf::Texture* tree_base = get_texture_from_cache("tiles_page1.rttex"); {
                    if (!tree_base)
                        continue;
                    int off_seed_x = foreground->m_tree_base;
                    int off_seed_y = 0;

                    float _left = off_seed_x * 32;
                    float _right = (off_seed_x * 32) + 32;
                    float _top = (off_seed_y + 19) * 32;
                    float _bottom = ((off_seed_y + 19) * 32) + 32;

                    size += 4;
                    v_block_shadows.resize(size);
                    sf::Vertex* quad = &v_block_shadows[size - 4];

                    quad[0].position = sf::Vector2f(left - 4, bottom - 4);
                    quad[1].position = sf::Vector2f(left - 4, top - 4);
                    quad[2].position = sf::Vector2f(right - 4, top - 4);
                    quad[3].position = sf::Vector2f(right - 4, bottom - 4);

                    quad[0].texCoords = sf::Vector2f(_left, _bottom);
                    quad[1].texCoords = sf::Vector2f(_left, _top);
                    quad[2].texCoords = sf::Vector2f(_right, _top);
                    quad[3].texCoords = sf::Vector2f(_right, _bottom);

                    quad[0].color = sf::Color(0, 0, 0, 0xFF);
                    quad[1].color = sf::Color(0, 0, 0, 0xFF);
                    quad[2].color = sf::Color(0, 0, 0, 0xFF);
                    quad[3].color = sf::Color(0, 0, 0, 0xFF);

                    r_block_shadows.draw(v_block_shadows, tree_base);
                    v_block_shadows.clear();
                    size = 0;

                    offset_x = 0;
                    offset_y = 0;
                }
                sf::Texture* tree_leaves = get_texture_from_cache("tiles_page1.rttex"); {
                    if (!tree_leaves)
                        continue;

                    int off_seed_x = foreground->m_tree_leaves;
                    int off_seed_y = 0;

                    float _left = off_seed_x * 32;
                    float _right = (off_seed_x * 32) + 32;
                    float _top = (off_seed_y + 18) * 32;
                    float _bottom = ((off_seed_y + 18) * 32) + 32;

                    size += 4;
                    v_block_shadows.resize(size);
                    sf::Vertex* quad = &v_block_shadows[size - 4];

                    quad[0].position = sf::Vector2f(left - 4, bottom - 4);
                    quad[1].position = sf::Vector2f(left - 4, top - 4);
                    quad[2].position = sf::Vector2f(right - 4, top - 4);
                    quad[3].position = sf::Vector2f(right - 4, bottom - 4);

                    quad[0].texCoords = sf::Vector2f(_left, _bottom);
                    quad[1].texCoords = sf::Vector2f(_left, _top);
                    quad[2].texCoords = sf::Vector2f(_right, _top);
                    quad[3].texCoords = sf::Vector2f(_right, _bottom);

                    quad[0].color = sf::Color(0, 0, 0, 0xFF);
                    quad[1].color = sf::Color(0, 0, 0, 0xFF);
                    quad[2].color = sf::Color(0, 0, 0, 0xFF);
                    quad[3].color = sf::Color(0, 0, 0, 0xFF);

                    r_block_shadows.draw(v_block_shadows, tree_leaves);
                    v_block_shadows.clear();
                    size = 0;

                    offset_x = 0;
                    offset_y = 0;
                }
            }
        } {
            sf::Texture block_shadow_texture(r_block_shadows.getTexture());

            size += 4;
            v_block_shadows.resize(size);
            sf::Vertex* bg_vert_block = &v_block_shadows[size - 4];

            float pixel_left_block = 0;
            float pixel_right_block = 3200;
            float pixel_top_block = 0;
            float pixel_bottom_block = 1920;

            bg_vert_block[0].position = sf::Vector2f(pixel_left_block, pixel_bottom_block);
            bg_vert_block[1].position = sf::Vector2f(pixel_left_block, pixel_top_block);
            bg_vert_block[2].position = sf::Vector2f(pixel_right_block, pixel_top_block);
            bg_vert_block[3].position = sf::Vector2f(pixel_right_block, pixel_bottom_block);

            float texture_left_block = 0;
            float texture_right_block = 3200;
            float texture_top_block = 0;
            float texture_bottom_block = 1920;

            std::swap(texture_top_block, texture_bottom_block);

            bg_vert_block[0].texCoords = sf::Vector2f(texture_left_block, texture_bottom_block);
            bg_vert_block[1].texCoords = sf::Vector2f(texture_left_block, texture_top_block);
            bg_vert_block[2].texCoords = sf::Vector2f(texture_right_block, texture_top_block);
            bg_vert_block[3].texCoords = sf::Vector2f(texture_right_block, texture_bottom_block);

            bg_vert_block[0].color = sf::Color(0, 0, 0, 145);
            bg_vert_block[1].color = sf::Color(0, 0, 0, 145);
            bg_vert_block[2].color = sf::Color(0, 0, 0, 145);
            bg_vert_block[3].color = sf::Color(0, 0, 0, 145); // 145

            r_render_texture.draw(v_block_shadows, &block_shadow_texture);
            v_block_shadows.clear();
            size = 0;
        }

        for (std::size_t index = 0; index < world->GetTiles().size(); ++index) {
            int x = static_cast<int>(index) % world->GetSize().m_x, y = static_cast<int>(index) / world->GetSize().m_x;
            ItemInfo* background = ItemDatabase::GetItem(world->GetTile(index)->GetBackground());
            ItemInfo* foreground = ItemDatabase::GetItem(world->GetTile(index)->GetForeground());

            int scalePX = 0; 
            float xoff = 0;
            float yoff = 0;

            if (foreground->m_item_type == ITEMTYPE_SEED && high_resolution_clock::now() - world->GetTile(index)->GetPlantedDate() < std::chrono::seconds(foreground->m_grow_time)) {
                auto remaining_seconds = high_resolution_clock::now() - world->GetTile(index)->GetPlantedDate();
                int min_size = 13;

                min_size -= (remaining_seconds / (std::chrono::seconds(foreground->m_grow_time) * 1.0f)) * min_size;

                scalePX = utils::always_negative(min_size);
                yoff = min_size;
            }

            float left = ((x * 32) - scalePX) + xoff;
            float right = (((x * 32) + 32) + scalePX) + xoff;
            float top = (((world->GetSize().m_y - y) * 32) + scalePX) - yoff;
            float bottom = ((((world->GetSize().m_y - y) * 32) - 32) - scalePX) - yoff;

            switch (foreground->m_item_type) { // Fist Layer TileExtra
                case ITEMTYPE_DISPLAY_BLOCK:
                    break;
                default:
                    break;
            }

            if (foreground->m_id != ITEM_BLANK) { 
                int offset_x = 0;
                int offset_y = 0;
                     
                if (foreground->m_item_type != ITEMTYPE_SEED) {
                    sf::Texture* fg_texture = get_texture_from_cache(foreground->m_texture);
                    if (!fg_texture)
                        continue;
                    switch (foreground->m_spread_type) {
                        case TILESPREAD_DIRT: {
                            if (foreground->m_item_type == ITEMTYPE_SEED)
                                break;
                            bool top_left_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                            bool top_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x))->GetForeground())->m_id == foreground->m_id ? true : false;
                            bool top_right_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) + 1)->GetForeground())->m_id == foreground->m_id ? true : false;

                            bool left_2 = ItemDatabase::GetItem(world->GetTile(index - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                            bool right_2 = ItemDatabase::GetItem(world->GetTile(index + 1)->GetForeground())->m_id == foreground->m_id ? true : false;

                            bool bottom_left_2 = false;
                            bool bottom_2 = false;
                            bool bottom_right_2 = false;

                            if (index < 5900) {
                                bottom_left_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                                bottom_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x))->GetForeground())->m_id == foreground->m_id ? true : false;
                                bottom_right_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) + 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                            }

                            if (!left_2 || !top_2)
                                top_left_2 = false;
                            if (!left_2 || !bottom_2)
                                bottom_left_2 = false;
                            if (!right_2 || !top_2)
                                top_right_2 = false;
                            if (!right_2 || !bottom_2)
                                bottom_right_2 = false;

                            int bit = 1 * top_left_2 + 2 * top_2 + 4 * top_right_2 + 8 * left_2 + 16 * right_2 + 32 * bottom_left_2 + 64 * bottom_2 + 128 * bottom_right_2;

                            offset_x = lut_8bit[bit] % 8;
                            offset_y = lut_8bit[bit] / 8;
                        } break;
                        case TILESPREAD_LAVA: {
                            if (foreground->m_item_type == ITEMTYPE_SEED)
                                break;
                            bool left_2 = ItemDatabase::GetItem(world->GetTile(index - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                            bool right_2 = ItemDatabase::GetItem(world->GetTile(index + 1)->GetForeground())->m_id == foreground->m_id ? true : false;

                            if (!left_2 && !right_2)
                                offset_x = 3;
                            else if (!left_2 && right_2)
                                offset_x = 0;
                            else if (left_2 && !right_2)
                                offset_x = 2;
                            else if (left_2 && right_2)
                                offset_x = 1;
                        } break;
                        case TILESPREAD_PLATFORM: {
                            if (foreground->m_item_type == ITEMTYPE_SEED)
                                break;
                            bool top_pos = ItemDatabase::GetItem(world->GetTile(index - world->GetSize().m_x)->GetForeground())->m_id == foreground->m_id ? true : false;
                            bool left_pos = ItemDatabase::GetItem(world->GetTile(index - 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                            bool right_pos = ItemDatabase::GetItem(world->GetTile(index + 1)->GetForeground())->m_id == foreground->m_id ? true : false;
                            bool bottom_pos = ItemDatabase::GetItem(world->GetTile(index + world->GetSize().m_x)->GetForeground())->m_id == foreground->m_id ? true : false;
                            int bit = 1 * top_pos + 2 * left_pos + 4 * right_pos + 8 * bottom_pos;
                            offset_x = lut_4bit[bit] % 8;
                            offset_y = lut_4bit[bit] / 8;
                        } break;
                        case TILESPREAD_PILLAR: {
                            bool up = ItemDatabase::GetItem(world->GetTile(index - world->GetSize().m_x)->GetForeground())->m_id == foreground->m_id ? true : false;
                            bool down = ItemDatabase::GetItem(world->GetTile(index + world->GetSize().m_x)->GetForeground())->m_id == foreground->m_id ? true : false;

                            if (!up && !down)
                                offset_x = 3;
                            else if (!down && up)
                                offset_x = 0;
                            else if (down && !up)
                                offset_x = 2;
                            else if (up && down)
                                offset_x = 1;
                        } break;
                    }

                    switch (foreground->m_item_type) {
                        case ITEMTYPE_VENDING_MACHINE:
                            break; // TODO: Vending
                        case ITEMTYPE_SWITCHEROO2:
                        case ITEMTYPE_CHEST:
                        case ITEMTYPE_LAB:
                        case ITEMTYPE_DONATION_BOX:
                        case ITEMTYPE_SWITCHEROO:
                        case ITEMTYPE_WEATHER_MACHINE:
                        case ITEMTYPE_BOOMBOX: {
                            if (world->GetTile(index)->IsFlagOn(TILEFLAG_OPEN))
                                offset_x = 1;
                        } break;
                        case ITEMTYPE_DICE: {
                           offset_x = world->GetTile(index)->m_random_value;
                        } break;
                        case ITEMTYPE_PROVIDER: {
                            if ((high_resolution_clock::now() - world->GetTile(index)->GetPlantedDate()) >= std::chrono::seconds(world->GetTile(index)->GetBaseItem()->m_grow_time))
                                offset_x = 2;
                        } break;
                        case ITEMTYPE_HEART_MONITOR: {
                            if (world->GetTile(index)->IsFlagOn(TILEFLAG_OPEN))
                                offset_x = 2;
                        } break;
                        case ITEMTYPE_LOCK: {
                            offset_x = 2;
                        } break;
                        case ITEMTYPE_SEED:
                            break;
                    }

                    if (foreground->m_id == ITEM_WINTERFEST_DANCEFLOOR)
                        offset_x = d;

                    float _left = (offset_x + foreground->m_texture_x) * 32;
                    float _right = ((offset_x + foreground->m_texture_x) * 32) + 32;
                    float _top = (offset_y + foreground->m_texture_y) * 32;
                    float _bottom = ((offset_y + foreground->m_texture_y) * 32) + 32;

                    if (world->GetTile(index)->IsFlagOn(TILEFLAG_FLIPPED))
                        std::swap(_left, _right);

                    size += 4;
                    v_background_array.resize(size);
                    sf::Vertex* quad = &v_background_array[size - 4];

                    uint8_t t_red = world->GetTile(index)->IsFlagOn(TILEFLAG_RED) ? 0xFF : 70;
                    uint8_t t_green = world->GetTile(index)->IsFlagOn(TILEFLAG_GREEN) ? 0xFF : 70;
                    uint8_t t_blue = world->GetTile(index)->IsFlagOn(TILEFLAG_BLUE) ? 0xFF : 70;
                    
                    if (t_red == 0xFF || t_green == 0xFF || t_blue == 0xFF) {
                        if (t_red == 0xFF && t_green == 0xFF && t_blue == 0xFF) {
                            t_red = 70;
                            t_green = 70;
                            t_blue = 70; 
                        }                     
                        quad[0].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                        quad[1].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                        quad[2].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                        quad[3].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                    }
                    quad[0].position = sf::Vector2f(left, bottom);
                    quad[1].position = sf::Vector2f(left, top);
                    quad[2].position = sf::Vector2f(right, top);
                    quad[3].position = sf::Vector2f(right, bottom);

                    quad[0].texCoords = sf::Vector2f(_left, _bottom);
                    quad[1].texCoords = sf::Vector2f(_left, _top);
                    quad[2].texCoords = sf::Vector2f(_right, _top);
                    quad[3].texCoords = sf::Vector2f(_right, _bottom);

                    switch (foreground->m_item_type) {
                    case ITEMTYPE_GAME_RESOURCES: {
                        float red = 0, blue = 0, green = 0, alpha = 0xFF;
                        switch (world->GetTile(index)->GetItemId()) {
                        case 0: {
                            red = 186, green = 0, blue = 1;
                        } break;
                        case 1: {
                            red = 5, green = 105, blue = 210;
                        } break;
                        case 2: {
                            red = 207, green = 161, blue = 1;
                        } break;
                        case 3: {
                            red = 119, green = 0, blue = 158;
                        } break;
                        case 4: {
                            red = 118, green = 118, blue = 118;
                        } break;
                        default:
                            break;
                        }
                        
                        quad[0].color = sf::Color(red, green, blue, alpha);
                        quad[1].color = sf::Color(red, green, blue, alpha);
                        quad[2].color = sf::Color(red, green, blue, alpha);
                        quad[3].color = sf::Color(red, green, blue, alpha);
                    } break;
                    default: {
                        switch (foreground->m_id) {
                        case ITEM_SHIFTY_BLOCK: {
                            float red = 0, blue = 0, green = 0, alpha = 0xFF;
                            switch (c) {
                            case 1: {
                                red = 230;
                                green = 160;
                                blue = 0;
                            } break;
                            case 2: {
                                red = 230;
                                green = 27;
                                blue = 0;
                            } break;
                            case 3: {
                                red = 230;
                                green = 0;
                                blue = 69;
                            } break;
                            case 4: {
                                red = 230;
                                green = 0;
                                blue = 177;
                            } break;
                            case 5: {
                                red = 150;
                                green = 0;
                                blue = 230;
                            } break;
                            case 6: {
                                red = 0;
                                green = 131;
                                blue = 230;
                            } break;
                            case 7: {
                                red = 0;
                                green = 230;
                                blue = 195;
                            } break;
                            case 8: {
                                red = 95;
                                green = 230;
                                blue = 0;
                            } break;
                            }

                            quad[0].color = sf::Color(red, blue, green, alpha);
                            quad[1].color = sf::Color(red, blue, green, alpha);
                            quad[2].color = sf::Color(red, blue, green, alpha);
                            quad[3].color = sf::Color(red, blue, green, alpha);
                        } break;
                        default:
                            break;
                        }
                    } break;
                    }

                    // TODO: IsHasDisplayItem && FG == ITEM_TYPE_DISPLAYBLOCK
                    
                    r_render_texture.draw(v_background_array, fg_texture);
                    v_background_array.clear();
                    size = 0;

                    offset_x = 0;
                    offset_y = 0;
                } else {
                    sf::Texture* tree_base = get_texture_from_cache("tiles_page1.rttex"); {
                        if (!tree_base)
                            continue;
                        int off_seed_x = foreground->m_tree_base;
                        int off_seed_y = 0;

                        float _left = off_seed_x * 32;
                        float _right = (off_seed_x * 32) + 32;
                        float _top = (off_seed_y + 19) * 32;
                        float _bottom = ((off_seed_y + 19) * 32) + 32;

                        size += 4;
                        v_background_array.resize(size);
                        sf::Vertex* quad = &v_background_array[size - 4];

                        int seed_base_color = foreground->m_seed_color;

                        uint8_t seed_base_r = (seed_base_color >> 8) & 0xFF;
                        uint8_t seed_base_g = (seed_base_color >> 16) & 0xFF;
                        uint8_t seed_base_b = (seed_base_color >> 24) & 0xFF;
                        uint8_t seed_base_a = seed_base_color & 0xFF;

                        auto high = std::max( {seed_base_r, seed_base_g, seed_base_b} );
                        if (seed_base_r != high)
                            seed_base_r += ((0xFF - seed_base_r) + 20) >= 0xFF ? 0xFF : 0;
                        if (seed_base_g != high)
                            seed_base_g += ((0xFF - seed_base_g) + 20) >= 0xFF ? 0xFF : 0;
                        if (seed_base_b != high)
                            seed_base_b += ((0xFF - seed_base_b) + 20) >= 0xFF ? 0xFF : 0;

                        quad[0].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);
                        quad[1].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);
                        quad[2].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);
                        quad[3].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);

                        quad[0].position = sf::Vector2f(left, bottom);
                        quad[1].position = sf::Vector2f(left, top);
                        quad[2].position = sf::Vector2f(right, top);
                        quad[3].position = sf::Vector2f(right, bottom);

                        quad[0].texCoords = sf::Vector2f(_left, _bottom);
                        quad[1].texCoords = sf::Vector2f(_left, _top);
                        quad[2].texCoords = sf::Vector2f(_right, _top);
                        quad[3].texCoords = sf::Vector2f(_right, _bottom);

                        r_render_texture.draw(v_background_array, tree_base);
                        v_background_array.clear();
                        size = 0;

                        offset_x = 0;
                        offset_y = 0;
                    }
                    sf::Texture* tree_leaves = get_texture_from_cache("tiles_page1.rttex"); {
                        if (!tree_leaves)
                            continue;

                        int off_seed_x = foreground->m_tree_leaves;
                        int off_seed_y = 0;

                        float _left = off_seed_x * 32;
                        float _right = (off_seed_x * 32) + 32;
                        float _top = (off_seed_y + 18) * 32;
                        float _bottom = ((off_seed_y + 18) * 32) + 32;

                        size += 4;
                        v_background_array.resize(size);
                        sf::Vertex* quad = &v_background_array[size - 4];

                        int seed_base_color = foreground->m_seed_color;

                        int seed_base_r = ((seed_base_color >> 8) & 0xFF);
                        int seed_base_g = ((seed_base_color >> 16) & 0xFF);
                        int seed_base_b = ((seed_base_color >> 24) & 0xFF);
                        int seed_base_a = seed_base_color & 0xFF;      

                        auto high = std::max( {seed_base_r, seed_base_g, seed_base_b} );
                     /*   if (seed_base_r != high)
                            seed_base_r += ((0xFF - seed_base_r) + 45) >= 0xFF ? 0xFF : 0;
                        if (seed_base_g != high)
                            seed_base_g += ((0xFF - seed_base_g) + 45) >= 0xFF ? 0xFF : 0;
                        if (seed_base_b != high)
                            seed_base_b += ((0xFF - seed_base_b) + 45) >= 0xFF ? 0xFF : 0;*/


                        quad[0].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);
                        quad[1].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);
                        quad[2].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);
                        quad[3].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);

                        quad[0].position = sf::Vector2f(left, bottom);
                        quad[1].position = sf::Vector2f(left, top);
                        quad[2].position = sf::Vector2f(right, top);
                        quad[3].position = sf::Vector2f(right, bottom);

                        quad[0].texCoords = sf::Vector2f(_left, _bottom);
                        quad[1].texCoords = sf::Vector2f(_left, _top);
                        quad[2].texCoords = sf::Vector2f(_right, _top);
                        quad[3].texCoords = sf::Vector2f(_right, _bottom);

                        r_render_texture.draw(v_background_array, tree_leaves);
                        v_background_array.clear();
                        size = 0;

                        offset_x = 0;
                        offset_y = 0;
                    }

                    if ((high_resolution_clock::now() - world->GetTile(index)->GetPlantedDate()) >= std::chrono::seconds(foreground->m_grow_time)) {
                        int fruits = world->GetTile(index)->GetFruitCount();
                        ItemInfo* fruit = ItemDatabase::GetItem(foreground->m_id - 1);

                        for (int i = 0; i < 5; i++) {
                            if (fruits < 1) 
                                break;
                            fruits -= 1;

                            sf::Texture* tree_fruit = get_texture_from_cache(fruit->m_texture);
                            if (!tree_fruit)
                                break;
                            int scalePX = -11;                        
                            float xoff = -15;
                            float yoff = -8;

                            const int block_size = std::abs((((world->GetSize().m_y - y) * 32) + scalePX) - ((((world->GetSize().m_y - y) * 32) - 32) - scalePX));        
                            if (i == 0 || i == 1) {   
                                xoff += (i * (block_size + 2)) + 7;
                                yoff += 0;
                            } else {
                                yoff = -12;  
                                yoff += block_size + 5;

                                int sad = i - 2;
                                xoff = (-8) - (8 + block_size) + 5;
                                xoff += (sad * (block_size + 2) + 7);
                            }    

                            float left = ((x * 32) - scalePX) + xoff;
                            float right = (((x * 32) + 32) + scalePX) + xoff;
                            float top = (((world->GetSize().m_y - y) * 32) + scalePX) - yoff;
                            float bottom = ((((world->GetSize().m_y - y) * 32) - 32) - scalePX) - yoff;     

                            float _left = (offset_x + fruit->m_default_texture_x) * 32;
                            float _right = ((offset_x + fruit->m_default_texture_x) * 32) + 32;
                            float _top = (offset_y + fruit->m_default_texture_y) * 32;
                            float _bottom = ((offset_y + fruit->m_default_texture_y) * 32) + 32;

                            size += 4;
                            v_background_array.resize(size);
                            sf::Vertex* quad = &v_background_array[size - 4];

                            quad[0].position = sf::Vector2f(left, bottom);
                            quad[1].position = sf::Vector2f(left, top);
                            quad[2].position = sf::Vector2f(right, top);
                            quad[3].position = sf::Vector2f(right, bottom);

                            quad[0].texCoords = sf::Vector2f(_left, _bottom);
                            quad[1].texCoords = sf::Vector2f(_left, _top);
                            quad[2].texCoords = sf::Vector2f(_right, _top);
                            quad[3].texCoords = sf::Vector2f(_right, _bottom);

                            r_render_texture.draw(v_background_array, tree_fruit);
                            v_background_array.clear();
                            size = 0;

                            offset_x = 0;
                            offset_y = 0;
                        }
                    }
                }
            
                switch (foreground->m_item_type) {
                case ITEMTYPE_GAME_RESOURCES: {
                    if (!(foreground->m_id == ITEM_GAME_BLOCK || foreground->m_id == ITEM_GAME_GRAVE || foreground->m_id == ITEM_GAME_GOAL))
                        break;
                    sf::Texture* icon_texture = this->get_texture_from_cache("game_icons.rttex");
                    if (!icon_texture)
                        break;
                    auto* tile = world->GetTile(index);
                    float _left = (world->GetTile(index)->GetItemId()) * 16;
                    float _right = ((world->GetTile(index)->GetItemId()) * 16) + 16;
                    float _top = 0;
                    float _bottom = 16;
                    {
                        scalePX = -7; 
                        left = ((x * 32) - scalePX) + xoff;
                        right = (((x * 32) + 32) + scalePX) + xoff;
                        top = (((world->GetSize().m_y - y) * 32) + scalePX) - yoff;
                        bottom = ((((world->GetSize().m_y - y) * 32) - 32) - scalePX) - yoff;
        
                        size += 4;
                        v_background_array.resize(size);
                        sf::Vertex* quad = &v_background_array[size - 4];
                        
                        quad[0].position = sf::Vector2f(left - 0.5, bottom + 0.5);
                        quad[1].position = sf::Vector2f(left - 0.5, top - 0.5);
                        quad[2].position = sf::Vector2f(right + 0.5, top - 0.5);
                        quad[3].position = sf::Vector2f(right + 0.5, bottom + 0.5);

                        quad[0].texCoords = sf::Vector2f(_left, _bottom);
                        quad[1].texCoords = sf::Vector2f(_left, _top);
                        quad[2].texCoords = sf::Vector2f(_right, _top);
                        quad[3].texCoords = sf::Vector2f(_right, _bottom);

                        quad[0].color = sf::Color(0, 0, 0, 0xFF);
                        quad[1].color = sf::Color(0, 0, 0, 0xFF);
                        quad[2].color = sf::Color(0, 0, 0, 0xFF);
                        quad[3].color = sf::Color(0, 0, 0, 0xFF);

                        r_render_texture.draw(v_background_array, icon_texture);
                        v_background_array.clear();
                        size = 0;
                    } {
                        scalePX = -8;
                    
                        left = ((x * 32) - scalePX) + xoff;
                        right = (((x * 32) + 32) + scalePX) + xoff;
                        top = (((world->GetSize().m_y - y) * 32) + scalePX) - yoff;
                        bottom = ((((world->GetSize().m_y - y) * 32) - 32) - scalePX) - yoff;

                        size += 4;
                        v_background_array.resize(size);
                        sf::Vertex* quad = &v_background_array[size - 4];
                        
                        quad[0].position = sf::Vector2f(left, bottom);
                        quad[1].position = sf::Vector2f(left, top);
                        quad[2].position = sf::Vector2f(right, top);
                        quad[3].position = sf::Vector2f(right, bottom);

                        quad[0].texCoords = sf::Vector2f(_left, _bottom);
                        quad[1].texCoords = sf::Vector2f(_left, _top);
                        quad[2].texCoords = sf::Vector2f(_right, _top);
                        quad[3].texCoords = sf::Vector2f(_right, _bottom);

                        r_render_texture.draw(v_background_array, icon_texture);
                        v_background_array.clear();
                        size = 0;
                    }
                } break;
                case ITEMTYPE_FLAG: {
                    sf::Texture* icon_texture = this->get_texture_from_cache(fmt::format("{}.rttex", world->GetTile(index)->GetLabel()));
                    if (!icon_texture)
                        break;
                    auto* tile = world->GetTile(index);
                  
                    sf::Sprite p_flag;
                    p_flag.setTexture(*icon_texture);
                    p_flag.setTextureRect(sf::IntRect(0, 0, 15, 10));
                    p_flag.setScale(1, 1.3);
                    if (tile->IsFlagOn(TILEFLAG_FLIPPED)) {
                        p_flag.setPosition((x * 32) - 1, ((world->GetSize().m_y - y) * 32) - 2);
                    } else {    
                        p_flag.setPosition(x * 32, ((world->GetSize().m_y - y) * 32));
                    }
                    r_render_texture.draw(p_flag);
                } break;
                default:
                    break;
                }
            }

            switch(foreground->m_item_type) {
            case ITEMTYPE_STEAMPUNK: {
                sf::Texture* steam_outline = get_texture_from_cache("tiles_page5.rttex");
                if (!steam_outline)
                    break;
                int offset_x = 0;
                int offset_y = 0;

                bool top_left_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) - 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                bool top_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x))->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                bool top_right_2 = ItemDatabase::GetItem(world->GetTile((index - world->GetSize().m_x) + 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;

                bool left_2 = ItemDatabase::GetItem(world->GetTile(index - 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                bool right_2 = ItemDatabase::GetItem(world->GetTile(index + 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;

                bool bottom_left_2 = false;
                bool bottom_2 = false;
                bool bottom_right_2 = false;

                if (index < 5900) {
                    bottom_left_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) - 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                    bottom_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x))->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                    bottom_right_2 = ItemDatabase::GetItem(world->GetTile((index + world->GetSize().m_x) + 1)->GetForeground())->m_item_type == foreground->m_item_type ? true : false;
                }

                if (!left_2 || !top_2)
                    top_left_2 = false;
                if (!left_2 || !bottom_2)
                    bottom_left_2 = false;
                if (!right_2 || !top_2)
                    top_right_2 = false;
                if (!right_2 || !bottom_2)
                    bottom_right_2 = false;

                int bit = 1 * top_left_2 + 2 * top_2 + 4 * top_right_2 + 8 * left_2 + 16 * right_2 + 32 * bottom_left_2 + 64 * bottom_2 + 128 * bottom_right_2;

                offset_x = lut_8bit[bit] % 8;
                offset_y = lut_8bit[bit] / 8;

                float _left = (offset_x + 8) * 32;
                float _right = ((offset_x + 8) * 32) + 32;
                float _top = (offset_y + 0) * 32;
                float _bottom = ((offset_y + 0) * 32) + 32;

                if (world->GetTile(index)->IsFlagOn(TILEFLAG_FLIPPED))
                    std::swap(_left, _right);

                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                uint8_t t_red = world->GetTile(index)->IsFlagOn(TILEFLAG_RED) ? 0xFF : 70;
                uint8_t t_green = world->GetTile(index)->IsFlagOn(TILEFLAG_GREEN) ? 0xFF : 70;
                uint8_t t_blue = world->GetTile(index)->IsFlagOn(TILEFLAG_BLUE) ? 0xFF : 70;
                
                if (t_red == 0xFF || t_green == 0xFF || t_blue == 0xFF) {
                    if (t_red == 0xFF && t_green == 0xFF && t_blue == 0xFF) {
                        t_red = 70;
                        t_green = 70;
                        t_blue = 70; 
                    }                     
                    quad[0].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                    quad[1].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                    quad[2].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                    quad[3].color = sf::Color(t_red, t_green, t_blue, 0xFF);
                }
                quad[0].position = sf::Vector2f(left, bottom);
                quad[1].position = sf::Vector2f(left, top);
                quad[2].position = sf::Vector2f(right, top);
                quad[3].position = sf::Vector2f(right, bottom);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                r_render_texture.draw(v_background_array, steam_outline);
                v_background_array.clear();
                size = 0;

                offset_x = 0;
                offset_y = 0;
            } break;
            }
        }

        for (auto it = world->GetObjects().cbegin(); it != world->GetObjects().cend();) {
            int x = static_cast<int>(it->second.m_pos.m_x), y = static_cast<int>(it->second.m_pos.m_y);
            ItemInfo* object = ItemDatabase::GetItem(it->second.m_item_id);
            const uint8_t object_count = it->second.m_item_amount;

            if (object->m_id != ITEM_GEMS && object->m_item_type != ITEMTYPE_SEED) {
                float left2 = x;
                float right2 = x + 20;
                float top2 = (world->GetSize().m_y * 32) - y;
                float bottom2 = ((world->GetSize().m_y * 32) - y) - 20;

                int offset_x2 = 0;
                int offset_y2 = 0;

                if (object->m_item_type == ITEMTYPE_LOCK)
                    offset_x2 = 6;
                else if (object->m_item_category & ITEMFLAG2_UNTRADABLE)
                    offset_x2 = 8;
                else if (object->m_item_type == ITEMTYPE_CONSUMABLE)
                    offset_x2 = 5;

                float _left2 = offset_x2 * 20;
                float _right2 = (offset_x2 * 20) + 20;
                float _top2 = offset_y2 * 32;
                float _bottom2 = (offset_y2 * 20) + 20;

                sf::Texture* frame_texture = get_texture_from_cache("pickup_box.rttex");
                if (!frame_texture)
                    continue;
                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                quad[0].position = sf::Vector2f(left2, bottom2);
                quad[1].position = sf::Vector2f(left2, top2);
                quad[2].position = sf::Vector2f(right2, top2);
                quad[3].position = sf::Vector2f(right2, bottom2);

                quad[0].texCoords = sf::Vector2f(_left2, _bottom2);
                quad[1].texCoords = sf::Vector2f(_left2, _top2);
                quad[2].texCoords = sf::Vector2f(_right2, _top2);
                quad[3].texCoords = sf::Vector2f(_right2, _bottom2);

                quad[0].color = sf::Color(0xFF, 0xFF, 0xFF, 145);
                quad[1].color = sf::Color(0xFF, 0xFF, 0xFF, 145);
                quad[2].color = sf::Color(0xFF, 0xFF, 0xFF, 145);
                quad[3].color = sf::Color(0xFF, 0xFF, 0xFF, 145);

                r_render_texture.draw(v_background_array, frame_texture);
                v_background_array.clear();
                size = 0;
            }

            if (object->m_item_type != ITEMTYPE_SEED) {
                float left = x + 2;
                float right = x + 18;
                float top = (world->GetSize().m_y * 32) - (y + 2);
                float bottom = ((world->GetSize().m_y * 32) - y) - 18;

                int offset_x = 0;
                int offset_y = 0;
 
                if (object->m_id == ITEM_GEMS) {
                    if (it->second.m_item_amount == 5)
                        offset_x = 1;
                    else if (it->second.m_item_amount == 10)
                        offset_x = 2;
                    else if (it->second.m_item_amount == 50)
                        offset_x = 3;
                    else if (it->second.m_item_amount == 100)
                        offset_x = 4;
                }

                float _left = (offset_x + object->m_default_texture_x * 32);
                float _right = ((offset_x + object->m_default_texture_x) * 32) + 32;
                float _top = (offset_y + object->m_default_texture_y) * 32;
                float _bottom = ((offset_y + object->m_default_texture_y) * 32) + 32;

                sf::Texture* object_texture = get_texture_from_cache(object->m_texture);
                if (!object_texture)
                    continue;
                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                quad[0].position = sf::Vector2f(left, bottom);
                quad[1].position = sf::Vector2f(left, top);
                quad[2].position = sf::Vector2f(right, top);
                quad[3].position = sf::Vector2f(right, bottom);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                r_render_texture.draw(v_background_array, object_texture);
                v_background_array.clear();
                size = 0;
            } else {
                float left = x + 2;
                float right = x + 18;
                float top = (world->GetSize().m_y * 32) - (y + 2);
                float bottom = ((world->GetSize().m_y * 32) - y) - 18;

                sf::Texture* seed_background = get_texture_from_cache("seed.rttex"); {
                    if (!seed_background)
                        continue;
                    int offset_x = object->m_seed_base;
                    int offset_y = 0;   

                    int seed_base_color = object->m_seed_color;

                    int seed_base_r = (seed_base_color >> 8) & 0xFF;
                    int seed_base_g = (seed_base_color >> 16) & 0xFF;
                    int seed_base_b = (seed_base_color >> 24) & 0xFF;
                    int seed_base_a = seed_base_color & 0xFF;

                    float _left = offset_x * 16;
                    float _right = (offset_x * 16) + 16;
                    float _top = offset_y * 16;
                    float _bottom = (offset_y * 16) + 16;

                    size += 4;

                    v_background_array.resize(size);
                    sf::Vertex* quad = &v_background_array[size - 4];

                    quad[0].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);
                    quad[1].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);
                    quad[2].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);
                    quad[3].color = sf::Color(seed_base_r, seed_base_g, seed_base_b, seed_base_a);

                    quad[0].position = sf::Vector2f(left, bottom);
                    quad[1].position = sf::Vector2f(left, top);
                    quad[2].position = sf::Vector2f(right, top);
                    quad[3].position = sf::Vector2f(right, bottom);

                    quad[0].texCoords = sf::Vector2f(_left, _bottom);
                    quad[1].texCoords = sf::Vector2f(_left, _top);
                    quad[2].texCoords = sf::Vector2f(_right, _top);
                    quad[3].texCoords = sf::Vector2f(_right, _bottom);

                    r_render_texture.draw(v_background_array, seed_background);
                    v_background_array.clear();
                    size = 0;
                }

                sf::Texture* seed_foreground = get_texture_from_cache("seed.rttex"); {
                    if (!seed_foreground)
                        continue;
                    int offset_x = object->m_seed_overlay;
                    int offset_y = 1;   

                    int seed_overlay_color = object->m_seed_overlay_color;
                    
                    int seed_overlar_r = (seed_overlay_color >> 8) & 0xFF;
                    int seed_overlar_g = (seed_overlay_color >> 16) & 0xFF;
                    int seed_overlar_b = (seed_overlay_color >> 24) & 0xFF;
                    int seed_overlar_a = seed_overlay_color & 0xFF;

                    float _left = offset_x * 16;
                    float _right = (offset_x * 16) + 16;
                    float _top = offset_y * 16;
                    float _bottom = (offset_y * 16) + 16;

                    size += 4;

                    v_background_array.resize(size);
                    sf::Vertex* quad = &v_background_array[size - 4];

                    quad[0].color = sf::Color(seed_overlar_r, seed_overlar_g, seed_overlar_b, seed_overlar_a);
                    quad[1].color = sf::Color(seed_overlar_r, seed_overlar_g, seed_overlar_b, seed_overlar_a);
                    quad[2].color = sf::Color(seed_overlar_r, seed_overlar_g, seed_overlar_b, seed_overlar_a);
                    quad[3].color = sf::Color(seed_overlar_r, seed_overlar_g, seed_overlar_b, seed_overlar_a);

                    quad[0].position = sf::Vector2f(left, bottom);
                    quad[1].position = sf::Vector2f(left, top);
                    quad[2].position = sf::Vector2f(right, top);
                    quad[3].position = sf::Vector2f(right, bottom);

                    quad[0].texCoords = sf::Vector2f(_left, _bottom);
                    quad[1].texCoords = sf::Vector2f(_left, _top);
                    quad[2].texCoords = sf::Vector2f(_right, _top);
                    quad[3].texCoords = sf::Vector2f(_right, _bottom);

                    r_render_texture.draw(v_background_array, seed_foreground);
                    v_background_array.clear();
                    size = 0;
                }
            }

            if (object->m_id != ITEM_GEMS && object->m_item_type != ITEMTYPE_SEED) {
                float left2 = x;
                float right2 = x + 20;
                float top2 = (world->GetSize().m_y * 32) - y;
                float bottom2 = ((world->GetSize().m_y * 32) - y) - 20;

                int offset_x2 = 0;
                int offset_y2 = 0;

                if (object->m_item_type == ITEMTYPE_LOCK)
                    offset_x2 = 6;
                else if (object->m_item_category & ITEMFLAG2_UNTRADABLE)
                    offset_x2 = 8;
                else if (object->m_item_type == ITEMTYPE_CONSUMABLE)
                    offset_x2 = 5;

                float _left2 = offset_x2 * 20;
                float _right2 = (offset_x2 * 20) + 20;
                float _top2 = offset_y2 * 32;
                float _bottom2 = (offset_y2 * 20) + 20;

                sf::Texture* frame_texture = get_texture_from_cache("pickup_box.rttex"); {
                    if (!frame_texture)
                        continue;
                    size += 4;
                    v_background_array.resize(size);
                    sf::Vertex* quad = &v_background_array[size - 4];

                    quad[0].position = sf::Vector2f(left2, bottom2);
                    quad[1].position = sf::Vector2f(left2, top2);
                    quad[2].position = sf::Vector2f(right2, top2);
                    quad[3].position = sf::Vector2f(right2, bottom2);

                    quad[0].texCoords = sf::Vector2f(_left2, _bottom2);
                    quad[1].texCoords = sf::Vector2f(_left2, _top2);
                    quad[2].texCoords = sf::Vector2f(_right2, _top2);
                    quad[3].texCoords = sf::Vector2f(_right2, _bottom2);

                    quad[0].color = sf::Color(0xFF, 0xFF, 0xFF, 110);
                    quad[1].color = sf::Color(0xFF, 0xFF, 0xFF, 110);
                    quad[2].color = sf::Color(0xFF, 0xFF, 0xFF, 110);
                    quad[3].color = sf::Color(0xFF, 0xFF, 0xFF, 110);

                    r_render_texture.draw(v_background_array, frame_texture);
                    v_background_array.clear();
                    size = 0;
                }
                if (object_count > 1) {
                    sf::Text count_txt;
                    count_txt.setFont(*sf_century);
                    count_txt.setString(fmt::format("OC -> {}", object_count));
                    count_txt.setCharacterSize(55);
                    count_txt.setScale(1, -1);
                    count_txt.setFillColor(sf::Color(0xFF, 0xFF, 0xFF));
                    count_txt.setPosition(sf::Vector2f(_right2 + 5, _top2 - 4));

                    r_render_texture.draw(count_txt);
                }
            }
            ++it;
        }

        for (std::size_t index = 0; index < world->GetTiles().size(); ++index) {
            int x = static_cast<int>(index) % world->GetSize().m_x, y = static_cast<int>(index) / world->GetSize().m_x;
            ItemInfo* background = ItemDatabase::GetItem(world->GetTile(index)->GetBackground());
            ItemInfo* foreground = ItemDatabase::GetItem(world->GetTile(index)->GetForeground());

            float left = x * 32;
            float right = (x * 32) + 32;
            float top = (world->GetSize().m_y - y) * 32;
            float bottom = ((world->GetSize().m_y - y) * 32) - 32;

            /*if (world->GetTile(index)->IsFlagOn(TILEFLAG_WATER)) {
                sf::Texture* bg_texture = get_texture_from_cache("water.rttex");
                if (!bg_texture)
                    continue;
                
                int offset_x = 0;
                int offset_y = 0;

                bool top_left_2 = world->GetTile((index - world->GetSize().m_x) - 1)->IsFlagOn(TILEFLAG_WATER) ? true : false;
                bool top_2 = world->GetTile((index - world->GetSize().m_x))->IsFlagOn(TILEFLAG_WATER) ? true : false;
                bool top_right_2 = world->GetTile((index - world->GetSize().m_x) + 1)->IsFlagOn(TILEFLAG_WATER) ? true : false;

                bool left_2 = world->GetTile(index - 1)->IsFlagOn(TILEFLAG_WATER) ? true : false;
                bool right_2 = world->GetTile(index + 1)->IsFlagOn(TILEFLAG_WATER) ? true : false;

                bool bottom_left_2 = false;
                bool bottom_2 = false;
                bool bottom_right_2 = false;

                if (index < 5900) {
                    bottom_left_2 = world->GetTile((index + world->GetSize().m_x) - 1)->IsFlagOn(TILEFLAG_WATER) ? true : false;
                    bottom_2 = world->GetTile((index + world->GetSize().m_x))->IsFlagOn(TILEFLAG_WATER) ? true : false;
                    bottom_right_2 = world->GetTile((index + world->GetSize().m_x) + 1)->IsFlagOn(TILEFLAG_WATER) ? true : false;
                }

                if (!left_2 || !top_2)
                    top_left_2 = false;
                if (!left_2 || !bottom_2)
                    bottom_left_2 = false;
                if (!right_2 || !top_2)
                    top_right_2 = false;
                if (!right_2 || !bottom_2)
                    bottom_right_2 = false;

                int bit = 1 * top_left_2 + 2 * top_2 + 4 * top_right_2 + 8 * left_2 + 16 * right_2 + 32 * bottom_left_2 + 64 * bottom_2 + 128 * bottom_right_2;

                offset_x = lut_8bit[bit] % 8;
                offset_y = lut_8bit[bit] / 8;

                float _left = (offset_x)*32;
                float _right = ((offset_x)*32) + 32;
                float _top = (offset_y)*32;
                float _bottom = ((offset_y)*32) + 32;

                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                quad[0].position = sf::Vector2f(left, bottom);
                quad[1].position = sf::Vector2f(left, top);
                quad[2].position = sf::Vector2f(right, top);
                quad[3].position = sf::Vector2f(right, bottom);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                quad[0].color = sf::Color(0xFF, 0xFF, 0xFF, 160);
                quad[1].color = sf::Color(0xFF, 0xFF, 0xFF, 160);
                quad[2].color = sf::Color(0xFF, 0xFF, 0xFF, 160);
                quad[3].color = sf::Color(0xFF, 0xFF, 0xFF, 160);

                r_render_texture.draw(v_background_array, bg_texture);
                v_background_array.clear();
                size = 0;
            } else */if (world->GetTile(index)->IsFlagOn(TILEFLAG_FIRE)) {
                sf::Texture* bg_texture = get_texture_from_cache("fire.rttex");
                if (!bg_texture)
                    continue;
                int offset_x = 0;
                int offset_y = 0;

                bool top_left_2 = world->GetTile((index - world->GetSize().m_x) - 1)->IsFlagOn(TILEFLAG_FIRE) ? true : false;
                bool top_2 = world->GetTile((index - world->GetSize().m_x))->IsFlagOn(TILEFLAG_FIRE) ? true : false;
                bool top_right_2 = world->GetTile((index - world->GetSize().m_x) + 1)->IsFlagOn(TILEFLAG_FIRE) ? true : false;

                bool left_2 = world->GetTile(index - 1)->IsFlagOn(TILEFLAG_FIRE) ? true : false;
                bool right_2 = world->GetTile(index + 1)->IsFlagOn(TILEFLAG_FIRE) ? true : false;

                bool bottom_left_2 = false;
                bool bottom_2 = false;
                bool bottom_right_2 = false;

                if (index < 5900) {
                    bottom_left_2 = world->GetTile((index + world->GetSize().m_x) - 1)->IsFlagOn(TILEFLAG_FIRE) ? true : false;
                    bottom_2 = world->GetTile((index + world->GetSize().m_x))->IsFlagOn(TILEFLAG_FIRE) ? true : false;
                    bottom_right_2 = world->GetTile((index + world->GetSize().m_x) + 1)->IsFlagOn(TILEFLAG_FIRE) ? true : false;
                }

                if (!left_2 || !top_2)
                    top_left_2 = false;
                if (!left_2 || !bottom_2)
                    bottom_left_2 = false;
                if (!right_2 || !top_2)
                    top_right_2 = false;
                if (!right_2 || !bottom_2)
                    bottom_right_2 = false;

                int bit = 1 * top_left_2 + 2 * top_2 + 4 * top_right_2 + 8 * left_2 + 16 * right_2 + 32 * bottom_left_2 + 64 * bottom_2 + 128 * bottom_right_2;

                offset_x = lut_8bit[bit] % 8;
                offset_y = lut_8bit[bit] / 8;

                float _left = (offset_x)*32;
                float _right = ((offset_x)*32) + 32;
                float _top = (offset_y)*32;
                float _bottom = ((offset_y)*32) + 32;

                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                quad[0].position = sf::Vector2f(left, bottom);
                quad[1].position = sf::Vector2f(left, top);
                quad[2].position = sf::Vector2f(right, top);
                quad[3].position = sf::Vector2f(right, bottom);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                quad[0].color = sf::Color(0xFF, 0xFF, 0xFF, 150);
                quad[1].color = sf::Color(0xFF, 0xFF, 0xFF, 150);
                quad[2].color = sf::Color(0xFF, 0xFF, 0xFF, 150);
                quad[3].color = sf::Color(0xFF, 0xFF, 0xFF, 150);

                r_render_texture.draw(v_background_array, bg_texture);
                v_background_array.clear();
                size = 0;
            }

            if (!(world->GetTile(index)->IsFlagOn(TILEFLAG_LOCKED) && world->GetTile(index)->GetParent() != 0))
                continue;
            bool top_pos_locked = world->GetTile(index - world->GetSize().m_x)->IsFlagOn(TILEFLAG_LOCKED) ? true : false;
            bool left_pos_locked = world->GetTile(index - 1)->IsFlagOn(TILEFLAG_LOCKED) ? true : false;
            bool right_pos_locked = world->GetTile(index + 1)->IsFlagOn(TILEFLAG_LOCKED) ? true : false;
            bool bottom_pos_locked = world->GetTile(index + world->GetSize().m_x)->IsFlagOn(TILEFLAG_LOCKED) ? true : false;
            bool center_pos_locked = world->GetTile(index)->IsFlagOn(TILEFLAG_LOCKED) ? true : false;

            if ((center_pos_locked || (world->GetTile(index)->GetBaseItem()->m_item_type == ITEMTYPE_LOCK && !world->GetTile(index)->GetBaseItem()->IsWorldLock()))
            && (!top_pos_locked && world->GetTile(index - world->GetSize().m_x)->GetParent() != world->GetTile(index)->GetParent())) {
                sf::Texture* bg_texture = get_texture_from_cache("lock_outline.rttex");
                if (!bg_texture)
                    continue;
                int offset_x = 3;
                int offset_y = 0;

                float _left = (offset_x)*32;
                float _right = ((offset_x)*32) + 32;
                float _top = (offset_y)*32;
                float _bottom = ((offset_y)*32) + 32;

                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                quad[0].position = sf::Vector2f(left, bottom);
                quad[1].position = sf::Vector2f(left, top);
                quad[2].position = sf::Vector2f(right, top);
                quad[3].position = sf::Vector2f(right, bottom);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                quad[0].color = sf::Color(172, 0, 0, 0xFF);
                quad[1].color = sf::Color(172, 0, 0, 0xFF);
                quad[2].color = sf::Color(172, 0, 0, 0xFF);
                quad[3].color = sf::Color(172, 0, 0, 0xFF);

                r_render_texture.draw(v_background_array, bg_texture);
                v_background_array.clear();
                size = 0;
            }

            if ((center_pos_locked || (world->GetTile(index)->GetBaseItem()->m_item_type == ITEMTYPE_LOCK && !world->GetTile(index)->GetBaseItem()->IsWorldLock()))
            && (!left_pos_locked && world->GetTile(index - 1)->GetParent() != world->GetTile(index)->GetParent())) {
                sf::Texture* bg_texture = get_texture_from_cache("lock_outline.rttex");
                if (!bg_texture)
                    continue;
                int offset_x = 2;
                int offset_y = 0;

                float _left = (offset_x)*32;
                float _right = ((offset_x)*32) + 32;
                float _top = (offset_y)*32;
                float _bottom = ((offset_y)*32) + 32;

                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                quad[0].position = sf::Vector2f(left, bottom);
                quad[1].position = sf::Vector2f(left, top);
                quad[2].position = sf::Vector2f(right, top);
                quad[3].position = sf::Vector2f(right, bottom);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                quad[0].color = sf::Color(172, 0, 0, 0xFF);
                quad[1].color = sf::Color(172, 0, 0, 0xFF);
                quad[2].color = sf::Color(172, 0, 0, 0xFF);
                quad[3].color = sf::Color(172, 0, 0, 0xFF);

                r_render_texture.draw(v_background_array, bg_texture);
                v_background_array.clear();
                size = 0;
            }

            if ((center_pos_locked || (world->GetTile(index)->GetBaseItem()->m_item_type == ITEMTYPE_LOCK && !world->GetTile(index)->GetBaseItem()->IsWorldLock()))
            && (!bottom_pos_locked && world->GetTile(index + world->GetSize().m_x)->GetParent() != world->GetTile(index)->GetParent())) {
                sf::Texture* bg_texture = get_texture_from_cache("lock_outline.rttex");
                if (!bg_texture)
                    continue;
                int offset_x = 1;
                int offset_y = 0;

                float _left = (offset_x)*32;
                float _right = ((offset_x)*32) + 32;
                float _top = (offset_y)*32;
                float _bottom = ((offset_y)*32) + 32;

                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                quad[0].position = sf::Vector2f(left, bottom);
                quad[1].position = sf::Vector2f(left, top);
                quad[2].position = sf::Vector2f(right, top);
                quad[3].position = sf::Vector2f(right, bottom);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                quad[0].color = sf::Color(172, 0, 0, 0xFF);
                quad[1].color = sf::Color(172, 0, 0, 0xFF);
                quad[2].color = sf::Color(172, 0, 0, 0xFF);
                quad[3].color = sf::Color(172, 0, 0, 0xFF);

                r_render_texture.draw(v_background_array, bg_texture);
                v_background_array.clear();
                size = 0;
            }
 
            if ((center_pos_locked || (world->GetTile(index)->GetBaseItem()->m_item_type == ITEMTYPE_LOCK && !world->GetTile(index)->GetBaseItem()->IsWorldLock()))
            && (!right_pos_locked && world->GetTile(index + 1)->GetParent() != world->GetTile(index)->GetParent())) {
                sf::Texture* bg_texture = get_texture_from_cache("lock_outline.rttex");
                if (!bg_texture)
                    continue;
                int offset_x = 0;
                int offset_y = 0;

                float _left = (offset_x)*32;
                float _right = ((offset_x)*32) + 32;
                float _top = (offset_y)*32;
                float _bottom = ((offset_y)*32) + 32;

                size += 4;
                v_background_array.resize(size);
                sf::Vertex* quad = &v_background_array[size - 4];

                quad[0].position = sf::Vector2f(left, bottom);
                quad[1].position = sf::Vector2f(left, top);
                quad[2].position = sf::Vector2f(right, top);
                quad[3].position = sf::Vector2f(right, bottom);

                quad[0].texCoords = sf::Vector2f(_left, _bottom);
                quad[1].texCoords = sf::Vector2f(_left, _top);
                quad[2].texCoords = sf::Vector2f(_right, _top);
                quad[3].texCoords = sf::Vector2f(_right, _bottom);

                quad[0].color = sf::Color(172, 0, 0, 0xFF);
                quad[1].color = sf::Color(172, 0, 0, 0xFF);
                quad[2].color = sf::Color(172, 0, 0, 0xFF);
                quad[3].color = sf::Color(172, 0, 0, 0xFF);

                r_render_texture.draw(v_background_array, bg_texture);
                v_background_array.clear();
                size = 0;
            }
        }
        float left2 = 0;
        float right2 = 3200;
        float top2 = 192;
        float bottom2 = 0;

        size += 4;
        v_background_array.resize(size);
        sf::Vertex* footer = &v_background_array[size - 4];

        footer[0].position = sf::Vector2f(left2, bottom2);
        footer[1].position = sf::Vector2f(left2, top2);
        footer[2].position = sf::Vector2f(right2, top2);
        footer[3].position = sf::Vector2f(right2, bottom2);

        footer[0].color = sf::Color(0, 0, 0, 140);
        footer[1].color = sf::Color(0, 0, 0, 140);
        footer[2].color = sf::Color(0, 0, 0, 140);
        footer[3].color = sf::Color(0, 0, 0, 140);

        r_render_texture.draw(v_background_array);
        v_background_array.clear();
        size = 0;

        float x_text_offset = 170;
        float y_text_offset = 62;

        sf::Text footer_02;
        footer_02.setFont(*sf_century);
        footer_02.setString("in");
        footer_02.setCharacterSize(55);
        footer_02.setScale(1, -1);
        footer_02.setFillColor(sf::Color(0xFF, 144, 243));
        footer_02.setPosition(sf::Vector2f(3096 - x_text_offset, 190 - y_text_offset));

        r_render_texture.draw(footer_02);

        sf::Text footer_03;
        footer_03.setFont(*sf_century);
        footer_03.setString(fmt::format("\"{}\"", world->GetName()));
        footer_03.setCharacterSize(55);
        footer_03.setScale(1, -1);
        footer_03.setFillColor(sf::Color(0xFF, 0xFF, 0xFF));
        footer_03.setPosition(sf::Vector2f(3078 - footer_03.getGlobalBounds().width - x_text_offset, 190 - y_text_offset));

        r_render_texture.draw(footer_03);

        sf::Text footer_01;
        footer_01.setFont(*sf_century);
        footer_01.setString("Visit");
        footer_01.setCharacterSize(55);
        footer_01.setScale(1, -1);
        footer_01.setFillColor(sf::Color(0xFF, 144, 243));
        footer_01.setPosition(sf::Vector2f(2956 - (footer_03.getGlobalBounds().width + 3) - x_text_offset, 190 - y_text_offset));
        r_render_texture.draw(footer_01);

        sf::Texture* server_logo = get_texture_from_cache("server_logo.rttex");
        if (!server_logo)
            return RENDER_RESULT_FAILED;
        size += 4;
        v_background_array.resize(size);
        sf::Vertex* main_logo = &v_background_array[size - 4];

        float _left = 0;
        float _right = 427;
        float _top = 0;
        float _bottom = 427;

        float offx = 3000;
        float offy = 10;

        float left = offx + 0;
        float right = offx + 175;
        float top = offy + 0;
        float bottom = offy + 180;

        main_logo[0].position = sf::Vector2f(left, top);
        main_logo[1].position = sf::Vector2f(left, bottom);
        main_logo[2].position = sf::Vector2f(right, bottom);
        main_logo[3].position = sf::Vector2f(right, top);

        main_logo[0].texCoords = sf::Vector2f(_left, _bottom);
        main_logo[1].texCoords = sf::Vector2f(_left, _top);
        main_logo[2].texCoords = sf::Vector2f(_right, _top);
        main_logo[3].texCoords = sf::Vector2f(_right, _bottom);

        r_render_texture.draw(v_background_array, server_logo);
        v_background_array.clear();
        size = 0;
        
        if (world->IsOwned()) {
            std::shared_ptr<Player> target = server_pool->GetPlayerByUserID(world->GetOwnerId());
            if (!target)
                return RENDER_RESULT_FAILED;
            auto skin_color = target->GetSkinColor();
            std::array<ItemInfo*, NUM_BODY_PARTS> item {
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_HAIR)),
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_SHIRT)),
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_PANTS)),
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_FEET)),
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_FACE)),
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_HAND)),
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_BACK)),
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_MASK)),
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_NECKLACE)),
                 ItemDatabase::GetItem(target->GetCloth(CLOTHTYPE_ANCES))
            };

            int owner_style = (std::rand() % 3) + 1;
            switch (owner_style) {    
                case 1:
                case 2:
                case 3: {
                    int x = 56;
                    int y = 125;

                    sf::Texture* player_back = get_texture_from_cache(item[CLOTHTYPE_BACK]->m_texture);
                    if (player_back && item[CLOTHTYPE_BACK]->m_id != ITEM_BLANK) {
                        int x_off = item[CLOTHTYPE_BACK]->m_texture_x;
                        int y_off = item[CLOTHTYPE_BACK]->m_texture_y;

                        sf::Sprite p_back;
                        p_back.setTexture(*player_back);
                        p_back.setTextureRect(sf::IntRect(x_off * 32, (y_off * 2) * 32, 32, 32));
                        p_back.setPosition(x, y);
                        p_back.setScale(3, -3);
                        r_render_texture.draw(p_back);
                    }

                    sf::Texture* player_arm_right = get_texture_from_cache("player_arm.rttex");
                    if (player_arm_right) {
                        sf::Sprite p_arm_right;
                        p_arm_right.setTexture(*player_arm_right);
                        p_arm_right.setTextureRect(sf::IntRect(0, 0, 32, 32));
                        p_arm_right.setPosition(x + 63, y - 54);
                        p_arm_right.setScale(3, -3);
                        p_arm_right.setColor(sf::Color(skin_color.GetRed(), skin_color.GetGreen(), skin_color.GetBlue(), skin_color.GetAlpha()));
                        r_render_texture.draw(p_arm_right);
                    }

                    sf::Texture* player_body = get_texture_from_cache("player_head.rttex");
                    if (player_body) {
                        sf::Sprite p_body;
                        p_body.setTexture(*player_body);
                        p_body.setTextureRect(sf::IntRect(0, 0, 32, 32));
                        p_body.setPosition(x, y);
                        p_body.setScale(3, -3);
                        p_body.setColor(sf::Color(skin_color.GetRed(), skin_color.GetGreen(), skin_color.GetBlue(), skin_color.GetAlpha()));
                        r_render_texture.draw(p_body);
                    }

                    sf::Texture* player_extraleg = get_texture_from_cache("player_extraleg.rttex");
                    if (player_extraleg) { // TODO
                        sf::Sprite p_extraleg;
                        p_extraleg.setTexture(*player_extraleg);
                        p_extraleg.setTextureRect(sf::IntRect(0, 0, 16, 16));
                        p_extraleg.setPosition(x + 24, y - 84);
                        p_extraleg.setScale(3, -3);
                        p_extraleg.setColor(sf::Color(skin_color.GetRed(), skin_color.GetGreen(), skin_color.GetBlue(), skin_color.GetAlpha()));
                        r_render_texture.draw(p_extraleg);
                    }
                
                    sf::Texture* player_feet = get_texture_from_cache(item[CLOTHTYPE_FEET]->m_id == ITEM_BLANK ? "player_feet.rttex" : item[CLOTHTYPE_FEET]->m_texture);
                    if (player_feet) {
                        int x_off = item[CLOTHTYPE_FEET]->m_texture_x,
                            y_off = item[CLOTHTYPE_FEET]->m_texture_y;

                        sf::Sprite p_feet_left;
                        p_feet_left.setTexture(*player_feet);
                        p_feet_left.setTextureRect(sf::IntRect(x_off * 32, (y_off * 2) * 32, 32, 32));
                        p_feet_left.setPosition(x, y);
                        p_feet_left.setScale(3, -3);
                        if (item[CLOTHTYPE_FEET]->m_id == ITEM_BLANK)
                            p_feet_left.setColor(sf::Color(skin_color.GetRed(), skin_color.GetGreen(), skin_color.GetBlue(), skin_color.GetAlpha()));
                        r_render_texture.draw(p_feet_left);

                        sf::Sprite p_feet_right;
                        p_feet_right.setTexture(*player_feet);
                        p_feet_right.setTextureRect(sf::IntRect(x_off * 32, ((y_off * 2) * 32) + 32, 32, 32));
                        p_feet_right.setPosition(x, y);
                        p_feet_right.setScale(3, -3);
                        if (item[CLOTHTYPE_FEET]->m_id == ITEM_BLANK)
                            p_feet_right.setColor(sf::Color(skin_color.GetRed(), skin_color.GetGreen(), skin_color.GetBlue(), skin_color.GetAlpha()));
                        r_render_texture.draw(p_feet_right);
                    }

                    sf::Texture* player_eyes_default = get_texture_from_cache("player_eyes.rttex");
                    if (player_eyes_default) {
                        int x_off = 0;
                        int y_off = 0;

                        sf::Sprite p_eyes;
                        p_eyes.setTexture(*player_eyes_default);
                        p_eyes.setTextureRect(sf::IntRect(x_off * 32, y_off * 32, 32, 32));
                        p_eyes.setPosition(x, y);
                        p_eyes.setScale(3, -3);
                        r_render_texture.draw(p_eyes);
                    }

                    sf::Texture* player_eyes_default_2 = get_texture_from_cache("player_eyes.rttex");
                    if (player_eyes_default_2) {
                        int x_off = 0;
                        int y_off = 4;

                        sf::Sprite p_eyes;
                        p_eyes.setTexture(*player_eyes_default_2);
                        p_eyes.setTextureRect(sf::IntRect(x_off * 32, y_off * 32, 32, 32));
                        p_eyes.setPosition(x, y);
                        p_eyes.setScale(3, -3);
                        p_eyes.setColor(sf::Color(skin_color.GetRed(), skin_color.GetGreen(), skin_color.GetBlue(), skin_color.GetAlpha()));
                        r_render_texture.draw(p_eyes);
                    }

                    sf::Texture* player_eyes_default_3 = get_texture_from_cache("player_eyes2.rttex");
                    if (player_eyes_default_3) {
                        int x_off = 0;
                        int y_off = 0;

                        sf::Sprite p_eyes;
                        p_eyes.setTexture(*player_eyes_default_3);
                        p_eyes.setTextureRect(sf::IntRect(x_off * 32, y_off * 32, 32, 32));
                        p_eyes.setPosition(x, y);
                        p_eyes.setScale(3, -3);
                        p_eyes.setColor(sf::Color(0, 0, 0, 225));
                        r_render_texture.draw(p_eyes);
                    }

                    sf::Texture* player_neck = get_texture_from_cache(item[CLOTHTYPE_NECKLACE]->m_texture);
                    if (player_neck && item[CLOTHTYPE_NECKLACE]->m_id != ITEM_BLANK) {
                        int x_off = item[CLOTHTYPE_NECKLACE]->m_texture_x,
                            y_off = item[CLOTHTYPE_NECKLACE]->m_texture_y;

                        sf::Sprite p_neck;
                        p_neck.setTexture(*player_neck);
                        p_neck.setTextureRect(sf::IntRect(x_off * 32, y_off * 32, 32, 32));
                        p_neck.setPosition(x, y);
                        p_neck.setScale(3, -3);
                        r_render_texture.draw(p_neck);
                    }

                    sf::Texture* player_eyes = get_texture_from_cache(item[CLOTHTYPE_FACE]->m_texture);
                    if (player_eyes && item[CLOTHTYPE_FACE]->m_id != ITEM_BLANK) {
                        int x_off = item[CLOTHTYPE_FACE]->m_texture_x,
                            y_off = item[CLOTHTYPE_FACE]->m_texture_y;

                        sf::Sprite p_eyes;
                        p_eyes.setTexture(*player_eyes);
                        p_eyes.setTextureRect(sf::IntRect(x_off * 32, y_off * 32, 32, 32));
                        p_eyes.setPosition(x, y);
                        p_eyes.setScale(3, -3);
                        r_render_texture.draw(p_eyes);
                    }

                    sf::Texture* player_hair = get_texture_from_cache(item[CLOTHTYPE_MASK]->m_texture);
                    if (player_hair && item[CLOTHTYPE_MASK]->m_id != ITEM_BLANK) {
                        int x_off = item[CLOTHTYPE_MASK]->m_texture_x,
                            y_off = item[CLOTHTYPE_MASK]->m_texture_y;
                        int actual_x = x, actual_y = y + 48;
                        
                        if (item[CLOTHTYPE_MASK]->m_texture == "player_hairhair.rttex") {
                            if ((x_off == 4 && y_off == 0)) { actual_y = y; }
                            else if ((x_off == 4 && y_off == 3)) { actual_y = y; }
                            else if ((x_off == 7 && y_off == 3)) { actual_y = y; }
                            else if ((x_off == 0 && y_off == 4)) { actual_y = y; }
                            else if ((x_off == 1 && y_off == 4)) { actual_y = y; }
                            else if ((x_off == 2 && y_off == 5)) { actual_y = y; }
                            else if ((x_off == 4 && y_off == 7)) { actual_y = y; }
                            else if ((x_off == 6 && y_off == 7)) { actual_y = y; }
                        }
                        else if (item[CLOTHTYPE_MASK]->m_texture == "player_hairhair2.rttex") {
                            if ((x_off == 7 && y_off == 6)) { actual_y = y; }
                        }

                        sf::Sprite p_mask;
                        p_mask.setTexture(*player_hair);
                        p_mask.setTextureRect(sf::IntRect(x_off * 32, y_off * 32, 32, 32));
                        p_mask.setPosition(actual_x, actual_y);
                        p_mask.setScale(3, -3);
                        r_render_texture.draw(p_mask);
                    }

                    sf::Texture* player_hat = get_texture_from_cache(item[CLOTHTYPE_HAIR]->m_texture);
                    if (player_hat && item[CLOTHTYPE_HAIR]->m_id != ITEM_BLANK) {
                        int x_off = item[CLOTHTYPE_HAIR]->m_texture_x,
                            y_off = item[CLOTHTYPE_HAIR]->m_texture_y;

                        sf::Sprite p_hat;
                        p_hat.setTexture(*player_hat);
                        p_hat.setTextureRect(sf::IntRect(x_off * 32, y_off * 32, 32, 32));
                        p_hat.setPosition(x, y + 45);
                        p_hat.setScale(3, -3);
                        r_render_texture.draw(p_hat);
                    }

                    sf::Texture* player_hand = get_texture_from_cache(item[CLOTHTYPE_HAND]->m_texture);
                    if (player_hand && item[CLOTHTYPE_HAND]->m_id != ITEM_BLANK) {
                        int x_off = item[CLOTHTYPE_HAND]->m_texture_x,
                            y_off = item[CLOTHTYPE_HAND]->m_texture_y;

                        sf::Sprite p_hand;
                        p_hand.setTexture(*player_hand);
                        p_hand.setTextureRect(sf::IntRect(x_off * 32, y_off * 32, 32, 32));
                        p_hand.setPosition(x - 21, y - 54);
                        p_hand.setScale(3, -3);
                        r_render_texture.draw(p_hand);
                    }

                    sf::Texture* player_arm_left = get_texture_from_cache("player_arm.rttex");
                    if (player_arm_left) {
                        sf::Sprite p_arm_left;
                        p_arm_left.setTexture(*player_arm_left);
                        p_arm_left.setTextureRect(sf::IntRect(0, 0, 32, 32));
                        p_arm_left.setPosition(x + 18, y - 54);
                        p_arm_left.setScale(3, -3);
                        p_arm_left.setColor(sf::Color(skin_color.GetRed(), skin_color.GetGreen(), skin_color.GetBlue(), skin_color.GetAlpha()));
                        r_render_texture.draw(p_arm_left);
                    }
    
                    sf::String p_name = target->GetRawName();
                    sf::String p_shadow_name = remove_gt_color("`", p_name);

                    sf::Text player_name_shadow;
                    player_name_shadow.setFont(*sf_century);
                    player_name_shadow.setString(p_shadow_name);
                    player_name_shadow.setCharacterSize(22);
                    player_name_shadow.setScale(1, -1);
                    player_name_shadow.setFillColor(sf::Color(0, 0, 0, 110));
                    player_name_shadow.setPosition(sf::Vector2f(((x + 29) - (player_name_shadow.getLocalBounds().width / 2) - 2) + 35, y + 35 - 2));
                    r_render_texture.draw(player_name_shadow);

                    FText player_name;
                    player_name.setFont(*sf_century);
                    player_name.setString(p_name);
                    player_name.setCharacterSize(22);
                    player_name.setScale(1, -1);
                   // player_name.setFillColor(sf::Color(73,252,0));
                    player_name.setPosition(sf::Vector2f(((x + 29) - (player_name.getLocalBounds().width / 2)) + 35, y + 35));
                    r_render_texture.draw(player_name);

                    if (target->GetRole() == PLAYER_ROLE_DEVELOPER) {
                        sf::Texture* player_flag = get_texture_from_cache("zz.rttex");
                        if (player_flag) {
                            sf::Sprite p_flag;
                            p_flag.setTexture(*player_flag);
                            p_flag.setTextureRect(sf::IntRect(0, 0, 15, 10));
                            p_flag.setPosition((x - player_name.getLocalBounds().width / 2) + 29, y + 32);
                            p_flag.setScale(2, -2);
                            r_render_texture.draw(p_flag);
                        }
                    }
                    else {
                        sf::Texture* player_flag = get_texture_from_cache(fmt::format("{}.rttex", target->GetLoginDetail()->m_country));
                        if (player_flag) {
                            sf::Sprite p_flag;
                            p_flag.setTexture(*player_flag);
                            p_flag.setTextureRect(sf::IntRect(0, 0, 15, 10));
                            p_flag.setPosition((x - player_name.getLocalBounds().width / 2) + 29, y + 32);
                            p_flag.setScale(2, -2);
                            r_render_texture.draw(p_flag);
                        }
                    }

                    break;
                }
                default:
                    break;
            }
            if (!server_pool->HasPlayer(world->GetOwnerId()))
                target.reset();
        }

        sf::Texture texture = r_render_texture.getTexture();
        sf::Image image = texture.copyToImage();

        if(!image.saveToFile(fmt::format("renders/{}.png", world->GetName())))
            fmt::print("WorldRender::Render -> couldn't output image file for world \"{}\"", world->GetName());
        }
        catch(std::exception& e) {
            fmt::print("error: {}\n", e.what());
        }
        return RENDER_RESULT_SUCCESS;
    }
}