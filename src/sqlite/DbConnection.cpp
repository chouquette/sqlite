/*****************************************************************************
 * DcConnection.h: Handles the connection to SQLite database
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

#include "DbConnection.h"

#include "sqlite.hpp"

using namespace vsqlite;

DBConnection::DBConnection(const std::string& dbPath)
{
    int res = sqlite3_open( dbPath.c_str(), &m_db );
    m_isValid = ( res == 0 );
}

DBConnection::~DBConnection()
{
    sqlite3_close( m_db );
}

bool
DBConnection::isValid() const
{
    return m_isValid;
}

const char*
DBConnection::errorMsg() const
{
    return sqlite3_errmsg( m_db );
}

DBConnection&
DBConnection::operator<<(const Operation& op)
{
    if ( m_isValid == false )
    {
        fprintf(stderr, "Ignoring request on invalid connection");
        return *this;
    }
    char* errorMessage = NULL;
    if ( sqlite3_exec( m_db, op.m_request.c_str(), op.m_callback, NULL, &errorMessage) != SQLITE_OK)
    {
        fprintf(stderr, "SQLite error: %s", errorMessage);
        sqlite3_free( errorMessage );
        m_isValid = false;
    }
    return *this;
}

sqlite3*
DBConnection::rawConnection()
{
    return m_db;
}
