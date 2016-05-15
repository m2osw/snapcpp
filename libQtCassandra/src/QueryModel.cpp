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

#include "QtCassandra/QueryModel.h"
#include "QtCassandra/QCassandraQuery.h"
#include "QtCassandra/QCassandraSession.h"

#include "NotUsed.h"

#include <QSettings>
#include <QVariant>

#include <iostream>
#include <exception>

//#include "poison.h"


namespace QtCassandra
{


namespace
{
    const int g_timer_res = 0;
}


QueryModel::QueryModel()
    : f_isMore(false)
{
}


void QueryModel::reset()
{
    beginResetModel();
    endResetModel();
}


void QueryModel::displayError( std::exception const& except, QString const& message ) const
{
    std::cerr << "Exception caught! what=[" << except.what() << "], message=[" << message.toUtf8().data() << "]" << std::endl;
    emit exceptionCaught( except.what(), message );
}


void QueryModel::init
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


void QueryModel::doQuery( QCassandraQuery::pointer_t query )
{
    f_rows.clear();
    f_isMore = true;

    try
    {
        f_query = query;
        f_query->start( false /*don't block*/ );
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot start query!") );
    }

    reset();
}


void QueryModel::clear()
{
    f_query.reset();
    f_session.reset();
    f_keyspaceName.clear();
    f_tableName.clear();
    f_rows.clear();
    reset();
}


bool QueryModel::fetchFilter( const QByteArray& key )
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


bool QueryModel::canFetchMore ( const QModelIndex & prnt ) const
{
    NOTUSED(prnt);
    return f_isMore;
}


void QueryModel::fetchCustomData( QCassandraQuery::pointer_t q )
{
    // Default does nothing
    //
    NOTUSED(q);
}


void QueryModel::fetchMore ( const QModelIndex & prnt )
{
    NOTUSED(prnt);

    if( !f_query )
    {
        return;
    }

    try
    {
        f_query->getQueryResult();

        while( f_query->nextRow() )
        {
            const QByteArray key( f_query->getByteArrayColumn(0) );
            if( fetchFilter( key ) )
            {
                beginInsertRows
                        ( QModelIndex()
                          , f_rows.size()
                          , f_rows.size()+1
                          );
                f_rows.push_back( key );
                fetchCustomData( f_query );
                endInsertRows();
            }
        }

        f_isMore = f_query->nextPage( false /*block*/ );
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot read from database!") );
    }
}


Qt::ItemFlags QueryModel::flags( QModelIndex const & idx ) const
{
    NOTUSED(idx);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant QueryModel::data( QModelIndex const & idx, int role ) const
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


int QueryModel::rowCount( QModelIndex const & prnt ) const
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

int QueryModel::columnCount( QModelIndex const & prnt ) const
{
    NOTUSED(prnt);
    return 1;
}


}
// namespace QtCassandra

// vim: ts=4 sw=4 et
