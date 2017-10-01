/*
 * Text:
 *      src/schema.cpp
 *
 * Description:
 *      Database schema metadata implementation.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2017 Made to Order Software Corp.
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

#include "casswrapper/schema.h"
#include "casswrapper/query.h"

#include <cassvalue/encoder.h>

#include "cassandra.h"

#include "casswrapper_impl.h"

#include <memory>
#include <map>
#include <QtCore>

using namespace cassvalue;

namespace casswrapper
{


namespace schema
{


//================================================================/
// SessionMeta
//
SessionMeta::SessionMeta( Session::pointer_t s )
    : f_session(s)
{
}


SessionMeta::~SessionMeta()
{
}


SessionMeta::pointer_t SessionMeta::create( Session::pointer_t s )
{
    return std::make_shared<SessionMeta>(s);
}


void SessionMeta::loadSchema()
{
    schema_meta schema( f_session->getSession() );
    iterator iter( schema.get_keyspaces() );
    while( iter.next() )
    {
        keyspace_meta p_keyspace( iter.get_keyspace_meta() );
        KeyspaceMeta::pointer_t keyspace( std::make_shared<KeyspaceMeta>(p_keyspace) );
        f_keyspaces[keyspace->getName()] = keyspace;
    }
}


Session::pointer_t SessionMeta::get_session() const
{
    return f_session;
}


const KeyspaceMeta::map_t& SessionMeta::getKeyspaces()
{
    return f_keyspaces;
}


/** \brief Transform a SessionMeta in a blob.
 *
 * This function transforms a SessionMeta object into a blob that can be
 * transfered from snapdbproxy to a client.
 *
 * \return The meta data in a blob.
 */
QByteArray SessionMeta::encodeSessionMeta() const
{
    // at this time ours is nearly 120Kb... so reserve one block
    // 200Kb from the get go
    //
    Encoder encoder(200 * 1024);

    // save the number of keyspaces
    //
    encoder.appendUInt16Value(f_keyspaces.size());
    for(auto keyspace : f_keyspaces)
    {
        keyspace.second->encodeKeyspaceMeta(encoder);
    }

    return encoder.result();
}


void SessionMeta::decodeSessionMeta(const QByteArray& code)
{
    Decoder const decoder(code);

    size_t const keyspace_max(decoder.uint16Value());
    for(size_t idx(0); idx < keyspace_max; ++idx)
    {
        KeyspaceMeta::pointer_t keyspace(std::make_shared<KeyspaceMeta>(decoder));
        f_keyspaces[keyspace->getName()] = keyspace;
    }
}


//================================================================/
// KeyspaceMeta
//
KeyspaceMeta::KeyspaceMeta( QString const& keyspace_name )
    : f_name(keyspace_name)
{
}

KeyspaceMeta::KeyspaceMeta( const cassvalue::Decoder& decoder )
{
   decodeKeyspaceMeta(decoder);
}

KeyspaceMeta::KeyspaceMeta( keyspace_meta const& km )
    : f_name(km.get_name())
{
    iterator fields_iter( km.get_fields() );
    while( fields_iter.next() )
    {
        const QString field_name( fields_iter.get_meta_field_name() );
        Value val;
        val.readValue(fields_iter);
        f_fields[field_name] = val;
    }

    iterator tables_iter( km.get_tables() );
    while( tables_iter.next() )
    {
        table_meta p_table(tables_iter.get_table_meta());
        TableMeta::pointer_t table( std::make_shared<TableMeta>(p_table) );
        f_tables[table->getName()] = table;
    }
}


/** \brief Generate CQL string to create the keyspace
 */
QString	KeyspaceMeta::getKeyspaceCql() const
{
    QStringList keyspace_cql;
    keyspace_cql << QString("CREATE KEYSPACE IF NOT EXISTS %1").arg(f_name);

    QString sep("  WITH");
    for( auto field : f_fields )
    {
        if( field.first == "keyspace_name" ) continue;

        keyspace_cql << QString("%1 %2 = %3")
               .arg(sep)
               .arg(field.first)
               .arg(field.second.output())
               ;

        sep = "  AND";
    }

    keyspace_cql << "  ;\n";

    return keyspace_cql.join('\n');
}


KeyspaceMeta::string_map_t KeyspaceMeta::getTablesCql() const
{
    string_map_t ret_map;
    for( auto table : f_tables )
    {
        ret_map[table.first] = table.second->getCqlString(f_name);
    }

    return ret_map;
}


const QString& KeyspaceMeta::getName() const
{
    return f_name;
}


const Value::map_t& KeyspaceMeta::getFields() const
{
    return f_fields;
}


