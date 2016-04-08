/*
 * Text:
 *      QCassandraSchema.h
 *
 * Description:
 *      Database schema metadata.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2016 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "QtCassandra/QCassandraSchema.h"
#include "QtCassandra/QCassandraQuery.h"

#include "cassandra.h"

#include "CassTools.h"

#include <memory>
#include <map>
#include <QtCore>


namespace QtCassandra
{


using namespace CassTools;


namespace QCassandraSchema
{


//================================================================/
// SessionMeta
//
SessionMeta::SessionMeta( QCassandraSession::pointer_t session )
    : f_session(session)
{
}


SessionMeta::pointer_t SessionMeta::create( QCassandraSession::pointer_t session )
{
    return std::make_shared<SessionMeta>(session);
}


void SessionMeta::loadSchema()
{
    schema_meta_pointer_t schema_meta
        ( cass_session_get_schema_meta(f_session->session().get())
        , schemaMetaDeleter()
        );

    iterator_pointer_t iter
        ( cass_iterator_keyspaces_from_schema_meta( schema_meta.get() )
        , iteratorDeleter()
        );

    while( cass_iterator_next( iter.get() ) )
    {
        keyspace_meta_pointer_t p_keyspace( cass_iterator_get_keyspace_meta( iter.get() ), keyspaceMetaDeleter() );
        const char * name;
        size_t len;
        cass_keyspace_meta_name( p_keyspace.get(), &name, &len );
        KeyspaceMeta::pointer_t keyspace( std::make_shared<KeyspaceMeta>(shared_from_this()) );
        keyspace->f_name = QString::fromUtf8(name,len);
        f_keyspaces[keyspace->f_name] = keyspace;

        iterator_pointer_t fields_iter
            ( cass_iterator_fields_from_keyspace_meta( p_keyspace.get() )
            , iteratorDeleter()
            );
        while( cass_iterator_next( fields_iter.get() ) )
        {
            CassError rc = cass_iterator_get_meta_field_name( fields_iter.get(), &name, &len );
            if( rc != CASS_OK )
            {
                throw std::runtime_error( "Cannot get field name from iterator!" );
            }

            const QString field_name( QString::fromUtf8(name,len) );
            auto val( Value::create() );
            val->readValue(fields_iter);
            keyspace->f_fields[field_name] = val;
        }

        iterator_pointer_t tables_iter
            ( cass_iterator_tables_from_keyspace_meta(p_keyspace.get())
            , iteratorDeleter()
            );
        while( cass_iterator_next(tables_iter.get()) )
        {
            table_meta_pointer_t p_table
                ( cass_iterator_get_table_meta( tables_iter.get() )
                , tableMetaDeleter()
                );
            cass_table_meta_name( p_table.get(), &name, &len );
            using TableMeta = KeyspaceMeta::TableMeta;
            TableMeta::pointer_t table
                    ( std::make_shared<TableMeta>(keyspace) );
            table->f_keyspace = keyspace;
            table->f_name     = QString::fromUtf8(name,len);
            keyspace->f_tables[table->f_name] = table;

            iterator_pointer_t columns_iter
                ( cass_iterator_columns_from_table_meta( p_table.get() )
                , iteratorDeleter()
                );
            while( cass_iterator_next( columns_iter.get() ) )
            {
                column_meta_pointer_t p_col
                    ( cass_iterator_get_column_meta( columns_iter.get() )
                    , columnMetaDeleter()
                    );
                cass_column_meta_name( p_col.get(), &name, &len );

                using ColumnMeta = TableMeta::ColumnMeta;
                ColumnMeta::pointer_t column( std::make_shared<ColumnMeta>(table) );
                column->f_table = table;
                column->f_name  = QString::fromUtf8(name,len);
                table->f_columns[column->f_name] = column;

                CassColumnType type = cass_column_meta_type( p_col.get() );
                switch( type )
                {
                case CASS_COLUMN_TYPE_REGULAR        : column->f_type = ColumnMeta::TypeRegular;        break;
                case CASS_COLUMN_TYPE_PARTITION_KEY  : column->f_type = ColumnMeta::TypePartitionKey;   break;
                case CASS_COLUMN_TYPE_CLUSTERING_KEY : column->f_type = ColumnMeta::TypeClusteringKey;  break;
                case CASS_COLUMN_TYPE_STATIC         : column->f_type = ColumnMeta::TypeStatic;         break;
                case CASS_COLUMN_TYPE_COMPACT_VALUE  : column->f_type = ColumnMeta::TypeCompactValue;   break;
                }

                iterator_pointer_t meta_iter
                    ( cass_iterator_fields_from_column_meta( p_col.get() )
                    , iteratorDeleter()
                    );
                while( cass_iterator_next( meta_iter.get() ) )
                {
                    CassError rc = cass_iterator_get_meta_field_name( meta_iter.get(), &name, &len );
                    if( rc != CASS_OK )
                    {
                        throw std::runtime_error( "Cannot read field from set!" );
                    }
                    const QString field_name( QString::fromUtf8(name,len) );
                    auto val( Value::create() );
                    val->readValue(meta_iter);
                    column->f_fields[field_name] = val;
                }
            }
        }
    }
}


SessionMeta::~SessionMeta()
{
}


QCassandraSession::pointer_t SessionMeta::session() const
{
    return f_session;
}


uint32_t SessionMeta::snapshotVersion() const
{
    return f_version;
}



const SessionMeta::KeyspaceMeta::map_t& SessionMeta::getKeyspaces()
{
    return f_keyspaces;
}


//================================================================/
// KeyspaceMeta
//
SessionMeta::KeyspaceMeta::KeyspaceMeta( SessionMeta::pointer_t session_meta )
    : f_session(session_meta)
{
    // TODO add sub-fields
}


const QString& SessionMeta::KeyspaceMeta::getName() const
{
    return f_name;
}


const SessionMeta::map_t&
    SessionMeta::KeyspaceMeta::getFields() const
{
    return f_fields;
}


//================================================================/
// TableMeta
//
SessionMeta::KeyspaceMeta::TableMeta::TableMeta( KeyspaceMeta::pointer_t kysp )
    : f_keyspace(kysp)
{
}


const QString& SessionMeta::KeyspaceMeta::TableMeta::getName() const
{
    return f_name;
}


const SessionMeta::KeyspaceMeta::TableMeta::map_t&
    SessionMeta::KeyspaceMeta::getTables() const
{
    return f_tables;
}


const SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::map_t&
    SessionMeta::KeyspaceMeta::TableMeta::getColumns() const
{
    return f_columns;
}


//================================================================/
// ColumnMeta
//
SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::ColumnMeta( SessionMeta::KeyspaceMeta::TableMeta::pointer_t table )
    : f_table(table)
{
}


const QString&
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getName() const
{
    return f_name;
}


SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::type_t
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getType() const
{
    return f_type;
}


const SessionMeta::map_t&
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getFields() const
{
    return f_fields;
}


}
//namespace QCassandraSchema


}
// namespace QtCassandra

// vim: ts=4 sw=4 et
