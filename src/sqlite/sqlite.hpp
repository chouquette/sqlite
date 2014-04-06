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

#include <vector>
#include <stdint.h>
#include <memory>

#define DECLARE_TITLE(type, name) \
    public: Field<type>& name;

namespace vsqlite
{

class Attribute
{
    public:
        Attribute(const std::string& name)
            : m_name( name )
        {
        }

    private:
        std::string m_name;
};

template <typename T, typename U>
class Column : public Attribute
{
    public:
        Column(T U::* fieldPtr, const std::string& name)
            : Attribute(name)
            , m_fieldPtr( fieldPtr )
        {
        }

        //        virtual void set(const T& value)
        //        {
        //            *this = value;
        //        }
        //        virtual const T& get(const T&)
        //        {
        //            return *this;
        //        }

        //        operator const T&()
        //        {
        //            // insert lazy loading & traits function here
        //            return m_field;
        //        }
        //        T& operator=(const T& value)
        //        {
        //            m_field = value;
        //            return m_field;
        //        }
    private:
        T U::* m_fieldPtr;
};

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
        }

        template <typename T, typename U>
        static std::shared_ptr<Attribute> createField(T U::* attributePtr, const std::string& name)
        {
            return std::make_shared<Attribute>(Column<T, U>(attributePtr, name));
        }

        Table(const std::string& name) : m_name(name) {}

    private:
        std::string m_name;
        std::vector<Attribute> m_attributes;
};

}

#endif // SQLITE_HPP