Value::map_t& KeyspaceMeta::getFields()
{
    return f_fields;
}


Value& KeyspaceMeta::operator[]( const QString& name )
{
    return f_fields[name];
}


const TableMeta::map_t& KeyspaceMeta::getTables() const
{
    return f_tables;
}


/** \brief Transform a KeyspaceMeta in a blob.
 *
 * This function transforms a KeyspaceMeta object into a blob that can be
 * transferred from snapdbproxy to a client.
 *
 * \return The meta data in a blob.
 */
void KeyspaceMeta::encodeKeyspaceMeta(Encoder& encoder) const
{
    // save the name as a PSTR with a size on 2 bytes
    //
    encoder.appendP16StringValue(f_name);

    // save the keyspace fields
    // first the size on 2 bytes then each field
    encoder.appendUInt16Value(f_fields.size());
    for( auto f : f_fields )
    {
        // field name
        encoder.appendP16StringValue(f.first);

        // field value
        f.second.encodeValue(encoder);
    }

    // save the tables
    // first the size on 2 bytes then each table
    encoder.appendUInt16Value(f_tables.size());
    for( auto t : f_tables )
    {
        t.second->encodeTableMeta(encoder);
    }
}


/** \brief Decode a KeyspaceMeta object from a blob.
 *
 */
void KeyspaceMeta::decodeKeyspaceMeta(const Decoder& decoder)
{
    // retrieve the keyspace name
    //
    f_name = decoder.p16StringValue();

    // read field values
    //
    size_t const field_max(decoder.uint16Value());
    for(size_t idx(0); idx < field_max; ++idx)
    {
        // field name
        QString const name(decoder.p16StringValue());

        // field value
        Value field;
        field.decodeValue(decoder);

        // save field in our map
        f_fields[name] = field;
    }

    // retrieve the tables
    //
    size_t const table_max(decoder.uint16Value());
    for(size_t idx(0); idx < table_max; ++idx)
    {
        TableMeta::pointer_t table(std::make_shared<TableMeta>(decoder));
        f_tables[table->getName()] = table;
    }
}



//================================================================/
// TableMeta
//
TableMeta::TableMeta( QString const& table_name )
    : f_name(table_name)
{
}

TableMeta::TableMeta( const cassvalue::Decoder& decoder )
{
    decodeTableMeta(decoder);
}

TableMeta::TableMeta( table_meta const& tm )
    //: f_table_meta(tm)
    : f_name(tm.get_name())
{
    iterator table_fields_iter( tm.get_fields() );
    while( table_fields_iter.next() )
    {
        const QString field_name( table_fields_iter.get_meta_field_name() );
        Value val;
        val.readValue(table_fields_iter);
        f_fields[field_name] = val;
    }

    iterator columns_iter( tm.get_columns() );
    while( columns_iter.next() )
    {
        column_meta p_col( columns_iter.get_column_meta() );

        ColumnMeta::pointer_t column( std::make_shared<ColumnMeta>(p_col) );
        f_columns[column->getName()] = column;
    }
}


const QString& TableMeta::getName() const
{
    return f_name;
}


const Value::map_t& TableMeta::getFields() const
{
    return f_fields;
}


Value::map_t& TableMeta::getFields()
{
    return f_fields;
}


Value& TableMeta::operator[]( const QString& name )
{
    return f_fields[name];
}


const ColumnMeta::map_t& TableMeta::getColumns() const
{

    return f_columns;
}


/** \brief Transform a TableMeta in a blob.
 *
 * This function transforms a TableMeta object into a blob that can be
 * transferred from snapdbproxy to a client.
 *
 * \return The meta data in a blob.
 */
void TableMeta::encodeTableMeta(Encoder& encoder) const
{
    // save the name as a PSTR with a size on 2 bytes
    //
    encoder.appendP16StringValue(f_name);

    // save the table fields
    // first the size on 2 bytes then each field
    encoder.appendUInt16Value(f_fields.size());
    //for( auto f(f_fields.begin()); f != f_fields.end(); ++f )
    for( auto f : f_fields )
    {
        // field name
        encoder.appendP16StringValue(f.first);

        // field value
        f.second.encodeValue(encoder);
    }

    // save the columns
    // first the size on 2 bytes then each table
    encoder.appendUInt16Value(f_columns.size());
    for( auto c : f_columns )
    {
        c.second->encodeColumnMeta(encoder);
    }
}


/** \brief Decode a TableMeta object from a blob.
 *
 */
