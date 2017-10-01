/*
 * Text:
 *      src/casswrapper/schema.h
 *
 * Description:
 *      Database schema metadata.
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
#pragma once

#include "casswrapper/session.h"
#include "casswrapper/schema_value.h"

#include <cassvalue/encoder.h>

#include <QString>


namespace casswrapper
{

class keyspace_meta;
class table_meta;
class column_meta;

namespace schema
{

enum class column_type_t
{
    TypeUnknown    ,
    TypeCustom     ,
    TypeDecimal    ,
    TypeLast_entry ,
    TypeUdt        ,
    TypeList       ,
    TypeSet        ,
    TypeTuple      ,
    TypeMap        ,
    TypeBlob       ,
    TypeBoolean    ,
    TypeFloat      ,
    TypeDouble     ,
    TypeTinyInt    ,
    TypeSmallInt   ,
    TypeInt        ,
    TypeVarint     ,
    TypeBigint     ,
    TypeCounter    ,
    TypeAscii      ,
    TypeDate       ,
    TypeText       ,
    TypeTime       ,
    TypeTimestamp  ,
    TypeVarchar    ,
    TypeUuid       ,
    TypeTimeuuid   ,
    TypeInet
};

class ColumnMeta
{
public:
    typedef std::shared_ptr<ColumnMeta>         pointer_t;
    typedef std::weak_ptr<ColumnMeta>           weak_pointer_t;
    typedef std::map<QString, pointer_t>        map_t;

    enum class type_t {
        TypeRegular,
        TypePartitionKey,
        TypeClusteringKey,
        TypeStatic,
        TypeCompactValue
    };

    ColumnMeta( QString const& column_name     );
    ColumnMeta( const cassvalue::Decoder& code );
    ColumnMeta( column_meta const& cm          );

    const QString&                  getName() const;
    type_t                          getType() const;
    column_type_t                   getColumnType() const;
    const Value::map_t&             getFields() const;
    Value::map_t&                   getFields();

    Value&                          operator [] ( const QString& name );

    void                            encodeColumnMeta(cassvalue::Encoder& encoded) const;

    QString     					getCqlString() const;

    static column_type_t            getValueType( int const cass_type );

private:
    QString                         f_name;
    Value::map_t                    f_fields;
    type_t                          f_type       = type_t::TypeRegular;
    column_type_t                   f_columnType = column_type_t::TypeUnknown;

    void                            decodeColumnMeta(const cassvalue::Decoder& decoder);
};

class TableMeta
{
public:
    typedef std::shared_ptr<TableMeta>          pointer_t;
    typedef std::weak_ptr<TableMeta>            weak_pointer_t;
    typedef std::map<QString, pointer_t>        map_t;

    TableMeta( QString const& table_name         );
    TableMeta( const cassvalue::Decoder& decoder );
    TableMeta( table_meta const & tm             );

    const QString&                  getName()   const;
    const Value::map_t&             getFields() const;
    Value::map_t&                   getFields();

    Value&                          operator [] ( const QString& name );

    const ColumnMeta::map_t&        getColumns() const;

    void                            encodeTableMeta(cassvalue::Encoder& encoded) const;

    QString      					getCqlString( QString const& keyspace_name ) const;

private:
    QString                         f_name;
    Value::map_t                    f_fields;
    ColumnMeta::map_t               f_columns;

    void                            decodeTableMeta(const cassvalue::Decoder& decoder);
};

class KeyspaceMeta
{
public:
    typedef std::shared_ptr<KeyspaceMeta>       pointer_t;
    typedef std::weak_ptr<KeyspaceMeta>         weak_pointer_t;
    typedef std::map<QString, pointer_t>        map_t;
    typedef std::map<QString, QString>          string_map_t;

    KeyspaceMeta( const QString& keyspace_name      );
    KeyspaceMeta( const cassvalue::Decoder& decoder );
    KeyspaceMeta( keyspace_meta const & km          );

    const QString&                  getName()   const;
    const Value::map_t&             getFields() const;
    Value::map_t&                   getFields();

    Value&                          operator [] ( const QString& name );

    const TableMeta::map_t&         getTables() const;

    void                            encodeKeyspaceMeta(cassvalue::Encoder& encoded) const;

    QString							getKeyspaceCql() const;
    string_map_t					getTablesCql() const;

private:
    QString                         f_name;
    Value::map_t                    f_fields;
    TableMeta::map_t                f_tables;

    void                            decodeKeyspaceMeta(const cassvalue::Decoder& decoder);
};

class SessionMeta
        : public std::enable_shared_from_this<SessionMeta>
{
public:
    typedef std::shared_ptr<SessionMeta>        pointer_t;
    typedef std::weak_ptr<SessionMeta>          weak_pointer_t;
    typedef std::map<QString, pointer_t>        map_t;

    SessionMeta( Session::pointer_t session = Session::pointer_t() );
    ~SessionMeta();

    static pointer_t                create( Session::pointer_t session );
    void                            loadSchema();
    Session::pointer_t              get_session() const;
    uint32_t                        snapshotVersion() const;
    const KeyspaceMeta::map_t &     getKeyspaces();
    QByteArray                      encodeSessionMeta() const;
    void                            decodeSessionMeta(const QByteArray& encoded);

private:
    Session::pointer_t              f_session;
    KeyspaceMeta::map_t             f_keyspaces;
};


} //namespace schema

} //namespace casswrapper

// vim: ts=4 sw=4 et
