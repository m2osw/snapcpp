/*
 * Text:
 *      CassWrapper.h
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

#include "casswrapper/CassStubs.h"
#include "cassandra.h"

#include <memory>

#include <QString>

namespace CassWrapper
{


class collection
{
public:
    struct deleter_t
    {
        void operator()(CassCollection* p) const;
    };

    collection( CassCollectionType type, size_t item_count );

    void append_string( const std::string& value ) const;

    void reset() { f_ptr.reset(); }

    friend class statement;

private:
    std::shared_ptr<CassCollection> f_ptr;
};


class column_meta
{
public:
    struct deleter_t
    {
        void operator()(const CassColumnMeta* p) const;
    };

    column_meta( CassColumnMeta* );

    QString        get_name()        const;
    CassColumnType get_column_type() const;
    CassValueType  get_value_type()  const;
    iterator       get_fields()      const;

    void reset() { f_ptr.reset(); }

private:
    std::shared_ptr<CassColumnMeta> f_ptr;
};


class cluster
{
public:
    struct deleter_t
    {
        void operator()(CassCluster* p) const;
    };

    cluster();

    void set_contact_points              ( const QString&  host_list );
    void set_port                        ( const int       port      );
    void set_request_timeout             ( const timeout_t timeout   );
    void set_write_bytes_low_water_mark  ( const uint32_t  low       );
    void set_write_bytes_high_water_mark ( const uint32_t  high      );

    void reset_ssl() const;
    void set_ssl( const ssl& ) const;

    void reset() { f_ptr.reset(); }

    friend class future;

private:
    std::shared_ptr<CassCluster> f_ptr;
};


class future
{
public:
    struct deleter_t
    {
        void operator()(CassFuture* p) const;
    };

    future();
    future( CassFuture* );
    future( const session&, const cluster& );

    CassError   get_error_code()    const;
    QString     get_error_message() const;
    result      get_result()        const;
    bool        is_ready() const;

    void set_callback( void* callback, void* data );

    void wait() const;
    void reset() { f_ptr.reset(); }

    bool operator ==( const future& );
    bool operator !=( const future& );

private:
    std::shared_ptr<CassFuture> f_ptr;
};


class iterator
{
public:
    struct deleter_t
    {
        void operator()(CassIterator* p) const;
    };

    iterator( CassIterator*   );
    iterator( const iterator& );

    bool          next()                 const;

    value         get_map_key()          const;
    value         get_map_value()        const;

    value         get_value()            const;

    QString       get_meta_field_name()  const;
    value         get_meta_field_value() const;

    row           get_row()              const;

    keyspace_meta get_keyspace_meta()    const;
    table_meta    get_table_meta()       const;
    column_meta   get_column_meta()      const;

    void reset() { f_ptr.reset(); }

private:
    std::shared_ptr<CassIterator> f_ptr;
};


class keyspace_meta
{
public:
    struct deleter_t
    {
        void operator()(CassKeyspaceMeta* p) const;
    };

    keyspace_meta( CassKeyspaceMeta* );

    iterator get_fields() const;
    iterator get_tables() const;
    QString  get_name()   const;

    void reset() { f_ptr.reset(); }

private:
    std::shared_ptr<CassKeyspaceMeta> f_ptr;
};


class result
{
public:
    struct deleter_t
    {
        void operator()(const CassResult* p) const;
    };

    result( CassResult*   );
    result( const result& );

    iterator    get_iterator()   const;
    size_t      get_row_count()  const;
    bool        has_more_pages() const;

    void reset() { f_ptr.reset(); }

    friend class statement;

private:
    std::shared_ptr<CassResult> f_ptr;
};


class row
{
public:
    struct deleter_t
    {
        void operator()(CassRow* p) const;
    };

    row( CassRow* );

    value   get_column_by_name ( const QString& ) const;
    value   get_column         ( const int num ) const;

private:
    std::shared_ptr<CassRow> f_ptr;
};


class schema_meta
{
public:
    struct deleter_t
    {
        void operator()(const CassSchemaMeta* p) const;
    };

    schema_meta( const session& );

    iterator    get_keyspaces() const;

    void reset() { f_ptr.reset(); }

private:
    std::shared_ptr<CassSchemaMeta> f_ptr;
};


class session
{
public:
    struct deleter_t
    {
        void operator()(CassSession* p) const;
    };

    session();

    //schema_meta get_schema_meta() const;
    future      execute( const statement& ) const;
    future      close() const;
    void        reset() { f_ptr.reset(); }

    friend class future;
    friend class schema_meta;

private:
    std::shared_ptr<CassSession>    f_ptr;
};


class ssl
{
public:
    struct deleter_t
    {
        void operator()(CassSsl* p) const;
    };

    ssl();

    void add_trusted_cert( const QString& cert );
    void reset() { f_ptr.reset(); }

    friend class cluster;

private:
    std::shared_ptr<CassSsl>    f_ptr;
};


class statement
{
public:
    struct deleter_t
    {
        void operator()(CassStatement* p) const;
    };

    statement( const QString& query, const int bind_count = 0 );

    void set_consistency  ( const CassConsistency consist   ) const;
    void set_timestamp    ( const int64_t         timestamp ) const;
    void set_paging_size  ( const int             size      ) const;
    void set_paging_state ( const result&         res       ) const;

    void bind_bool       ( const size_t num, const bool         value ) const;
    void bind_int32      ( const size_t num, const int32_t      value ) const;
    void bind_int64      ( const size_t num, const int64_t      value ) const;
    void bind_float      ( const size_t num, const float        value ) const;
    void bind_double     ( const size_t num, const double       value ) const;
    void bind_string     ( const size_t num, const std::string& value ) const;
    void bind_string     ( const size_t num, const QString&     value ) const;
    void bind_blob       ( const size_t num, const QByteArray&  value ) const;
    void bind_collection ( const size_t num, const collection&  value ) const;

    void reset() { f_ptr.reset(); }

    friend class session;

private:
    std::shared_ptr<CassStatement>  f_ptr;
    QString                         f_query;
};


class table_meta
{
public:
    struct deleter_t
    {
        void operator()(CassTableMeta* p) const;
    };

    table_meta( CassTableMeta* );

    iterator get_fields()  const;
    iterator get_columns() const;

    QString  get_name() const;

    void reset() { f_ptr.reset(); }

private:
    std::shared_ptr<CassTableMeta>  f_ptr;
};


class value
{
public:
    struct deleter_t
    {
        void operator()(CassValue* p) const;
    };

    value( CassValue* );

    iterator      get_iterator_from_map()        const;
    iterator      get_iterator_from_collection() const;
    iterator      get_iterator_from_tuple()      const;

    CassValueType get_type()           const;
    QString       get_string()         const;
    QByteArray    get_blob()           const;
    bool          get_bool()           const;
    float         get_float()          const;
    double        get_double()         const;
    int8_t        get_int8()           const;
    int16_t       get_int16()          const;
    int32_t       get_int32()          const;
    int64_t       get_int64()          const;
    QString       get_uuid()           const;
    qulonglong    get_uuid_timestamp() const;
    QString       get_inet()           const;

    void reset() { f_ptr.reset(); }

private:
    std::shared_ptr<CassValue>  f_ptr;
};


}
// namespace CassWrapper

// vim: ts=4 sw=4 et
