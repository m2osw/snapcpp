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

#include "TableModel.h"
#include <snapwebsites/dbutils.h>
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>

#include <QSettings>
#include <QVariant>

#include <iostream>
#include <exception>

#include "poison.h"

using namespace QtCassandra;


TableModel::TableModel()
{
}


void TableModel::doQuery()
{
    auto q = std::make_shared<QCassandraQuery>(f_session);
    q->query(
        QString("SELECT DISTINCT key FROM %1.%2")
            .arg(f_keyspaceName)
            .arg(f_tableName)
            );
    q->setPagingSize( 10 );

    QueryModel::doQuery( q );
}


QVariant TableModel::data( QModelIndex const & idx, int role ) const
{
    if( role == Qt::UserRole )
    {
        return QueryModel::data( idx, role );
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    if( static_cast<int>(f_rows.size()) <= idx.row() )
    {
        return QVariant();
    }

    QSettings settings;
    const QString snap_keyspace( settings.value("snap_keyspace","snap_websites").toString() );
    if( f_keyspaceName == snap_keyspace )
    {
        snap::dbutils du( f_tableName, "" );
        return du.get_row_name( f_rows[idx.row()] );
    }

    return QueryModel::data( idx, role );
}


// vim: ts=4 sw=4 et
