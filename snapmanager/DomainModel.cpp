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

#include "DomainModel.h"
#include <snapwebsites/snapwebsites.h>

#include <QSettings>
#include <QVariant>

#include <iostream>
#include <exception>

#include "poison.h"

using namespace QtCassandra;


DomainModel::DomainModel()
{
}


void DomainModel::doQuery()
{
    QString const context_name(snap::get_name(snap::name_t::SNAP_NAME_CONTEXT));
    QString const table_name(snap::get_name(snap::name_t::SNAP_NAME_DOMAINS));

    auto q = std::make_shared<QCassandraQuery>(f_session);
    q->query(
        QString("SELECT DISTINCT key FROM %1.%2")
            .arg(context_name)
            .arg(table_name)
        );
    q->setPagingSize( 100 );

    QueryModel::doQuery( q );
}


bool DomainModel::fetchFilter( const QByteArray& key )
{
    if( !QueryModel::fetchFilter( key ) )
    {
        return false;
    }

    QString const row_index_name(snap::get_name(snap::name_t::SNAP_NAME_INDEX));
    if( key == row_index_name )
    {
        // Ignore *index* entries
        return false;
    }

    return true;
}


#if 0
QVariant DomainModel::data( QModelIndex const & idx, int role ) const
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
#endif


// vim: ts=4 sw=4 et
