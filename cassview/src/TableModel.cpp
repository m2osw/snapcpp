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

#include <QtCassandra/QCassandraQuery.h>
#include <QtCassandra/QCassandraSession.h>

#include <QMutexLocker>
#include <QSettings>
#include <QTimer>
#include <QVariant>

#include <iostream>
#include <exception>

#include "poison.h"


using namespace QtCassandra;
using namespace QCassandraSchema;

namespace
{
    const int g_timer_res = 0;
}


TableModel::TableModel()
    //: f_queryTimer(this)
    //, f_stopTimer(false)
{
    //connect( &f_queryTimer, SIGNAL(timeout()), this, SLOT(onQueryTimer()) );
}


void TableModel::setSession
    ( QCassandraSession::pointer_t session
    , const QString& keyspace_name
    , const QString& table_name
    , const QRegExp& filter
    , const int32_t page_row_count
    )
{
    QMutexLocker locker( &f_mutex );
    f_session      = session;
    f_keyspaceName = keyspace_name;
    f_tableName    = table_name;
    f_filter       = filter;
    f_pageRowCount = page_row_count;
}

void TableModel::doQuery()
{
    f_rows.clear();

    f_query = std::make_shared<QCassandraQuery>(f_session);
    connect( f_query.get(), SIGNAL(queryFinished()), this, SLOT(onFetchQueryFinished()) );

    f_query->query(
        QString("SELECT key FROM %1.%2")
            .arg(f_keyspaceName)
            .arg(f_tableName)
            );
    f_query->setPagingSize( f_pageRowCount );
    f_query->start( false /*don't block*/ );

    reset();

    //f_queryTimer.start(g_timer_res);
}


void TableModel::clear()
{
    QMutexLocker locker( &f_mutex );
    f_query.reset();
    f_session.reset();
    f_keyspaceName.clear();
    f_tableName.clear();
    f_rows.clear();
    f_queryTimer.stop();
    while( !f_pendingRows.empty() ) f_pendingRows.pop();
    reset();
}


void TableModel::onFetchQueryFinished()
{
    QMutexLocker locker( &f_mutex );

    f_query->getQueryResult();

    while( f_query->nextRow() )
    {
        f_pendingRows.push( f_query->getByteArrayColumn(0) );
    }

    f_query->nextPage( false /*block*/ );
}

bool TableModel::canFetchMore ( const QModelIndex & parent ) const
{
    QMutexLocker locker( &f_mutex );
    return !f_pendingRows.empty();
}

void TableModel::fetchMore ( const QModelIndex & parent )
{
    QMutexLocker locker( &f_mutex );

    const int start_size = f_rows.size();

    while( !f_pendingRows.empty() )
    {
        f_rows.push_back( f_pendingRows.top() );
        f_pendingRows.pop();
    }

    beginInsertRows
            ( QModelIndex()
              , start_size
              , f_rows.size() + start_size
              );
    endInsertRows();
}

#if 0
void TableModel::onQueryTimer()
{
    QMutexLocker locker( &f_mutex );

    f_queryTimer.stop();

    if( !f_pendingRows.empty() )
    {
        const size_t size( f_rows.size()-1 );
#if 0
        beginInsertRows
                ( QModelIndex()
                  , size
                  , size//+f_pendingRows.size()
                  );
#endif
        f_rows[f_rows.size()] = f_pendingRows.top();
        f_pendingRows.pop();
        //endInsertRows();
    }

    if( f_stopTimer && f_pendingRows.empty() )
    {
        f_stopTimer = false;
    }
    else
    {
        f_queryTimer.start(g_timer_res);
    }
}
#endif


Qt::ItemFlags TableModel::flags( QModelIndex const & idx ) const
{
    snap::NOTUSED(idx);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant TableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    snap::NOTUSED(section);
    snap::NOTUSED(orientation);

    if( role != Qt::DisplayRole )
    {
        return QVariant();
    }

#if 0
    try
    {
        // the rows array is used immediately, so we can keep a temporary
        // reference here
        if( f_rows.size() <= section )
        {
            return QVariant();
        }

        if( orientation == Qt::Horizontal )
        {
            auto const & row( *(f_rows.begin()) );
            Q_ASSERT(row);

            auto const & cell( *(f_row->cells().begin() + section) );
            return cell->columnName();
        }
    }
    catch( std::exception const & x )
    {
        SNAP_LOG_ERROR() << "Exception caught! [" << x.what() << "]";
    }
#endif

    return QVariant("Row Key");
}


QVariant TableModel::data( QModelIndex const & idx, int role ) const
{
    if( static_cast<int>(f_rows.size()) <= idx.row() )
    {
        return QVariant();
    }

    QSettings settings;
    const QString snap_keyspace( settings.value("snap_keyspace","snap_websites").toString() );
    if( role == Qt::DisplayRole || role == Qt::EditRole )
    {
        QString ret_name;
        if( f_keyspaceName == snap_keyspace )
        {
            snap::dbutils du( f_tableName, "" );
            ret_name = du.get_row_name( f_rows[idx.row()] );
        }
        else
        {
            ret_name = QString::fromUtf8( f_rows[idx.row()].data() );
        }
        //
        return ret_name;
    }

    return QVariant();
}


int TableModel::rowCount( QModelIndex const & prnt ) const
{
    if( prnt.isValid() )
    {
        return 1;
    }

    try
    {
        return f_rows.size();
    }
    catch( std::exception const & x )
    {
        SNAP_LOG_ERROR() << "Exception caught! [" << x.what() << "]";
    }

    return 0;
}


// vim: ts=4 sw=4 et
