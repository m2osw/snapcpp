/*
 * Text:
 *      src/casswrapper/query.h
 *
 * Description:
 *      Handling of the cassandra::CfDef (Column Family Definition).
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2019  Made to Order Software Corp.  All Rights Reserved
 * 
 *      https://snapwebsites.org/
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

#include <exception>
#include <map>
#include <memory>
#include <string>
#include <mutex>

#include <QByteArray>
#include <QObject>
#include <QString>

#include "casswrapper/batch.h"
#include "casswrapper/schema.h"
#include "casswrapper/session.h"



namespace casswrapper
{

struct data;
class batch;
class value;


class QueryCallback
{
public:
    virtual         ~QueryCallback() {}

    virtual void    threadFinished() = 0;
};




// this class has two problems with -Weffc++:
//   1. the std::enable_shared_from_this<>() which does not have a virtual
//      destructor (as expected)
//   2. the 'data' structure is not defined and std::unique_ptr<>() can't
//      do sizeof(data) as a consequence
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class Query
    : public QObject
    , public QEnableSharedFromThis<Query>
{
    Q_OBJECT

public:
    enum class consistency_level_t
    {
        level_default       ,
        level_one           ,
        level_quorum        ,
        level_local_quorum  ,
        level_each_quorum   ,
        level_all           ,
        level_any           ,
        level_two           ,
        level_three
    };

    // somehow the registration of pointer_t when it's a shared_ptr<> failed
    // and Q_DECLARE_SMART_POINTER_METATYPE(std::shared_ptr) did not help
    // however, it could be that I missed the "pointer_t" string in the
    // qRegisterMetaType() call...
    //
    //typedef std::shared_ptr<Query>              pointer_t;

    typedef QSharedPointer<Query>               pointer_t;
    typedef std::map<std::string, std::string>  string_map_t;

    virtual             ~Query              () override;

    static pointer_t    create              ( Session::pointer_t session );

    void                addCallback         ( QueryCallback * callback );
    void                removeCallback      ( QueryCallback * callback );

    Session::pointer_t  getSession          () const;

    const QString&      description         () const;
    void                setDescription      ( const QString& val );

    consistency_level_t consistencyLevel    () const;
    void                setConsistencyLevel ( consistency_level_t level );

    int64_t             timestamp           () const;
    void                setTimestamp        ( int64_t val );

    void                query               ( const QString& query_string, const int bind_count = -1 );
    int                 getBindCount        () const;
    int                 pagingSize          () const;
    void                setPagingSize       ( const int size );

    void                bindByteArray       ( const size_t   id, const QByteArray&   value );
    void                bindByteArray       ( const QString& id, const QByteArray&   value );
    void                bindVariant         ( const size_t   id, const QVariant&     value );
    void                bindVariant         ( const QString& id, const QVariant&     value );
    void                bindJsonMap         ( const size_t   id, const string_map_t& value );
    void                bindJsonMap         ( const QString& id, const string_map_t& value );
    void                bindMap             ( const size_t   id, const string_map_t& value );
    void                bindMap             ( const QString& id, const string_map_t& value );

    void                start               ( const bool block = true );
    bool                isReady             () const;
    bool                queryActive         () const;
    size_t              rowCount            () const;
    size_t              columnCount         () const;
    QString             columnName          ( size_t const index );
    schema::column_type_t columnType        ( size_t const index );
    bool                nextRow             ();
    bool                nextPage            ( const bool block = true );
    void                end                 ();
    void                reset               ();

    QVariant            getVariantColumn    ( const size_t   id   ) const;
    QVariant            getVariantColumn    ( const QString& id   ) const;
    QByteArray          getByteArrayColumn  ( const char *   name ) const;
    QByteArray          getByteArrayColumn  ( const QString& name ) const;
    QByteArray          getByteArrayColumn  ( const int      num  ) const;
    string_map_t        getJsonMapColumn    ( const QString& name ) const;
    string_map_t        getJsonMapColumn    ( const int num ) const;
    string_map_t        getMapColumn        ( const QString& name ) const;
    string_map_t        getMapColumn        ( const int num ) const;

signals:
    void                queryFinished       ( pointer_t q );

private:
    friend class Batch;

    typedef std::vector<pointer_t>                  pointer_list_t;
    typedef std::lock_guard<std::recursive_mutex>   lock_t;
    typedef std::vector<QueryCallback *>            callback_list_t;

                        Query                   ( Session::pointer_t session );

    void                addToBatch              ( batch * batch_ptr );
    void                setStatementConsistency ();
    void                setStatementTimestamp   ();
    void                throwIfError            ( const QString& msg );
    void                internalStart           ( const bool block, batch * batch_ptr = nullptr );

    void                getQueryResult          ();
    casswrapper::value  getColumnValue          ( const size_t   id ) const;
    casswrapper::value  getColumnValue          ( const QString& id ) const;

    static void         queryCallbackFunc       ( void * future, void * data );
    void                addToPendingList        ();
    void                threadQueryFinished     ();

    // Current query
    //
    Session::pointer_t              f_session           = Session::pointer_t();
    QString                         f_description       = QString();
    QString                         f_queryString       = QString();
    //
    std::unique_ptr<data>           f_data;  // = std::unique_ptr<data>(); -- data is an incomplete type at this point
    //
    consistency_level_t             f_consistencyLevel  = consistency_level_t::level_default;
    int64_t                         f_timestamp         = 0;
    int64_t                         f_timeout           = 0;
    int                             f_pagingSize        = -1;
    int                             f_bindCount         = -1;

    // Background thread management
    //
    pointer_list_t                  f_selves            = pointer_list_t();
    callback_list_t                 f_callbackList      = callback_list_t();

    static std::recursive_mutex     g_mutex;
};
#pragma GCC diagnostic pop


}
// namespace casswrapper

Q_DECLARE_METATYPE( casswrapper::Query::pointer_t )


// vim: ts=4 sw=4 et
