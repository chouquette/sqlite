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
#include "sqlite/Table.hpp"

class TestTable : vsqlite::Table<TestTable>
{
    public:
        static vsqlite::TableSchema<TestTable>& table()
        {
            static auto table = Register("TestTable",
                                           createPrimaryKey(&TestTable::primaryKey, "id"),
                                           createField(&TestTable::someText, "text"),
                                           createField(&TestTable::moreText, "otherField") );
            return table;
        }

    public:
        vsqlite::Column<int> primaryKey;
        vsqlite::Column<std::string> someText;
        vsqlite::Column<std::string> moreText;
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
    ASSERT_TRUE( conn->execute( TestTable::table().create() ) );
    const char* checkTableRequest = "pragma table_info(TestTable)";
    sqlite3_stmt* outHandle;
    sqlite3_prepare_v2(conn->rawConnection(), checkTableRequest, -1, &outHandle, NULL);
    for (int i = 0; i < TestTable::table().columns().size(); ++i)
    {
        auto attribute = TestTable::table().columns()[i];
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
    ASSERT_EQ( TestTable::table().primaryKey().name(), "id" );
    sqlite3_finalize( outHandle );
}

TEST_F( Sqlite, Insert )
{
    ASSERT_TRUE( conn->execute( TestTable::table().create() ) );
    TestTable t;
    t.primaryKey = 1;
    t.someText = "sea";
    t.moreText = "otter";
    ASSERT_TRUE( conn->execute( TestTable::table().insert( t ) ) );
    const char* listEntries = "SELECT * FROM TestTable";
    sqlite3_stmt* outHandle;
    sqlite3_prepare_v2(conn->rawConnection(), listEntries, -1, &outHandle, NULL);
    ASSERT_EQ( sqlite3_step( outHandle ), SQLITE_ROW );
    int primaryKey = sqlite3_column_int( outHandle, 0 );
    ASSERT_TRUE( t.primaryKey == primaryKey );
    const unsigned char* someText = sqlite3_column_text( outHandle, 1 );
    ASSERT_EQ( t.someText, (const char*)someText );
    const unsigned char* moreText = sqlite3_column_text( outHandle, 2 );
    ASSERT_EQ( t.moreText, (const char*)moreText );
    sqlite3_finalize( outHandle );
}

TEST_F( Sqlite, LoadAll )
{
    ASSERT_TRUE( conn->execute( TestTable::table().create() ) );
    TestTable ts[10];
    for (int i = 0; i < 10; ++i)
    {
        TestTable& t = ts[i];
        t.primaryKey = i;
        t.someText = std::string("load") + (char)(i + '0');
        t.moreText = std::string("test") + (char)(i + '0');
    }
    ASSERT_TRUE( conn->execute( TestTable::table().insert( ts ) ) );
    std::vector<TestTable> t2s = conn->execute( TestTable::table().fetch() );

    ASSERT_EQ(t2s.size(), 10);
    for (int i = 0; i < 10; ++i)
    {
        const TestTable& t = ts[i];
        const TestTable& t2 = t2s[i];
        ASSERT_EQ( t.primaryKey, t2.primaryKey );
        ASSERT_EQ( t.someText, t2.someText );
        ASSERT_EQ( t.moreText, t2.moreText);
    }
}

TEST_F( Sqlite, LoadByPrimaryKey )
{
    conn->execute( TestTable::table().create() );
    TestTable ts[10];
    for (int i = 0; i < 10; ++i)
    {
        TestTable& t = ts[i];
        t.primaryKey = i;
        t.someText = std::string("load") + (char)(i + '0');
        t.moreText = std::string("test") + (char)(i + '0');
    }
    conn->execute( TestTable::table().insert( ts ) );
    for ( int i = 0; i < 10; ++i )
    {
        std::vector<TestTable> t2s = conn->execute( TestTable::table().fetch().where
                                                    ( TestTable::table().primaryKey() == i ) );

        ASSERT_EQ(1, t2s.size());
        TestTable& t2 = t2s[0];
        TestTable& t = ts[i];
        ASSERT_EQ( t.primaryKey, t2.primaryKey );
        ASSERT_EQ( t.someText, t2.someText );
        ASSERT_EQ( t.moreText, t2.moreText);
    }
}


TEST_F( Sqlite, LoadByColumnValue )
{
    conn->execute( TestTable::table().create() );
    TestTable ts[10];
    for (int i = 0; i < 10; ++i)
    {
        TestTable& t = ts[i];
        t.primaryKey = i;
        t.someText = std::string("load") + (char)(i + '0');
        t.moreText = std::string("test") + (char)(i + '0');
    }
    conn->execute( TestTable::table().insert( ts ) );
    auto attribute = TestTable::table().column( "otherField" );
    ASSERT_TRUE( (bool)attribute );
    auto res = conn->execute( TestTable::table().fetch().where( *attribute == "test5" ) );
    ASSERT_EQ( 1, res.size() );
    TestTable t = res[0];
    ASSERT_EQ( 5, t.primaryKey );
    ASSERT_EQ( t.someText, "load5" );
}

int main( int argc, char **argv )
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
