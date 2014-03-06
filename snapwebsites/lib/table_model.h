// Copyright (C) 2014  Made to Order Software Corp.
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
        table_model() : f_rowCount(1000), f_rowsRemaining(0), f_pos(0) {}

        QtCassandra::QCassandraTable::pointer_t getTable() const;
        void setTable( QtCassandra::QCassandraTable::pointer_t t );

        // Read only access
        //
        Qt::ItemFlags   flags       ( const QModelIndex & index ) const;
        QVariant        data        ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        QVariant        headerData  ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        int             rowCount    ( const QModelIndex & parent = QModelIndex() ) const;

        // Fecth more
        //
        bool canFetchMore ( const QModelIndex & /* index */ ) const;
        void fetchMore    ( const QModelIndex & /* index */ );

    private:
        QtCassandra::QCassandraTable::pointer_t f_table;
        QtCassandra::QCassandraRowPredicate     f_rowp;
        int f_rowCount;
        int f_rowsRemaining;
        int f_pos;
};

}
// namespace snap

// vim: ts=4 sw=4 et
