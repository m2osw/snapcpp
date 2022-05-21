/*
 * Header:
 *      src/casswrapper/query_model.h
 *
 * Description:
 *      To build queries to be sent to Cassandra.
 *
 * Documentation:
 *      See the corresponding .cpp file.
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

#include <casswrapper/query.h>
#include <casswrapper/session.h>

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QRegExp>

#include <memory>
#include <queue>
#include <vector>


namespace casswrapper
{


class query_model
    : public QAbstractItemModel
{
    Q_OBJECT

public:
                            query_model();

    void                    init( Session::pointer_t session
                                , const QString& keyspace_name
                                , const QString& table_name
                                , const QRegExp& filter = QRegExp()
                                );
    void                    clear();

    const QString&          keyspaceName() const { return f_keyspaceName; }
    const QString&          tableName()    const { return f_tableName;    }
    Query::pointer_t        query()        const { return f_query;        }

    // Read only access
    //
    virtual Qt::ItemFlags   flags        ( QModelIndex const & index ) const override;
    virtual QVariant        data         ( QModelIndex const & index, int role = Qt::DisplayRole ) const override;
    virtual QModelIndex     index        ( int row, int column, const QModelIndex &parent= QModelIndex() ) const override;
    virtual QModelIndex     parent       ( const QModelIndex &child ) const override;
    virtual int             rowCount     ( QModelIndex const & prnt = QModelIndex() ) const override;
    virtual int             columnCount  ( QModelIndex const & prnt = QModelIndex() ) const override;

    // Local virtual methods
    //
    virtual bool            fetchFilter     ( const QByteArray& key );
    virtual void            fetchCustomData ( Query::pointer_t q );

signals:
    void                    exceptionCaught( const QString & what, const QString & message ) const;
    void                    queryFinished() const;

protected:
    void                    doQuery      ( Query::pointer_t query );
    void                    displayError ( const std::exception & except, const QString & message ) const;

    Session::pointer_t      f_session      = Session::pointer_t();
    QString                 f_keyspaceName = QString();
    QString                 f_tableName    = QString();
    std::vector<QByteArray> f_rows         = std::vector<QByteArray>();
    QRegExp                 f_filter       = QRegExp();
    bool                    f_isMore       = false;
    int                     f_columnCount  = 1;
    const int               f_rowPageSize  = 10;    // This is for internal pagination--it has nothing to do with the query.

private:
    void                    reset();
    void                    fetchPageFromServer( std::vector<QByteArray>& fetched_rows );

    Query::pointer_t        f_query       = Query::pointer_t();
    std::queue<QByteArray>  f_pendingRows = std::queue<QByteArray>();

private slots:
    void                    onFetchMore();
    void                    onQueryFinished( Query::pointer_t q );
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
