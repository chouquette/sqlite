/*****************************************************************************
 * Tests.cpp: Unit test the sqlite wrapper
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

#include "gtest/gtest.h"
#include <string>

#include "sqlite/sqlite.hpp"
#include "sqlite/DbConnection.h"
#include "sqlite/Table.hpp"

class TestEntry
{
    public:
        static vsqlite::Table& table()
        {
            static vsqlite::Table table =
                    vsqlite::Table::Create("TestEntryTable",
                                           vsqlite::Table::createField(&TestEntry::primaryKey, "id"),
                                           vsqlite::Table::createField(&TestEntry::someText, "text") );
            return table;
        }

    private:
        int primaryKey;
        std::string someText;
};

static vsqlite::DBConnection* conn;

class Sqlite : public testing::Test
{
    virtual void SetUp()
    {
        conn = new vsqlite::DBConnection("test.db");
    }
    virtual void TearDown()
    {
        delete conn;
        unlink("test.db");
    }
};

TEST_F (Sqlite, Create)
{
    *conn << TestEntry::table().create();
    ASSERT_TRUE(conn->isValid());
}

int main( int argc, char **argv )
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
