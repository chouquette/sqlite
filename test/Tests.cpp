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

class TestTable : public vsqlite::Table<TestTable>
{
    public:
        static const vsqlite::TableSchema<TestTable>* schema;

    public:
        ColumnAttribute<int> id;
        ColumnAttribute<std::string> someText;
        ColumnAttribute<std::string> moreText;
};

const auto* TestTable::schema = TestTable::Register("TestTable",
                                          createPrimaryKey(&TestTable::id, "id"),
                                          createField(&TestTable::someText, "text"),
                                          createField(&TestTable::moreText, "otherField") );

static vsqlite::DBConnection* conn;

class Sqlite : public testing::Test
{
    virtual void SetUp()
    {
        bool res = vsqlite::DBConnection::init("test.db");
        ASSERT_TRUE( res );
        conn = &vsqlite::DBConnection::instance();
    }
    virtual void TearDown()
    {
        unlink("test.db");
    }
};

TEST_F( Sqlite, Create )
{
    const char* checkTableRequest = "pragma table_info(TestTable)";
    sqlite3_stmt* outHandle;
    sqlite3_prepare_v2(conn->rawConnection(), checkTableRequest, -1, &outHandle, NULL);
    const auto& columns = TestTable::schema->columns();
    for (size_t i = 0; i < columns.size(); ++i)
    {
        auto attribute = columns[i];
        int res = sqlite3_step( outHandle );
        ASSERT_EQ( SQLITE_ROW, res );
        // Name
        const unsigned char* columnName = sqlite3_column_text( outHandle, 1 );
        ASSERT_TRUE( attribute->name() == (const char*)columnName );
        // Type
        const unsigned char* psz_type = sqlite3_column_text( outHandle, 2 );
        std::string type( (const char*)psz_type );
        ASSERT_NE( attribute->typeName().find( type ), std::string::npos );
    }
    int res = sqlite3_step( outHandle );
    ASSERT_EQ( SQLITE_DONE, res );
    ASSERT_EQ( TestTable::primaryKey().name(), "id" );
    sqlite3_finalize( outHandle );
}

TEST_F( Sqlite, InsertOne )
{
    TestTable t;
    t.someText = "sea";
    t.moreText = "otter";
    bool success = t.insert().execute( conn->rawConnection() );
    ASSERT_TRUE( success );
    const char* listEntries = "SELECT * FROM TestTable";
    sqlite3_stmt* outHandle;
    sqlite3_prepare_v2(conn->rawConnection(), listEntries, -1, &outHandle, NULL);
    int res = sqlite3_step( outHandle );
    ASSERT_EQ( SQLITE_ROW, res );
    int primaryKey = sqlite3_column_int( outHandle, 0 );
    ASSERT_EQ( t.id, primaryKey );
    const unsigned char* someText = sqlite3_column_text( outHandle, 1 );
    ASSERT_EQ( t.someText, (const char*)someText );
    const unsigned char* moreText = sqlite3_column_text( outHandle, 2 );
    ASSERT_EQ( t.moreText, (const char*)moreText );
    sqlite3_finalize( outHandle );
}

TEST_F( Sqlite, LoadAll )
{
    TestTable ts[10];
    for (int i = 0; i < 10; ++i)
    {
        TestTable& t = ts[i];
        t.someText = std::string("load") + (char)(i + '0');
        t.moreText = std::string("test") + (char)(i + '0');
        bool res = t.insert();
        ASSERT_TRUE( res );
    }
    std::vector<TestTable> t2s = TestTable::fetch();

    ASSERT_EQ( 10u, t2s.size() );
    for (int i = 0; i < 10; ++i)
    {
        const TestTable& t = ts[i];
        const TestTable& t2 = t2s[i];
        ASSERT_EQ( t.id, t2.id );
        ASSERT_EQ( t.someText, t2.someText );
        ASSERT_EQ( t.moreText, t2.moreText);
    }
}

TEST_F( Sqlite, LoadByPrimaryKey )
{
    TestTable ts[10];
    for (int i = 0; i < 10; ++i)
    {
        TestTable& t = ts[i];
        t.someText = std::string("load") + (char)(i + '0');
        t.moreText = std::string("test") + (char)(i + '0');
        bool res = t.insert();
        ASSERT_TRUE( res );
    }
    for ( int i = 0; i < 10; ++i )
    {
        std::vector<TestTable> t2s = TestTable::fetch().where
                                                    ( TestTable::primaryKey() == i + 1 );

        ASSERT_EQ(1u, t2s.size());
        TestTable& t2 = t2s[0];
        TestTable& t = ts[i];
        ASSERT_EQ( t.id, t2.id );
        ASSERT_EQ( t.someText, t2.someText );
        ASSERT_EQ( t.moreText, t2.moreText);
    }
}

TEST_F( Sqlite, NullValue )
{
    TestTable t;
    t.someText = "moulaf";
    bool res = t.insert();
    ASSERT_TRUE( res );
    ASSERT_FALSE( t.someText.isNull() );
    ASSERT_TRUE( t.moreText.isNull() );

    TestTable t2 = TestTable::fetch().where( TestTable::primaryKey() == t.id );
    ASSERT_TRUE( t2.moreText.isNull() );
    ASSERT_EQ( t.someText, t2.someText );
}

TEST_F( Sqlite, LoadByColumnValue )
{
    TestTable ts[10];
    for (int i = 0; i < 10; ++i)
    {
        TestTable& t = ts[i];
        t.someText = std::string("load") + (char)(i + '0');
        t.moreText = std::string("test") + (char)(i + '0');
        bool res = t.insert();
        ASSERT_TRUE( res );
    }
    auto attribute = TestTable::schema->column( "otherField" );
    ASSERT_TRUE( (bool)attribute ); // check for non-null shared ptr
    std::vector<TestTable> res = TestTable::fetch().where( *attribute == "test5" );
    ASSERT_EQ( 1u, res.size() );
    TestTable t = res[0];
    ASSERT_EQ( 6, t.id ); // We index our values from 0 but autoincrement starts from 1
    ASSERT_EQ( t.someText, "load5" );
}

int main( int argc, char **argv )
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