void TableMeta::decodeTableMeta(const Decoder& decoder)
{
    // retrieve the table name
    //
    f_name = decoder.p16StringValue();

    // read field values
    //
    size_t const field_max(decoder.uint16Value());
    for(size_t idx(0); idx < field_max; ++idx)
    {
        // field name
        QString const name(decoder.p16StringValue());

        // field value
        Value field;
        field.decodeValue(decoder);

        // save field in our map
        f_fields[name] = field;
    }

    // retrieve the columns
    //
    size_t const column_max(decoder.uint16Value());
    for(size_t idx(0); idx < column_max; ++idx)
    {
        ColumnMeta::pointer_t column(std::make_shared<ColumnMeta>(decoder));
        f_columns[column->getName()] = column;
    }
}


/** \brief Generate CQL string to create the table
 */
QString TableMeta::getCqlString( QString const& keyspace_name ) const
{
    QStringList table_cql;
    table_cql << QString("CREATE TABLE IF NOT EXISTS %1.%2 (")
           .arg(keyspace_name)
           .arg(f_name);

    QString partition_key;
    QString clustering;
    for( auto column : f_columns )
    {
        table_cql <<  QString("  %1,")
                .arg(column.second->getCqlString())
                ;

        const QString kind( column.second->getFields()["kind"].variant().toString() );
        if( kind == "partition_key" )
        {
            partition_key = column.first;
        }
        else if( kind == "clustering" )
        {
            clustering = column.first;
        }
    }

    if( !partition_key.isEmpty() )
    {
        if( clustering.isEmpty() )
        {
            table_cql << QString("  PRIMARY KEY (%1)")
                .arg(partition_key)
                ;
        }
        else
        {
            table_cql << QString("  PRIMARY KEY (%1, %2)")
                .arg(partition_key)
                .arg(clustering)
                ;
        }
    }
    table_cql << ") WITH COMPACT STORAGE";

    for( auto field : f_fields )
    {
        if( field.first == "flags"         ) continue;
        if( field.first == "keyspace_name" ) continue;
        if( field.first == "table_name"    ) continue;

        table_cql << QString("  AND %1 = %2")
               .arg(field.first)
               .arg(field.second.output())
               ;
    }

    table_cql << "  ;\n";
    return table_cql.join('\n');
}


//================================================================/
// ColumnMeta
//
ColumnMeta::ColumnMeta( QString const& column_name )
    : f_name(column_name)
{
}

ColumnMeta::ColumnMeta( const cassvalue::Decoder& decoder )
{
    decodeColumnMeta(decoder);
}

ColumnMeta::ColumnMeta( column_meta const& cm )
    : f_name(cm.get_name())
{
    CassColumnType type = cm.get_column_type();
    switch( type )
    {
    case CASS_COLUMN_TYPE_REGULAR        : f_type = ColumnMeta::type_t::TypeRegular;        break;
    case CASS_COLUMN_TYPE_PARTITION_KEY  : f_type = ColumnMeta::type_t::TypePartitionKey;   break;
    case CASS_COLUMN_TYPE_CLUSTERING_KEY : f_type = ColumnMeta::type_t::TypeClusteringKey;  break;
    case CASS_COLUMN_TYPE_STATIC         : f_type = ColumnMeta::type_t::TypeStatic;         break;
    case CASS_COLUMN_TYPE_COMPACT_VALUE  : f_type = ColumnMeta::type_t::TypeCompactValue;   break;
    }

    f_columnType = getValueType( cm.get_value_type() );

    iterator meta_iter( cm.get_fields() );
    while( meta_iter.next() )
    {
        const QString field_name( meta_iter.get_meta_field_name() );
        Value val;
        val.readValue(meta_iter);
        f_fields[field_name] = val;
    }
}


