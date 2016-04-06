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

#pragma once

#include "QtCassandra/QCassandraSchema.h"

#include "cassandra.h"

#include "CassTools.h"

#include <memory>
#include <map>
#include <QString>


namespace QtCassandra
{


namespace QCassandraSchema
{


//================================================================/
// SessionMeta
//
SessionMeta::SessionMeta( QCassandraSession::pointer_t session )
    : f_session(session)
{
    schema_meta_pointer_t schema
        ( cass_session_get_schema_meta(f_session.get())
        , schemaMetaDeleter()
        );

    iterator_pointer_t iter
        ( cass_iterator_keyspaces_from_schema_meta( schema_meta )
        , iteratorDeleter()
        );

    while( cass_iterator_next( iter.get() ) )
    {
        keyspace_meta_pointer_t keyspace( cass_iterator_get_keyspace_meta( iter.get() ) );
        char * name;
        size_t len;
        cass_keyspace_meta_name( keyspace.get(), &name, &len );
        KeyspaceMeta::pointer_t km( std::shared_ptr<KeyspaceMeta>(shared_from_this()) );
        km->f_name = QString::fromUtf8(name,len);

        iterator_pointer_t fields_iter
            ( cass_iterator_fields_from_keyspace_meta( km.get() )
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
            rc = cass_iterator_get_meta_field_value( fields_iter.get(), name, len );
            if( rc != CASS_OK )
            {
                throw std::runtime_error( "Cannot get field value from iterator!" );
            }
            const QString field_value( QString::fromUtf8(name,len) );
            km->f_fields[field_name] = field_value;
        }

        iterator_pointer_t tables_iter
            ( cass_iterator_fields_from_table_meta( km.get()
            , iteratorDeleter()
            );
        while( cass_iterator_next( tables_iter.get() )
        {
            table_meta_pointer_t table
                ( cass_iterator_get_table_meta( tables_iter.get() )
                , tableMetaDeleter()
                );
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



const SessionMeta::KeyspaceMeta::map_t& getKeyspaces()
{
    return f_keyspaces;
}


//================================================================/
// KeyspaceMeta
//
SessionMeta::KeyspaceMeta( SessionMeta::pointer_t session_meta )
    : f_session(session_meta)
{
    // TODO add sub-fields
}


QString SessionMeta::KeyspaceMeta::getName() const
{
    return f_name;
}


SessionMeta::qstring_map_t
    SessionMeta::KeyspaceMeta::getFields() const
{
    return f_fields;
}


//================================================================/
// TableMeta
//
SessionMeta::KeyspaceMeta::TableMeta( SessionMeta::KeyspaceMeta::pointer_t kysp )
    : f_keyspaces(kysp)
{
}


QString SessionMeta::KeyspaceMeta::TableMeta::getName() const
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
    return f_tables;
}


//================================================================/
// ColumnMeta
//
SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta( SessionMeta::KeyspaceMeta::TableMeta::pointer_t tables )
    : f_tables(tables)
{
}


QString
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getName() const
{
    return f_name;
}


SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::type_t
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getType() const
{
    return f_type;
}


SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::qstring_map_t
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getFields() const
{
    return f_fields;
}


}
//namespace QCassandraSchema


}
// namespace QtCassandra

// vim: ts=4 sw=4 et
