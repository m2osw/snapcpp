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

#include "casswrapper/query_model.h"
#include "casswrapper/QCassandraQuery.h"
#include "casswrapper/QCassandraSession.h"

#include "NotUsed.h"

#include <QSettings>
#include <QTimer>
#include <QVariant>

#include <iostream>
#include <exception>

//#include "poison.h"


namespace casswrapper
{


namespace
{
    const int g_timer_res = 0;
}


query_model::query_model()
{
}


void query_model::reset()
{
    beginResetModel();
    endResetModel();
}


void query_model::displayError( std::exception const& except, QString const& message ) const
{
    std::cerr << "Exception caught! what=[" << except.what() << "], message=[" << message.toUtf8().data() << "]" << std::endl;
    emit exceptionCaught( except.what(), message );
}


void query_model::init
    ( QCassandraSession::pointer_t session
    , const QString& keyspace_name
    , const QString& table_name
    , const QRegExp& filter
    )
{
    f_session      = session;
    f_keyspaceName = keyspace_name;
    f_tableName    = table_name;
    f_filter       = filter;
}


void query_model::doQuery( QCassandraQuery::pointer_t q )
{
    if( f_query )
    {
        disconnect( f_query.get(), &QCassandraQuery::queryFinished, this, &query_model::onQueryFinished );
    }

    f_rows.clear();
    while( !f_pendingRows.empty() ) f_pendingRows.pop();
    f_isMore = true;

    try
    {
        f_query = q;
        connect( f_query.get(), &QCassandraQuery::queryFinished, this, &query_model::onQueryFinished );
        f_query->start( false /*don't block*/ );
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot start query!") );
    }

    reset();
}


void query_model::clear()
{
    if( f_query )
    {
        disconnect( f_query.get(), &QCassandraQuery::queryFinished, this, &query_model::onQueryFinished );
    }
    f_query.reset();
    f_session.reset();
    f_keyspaceName.clear();
    f_tableName.clear();
    f_rows.clear();
    while( !f_pendingRows.empty() ) f_pendingRows.pop();
    reset();
}


bool query_model::fetchFilter( const QByteArray& key )
{
    if( !f_filter.isEmpty() )
    {
        if( f_filter.indexIn( QString::fromUtf8( key.data() ) ) == -1 )
        {
            return false;
        }
    }
    //
    return true;
}


void query_model::onQueryFinished( QCassandraQuery::pointer_t q )
{
    try
    {
        q->getQueryResult();
        while( q->nextRow() )
        {
            const QByteArray key( q->getByteArrayColumn(0) );
            if( fetchFilter( key ) )
            {
                f_pendingRows.push( key );
                fetchCustomData( f_query );
            }
        }
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot read from database!") );
    }

    QTimer::singleShot( 1000, this, &query_model::onFetchMore );

    // Trigger a new page if there is more
    //
    f_isMore = q->nextPage( false /*block*/ );
}


void query_model::fetchCustomData( QCassandraQuery::pointer_t q )
{
    // Default does nothing
    //
    NOTUSED(q);
}


void query_model::onFetchMore()
{
    try
    {
        const size_t start_row = f_rows.size();
        size_t end_row = start_row;
        for( int idx = 0; idx < f_rowPageSize; ++idx, ++end_row )
        {
            if( f_pendingRows.empty() )
            {
                break;
            }

            f_rows.push_back( f_pendingRows.front() );
            f_pendingRows.pop();
        }
        beginInsertRows
                ( QModelIndex()
                  , start_row
                  , end_row
                  );
        endInsertRows();

        if( !f_isMore )
        {
            // Signal that we are completely done
            //
            emit queryFinished();
        }
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot read from database!") );
    }
}


Qt::ItemFlags query_model::flags( QModelIndex const & idx ) const
{
    NOTUSED(idx);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant query_model::data( QModelIndex const & idx, int role ) const
{
    if( static_cast<int>(f_rows.size()) <= idx.row() )
    {
        return QVariant();
    }

    if( role == Qt::DisplayRole || role == Qt::EditRole )
    {
        return QString::fromUtf8( f_rows[idx.row()].data() );
    }
    else if( role == Qt::UserRole )
    {
        return f_rows[idx.row()];
    }

    return QVariant();
}


QModelIndex query_model::index( int row, int column, const QModelIndex & ) const
{
    if( row < 0 || row >= static_cast<int>(f_rows.size()) )
    {
        return QModelIndex();
    }

    if( column < 0 || column >= f_columnCount )
    {
        return QModelIndex();
    }

    return createIndex( row, column, (void*) 0 );
}


QModelIndex query_model::parent( const QModelIndex & ) const
{
    return QModelIndex();
}


int query_model::rowCount( QModelIndex const & prnt ) const
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
        displayError( x, tr("Invalid row size!") );
    }

    return 0;
}

int query_model::columnCount( QModelIndex const & prnt ) const
{
    NOTUSED(prnt);
    return f_columnCount;
}


}
// namespace casswrapper

// vim: ts=4 sw=4 et
