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

namespace vsqlite
{

class Table
{
    private:
        template <typename T>
        static void Create(Table& t, T column)
        {
            //FIXME: static_assert that column is an Attribute instance
            t.m_attributes.push_back(column);
        }

        template <typename T, typename... COLUMNS>
        static void Create(Table& t, T column, COLUMNS... columns)
        {
            //FIXME: static_assert that column is an Attribute instance
            t.m_attributes.push_back(column);
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
        static std::shared_ptr<Attribute> createField(T U::* attributePtr, const std::string& name)
        {
            return std::make_shared<Column<T, U>>(attributePtr, name);
        }

        Table(const std::string& name) : m_name(name) {}

        bool create();

    private:
        std::string m_name;
        std::vector<std::shared_ptr<Attribute>> m_attributes;
};

}

#endif // TABLE_HPP