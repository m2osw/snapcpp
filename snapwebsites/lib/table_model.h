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

#include <QtCassandra/QCassandraTable.h>
#include <QtCassandra/QCassandraRowPredicate.h>
#include <QAbstractListModel>
#include <QModelIndex>

#include <memory>

namespace snap
{

class table_model
    : public QAbstractListModel
{
    Q_OBJECT

public:
    table_model()
        : f_rowCount(1000)
        //, f_rowsRemaining(0) -- auto-init
        //, f_pos(0) -- auto-init
    {
    }

    QtCassandra::QCassandraTable::pointer_t getTable() const;
    void                    setTable( QtCassandra::QCassandraTable::pointer_t t, QRegExp const & re );

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

private:
    QtCassandra::QCassandraTable::pointer_t f_table;
    QtCassandra::QCassandraRowPredicate     f_rowp;
    // TODO: use controlled_vars instead of constructor
    controlled_vars::mint32_t               f_rowCount;
    controlled_vars::zint32_t               f_rowsRemaining;
    controlled_vars::zint32_t               f_pos;
};

}
// namespace snap

// vim: ts=4 sw=4 et
