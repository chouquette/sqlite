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
class Traits
{
};

template <>
class Traits<int>
{
    public:
        static constexpr const char* name = "INT";
        static constexpr const bool need_escape = false;
};

template <>
class Traits<std::string>
{
    public:
        static constexpr const char* name = "VARCHAR (255)";
        static constexpr const bool need_escape = true;
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

    private:
        std::string m_name;
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
