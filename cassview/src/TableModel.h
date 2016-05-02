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

#include <controlled_vars/controlled_vars_need_init.h>

#include <QtCassandra/QCassandraQuery.h>
#include <QtCassandra/QCassandraSession.h>

#include <QAbstractListModel>
#include <QModelIndex>
#include <QMutex>
#include <QRegExp>
#include <QTimer>

#include <memory>
#include <stack>
#include <vector>

class TableModel
    : public QAbstractListModel
{
    Q_OBJECT

public:
    TableModel();

    void setSession
        ( QtCassandra::QCassandraSession::pointer_t session
        , const QString& keyspace_name
        , const QString& table_name
        , const QRegExp& filter = QRegExp()
        , const int32_t page_row_count = 1000
        );
    void doQuery();
    void clear();

    const QString& keyspaceName() const { return f_keyspaceName; }
    const QString& tableName()    const { return f_tableName;    }

    // Read only access
    //
    virtual Qt::ItemFlags   flags       ( QModelIndex const & index ) const;
    virtual QVariant        data        ( QModelIndex const & index, int role = Qt::DisplayRole ) const;
    virtual QVariant        headerData  ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual int             rowCount    ( QModelIndex const & parent = QModelIndex() ) const;

private slots:
    void onFetchQueryFinished();
    //void onQueryTimer();

private:
    QtCassandra::QCassandraSession::pointer_t f_session;
    QtCassandra::QCassandraQuery::pointer_t   f_query;

    QString                                   f_keyspaceName;
    QString                                   f_tableName;
    QRegExp									  f_filter;
    controlled_vars::zint32_t                 f_pageRowCount;
    std::vector<QByteArray>                   f_rows;
    std::stack<QByteArray>					  f_pendingRows;
    QMutex									  f_mutex;
    //QTimer									  f_queryTimer;
    //bool                                      f_stopTimer;
};


// vim: ts=4 sw=4 et
