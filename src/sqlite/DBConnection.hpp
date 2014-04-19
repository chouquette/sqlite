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

#include "Operation.hpp"

namespace vsqlite
{

class ITableSchema;

class DBConnection
{
    public:
        static DBConnection* init(const std::string& dbPath)
        {
            int res = sqlite3_open( dbPath.c_str(), &s_instance.m_db );
            s_instance.m_isValid = ( res == SQLITE_OK );
            if ( s_instance.m_isValid )
            {
                s_instance.createTables();
            }
            return &s_instance;
        }

        bool isValid() const { return m_isValid; }
        const char* errorMsg() const { return sqlite3_errmsg( m_db ); }

        // For test purposes. There might be a better way.
        sqlite3*    rawConnection() { return m_db; }

        template <typename T>
        std::vector<T> execute(Operation<T>&& op)
        {
            using ResType = std::vector<T>;
            if ( m_isValid == false )
            {
                std::cerr << "Ignoring request on invalid connection" << std::endl;
                return ResType();
            }
            if ( op.execute( m_db ) == false )
            {
                return ResType();
            }
            ResType result = parseResults<T>( op );
            return result;
            // The operation & the associated sqlite3_stmt now falls out of scope and is cleaned
        }

        bool execute(Operation<bool>&& op)
        {
            if ( m_isValid == false )
            {
                std::cerr << "Ignoring request on invalid connection" << std::endl;
                return false;
            }
            if ( op.execute( m_db ) == false )
            {
                std::cerr << "SQLite error: " << errorMsg() << std::endl;
                return false;
            }
            // We still need to step on the request for it to be executed.
            int res = sqlite3_step( op );
            while ( res != SQLITE_DONE && res != SQLITE_ERROR )
            {
                res = sqlite3_step( op );
            }
            return res != SQLITE_ERROR;
            // The operation & the associated sqlite3_stmt now falls out of scope and is cleaned
        }

        static void registerTableSchema( ITableSchema* schema );

    private:
        DBConnection()
            : m_isValid( false )
        {
        }

        ~DBConnection()
        {
            sqlite3_close( m_db );
        }

        template <typename T>
        std::vector<T> parseResults( sqlite3_stmt* statement )
        {
            std::vector<T> results;
            while ( sqlite3_step( statement ) != SQLITE_DONE )
            {
                T row;
                const auto& attributes = T::schema->columns();
                for ( auto a : attributes )
                {
                    a->load( statement, row );
                }

                results.push_back( std::move( row ) );
            }
            return results;
        }

        void createTables();

    private:
        sqlite3*    m_db;
        bool        m_isValid;
        static DBConnection s_instance;
        static std::vector<ITableSchema*> s_tables;
};

}

#endif // DBCONNECTION_HPP
