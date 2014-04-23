/*****************************************************************************
 * Table.hpp: Handles table related SQLite operations
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

#include <memory>

#include "Column.hpp"
#include "DBConnection.hpp"
#include "Operation.hpp"

namespace vsqlite
{

template <typename T> class Table;

class ITableSchema
{
    public:
        virtual CreateTableOperation create() const = 0;
        virtual const std::string& name() const = 0;
};

template <typename T>
class TableSchema : ITableSchema
{
    public:
        typedef std::shared_ptr<ColumnSchema<T>> ColumnSchemaPtr;
        typedef std::vector<ColumnSchemaPtr> Columns;

        TableSchema(const std::string& name) : m_name(name) {}

        virtual CreateTableOperation create() const
        {
            return CreateTableOperation( *this );
        }

        const std::string& name() const { return m_name; }
        const Columns& columns() const { return m_columns; }
        PrimaryKeySchema<T>& primaryKey() const { return *m_primaryKey; }

        const ColumnSchemaPtr column( const std::string& name ) const
        {
            for ( auto a : m_columns )
            {
                if ( a->name() == name )
                    return a;

            }
            return nullptr;
        }
    private:
        template <typename C>
        void appendColumn(std::shared_ptr<C> column)
        {
            static_assert(std::is_base_of<ColumnSchema<T>, C>::value,
                           "All table fields must inherit Column<> class");
            column->setColumnIndex( m_columns.size() );
            m_columns.push_back(column);
        }

        void appendColumn( std::shared_ptr<PrimaryKeySchema<T>> column )
        {
            appendColumn( static_cast<ColumnSchemaPtr>( column ) );
            m_primaryKey = column;
        }

    private:
        std::string m_name;
        std::shared_ptr<PrimaryKeySchema<T>> m_primaryKey;
        std::vector<ColumnSchemaPtr> m_columns;

        friend class Table<T>;
};

template <typename CLASS>
class Table
{
    public:
        Table()
        {
            const auto& columns = CLASS::schema->columns();
            for ( auto& column : columns )
            {
                column->setSchema( static_cast<CLASS*>( this ) );
            }
        }

        InsertOperation<CLASS> insert()
        {
            return InsertOperation<CLASS>( static_cast<CLASS&>( *this ) );
        }

        static FetchOperation<CLASS> fetch()
        {
            return FetchOperation<CLASS>( "SELECT * FROM " + CLASS::schema->name() );
        }

        static const PrimaryKeySchema<CLASS>& primaryKey()
        {
            return CLASS::schema->primaryKey();
        }

    private:
        template <typename C>
        static void Register(TableSchema<CLASS>* t, std::shared_ptr<C> column)
        {
            t->appendColumn(column);
        }

        template <typename C, typename... COLUMNS>
        static void Register(TableSchema<CLASS>* t, std::shared_ptr<C> column, COLUMNS... columns)
        {
            t->appendColumn(column);
            Register(t, columns...);
        }

    protected:
        // Ease up the declaration of columns and hide the fact that Columns are
        // also using the containing class as a template parameter.
        template <typename T>
        using ColumnAttribute = Column<CLASS, T>;
        template <typename FOREIGNTYPE, typename FOREIGNKEYTYPE>
        using ForeignKeyAttribute = ForeignKey<CLASS, FOREIGNTYPE, FOREIGNKEYTYPE>;

        template <typename... COLUMNS>
        static const TableSchema<CLASS>* Register(const std::string& name, COLUMNS... columns)
        {
            auto t = new TableSchema<CLASS>( name );
            Register(t, columns...);
            DBConnection::registerTableSchema( t );
            return t;
        }

        template <typename TYPE>
        static std::shared_ptr<ColumnSchemaImpl<CLASS, TYPE>> createField(Column<CLASS, TYPE> CLASS::* attributePtr, const std::string& name)
        {
            return std::make_shared<ColumnSchemaImpl<CLASS, TYPE>>(attributePtr, name);
        }

        static std::shared_ptr<PrimaryKeySchema<CLASS>> createPrimaryKey(Column<CLASS, int> CLASS::* attributePtr, const std::string& name)
        {
            return std::make_shared<PrimaryKeySchema<CLASS>>(attributePtr, name);
        }

        template <typename FOREIGNTYPE, typename FOREIGNKEYTYPE>
        static std::shared_ptr<ForeignKeySchema<CLASS, FOREIGNTYPE, FOREIGNKEYTYPE>> createForeignKey(ForeignKey<CLASS, FOREIGNTYPE, FOREIGNKEYTYPE> CLASS::* attributePtr, const std::string& name)
        {
            return std::make_shared<ForeignKeySchema<CLASS, FOREIGNTYPE, FOREIGNKEYTYPE>>(attributePtr, name);
        }
};

}

#endif // TABLE_HPP
