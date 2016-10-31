/*
 * Text:
 *      CassTools.h
 *
 * Description:
 *      Handling of the CQL interface.
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

#include "cassandra.h"

#include <QString>

namespace QtCassandra
{

namespace CassTools
{


class iterator;
class keyspace;
class result;
class schema_meta;


class collection
{
public:
    struct deleter_t
    {
        void operator()(CassCollection* p) const;
    };

    collection( CassCollectionType type, size_t item_count );
    ~collection();

    CassError append_string( const char* value );

private:
    std::unique_ptr<CassCollection> f_ptr;
};


class column_meta
{
public:
    struct deleter_t
    {
        void operator()(CassColumnMeta* p) const;
    };

    column_meta( const iterator& iter );
    ~column_meta();

    QString        get_name()        const;
    CassColumnType get_column_type() const;
    CassValueType  get_value_type()  const;
    iterator       get_fields()      const;

private:
    CassColumnMeta* f_ptr = nullptr;
};


class cluster
{
public:
    cluster( const CassCluster* );
    ~cluster();

private:
    CassCluster* f_ptr = nullptr;
};


class future
{
public:
    future( const CassFuture* );
    ~future();

private:
    CassFuture* f_ptr = nullptr;
};


class iterator
{
public:
    iterator( const CassIterator* iter   );
    ~iterator();

    iterator get_next()                      const;

    value    get_map_key()                   const;
    value    get_map_value()                 const;

    QString  get_meta_field_name()           const;

    keyspace_meta get_keyspace_meta()        const;
    table_meta    get_table_meta()           const;
    column_meta   get_column_meta()          const;

private:
    CassIterator*   f_ptr = nullptr;
};


class keyspace_meta
{
public:
    keyspace_meta( const CassKeyspaceMeta* );
    ~keyspace_meta();

    iterator get_fields() const;
    iterator get_tables() const;

private:
    CassKeyspaceMeta* f_ptr = nullptr;
};


class result
{
public:
    result( CassResult* result );
    ~result();

    iterator get_iterator() const;

private:
    CassResult* f_ptr = nullptr;
};


class table_meta
{
    table_meta( const CassTableMeta* );
    ~table_meta();

    iterator get_fields()  const;
    iterator get_columns() const;

private:
    CassTableMeta*  f_ptr = nullptr;
};


class schema_meta
{
public:
    schema_meta( const CassSchemaMeta* );
    ~schema_meta();

    iterator    get_keyspaces() const;

private:
    CassSchemaMeta* f_ptr = nullptr;
};


class session
{
    session( const CassSession* );
    ~session();

    schema_meta get_schema_meta() const;

private:
    CassSession*    f_ptr = nullptr;
};


class ssl
{
    ssl( const CassSsl* );
    ~ssl();

private:
    CassSsl*    f_ptr = nullptr;
};


class statement
{
public:
    statement( const CassStatement* );
    ~statement();

private:
    CassStatement*  f_ptr = nullptr;
};


class value
{
public:
    value( const CassValue* );
    ~value();

    iterator get_iterator_from_map();

private:
    CassValue*  f_ptr = nullptr;
};


}
// namespace CassTools

}
//namespace QtCassandra

// vim: ts=4 sw=4 et
