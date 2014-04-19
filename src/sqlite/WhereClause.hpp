/*****************************************************************************
 * WhereClause.hpp: Handles conditionnal part of SQL queries
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

#ifndef WHERECLAUSE_HPP
#define WHERECLAUSE_HPP

#include <iostream>
#include "sqlite.hpp"

namespace vsqlite
{

class Predicate
{
    public:
        Predicate( const std::string& fieldName, std::function<int(sqlite3_stmt*, int)> bind )
            : m_fieldName( fieldName )
            , m_bind( bind )
        {
        }

        Predicate( Predicate&& p ) = default;
        const std::string& fieldName() const { return m_fieldName; }
        int bind( sqlite3_stmt* stmt, int index )
        {
            return m_bind(stmt, index);
        }

    private:
        std::string m_fieldName;
        std::function<int(sqlite3_stmt*, int)> m_bind;
};

class WhereClause
{
    public:
        WhereClause(){}
        WhereClause( Predicate&& predicate )
        {
            m_predicates.push_back( std::move( predicate ) );
        }
        WhereClause( WhereClause&& ) = default;
        WhereClause& operator=( WhereClause&& ) = default;

        WhereClause( const WhereClause& ) = delete;

        std::string generate() const
        {
            if ( m_predicates.empty() )
                return {};
            std::string res = " WHERE 1=1";
            for ( auto& p : m_predicates )
                res += " AND " + p.fieldName() + " == ?";
            return res;
        }

        bool bind( sqlite3_stmt* statement )
        {
            int bindIndex = 1;
            for ( auto& p : m_predicates )
            {
                int resultCode = p.bind( statement, bindIndex++ );
                if ( resultCode != SQLITE_OK )
                {
                    std::cerr << "Failed to bind predicate " << p.fieldName()
                              << ". Error code #" << resultCode<< std::endl;
                    return false;
                }
            }
            return true;
        }

    private:
        std::vector<Predicate> m_predicates;
};

}

#endif // WHERECLAUSE_HPP
