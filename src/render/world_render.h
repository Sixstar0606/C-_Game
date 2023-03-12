#pragma once
#include <memory>
#include <unordered_map>
#include <SFML/Graphics.hpp>

namespace GTServer {
    class World;
    class ServerPool;
    class WorldRender {
    public:
        enum eRenderResult {
            RENDER_RESULT_SUCCESS,
            RENDER_RESULT_FAILED
        };

    public:
        WorldRender() = default;
        ~WorldRender();

        void load_caches();
        static sf::Texture* get_texture_from_cache(const std::string& file) { return get().get_texture_from_cache__interface(file); }
        static eRenderResult render(ServerPool* server_pool, const std::shared_ptr<World>& world) { return get().render__interface(server_pool, world); }
    public:
        static WorldRender& get() { static WorldRender ret; return ret; }

    private:
        sf::Texture* get_texture_from_cache__interface(const std::string& file);
        eRenderResult render__interface(ServerPool* server_pool, const std::shared_ptr<World>& world);
    private: 
        std::unordered_map<std::string, sf::Texture*> t_cache;
        std::unordered_map<std::string, sf::Texture*> t_weather_cache;
        std::unordered_map<std::string, sf::Texture*> t_border_cache;

        sf::Font* sf_century;
        sf::Font* sf_gothic_regular;

        int lut_8bit[0xFF];

    private:
        std::string v_background_path[65] = {
            "Sunny.png", // 0
            "Beach.png", // 1
            "Night.png", // 2
            "Arid.png", // 3
            "Sunny.png", // 4
            "Rainy.png", // 5
            "Harvest.png", // 6
            "Mars.png", // 7
            "Spooky.png", // 8
            "Nothing.png",
            "Nothing.png",
            "Snowy.png",
            "Nothing.png",
            "Nothing.png",
            "Undersea.png",
            "Warp.png",
            "Comet.png",
            "Comet.png",
            "Party.png",
            "Pineapples.png",
            "Snowy Night.png", // 20
            "Spring.png",
            "Howing Sky.png",
            "Sunny.png",
            "Sunny.png",
            "Sunny.png",
            "Sunny.png",
            "Sunny.png",
            "Sunny.png",
            "Sunny.png",
            "Pagoda.png",
            "Apocalypse.png",
            "Jungle.png",
            "Balloon.png",
            "Sunny.png", // 34
            "Autumn.png",
            "Valentines.png", // 36
            "St Patricks.png",
            "Epoch - Iceberg.png",
            "Epoch - Lava.png",
            "Epoch - Skylands.png",
            "Sunny.png", // 41
            "Digital Rain.png",
            "Monochrome.png",
            "Frozen Cliffs.png",
            "Sunny.png",
            "Sunny.png",
            "", // 47
            "",
            "",
            "", // 50
            "", 
            "",
            "",
            "vapor.png", // 54
            "princepersia.png", // 55
            "", // 56
            "",
            "",
            "",
            "",
            "",
            "",
            "", //63
        }; 
    };
}