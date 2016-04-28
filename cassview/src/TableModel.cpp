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

#include <QSettings>
#include <QTimer>
#include <QVariant>

#include <iostream>
#include <exception>

#include "poison.h"


using namespace QtCassandra;
using namespace QCassandraSchema;


TableModel::TableModel()
    //: f_tableName(name)
    //, f_rowCount(row_count)
    //, f_rowsRemaining(0) -- auto-init
    //, f_pos(0)           -- auto-init
{
}


void TableModel::setSession
    ( QCassandraSession::pointer_t session
    , const QString& keyspace_name
    , const QString& table_name
    , const QRegExp& filter
    , const int32_t row_count
    )
{
    f_session      = session;
    f_keyspaceName = keyspace_name;
    f_tableName    = table_name;
    f_filter       = filter;
    f_rowCount     = row_count;
    f_rows.clear();

    f_query = std::make_shared<QCassandraQuery>(f_session);
    f_query->query(
        QString("SELECT key FROM %1.%2")
            .arg(f_keyspaceName)
            .arg(f_tableName)
            );
    f_query->setPagingSize( f_rowCount );
    f_query->start( false /*don't block*/ );

    reset();

    fireQueryTimer();
}


void TableModel::clear()
{
    f_query.reset();
    f_session.reset();
    f_keyspaceName.clear();
    f_tableName.clear();
    f_rows.clear();
    reset();
}


/** \brief Fire single-shot query timer.
 */
void TableModel::fireQueryTimer()
{
    QTimer::singleShot( 500, this, SLOT(onQueryTimer()) );
}


/** \brief Fire single-shot page timer.
 */
void TableModel::firePageTimer()
{
    QTimer::singleShot( 500, this, SLOT(onPageTimer()) );
}


void TableModel::onQueryTimer()
{
    if( f_query->isReady() )
    {
        f_query->getQueryResult();
        firePageTimer();
        return;
    }

    fireQueryTimer();
}


void TableModel::onPageTimer()
{
    const int start_pos = static_cast<int>(f_rows.size()-1);
    int count = 0;
    while( f_query->nextRow() )
    {
        bool add = true;
        const QByteArray key( f_query->getByteArrayColumn("key") );
        if( !f_filter.isEmpty() )
        {
            snap::dbutils du( f_tableName, "" );
            if( f_filter.indexIn( du.get_row_name(key) ) == -1 )
            {
                add = false;
            }
        }
        //
        if( add )
        {
            f_rows.push_back( key );
            count++;
        }
    }
    //
    beginInsertRows
        ( QModelIndex()
        , start_pos
        , start_pos + count
        );
    endInsertRows();

    if( f_query->nextPage( false /*block*/ ) )
    {
        firePageTimer();
    }
}


#if 0
bool TableModel::canFetchMore(QModelIndex const & model_index) const
{
    return f_rows.find( model_index.row() ) == f_rows.end();
}


void TableModel::fetchMore(QModelIndex const & model_index)
{
    if( !f_query )
    {
        f_query = std::make_shared<QCassandraQuery>(f_sessionMeta
    }

    NOTUSED(model_index);

    if( !f_table )
    {
        return;
    }

    try
    {
        //f_table->clearCache();
        f_rowsRemaining = f_table->readRows( f_rowp );

        int32_t const itemsToFetch( qMin(static_cast<int32_t>(f_rowCount), static_cast<int32_t>(f_rowsRemaining)) );

        beginInsertRows( QModelIndex(), f_pos, f_pos + itemsToFetch - 1 );
        endInsertRows();

        f_pos += itemsToFetch;
    }
    catch( std::exception const & x )
    {
        SNAP_LOG_ERROR() << "Exception caught! [" << x.what() << "]";
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
