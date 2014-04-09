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
        static vsqlite::Table<TestTable>& table()
        {
            static auto table =
                    vsqlite::Table<TestTable>::Create("TestTable",
                                           vsqlite::Table<TestTable>::createPrimaryKey(&TestTable::primaryKey, "id"),
                                           vsqlite::Table<TestTable>::createField(&TestTable::someText, "text"),
                                           vsqlite::Table<TestTable>::createField(&TestTable::moreText, "otherField") );
            return table;
        }

    public:
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

TEST_F( Sqlite, Insert )
{
    *conn << TestTable::table().create();
    TestTable t;
    t.primaryKey = 1;
    t.someText = "sea";
    t.moreText = "otter";
    *conn << TestTable::table().insert( t );
    ASSERT_TRUE( conn->isValid() );
    const char* listEntries = "SELECT * FROM TestTable";
    sqlite3_stmt* outHandle;
    sqlite3_prepare_v2(conn->rawConnection(), listEntries, strlen(listEntries),
                       &outHandle, NULL);
    ASSERT_EQ( sqlite3_step( outHandle ), SQLITE_ROW );
    int primaryKey = sqlite3_column_int( outHandle, 0 );
    ASSERT_TRUE( t.primaryKey == primaryKey );
    const unsigned char* someText = sqlite3_column_text( outHandle, 1 );
    ASSERT_EQ( t.someText, (const char*)someText );
    const unsigned char* moreText = sqlite3_column_text( outHandle, 2 );
    ASSERT_EQ( t.moreText, (const char*)moreText );
    sqlite3_finalize( outHandle );
}

//TEST_F( Sqlite, Load )
//{
//    *conn << TestTable::table().create();
//    TestTable t;
//    t.id = 1;
//    t.someText = "load";
//    t.moreText = "test";
//    conn->insert( t );
//    TestTable t2;
//    conn->load( t2 );
//    ASSERT_EQ( t.id, t2.id );
//    ASSERT_EQ( t.someText, t2.someText );
//    ASSERT_EQ( t.moreText, t2.moreText);
//}

int main( int argc, char **argv )
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
