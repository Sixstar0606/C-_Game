#include <database/table/world_table.h>
#include <fmt/core.h>
#include <database/interface/world_i.h>
#include <config.h>
#include <utils/file_manager.h>

namespace GTServer {
    bool WorldTable::is_exist(const std::string& name) {
        WorldDB worlds{};
        for (const auto &row : (*m_connection)(select(all_of(worlds)).from(worlds).where(worlds.name == name)))
            if (row._is_valid)
                return true;
        return false;
    }

    uint32_t WorldTable::insert(std::shared_ptr<World> world) {
        if (this->is_exist(world->GetName()))
            return 0;
        WorldDB world_db{};
        auto now = sqlpp::chrono::floor<std::chrono::milliseconds>(system_clock::now());
        auto id = (*m_connection)(insert_into(world_db).set(
            world_db.name = world->GetName(),
            world_db.flags = world->GetFlags(),
            world_db.width = world->GetSize().m_x,
            world_db.height = world->GetSize().m_y,
            world_db.created_at = now,
            world_db.updated_at = now,
            world_db.objects = world->PackObjects(true),
            world_db.owner_id = world->GetOwnerId(),
            world_db.main_lock = world->GetMainLock(),
            world_db.weather_id = world->GetWeatherId(),
            world_db.base_weather_id = world->GetBaseWeatherId()
        ));
        const std::string& world_path{ fmt::format("{}_{}.bin", config::server::worlds_dir, id) };
        if (std::filesystem::exists(world_path))
            return 0;
        auto tiles{ world->PackTiles(true) };
        FileManager::write_all_bytes(world_path, reinterpret_cast<char*>(tiles.data()), tiles.size());
        return id;
    }
    bool WorldTable::save(std::shared_ptr<World> world) {
        try {
            if (!m_connection->is_valid()) {
                fmt::print("connection is dead, reconnecting...\n");
                m_connection->reconnect();
            }
            WorldDB world_db{};
            auto now = sqlpp::chrono::floor<std::chrono::milliseconds>(system_clock::now());
            (*m_connection)(update(world_db).set(
                world_db.name = world->GetName(),
                world_db.flags = world->GetFlags(),
                world_db.width = world->GetSize().m_x,
                world_db.height = world->GetSize().m_y,
                world_db.updated_at = now,
                world_db.objects = world->PackObjects(true),
                world_db.owner_id = world->GetOwnerId(),
                world_db.main_lock = world->GetMainLock(),
                world_db.weather_id = world->GetWeatherId(),
                world_db.base_weather_id = world->GetBaseWeatherId()
            ).where(world_db.id == world->GetID()));
            const std::string& world_path{ fmt::format("{}_{}.bin", config::server::worlds_dir, world->GetID()) };
            auto tiles{ world->PackTiles(true) };
            FileManager::write_all_bytes(world_path, reinterpret_cast<char*>(tiles.data()), tiles.size());
            return true;
        }
        catch(const std::exception &e) {
            fmt::print("exception from WorldTable::save -> {}\n", e.what());
            return false;
        }
        return false;
    }
    bool WorldTable::load(std::shared_ptr<World> world) {
        WorldDB world_db{};
        for (const auto &row : (*m_connection)(select(all_of(world_db)).from(world_db).where(
            world_db.name == world->GetName()
        ).limit(1u))) {
            if (row._is_valid) {
                world->SetID(static_cast<int32_t>(row.id));
                world->SetName(row.name);
                world->SetFlags(row.flags);
                world->SetSize(row.width, row.height);
                world->SetCreatedAt({ row.created_at.value() });
                world->SetUpdatedAt({ row.updated_at.value() });
                if (row.objects.value().size() > 0) {
                    BinaryReader br{ row.objects.value() };
                    auto& objects  = world->GetObjects();
                    uint32_t objects_count{ br.read<uint32_t>() };
                    world->SetObjectId(br.read<uint32_t>());

                    for (uint32_t index = 0; index < objects_count; ++index) {
                        WorldObject object{};
                        object.m_item_id = br.read<uint16_t>();
                        object.m_pos = CL_Vec2f{ br.read<float>(), br.read<float>() };
                        object.m_item_amount = br.read<uint8_t>();
                        object.m_flags = br.read<uint8_t>();
                        uint32_t object_index = br.read<uint32_t>();
                        objects.insert_or_assign(object_index, object);
                    }
                }
                world->SetOwnerId(row.owner_id);
                world->SetMainLock(row.main_lock);
                world->SetWeatherId(row.weather_id);
                world->SetBaseWeatherId(row.base_weather_id);

                const std::string& world_path{ fmt::format("{}_{}.bin", config::server::worlds_dir, world->GetID()) };
                if (!std::filesystem::exists(world_path)) {
                    world->Generate(WORLD_TYPE_NORMAL);
                    return true;
                }
                auto tiles_data{ FileManager::read_all_bytes(world_path) };

                BinaryReader br{ tiles_data };
                uint32_t tiles_count{ br.read<uint32_t>() };
                auto& tiles{ world->GetTiles() };

                tiles.reserve(tiles_count);
                for (uint32_t index = 0; index < tiles_count; index++) {
                    Tile& tile = tiles.emplace_back(Tile{});
                    tile.Serialize(br);
                }
                tiles_data.clear();
                return true;
            }
        }
        return false;
    }
}