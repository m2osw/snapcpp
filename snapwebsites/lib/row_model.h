//===============================================================================
// Copyright (c) 2005-2013 by Made to Order Software Corporation
// 
// All Rights Reserved.
// 
// The source code in this file ("Source Code") is provided by Made to Order Software Corporation
// to you under the terms of the GNU General Public License, version 2.0
// ("GPL").  Terms of the GPL can be found in doc/GPL-license.txt in this distribution.
// 
// By copying, modifying or distributing this software, you acknowledge
// that you have read and understood your obligations described above,
// and agree to abide by those obligations.
// 
// ALL SOURCE CODE IN THIS DISTRIBUTION IS PROVIDED "AS IS." THE AUTHOR MAKES NO
// WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
// COMPLETENESS OR PERFORMANCE.
//===============================================================================

#pragma once

#include <QtCassandra/QCassandraRow.h>
#include <QAbstractTableModel>
#include <QModelIndex>
#include <QString>
#include <QVariant>

#include <memory>

namespace snap
{

class row_model
    : public QAbstractTableModel
{
    Q_OBJECT

    public:
        row_model() {}

        QtCassandra::QCassandraRow::pointer_t   getRow() const;
        void                                    setRow( QtCassandra::QCassandraRow::pointer_t c );

        Qt::ItemFlags   flags       ( const QModelIndex & index ) const;
        QVariant        data        ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        QVariant        headerData  ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        int             rowCount    ( const QModelIndex & parent = QModelIndex() ) const;
        int             columnCount ( const QModelIndex & parent = QModelIndex() ) const;

        // Write access
        //
        bool            setData       ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
        bool            setHeaderData ( int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole );

        // Resizable methods
        //
        bool insertNewRow( const QString& new_name, const QString& new_value );
        bool insertRows ( int row, int count, const QModelIndex & parent = QModelIndex() );
        bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );

    signals:
        void exceptionCaught( const QString& what, const QString& message );

    private:
        QtCassandra::QCassandraRow::pointer_t   f_row;
        QString f_newName;
        QString f_newValue;

        void displayError( const std::exception& except, const QString& message );
};

}
// namespace snap


// vim: ts=4 sw=4 et syntax=cpp.doxygen
