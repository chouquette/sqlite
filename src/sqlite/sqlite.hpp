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

/*
 * Heavily inspired from https://github.com/burner/sweet.hpp/blob/master/sweetql.hpp
 */

namespace vsqlite
{

template <typename T>
class Traits
{
};

template <>
class Traits<int>
{
    public:
        static constexpr const char* name = "INT";
};

template <>
class Traits<std::string>
{
    public:
        static constexpr const char* name = "VARCHAR (255)";
};

class Attribute
{
    public:
        Attribute(const std::string& name)
            : m_name( name )
        {
        }

        const std::string& name() const { return m_name; }
        virtual const char* typeName() const = 0;

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

        virtual const char* typeName() const
        {
            return Traits<T>::name;
        }

    private:
        T U::* m_fieldPtr;
};

class Operation
{
    public:
        typedef int(*Callback)(void*,int,char**,char**);
        Operation(const std::string& request, Callback cb)
            : m_request( request )
            , m_callback( cb )
        {
        }

    private:
        std::string m_request;
        Callback m_callback;

        friend class DBConnection;
};

}

#endif // SQLITE_HPP
