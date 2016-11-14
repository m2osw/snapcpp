// Copyright (C) 2015-2016  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
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

    void init
        ( Session::pointer_t session
        , const QString& keyspace_name
        , const QString& table_name
        , const QRegExp& filter = QRegExp()
        );
    void clear();

    const QString&              keyspaceName() const { return f_keyspaceName; }
    const QString&              tableName()    const { return f_tableName;    }
    Query::pointer_t  			query()        const { return f_query;        }

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
    virtual bool			fetchFilter     ( const QByteArray& key );
    virtual void			fetchCustomData ( Query::pointer_t q );

signals:
    void exceptionCaught( const QString & what, const QString & message ) const;
    void queryFinished() const;

protected:
    Session::pointer_t           f_session;
    QString                      f_keyspaceName;
    QString                      f_tableName;
    std::vector<QByteArray>      f_rows;
    QRegExp                      f_filter;
    bool                         f_isMore      = false;
    int                          f_columnCount = 1;
    const int                    f_rowPageSize = 10;    // This is for internal pagination--it has nothing to do with the query.

    void doQuery      ( Query::pointer_t query );
    void displayError ( const std::exception & except, const QString & message ) const;

private:
    Query::pointer_t        f_query;
    std::queue<QByteArray>  f_pendingRows;

    void reset();
    void fetchPageFromServer( std::vector<QByteArray>& fetched_rows );

private slots:
    void onFetchMore();
    void onQueryFinished( Query::pointer_t q );
};


}
// namespace casswrapper

// vim: ts=4 sw=4 et