column_type_t ColumnMeta::getValueType( int const cass_type )
{
    CassValueType vt = static_cast<CassValueType>(cass_type);
    column_type_t ct;
    switch( vt )
    {
    case CASS_VALUE_TYPE_UNKNOWN    :   ct = column_type_t::TypeUnknown    ; break;
    case CASS_VALUE_TYPE_CUSTOM     :   ct = column_type_t::TypeCustom     ; break;
    case CASS_VALUE_TYPE_DECIMAL    :   ct = column_type_t::TypeDecimal    ; break;
    case CASS_VALUE_TYPE_LAST_ENTRY :   ct = column_type_t::TypeLast_entry ; break;
    case CASS_VALUE_TYPE_UDT        :   ct = column_type_t::TypeUdt        ; break;
    case CASS_VALUE_TYPE_LIST       :   ct = column_type_t::TypeList       ; break;
    case CASS_VALUE_TYPE_SET        :   ct = column_type_t::TypeSet        ; break;
    case CASS_VALUE_TYPE_TUPLE      :   ct = column_type_t::TypeTuple      ; break;
    case CASS_VALUE_TYPE_MAP        :   ct = column_type_t::TypeMap        ; break;
    case CASS_VALUE_TYPE_BLOB       :   ct = column_type_t::TypeBlob       ; break;
    case CASS_VALUE_TYPE_BOOLEAN    :   ct = column_type_t::TypeBoolean    ; break;
    case CASS_VALUE_TYPE_FLOAT      :   ct = column_type_t::TypeFloat      ; break;
    case CASS_VALUE_TYPE_DOUBLE     :   ct = column_type_t::TypeDouble     ; break;
    case CASS_VALUE_TYPE_TINY_INT   :   ct = column_type_t::TypeTinyInt    ; break;
    case CASS_VALUE_TYPE_SMALL_INT  :   ct = column_type_t::TypeSmallInt   ; break;
    case CASS_VALUE_TYPE_INT        :   ct = column_type_t::TypeInt        ; break;
    case CASS_VALUE_TYPE_VARINT     :   ct = column_type_t::TypeVarint     ; break;
    case CASS_VALUE_TYPE_BIGINT     :   ct = column_type_t::TypeBigint     ; break;
    case CASS_VALUE_TYPE_COUNTER    :   ct = column_type_t::TypeCounter    ; break;
    case CASS_VALUE_TYPE_ASCII      :   ct = column_type_t::TypeAscii      ; break;
    case CASS_VALUE_TYPE_DATE       :   ct = column_type_t::TypeDate       ; break;
    case CASS_VALUE_TYPE_TEXT       :   ct = column_type_t::TypeText       ; break;
    case CASS_VALUE_TYPE_TIME       :   ct = column_type_t::TypeTime       ; break;
    case CASS_VALUE_TYPE_TIMESTAMP  :   ct = column_type_t::TypeTimestamp  ; break;
    case CASS_VALUE_TYPE_VARCHAR    :   ct = column_type_t::TypeVarchar    ; break;
    case CASS_VALUE_TYPE_UUID       :   ct = column_type_t::TypeUuid       ; break;
    case CASS_VALUE_TYPE_TIMEUUID   :   ct = column_type_t::TypeTimeuuid   ; break;
    case CASS_VALUE_TYPE_INET       :   ct = column_type_t::TypeInet       ; break;
    }
    return ct;
}


const QString& ColumnMeta::getName() const
{
    return f_name;
}


QString ColumnMeta::getCqlString() const
{
    return QString("%1 %2")
            .arg(f_name)
            .arg(f_fields.at("type").variant().toString())
            ;
}


ColumnMeta::type_t ColumnMeta::getType() const
{
    return f_type;
}


column_type_t ColumnMeta::getColumnType() const
{
    return f_columnType;
}


const Value::map_t& ColumnMeta::getFields() const
{
    return f_fields;
}


Value::map_t& ColumnMeta::getFields()
{
    return f_fields;
}


Value& ColumnMeta::operator[]( const QString& name )
{
    return f_fields[name];
}


/** \brief Transform a ColumnMeta in a blob.
 *
 * This function transforms a ColumnMeta object into a blob that can be
 * transferred from snapdbproxy to a client.
 *
 * \return The meta data in a blob.
 */
void ColumnMeta::encodeColumnMeta(Encoder& encoder) const
{
    // save the name as a PSTR with a size on 2 bytes
    //
    encoder.appendP16StringValue(f_name);

    // save the table fields
    // first the size on 2 bytes then each field
    encoder.appendUInt16Value(f_fields.size());
    for( auto f : f_fields )
    {
        // field name
        encoder.appendP16StringValue(f.first);

        // field value
        f.second.encodeValue(encoder);
    }

    // save the column type
    // there are only a very few types so one char is enough
    encoder.appendUnsignedCharValue(static_cast<unsigned char>(f_type));
}


/** \brief Decode a ColumnMeta object from a blob.
 *
 */
void ColumnMeta::decodeColumnMeta(const Decoder& decoder)
{
    // retrieve the column name
    //
    f_name = decoder.p16StringValue();

    // read field values
    //
    size_t const field_max(decoder.uint16Value());
    for(size_t idx(0); idx < field_max; ++idx)
    {
        // field name
        QString const name(decoder.p16StringValue());

        // field value
        Value field;
        field.decodeValue(decoder);

        // save field in our map
        f_fields[name] = field;
    }

    // retrieve the column type
    //
    f_type = static_cast<type_t>(decoder.unsignedCharValue());
}




} //namespace schema
} // namespace casswrapper
// vim: ts=4 sw=4 et
