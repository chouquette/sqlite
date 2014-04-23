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
#include "WhereClause.hpp"

namespace vsqlite
{

template <typename T, typename U>
class ColumnSchemaImpl;

template <typename>
class PrimaryKeySchema;

template <typename, typename, typename>
class ForeignKeySchema;

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

        TYPE& operator=( const Column<CLASS, TYPE>& value )
        {
            m_value = value.m_value;
            m_isNull = value.m_isNull;
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

template <typename CLASS, typename FOREIGNVALUETYPE, typename FOREIGNKEYTYPE>
class ForeignKey
{
    public:
        Column<CLASS, FOREIGNKEYTYPE>& foreignKey()
        {
            return m_foreignKey;
        }
        const Column<CLASS, FOREIGNKEYTYPE>& foreignKey() const
        {
            return m_foreignKey;
        }
        FOREIGNVALUETYPE& operator=( const FOREIGNVALUETYPE& value )
        {
            const auto& fKeyImpl = static_cast<const PrimaryKeySchema<FOREIGNVALUETYPE>&>( FOREIGNVALUETYPE::schema->primaryKey() );
            m_foreignKey = fKeyImpl.load( value );
            m_value = value;
            return m_value;
        }

        FOREIGNVALUETYPE* operator->()
        {
            fetchForeignValue();
            return &m_value;
        }

    private:
        void fetchForeignValue()
        {
            if ( m_isNull == false )
                return ;
            m_value = FOREIGNVALUETYPE::fetch().where( FOREIGNVALUETYPE::primaryKey() == m_foreignKey );
        }

    private:
        FOREIGNVALUETYPE m_value;
        bool m_isNull = true;
        Column<CLASS, FOREIGNKEYTYPE> m_foreignKey;
        ForeignKeySchema<CLASS, FOREIGNVALUETYPE, FOREIGNKEYTYPE>* m_columnSchema;
        friend class ForeignKeySchema<CLASS, FOREIGNVALUETYPE, FOREIGNKEYTYPE>;
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

        // Overload provided for direct column in where clauses
        template <typename CLASS, typename TYPE>
        Predicate operator==( const Column<CLASS, TYPE>& column ) const
        {
            // Force the cast operator to use the actual value
            return operator==((TYPE)column);
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
            const auto& column = (record.*m_fieldPtr);
            if ( column.isNull() )
                return "NULL";
            std::ostringstream oss;
            oss << (TYPE)column;
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
            if ( std::is_pointer<LoadedType>::value == false ||
                 value )
            {
                record.*m_fieldPtr = value;
            }
        }

        virtual void setSchema( CLASS *inst )
        {
            (inst->*m_fieldPtr).m_columnSchema = this;
        }

        const TYPE& load( const CLASS& instance ) const
        {
            return (instance.*m_fieldPtr);
        }

        void set( CLASS& instance, const TYPE& value )
        {
            (instance.*m_fieldPtr) = value;
        }

    private:
        Column<CLASS, TYPE> CLASS::* m_fieldPtr;
};

template <typename CLASS>
class PrimaryKeySchema : public ColumnSchemaImpl<CLASS, int>
{
    public:
        PrimaryKeySchema(Column<CLASS, int> CLASS::* fieldPtr, const std::string& name)
            : ColumnSchemaImpl<CLASS, int>( fieldPtr, name )
        {
        }

        virtual std::string typeName() const
        {
            return ColumnSchemaImpl<CLASS, int>::typeName() + " PRIMARY KEY AUTOINCREMENT";
        }
};

// Type is the type of the linked entity.
template <typename CLASS, typename FOREIGNTYPE, typename FOREIGNKEYTYPE>
class ForeignKeySchema : public ColumnSchema<CLASS>
{
    public:
        ForeignKeySchema(ForeignKey<CLASS, FOREIGNTYPE, FOREIGNKEYTYPE> CLASS::* fieldPtr, const std::string& name)
            : ColumnSchema<CLASS>( name )
            , m_fieldPtr( fieldPtr )
            , m_foreignTypePrimaryKey( FOREIGNTYPE::schema->primaryKey() )
        {
        }

        virtual std::string typeName() const
        {
            std::string create = Traits<FOREIGNKEYTYPE>::name;
            create += ", FOREIGN KEY (" + ColumnSchema<CLASS>::m_name + ") REFERENCES "
                    + FOREIGNTYPE::schema->name() + " (" + m_foreignTypePrimaryKey.name() + ")";
            return create;
        }

        virtual std::string insert(const CLASS& record) const
        {
            const auto& column = (record.*m_fieldPtr).foreignKey();
            if ( column.isNull() )
                return "NULL";
            std::ostringstream oss;
            oss << (FOREIGNKEYTYPE)(column);
            if (Traits<FOREIGNKEYTYPE>::need_escape)
                return "\"" + oss.str() + "\"";
            return oss.str();
        }

        virtual void load( sqlite3_stmt *stmt, CLASS &record ) const
        {
            // When using string, we need to cast the result from Traits::Load from unsigned char to char.
            // This will be a no-op for other types
            using LoadedType = typename std::conditional<std::is_same<FOREIGNKEYTYPE, std::string>::value, char*, FOREIGNKEYTYPE>::type;
            LoadedType value = (LoadedType)Traits<FOREIGNKEYTYPE>::Load( stmt, ColumnSchema<CLASS>::m_columnIndex );
            (record.*m_fieldPtr).foreignKey() = value;
        }

        virtual void setSchema( CLASS *inst )
        {
            (inst->*m_fieldPtr).m_columnSchema = this;
        }

    private:
        ForeignKey<CLASS, FOREIGNTYPE, FOREIGNKEYTYPE> CLASS::* m_fieldPtr;
        const ColumnSchema<FOREIGNTYPE>& m_foreignTypePrimaryKey;
};

}

#endif // COLUMN_HPP
