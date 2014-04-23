/*****************************************************************************
 * Operation.hpp: Encapsulate a generated SQL request
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

#ifndef OPERATION_HPP
#define OPERATION_HPP

#include <cassert>
#include <sqlite3.h>
#include "DBConnection.hpp"

#include "WhereClause.hpp"

namespace vsqlite
{

class Operation
{
    public:
        Operation(const std::string& request)
            : m_request( request )
            , m_statement( NULL )
        {
        }

        Operation()
            : m_statement( NULL )
        {
        }

        virtual ~Operation()
        {
            sqlite3_finalize( m_statement );
        }

        Operation& operator+=( Operation&& op )
        {
            m_request += op.m_request;
            op.m_request = {};
            return *this;
        }

        Operation( const Operation& op ) = delete;
        Operation( Operation&& op ) = default;

    protected:
        virtual bool execute( sqlite3* db )
        {
            int resultCode = sqlite3_prepare_v2( db, m_request.c_str(), -1, &m_statement, NULL );
            if ( resultCode != SQLITE_OK )
            {
                std::cerr << "Failed to execute request " << m_request << '\n'
                             << "Error code: " << resultCode << '(' <<
                             DBConnection::instance().errorMsg() << ')' << std::endl;
                return false;
            }
            return true;
        }

        operator sqlite3_stmt*()
        {
            assert( m_statement != NULL );
            return m_statement;
        }

    protected:
        std::string m_request;
        sqlite3_stmt* m_statement;
};

template <typename T>
class FetchOperation : public Operation
{
    public:
        FetchOperation( const std::string& request )
            : Operation( request )
        {
        }

        operator T()
        {
            bool res = execute( DBConnection::instance().rawConnection() );
            if ( res == false )
                return T();
            auto results = parseResults();
            if ( results.size() == 0 )
                return T();
            return results[0];
        }

        operator std::vector<T>()
        {
            bool res = execute( DBConnection::instance().rawConnection() );
            if ( res == false )
                return std::vector<T>();
            auto results = parseResults();
            return results;
        }

        FetchOperation&& where( WhereClause&& clause )
        {
            m_whereClause = std::move( clause );
            return std::move( *this );
        }

        virtual bool execute( sqlite3 *db )
        {
            m_request += m_whereClause.generate();
            if ( Operation::execute( db ) == false )
                return false;
            return m_whereClause.bind( m_statement );
        }

    protected:
        std::vector<T> parseResults()
        {
            std::vector<T> results;
            while ( sqlite3_step( m_statement ) != SQLITE_DONE )
            {
                T row;
                const auto& attributes = T::schema->columns();
                for ( auto a : attributes )
                {
                    a->load( m_statement, row );
                }

                results.push_back( std::move( row ) );
            }
            return results;
        }

    protected:
        WhereClause m_whereClause;

};

template <typename CLASS>
class InsertOperation : public Operation
{
    public:
        InsertOperation( CLASS& record )
            : m_record( record )
        {
            m_request = "INSERT INTO " + CLASS::schema->name() + " VALUES(";
            const auto& columns = CLASS::schema->columns();
            for ( auto attr : columns )
            {
                m_request += attr->insert( record ) + ',';
            }
            m_request.replace(m_request.end() - 1, m_request.end(), ");");
        }

        virtual bool execute( sqlite3* db )
        {
            if ( Operation::execute( db ) == false )
                return false;
            // We still need to step on the request for it to be executed.
            int res = sqlite3_step( m_statement );
            while ( res != SQLITE_DONE && res != SQLITE_ERROR )
            {
                res = sqlite3_step( m_statement );
            }
            auto& pKey = CLASS::schema->primaryKey();
            int pKeyValue = sqlite3_last_insert_rowid( db );
            pKey.set( m_record, pKeyValue );
            return res != SQLITE_ERROR;
        }

        operator bool()
        {
            return execute( DBConnection::instance().rawConnection() );
        }

        InsertOperation( const InsertOperation& ) = delete;
        InsertOperation( InsertOperation&& ) = default;

    private:
        CLASS& m_record;
};

class CreateTableOperation : public Operation
{
    public:
        template <typename TABLESCHEMA>
        CreateTableOperation( const TABLESCHEMA& schema )
        {
            m_request = "CREATE TABLE IF NOT EXISTS " + schema.name() + '(';
            const auto& columns = schema.columns();
            for (auto c : columns)
              m_request += c->name() + ' ' + c->typeName() + ',';
            m_request.replace(m_request.end() - 1, m_request.end(), ")");
        }

        virtual bool execute( sqlite3* db )
        {
            if ( Operation::execute( db ) == false )
                return false;
            // We still need to step on the request for it to be executed.
            int res = sqlite3_step( m_statement );
            while ( res != SQLITE_DONE && res != SQLITE_ERROR )
            {
                res = sqlite3_step( m_statement );
            }
            return res != SQLITE_ERROR;
        }


};

}

#endif // OPERATION_HPP
