/*****************************************************************************
 * sqlite.hpp: Provides SQLite binding to C++ objects
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

#ifndef SQLITE_HPP
#define SQLITE_HPP

#include <memory>
#include <vector>
#include <stdint.h>
#include <string>
#include <sqlite3.h>
#include <sstream>

/*
 * Heavily inspired from https://github.com/burner/sweet.hpp/blob/master/sweetql.hpp
 */

namespace vsqlite
{

template <template <typename...> class REF, typename TESTED>
struct is_instantiation_of : std::false_type
{
};

template <template <typename...> class REF, typename... TS>
struct is_instantiation_of<REF, REF<TS...>> : std::true_type
{
};

template <typename T>
class Traits
{
};

template <>
class Traits<int>
{
    public:
        static constexpr const char* name = "INT";
        static constexpr const bool need_escape = false;
        static constexpr int (* const Load)(sqlite3_stmt*, int) = &sqlite3_column_int;
};

template <>
class Traits<std::string>
{
    public:
        static constexpr const char* name = "VARCHAR (255)";
        static constexpr const bool need_escape = true;
        static constexpr const unsigned char* (* const Load)(sqlite3_stmt*, int) = &sqlite3_column_text;
};

template <typename T>
class Attribute
{
    public:
        Attribute(const std::string& name)
            : m_name( name )
        {
        }

        const std::string& name() const { return m_name; }
        virtual const char* typeName() const = 0;
        virtual std::string insert(const T& record) const = 0;
        virtual void load(sqlite3_stmt* stmt, T& record) const = 0;
        void setColumnIndex( int index ) { m_columnIndex = index; }

    protected:
        std::string m_name;
        int m_columnIndex;
};

template <typename TYPE, typename CLASS>
class Column : public Attribute<CLASS>
{
    public:
        Column(TYPE CLASS::* fieldPtr, const std::string& name)
            : Attribute<CLASS>(name)
            , m_fieldPtr( fieldPtr )
        {
        }

        virtual const char* typeName() const
        {
            return Traits<TYPE>::name;
        }

        virtual std::string insert(const CLASS& record) const
        {
            std::ostringstream oss;
            oss << record.*m_fieldPtr;
            if (Traits<TYPE>::need_escape)
                return "\"" + oss.str() + "\"";
            return oss.str();
        }

        virtual void load( sqlite3_stmt *stmt, CLASS &record ) const
        {
            // When using string, we need to cast the result from Traits::Load from unsigned char to char.
            // This will be a no-op for other types
            using LoadedType = typename std::conditional<std::is_same<TYPE, std::string>::value, char*, TYPE>::type;
            LoadedType value = (LoadedType)Traits<TYPE>::Load( stmt, Attribute<CLASS>::m_columnIndex );
            record.*m_fieldPtr = value;
        }

    private:
        TYPE CLASS::* m_fieldPtr;
};

template <typename TYPE, typename CLASS>
class PrimaryKey : public Column<TYPE, CLASS>
{
    public:
        PrimaryKey(TYPE CLASS::* fieldPtr, const std::string& name)
            : Column<TYPE, CLASS>( fieldPtr, name )
        {
        }
};

class Operation
{
    public:
        Operation(const std::string& request)
            : m_request( request )
        {
        }

    private:
        std::string m_request;

        friend class DBConnection;
};

template <typename T>
class FetchOperation : public Operation
{
    public:
        FetchOperation(const std::string& request) : Operation(request) {}
};

class InsertOrUpdateOperation : public Operation
{
    public:
        InsertOrUpdateOperation(const std::string& request) : Operation(request) {}
};

class DBConnection
{
    public:
        DBConnection(const std::string& dbPath)
        {
            int res = sqlite3_open( dbPath.c_str(), &m_db );
            m_isValid = ( res == 0 );
        }

        ~DBConnection()
        {
            sqlite3_close( m_db );
        }

        bool isValid() const { return m_isValid; }
        const char* errorMsg() const { return sqlite3_errmsg( m_db ); }

        // For test purposes. There might be a better way.
        sqlite3*    rawConnection() { return m_db; }

        template <typename T>
        std::vector<T> execute(const FetchOperation<T>& op)
        {
            using ResType = std::vector<T>;
            if ( m_isValid == false )
            {
                fprintf(stderr, "Ignoring request on invalid connection");
                return ResType();
            }
            sqlite3_stmt* statement = NULL;
            int resultCode = sqlite3_prepare_v2( m_db, op.m_request.c_str(), -1, &statement, NULL );
            if ( resultCode != SQLITE_OK )
                return ResType();

            ResType result = parseResults<T>( statement );
            sqlite3_finalize( statement );
            return result;
        }

        bool execute(const InsertOrUpdateOperation &op)
        {

            if ( m_isValid == false )
            {
                fprintf(stderr, "Ignoring request on invalid connection");
                return false;
            }
            char* errorMessage = NULL;
            if ( sqlite3_exec( m_db, op.m_request.c_str(), NULL, NULL, &errorMessage) != SQLITE_OK)
            {
                fprintf(stderr, "SQLite error: %s", errorMessage);
                sqlite3_free( errorMessage );
                return false;
            }
            return true;
        }

    private:
        template <typename T>
        std::vector<T> parseResults( sqlite3_stmt* statement )
        {
            std::vector<T> results;
            while ( sqlite3_step( statement ) != SQLITE_DONE )
            {
                T row;
                auto& attributes = T::table().attributes();
                for ( auto a : attributes )
                {
                    a->load( statement, row );
                }

                results.push_back( std::move( row ) );
            }
            return results;
        }

    private:
        sqlite3*    m_db;
        bool        m_isValid;
};

}

#endif // SQLITE_HPP
