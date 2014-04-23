/*****************************************************************************
 * DBConnection.hpp: Wraps the DB connection
 *****************************************************************************
 * Copyright (C) 2008-2014 VideoLAN
 *
 * Authors: Hugo Beauzée-Luyssen <hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef DBCONNECTION_HPP
#define DBCONNECTION_HPP

#include <iostream>
#include <sqlite3.h>
#include <string>
#include <vector>

namespace vsqlite
{

class ITableSchema;

class DBConnection
{
    public:
        static bool init( const std::string& dbPath )
        {
            return instance()._init( dbPath );
        }

        static void close();

        static DBConnection& instance()
        {
            static DBConnection s_instance;
            return s_instance;
        }

        bool isValid() const { return m_isValid; }
        const char* errorMsg() const { return sqlite3_errmsg( m_db ); }

        sqlite3*    rawConnection() { return m_db; }

        static void registerTableSchema( ITableSchema* schema );

    private:
        DBConnection()
            : m_isValid( false )
        {
        }

        ~DBConnection();

        bool _init( const std::string& dbPath );
        void _close();
        void createTables();

    private:
        sqlite3*    m_db;
        bool        m_isValid;
        std::vector<ITableSchema*> m_tables;
};

}

#endif // DBCONNECTION_HPP
