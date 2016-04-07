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




namespace
{
    QVariant GetCassValue( iterator_pointer_t iter )
    {
        QVariant result;
        CassError rc = CASS_OK;

        const CassValue* val = cass_iterator_get_meta_field_value( iter.get() );
        const CassValueType type = cass_value_type( val );
        switch( type )
        {
            case CASS_VALUE_TYPE_UNKNOWN    :
            case CASS_VALUE_TYPE_CUSTOM     :
            case CASS_VALUE_TYPE_DECIMAL    :
            case CASS_VALUE_TYPE_LAST_ENTRY :
            case CASS_VALUE_TYPE_LIST      :
            case CASS_VALUE_TYPE_MAP       :
            case CASS_VALUE_TYPE_SET       :
            case CASS_VALUE_TYPE_UDT       :
            case CASS_VALUE_TYPE_TUPLE     :
                // TODO
                break;

            case CASS_VALUE_TYPE_BLOB       :
                {
                    const cass_byte_t* buff;
                    size_t len = 0;
                    rc = cass_value_get_bytes( val, &buff, &len );
                    result = QByteArray::fromRawData( reinterpret_cast<const char*>(buff), len );
                }
                break;

            case CASS_VALUE_TYPE_BOOLEAN    :
                {
                    cass_bool_t b;
                    rc = cass_value_get_bool( val, &b );
                    result = (b == cass_true);
                }
                break;

            case CASS_VALUE_TYPE_FLOAT      :
                {
                    cass_float_t f;
                    rc = cass_value_get_float( val, &f );
                    result = static_cast<float>(f);
                }
                break;

            case CASS_VALUE_TYPE_DOUBLE     :
                {
                    cass_double_t d;
                    rc = cass_value_get_double( val, &d );
                    result = static_cast<double>(d);
                }
                break;

            case CASS_VALUE_TYPE_TINY_INT  :
                {
                    cass_int8_t i;
                    rc = cass_value_get_int8( val, &i );
                    result = static_cast<int8_t>(i);
                }
                break;

            case CASS_VALUE_TYPE_SMALL_INT :
                {
                    cass_int16_t i;
                    rc = cass_value_get_int16( val, &i );
                    result = static_cast<int16_t>(i);
                }
                break;

            case CASS_VALUE_TYPE_INT       :
            case CASS_VALUE_TYPE_VARINT    :
                {
                    cass_int32_t i;
                    rc = cass_value_get_int32( val, &i );
                    result = static_cast<int32_t>(i);
                }
                break;

            case CASS_VALUE_TYPE_BIGINT     :
            case CASS_VALUE_TYPE_COUNTER    :
                {
                    cass_int64_t i;
                    rc = cass_value_get_int64( val, &i );
                    result = static_cast<int64_t>(i);
                }
                break;

            case CASS_VALUE_TYPE_ASCII     :
            case CASS_VALUE_TYPE_DATE      :
            case CASS_VALUE_TYPE_TEXT      :
            case CASS_VALUE_TYPE_TIME      :
            case CASS_VALUE_TYPE_TIMESTAMP :
            case CASS_VALUE_TYPE_VARCHAR   :
                {
                    const char* str;
                    size_t len = 0;
                    rc = cass_value_get_string( val, &str, &len );
                    result = QString::fromUtf8( str, len );
                }
                break;

            case CASS_VALUE_TYPE_UUID      :
                {
                    CassUuid uuid;
                    rc = cass_value_get_uuid( val, &uuid );
                    if( rc == CASS_OK )
                    {
                        char str[CASS_UUID_STRING_LENGTH+1];
                        result = static_cast<uint64_t>(cass_uuid_timestamp( uuid ));
                    }
                }
                break;

            case CASS_VALUE_TYPE_TIMEUUID  :
                {
                    CassUuid uuid;
                    rc = cass_value_get_uuid( val, &uuid );
                    if( rc == CASS_OK )
                    {
                        char str[CASS_UUID_STRING_LENGTH+1];
                        cass_uuid_string( uuid, str );
                        result = QString(str);
                    }
                }
                break;

            case CASS_VALUE_TYPE_INET      :
        }

        if( rc != CASS_OK )
        {
            throw std::runtime_error( "Failed!" );
        }

        return result;
    }
}

//================================================================/
// SessionMeta
//
SessionMeta::SessionMeta( QCassandraSession::pointer_t session )
    : f_session(session)
{
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
            keyspace->f_fields[field_name] = GetCassValue( fields_iter ).toString();
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
                    column->f_fields[field_name] = GetCassValue( meta_iter ).toString();
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
SessionMeta::KeyspaceMeta::TableMeta::TableMeta( KeyspaceMeta::pointer_t kysp )
    : f_keyspace(kysp)
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
    return f_columns;
}


//================================================================/
// ColumnMeta
//
SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::ColumnMeta( SessionMeta::KeyspaceMeta::TableMeta::pointer_t table )
    : f_table(table)
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


SessionMeta::qstring_map_t
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getFields() const
{
    return f_fields;
}


}
//namespace QCassandraSchema


}
// namespace QtCassandra

// vim: ts=4 sw=4 et
