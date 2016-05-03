//===============================================================================
// Copyright (c) 2005-2016 by Made to Order Software Corporation
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

#include "QueryModel.h"

class RowModel
    : public QueryModel
{
    Q_OBJECT

public:
    RowModel();

    const QByteArray&	rowKey() const		               { return f_rowKey; }
    void                setRowKey( const QByteArray& key ) { f_rowKey = key;  }

    QVariant            data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags		flags( const QModelIndex & idx ) const;

    // Write access
    //
    bool                setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

    // Resizable methods
    //
    bool                insertRows( int row, int count, const QModelIndex & parent = QModelIndex() );
    bool                removeRows( int row, int count, const QModelIndex & parent = QModelIndex() );

    void 				doQuery();

signals:
    void                exceptionCaught( const QString & what, const QString & message ) const;

private:
    void                displayError( const std::exception & except, const QString & message ) const;

    QByteArray          f_rowKey;
};


// vim: ts=4 sw=4 et
