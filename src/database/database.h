#pragma once
#include <algorithm>
#include <sqlpp11/sqlpp11.h>
#include <sqlpp11/mysql/mysql.h>
#include <config.h>
#include <server/server_pool.h>
#include <database/player_tribute.h>
#include <database/table/player_table.h>
#include <database/table/world_table.h>
#include <database/table/config_table.h>

#include <database/interface/player_i.h>
#include <database/interface/world_i.h>
#include <database/interface/config_i.h>

namespace GTServer
{
    class Database
    {
    public:
        enum eDatabaseTable
        {
            DATABASE_PLAYER_TABLE,
            DATABASE_WORLD_TABLE,
            DATABASE_CONFIG_TABLE
        };

    public:
        Database() = default;
        ~Database();

        bool Connect();

        static sqlpp::mysql::connection *GetConnection() { return Get().m_connection; }
        static void *GetTable(const eDatabaseTable &table) { return Get().GetTable_Interface(table); }

    public:
        static Database &Get()
        {
            static Database ret;
            return ret;
        }

    public:
        void* GetTable_Interface(const eDatabaseTable &table);

    private:
        sqlpp::mysql::connection *m_connection{nullptr};
        PlayerTable *m_player_table{nullptr};
        WorldTable *m_world_table{nullptr};
        ConfigTable *m_config_table{nullptr};
    };
}