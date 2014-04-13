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
struct Traits
{
};

template <>
struct Traits<int>
{
    static constexpr const char* name = "INT";
    static constexpr const bool need_escape = false;
    static constexpr int (* const Load)(sqlite3_stmt*, int) = &sqlite3_column_int;
    static constexpr int (* const Bind)(sqlite3_stmt*, int, int ) = &sqlite3_bind_int;
};

template <>
struct Traits<std::string>
{
    static constexpr const char* name = "VARCHAR (255)";
    static constexpr const bool need_escape = true;
    static constexpr const unsigned char* (* const Load)(sqlite3_stmt*, int) = &sqlite3_column_text;
    static constexpr int (* const Bind)(sqlite3_stmt*, int, const char*, int, void(*)(void*) ) = &sqlite3_bind_text;
};

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

        template <typename V>
        Predicate operator==(const V& value) const
        {
            auto bindFunction = [value](sqlite3_stmt* stmt, int bindIndex)
            {
                return Traits<V>::Bind( stmt, bindIndex, value );
            };
            return Predicate( m_name, bindFunction );
        }

        Predicate operator==( const std::string& value ) const
        {
            char* copy = strdup( value.c_str() );
            auto bindFunction = [copy](sqlite3_stmt* stmt, int bindIndex)
            {
                return Traits<std::string>::Bind( stmt, bindIndex, copy, -1, free );
            };
            return Predicate( m_name, bindFunction );
        }
        Predicate operator==( const char* value ) const
        {
            return operator==( std::string( value ) );
        }

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

template <typename T>
class Operation
{
    public:
        Operation(const std::string& request)
            : m_request( request )
            , m_statement( NULL )
        {
        }
        virtual ~Operation()
        {
            sqlite3_finalize( m_statement );
        }

        Operation( const Operation& op ) = delete;
        Operation( Operation&& op ) = default;

        bool execute( sqlite3* db )
        {
            m_request += m_whereClause.generate();
            int resultCode = sqlite3_prepare_v2( db, m_request.c_str(), -1, &m_statement, NULL );
            if ( resultCode != SQLITE_OK )
            {
                std::cerr << "Failed to execute request " << m_request << '\n'
                             << "Error code: " << resultCode << std::endl;
                return false;
            }
            return m_whereClause.bind( m_statement );
        }
        operator sqlite3_stmt*()
        {
            assert( m_statement != NULL );
            return m_statement;
        }

        Operation&& where( WhereClause&& clause )
        {
            m_whereClause = std::move( clause );
            return std::move( *this );
        }

    private:
        std::string m_request;
        sqlite3_stmt* m_statement;
        WhereClause m_whereClause;
};

class DBConnection
{
    public:
        DBConnection(const std::string& dbPath)
        {
            int res = sqlite3_open( dbPath.c_str(), &m_db );
            m_isValid = ( res == SQLITE_OK );
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
            return sqlite3_step( op );
            // The operation & the associated sqlite3_stmt now falls out of scope and is cleaned
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
