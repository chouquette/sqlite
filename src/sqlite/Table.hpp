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

class Table
{
    public:
        typedef std::shared_ptr<Attribute> AttributePtr;
        typedef std::vector<AttributePtr> Attributes;

    private:
        template <typename T>
        void appendColumn(std::shared_ptr<T> column)
        {
            static_assert(std::is_base_of<Attribute, T>::value,
                           "All table fields must inherit Attribute class");
            //FIXME: static_assert that column is an Attribute instance
            if (is_instantiation_of<PrimaryKey, T>::value)
                m_primaryKey = column;
            m_attributes.push_back(column);
        }

        template <typename T>
        static void Create(Table& t, T column)
        {
            t.appendColumn(column);
        }

        template <typename T, typename... COLUMNS>
        static void Create(Table& t, T column, COLUMNS... columns)
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

        template <typename T, typename U>
        static std::shared_ptr<Column<T,U>> createField(T U::* attributePtr, const std::string& name)
        {
            return std::make_shared<Column<T, U>>(attributePtr, name);
        }

        template <typename T, typename U>
        static std::shared_ptr<PrimaryKey<T, U>> createPrimaryKey(T U::* attributePtr, const std::string& name)
        {
            return std::make_shared<PrimaryKey<T, U>>(attributePtr, name);
        }

        Table(const std::string& name) : m_name(name) {}
        Operation create();
        const Attributes& attributes() const;
        const AttributePtr primaryKey() const;

    private:
        std::string m_name;
        AttributePtr m_primaryKey;
        std::vector<AttributePtr> m_attributes;
};

}

#endif // TABLE_HPP
