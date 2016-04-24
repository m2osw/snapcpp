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

#include <QtCassandra/QCassandraSchema.h>

#include <QAbstractListModel>
#include <QModelIndex>

#include <memory>

class TableModel
    : public QAbstractListModel
{
    Q_OBJECT

public:
    TableModel();

    void setSession
        ( QCassandraSession::pointer_t session
        , const QString& keyspace_name
        , const QString& table_name
        , const int32_t row_count = 100
        );

    // Read only access
    //
    virtual Qt::ItemFlags   flags       ( QModelIndex const & index ) const;
    virtual QVariant        data        ( QModelIndex const & index, int role = Qt::DisplayRole ) const;
    virtual QVariant        headerData  ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    virtual int             rowCount    ( QModelIndex const & parent = QModelIndex() ) const;

    // Fecth more
    //
    virtual bool            canFetchMore ( QModelIndex const & index ) const;
    virtual void            fetchMore    ( QModelIndex const & index );

private slots:
    void onTimer();

private:
    QString                                   f_keyspaceName;
    QString                                   f_tableName;
    std::map<int,QVariant>                    f_rows;
    QtCassandra::QCassandraSession::pointer_t f_session;
    QtCassandra::QCassandraQuery::pointer_t   f_query;

    void fireTimer();

    //controlled_vars::mint32_t               		f_rowCount;
    //controlled_vars::zint32_t               		f_rowsRemaining;
    //controlled_vars::zint32_t               		f_pos;
};


// vim: ts=4 sw=4 et
