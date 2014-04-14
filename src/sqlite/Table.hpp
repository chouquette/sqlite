/*****************************************************************************
 * Table.cpp: Handles table related SQLite operations
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

#ifndef TABLE_HPP
#define TABLE_HPP

#include "sqlite.hpp"
#include <iostream>
namespace vsqlite
{

template <typename T>
class Table
{
    public:
        typedef std::shared_ptr<Column<T>> ColumnPtr;
        typedef std::vector<ColumnPtr> Columns;

    private:
        template <typename C>
        void appendColumn(std::shared_ptr<C> column)
        {
            static_assert(std::is_base_of<Column<T>, C>::value,
                           "All table fields must inherit Attribute class");
            //FIXME: static_assert that column is an Attribute instance
            if (is_instantiation_of<PrimaryKey, C>::value)
                m_primaryKey = column;
            column->setColumnIndex( m_columns.size() );
            m_columns.push_back(column);
        }

        template <typename C>
        static void Create(Table& t, C column)
        {
            t.appendColumn(column);
        }

        template <typename C, typename... COLUMNS>
        static void Create(Table& t, C column, COLUMNS... columns)
        {
            t.appendColumn(column);
            Create(t, columns...);
        }

    public:

        template <typename... COLUMNS>
        static Table Create(const std::string& name, COLUMNS... columns)
        {
            Table t(name);
            Create(t, columns...);
            return t;
        }

        template <typename TYPE>
        static std::shared_ptr<ColumnImpl<TYPE, T>> createField(TYPE T::* attributePtr, const std::string& name)
        {
            return std::make_shared<ColumnImpl<TYPE, T>>(attributePtr, name);
        }

        template <typename TYPE>
        static std::shared_ptr<PrimaryKey<TYPE, T>> createPrimaryKey(TYPE T::* attributePtr, const std::string& name)
        {
            return std::make_shared<PrimaryKey<TYPE, T>>(attributePtr, name);
        }

        Table(const std::string& name) : m_name(name) {}

        Operation<bool> create()
        {
            std::string query = "CREATE TABLE IF NOT EXISTS " + m_name + '(';
            for (auto c : m_columns)
              query += c->name() + ' ' + c->typeName() + ',';
            query.replace(query.end() - 1, query.end(), ")");
            return Operation<bool>(query);
        }

        const Columns& columns() const { return m_columns; }
        const Column<T>& primaryKey() const { return *m_primaryKey; }

        Operation<bool> insert(const T& record)
        {
            return insert<1>( { record } );
        }

        template <size_t N>
        Operation<bool> insert( const T(&records)[N] )
        {
            std::string insertInto = "INSERT INTO " + m_name + '(';
            for (auto attr : m_columns)
            {
                insertInto += attr->name() + ",";
            }
            insertInto.replace(insertInto.end() - 1, insertInto.end(), ")");
            std::string values = "VALUES ";

            for ( auto& r : records )
            {
                values += '(';
                for ( auto attr : m_columns )
                {
                    values += attr->insert( r ) + ',';
                }
                values.replace(values.end() - 1, values.end(), "),");
            }
            values.replace(values.end() - 1, values.end(), ";");
            return Operation<bool>(insertInto + values);
        }

        Operation<T> fetch()
        {
            return Operation<T>( "SELECT * FROM " + m_name );
        }

        const ColumnPtr column( const std::string& name )
        {
            for ( auto a : m_columns )
            {
                if ( a->name() == name )
                    return a;

            }
            return nullptr;
        }

    private:
        std::string m_name;
        ColumnPtr m_primaryKey;
        std::vector<ColumnPtr> m_columns;
};

}

#endif // TABLE_HPP
