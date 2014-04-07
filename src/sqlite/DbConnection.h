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

#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <string>
#include <sqlite3.h>

namespace vsqlite
{

class Operation;

class DBConnection
{
    public:
        DBConnection(const std::string& dbPath);
        ~DBConnection();

        bool isValid() const;
        const char* errorMsg() const;

        DBConnection& operator<<(const Operation& op);

        // For test purposes. There might be a better way.
        sqlite3*    rawConnection();

    private:
        sqlite3*    m_db;
        bool        m_isValid;
};

}

#endif // DBCONNECTION_H
