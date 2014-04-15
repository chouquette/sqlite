/*****************************************************************************
 * Columns.hpp: Describes SQLite columns
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

#ifndef COLUMN_HPP
#define COLUMN_HPP

#include "sqlite.hpp"

namespace vsqlite
{

template <typename T>
class ColumnSchema
{
    public:
        ColumnSchema(const std::string& name)
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
class ColumnSchemaImpl : public ColumnSchema<CLASS>
{
    public:
        ColumnSchemaImpl(TYPE CLASS::* fieldPtr, const std::string& name)
            : ColumnSchema<CLASS>(name)
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
            LoadedType value = (LoadedType)Traits<TYPE>::Load( stmt, ColumnSchema<CLASS>::m_columnIndex );
            record.*m_fieldPtr = value;
        }

    private:
        TYPE CLASS::* m_fieldPtr;
};


template <typename TYPE, typename CLASS>
class PrimaryKey : public ColumnSchemaImpl<TYPE, CLASS>
{
    public:
        PrimaryKey(TYPE CLASS::* fieldPtr, const std::string& name)
            : ColumnSchemaImpl<TYPE, CLASS>( fieldPtr, name )
        {
        }
};

}

#endif // COLUMN_HPP
