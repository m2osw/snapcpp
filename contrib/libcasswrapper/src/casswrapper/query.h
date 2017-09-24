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

#include <exception>
#include <map>
#include <memory>
#include <string>

#include <QByteArray>
#include <QObject>
#include <QString>

#include "casswrapper/session.h"
#include "casswrapper/batch.h"


namespace casswrapper
{

struct data;

class Query
    : public QObject
    , public std::enable_shared_from_this<Query>
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

    typedef std::shared_ptr<Query>  pointer_t;
    typedef std::map<std::string,std::string> string_map_t;

    ~Query();

    static pointer_t    create( Session::pointer_t session );

    const QString&      description         () const;
    void                setDescription      ( const QString& val );

    consistency_level_t	consistencyLevel    () const;
    void                setConsistencyLevel ( consistency_level_t level );

    int64_t				timestamp           () const;
    void				setTimestamp        ( int64_t val );

    void                query               ( const QString& query_string, const int bind_count = -1 );
    int                 pagingSize          () const;
    void                setPagingSize       ( const int size );

    void                bindBool            ( const size_t num, const bool          value );
    void                bindInt32           ( const size_t num, const int32_t       value );
    void                bindInt64           ( const size_t num, const int64_t       value );
    void                bindFloat           ( const size_t num, const float         value );
    void                bindDouble          ( const size_t num, const double        value );
    void                bindString          ( const size_t num, const QString&      value );
    void                bindByteArray       ( const size_t num, const QByteArray&   value );
    void                bindJsonMap         ( const size_t num, const string_map_t& value );
    void                bindMap             ( const size_t num, const string_map_t& value );

    void                startBatch          ( Batch const& batch );
    void                addToBatch          ();
    void                endBatch            ( const bool block = true );

    void                start               ( const bool block = true );
    bool	            isReady             () const;
    bool				queryActive		    () const;
    void                getQueryResult      ();
    size_t              rowCount            () const;
    bool                nextRow             ();
    bool                nextPage            ( const bool block = true );
    void                end                 ();

    bool                getBoolColumn       ( const QString& name  ) const;
    bool                getBoolColumn       ( const int      num   ) const;
    int32_t             getInt32Column      ( const QString& name  ) const;
    int32_t             getInt32Column      ( const int      num   ) const;
    int64_t             getInt64Column      ( const QString& name  ) const;
    int64_t             getInt64Column      ( const int      num   ) const;
    float               getFloatColumn      ( const QString& name  ) const;
    float               getFloatColumn      ( const int      num   ) const;
    double              getDoubleColumn     ( const QString& name  ) const;
    double              getDoubleColumn     ( const int      num   ) const;
    QString             getStringColumn     ( const QString& name  ) const;
    QString             getStringColumn     ( const int      num   ) const;
    QByteArray          getByteArrayColumn  ( const char *   name  ) const;
    QByteArray          getByteArrayColumn  ( const QString& name  ) const;
    QByteArray          getByteArrayColumn  ( const int      num   ) const;

    string_map_t        getJsonMapColumn    ( const QString& name ) const;
    string_map_t        getJsonMapColumn    ( const int num ) const;
    string_map_t        getMapColumn        ( const QString& name ) const;
    string_map_t        getMapColumn        ( const int num ) const;

signals:
    void                threadQueryFinished( Query* q );
    void                queryFinished( Query::pointer_t q );

private slots:
    void                onThreadQueryFinished( Query* q );

private:
    Query( Session::pointer_t session );

    // Current query
    //
    Session::pointer_t           f_session;
    QString                      f_description;
    QString                      f_queryString;
    //
    std::unique_ptr<data>        f_data;
    //
    consistency_level_t          f_consistencyLevel = consistency_level_t::level_default;
    int64_t                      f_timestamp        = 0;
    int64_t                      f_timeout          = 0;
    int                          f_pagingSize       = -1;

    void 		        setStatementConsistency ();
    void 		        setStatementTimestamp   ();
    void                throwIfError            ( const QString& msg );
    void                internalStart           ( const bool block   );

    static void		    queryCallbackFunc( void* future, void *data );
};


}
// namespace casswrapper

Q_DECLARE_METATYPE( casswrapper::Query::pointer_t )

// vim: ts=4 sw=4 et
