/*****************************************************************************
 * DBConnection.cpp: Wraps the DB connection
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

#include "DBConnection.hpp"

#include "Table.hpp"

using namespace vsqlite;

void
DBConnection::registerTableSchema( ITableSchema* schema )
{
    instance().m_tables.push_back( schema );
}

void
DBConnection::createTables()
{
    for ( auto t : m_tables )
    {
        if ( t->create().execute( DBConnection::instance().rawConnection() ) == false )
        {
            std::cerr << "Failed to create table \"" << t->name() << '"' << std::endl;
            return;
        }
    }
}

bool
DBConnection::_init(const std::string& dbPath)
{
    int res = sqlite3_open( dbPath.c_str(), &m_db );
    m_isValid = ( res == SQLITE_OK );
    if ( m_isValid )
    {
        createTables();
    }
    return m_isValid;
}

DBConnection::~DBConnection()
{
    _close();
}

void
DBConnection::_close()
{
    sqlite3_close( instance().m_db );
    instance().m_db = NULL;
}

void DBConnection::close()
{
    instance()._close();
}
