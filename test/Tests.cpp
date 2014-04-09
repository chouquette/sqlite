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

class TestTable
{
    public:
        static vsqlite::Table& table()
        {
            static vsqlite::Table table =
                    vsqlite::Table::Create("TestTable",
                                           vsqlite::Table::createPrimaryKey(&TestTable::primaryKey, "id"),
                                           vsqlite::Table::createField(&TestTable::someText, "text"),
                                           vsqlite::Table::createField(&TestTable::moreText, "otherField") );
            return table;
        }

    private:
        int primaryKey;
        std::string someText;
        std::string moreText;
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
    *conn << TestTable::table().create();
    ASSERT_TRUE(conn->isValid());
    const char* checkTableRequest = "pragma table_info(TestTable)";
    sqlite3_stmt* outHandle;
    sqlite3_prepare_v2(conn->rawConnection(), checkTableRequest, strlen(checkTableRequest),
                       &outHandle, NULL);
    for (int i = 0; i < TestTable::table().attributes().size(); ++i)
    {
        auto attribute = TestTable::table().attributes()[i];
        ASSERT_EQ( sqlite3_step( outHandle ), SQLITE_ROW );
        // Name
        const unsigned char* columnName = sqlite3_column_text( outHandle, 1 );
        ASSERT_TRUE( attribute->name() == (const char*)columnName );
        // Type
        const unsigned char* psz_type = sqlite3_column_text( outHandle, 2 );
        std::string type( (const char*)psz_type );
        ASSERT_NE( type.find( attribute->typeName() ), std::string::npos );

    }
    ASSERT_EQ( sqlite3_step( outHandle ), SQLITE_DONE );
    ASSERT_EQ( TestTable::table().primaryKey()->name(), "id" );
    sqlite3_finalize( outHandle );

}

int main( int argc, char **argv )
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
