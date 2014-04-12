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
        typedef std::shared_ptr<Attribute<T>> AttributePtr;
        typedef std::vector<AttributePtr> Attributes;

    private:
        template <typename C>
        void appendColumn(std::shared_ptr<C> column)
        {
            static_assert(std::is_base_of<Attribute<T>, C>::value,
                           "All table fields must inherit Attribute class");
            //FIXME: static_assert that column is an Attribute instance
            if (is_instantiation_of<PrimaryKey, C>::value)
                m_primaryKey = column;
            m_attributes.push_back(column);
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
        static std::shared_ptr<Column<TYPE, T>> createField(TYPE T::* attributePtr, const std::string& name)
        {
            return std::make_shared<Column<TYPE, T>>(attributePtr, name);
        }

        template <typename TYPE>
        static std::shared_ptr<PrimaryKey<TYPE, T>> createPrimaryKey(TYPE T::* attributePtr, const std::string& name)
        {
            return std::make_shared<PrimaryKey<TYPE, T>>(attributePtr, name);
        }

        Table(const std::string& name) : m_name(name) {}
        InsertOrUpdateOperation create()
        {
            std::string query = "CREATE TABLE IF NOT EXISTS " + m_name + '(';
            for (auto c : m_attributes)
              query += c->name() + ' ' + c->typeName() + ',';
            query.replace(query.end() - 1, query.end(), ")");
            return InsertOrUpdateOperation(query);
        }

        const Attributes& attributes() const { return m_attributes; }
        const AttributePtr primaryKey() const { return m_primaryKey; }

        InsertOrUpdateOperation insert(const T& record)
        {
            std::string insertInto = "INSERT INTO " + m_name + '(';
            std::string values = "VALUES (";
            for (auto attr : m_attributes)
            {
                insertInto += attr->name() + ",";
                values += attr->insert(record) + ",";
            }
            insertInto.replace(insertInto.end() - 1, insertInto.end(), ")");
            values.replace(values.end() - 1, values.end(), ")");
            return InsertOrUpdateOperation(insertInto + values + ';');
        }

        FetchOperation<T> fetch()
        {
            return FetchOperation<T>("");
        }

    private:
        std::string m_name;
        AttributePtr m_primaryKey;
        std::vector<AttributePtr> m_attributes;
};

}

#endif // TABLE_HPP
