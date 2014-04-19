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

#include <cstring>
#include <sstream>

#include "Tools.hpp"

namespace vsqlite
{

template <typename T, typename U>
class ColumnSchemaImpl;

/*
 * CLASS: The class containing the column
 * TYPE: The column type
 */
template <typename CLASS, typename TYPE>
class Column
{
    public:
        bool isNull() const
        {
            return m_isNull;
        }

        TYPE& operator=( const TYPE& value )
        {
            m_value = value;
            m_isNull = false;
            return m_value;
        }

        TYPE& operator=( TYPE&& value )
        {
            m_value = std::move( value );
            m_isNull = false;
            return m_value;
        }

        bool operator==( const TYPE& rvalue ) const
        {
            return m_value == rvalue;
        }

        operator TYPE&()
        {
            assert( isNull() == false );
            return m_value;
        }

        operator const TYPE&() const
        {
            assert( isNull() == false );
            return m_value;
        }

    private:
        TYPE    m_value;
        bool m_isNull = true;
        ColumnSchemaImpl<CLASS, TYPE>* m_columnSchema;

        friend class ColumnSchemaImpl<CLASS, TYPE>;
};

template <typename T>
class ColumnSchema
{
    public:
        ColumnSchema(const std::string& name)
            : m_name( name )
        {
        }

        const std::string& name() const { return m_name; }
        virtual std::string typeName() const = 0;
        virtual std::string insert(const T& record) const = 0;
        virtual void load(sqlite3_stmt* stmt, T& record) const = 0;
        virtual void setSchema( T* inst ) = 0;
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

template <typename CLASS, typename TYPE>
class ColumnSchemaImpl : public ColumnSchema<CLASS>
{
    public:
        ColumnSchemaImpl(Column<CLASS, TYPE> CLASS::* fieldPtr, const std::string& name)
            : ColumnSchema<CLASS>(name)
            , m_fieldPtr( fieldPtr )
        {
        }

        virtual std::string typeName() const
        {
            return Traits<TYPE>::name;
        }

        virtual std::string insert(const CLASS& record) const
        {
            std::ostringstream oss;
            oss << (TYPE)(record.*m_fieldPtr);
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

        virtual void setSchema( CLASS *inst )
        {
            (inst->*m_fieldPtr).m_columnSchema = this;
        }

    private:
        Column<CLASS, TYPE> CLASS::* m_fieldPtr;
};


template <typename CLASS, typename TYPE>
class PrimaryKeySchema : public ColumnSchemaImpl<CLASS, TYPE>
{
    public:
        PrimaryKeySchema(Column<CLASS, TYPE> CLASS::* fieldPtr, const std::string& name)
            : ColumnSchemaImpl<CLASS, TYPE>( fieldPtr, name )
        {
        }

        virtual std::string typeName() const
        {
            return ColumnSchemaImpl<CLASS, TYPE>::typeName() + " PRIMARY KEY";
        }
};

}

#endif // COLUMN_HPP
