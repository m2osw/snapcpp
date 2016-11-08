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

#include <casswrapper/query_model.h>

#include <snapwebsites/dbutils.h>

#include <memory>

class RowModel
    : public casswrapper::query_model
{
    Q_OBJECT

public:
    RowModel();

    const QByteArray&	  rowKey() const		             { return f_rowKey; }
    void                  setRowKey( const QByteArray& key ) { f_rowKey = key;  }

    virtual bool          fetchFilter( const QByteArray& key ) override;

    virtual QVariant      data  ( const QModelIndex & index, int role = Qt::DisplayRole ) const override;
    virtual Qt::ItemFlags flags ( const QModelIndex & idx ) const override;

    // Write access
    //
    bool                setData( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole ) override;

    // Resizable methods
    //
    bool                insertRows( int row, int count, const QModelIndex & parent = QModelIndex() ) override;
    virtual bool        removeRows( int row, int count, const QModelIndex & parent = QModelIndex() ) override;

    void 				doQuery();

private:
    QByteArray                      f_rowKey;
    std::shared_ptr<snap::dbutils>  f_dbutils;
};


// vim: ts=4 sw=4 et
