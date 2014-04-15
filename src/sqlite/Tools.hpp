/*****************************************************************************
 * Tools.hpp: Contains utility classes
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

#ifndef TOOLS_HPP
#define TOOLS_HPP

#include "sqlite.hpp"

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
struct Traits
{
};

template <>
struct Traits<int>
{
    static constexpr const char* name = "INT";
    static constexpr const bool need_escape = false;
    static constexpr int (* const Load)(sqlite3_stmt*, int) = &sqlite3_column_int;
    static constexpr int (* const Bind)(sqlite3_stmt*, int, int ) = &sqlite3_bind_int;
};

template <>
struct Traits<std::string>
{
    static constexpr const char* name = "VARCHAR (255)";
    static constexpr const bool need_escape = true;
    static constexpr const unsigned char* (* const Load)(sqlite3_stmt*, int) = &sqlite3_column_text;
    static constexpr int (* const Bind)(sqlite3_stmt*, int, const char*, int, void(*)(void*) ) = &sqlite3_bind_text;
};

}

#endif // TOOLS_HPP
