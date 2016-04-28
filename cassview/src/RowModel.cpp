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

#include "RowModel.h"

#include <snapwebsites/dbutils.h>
#include <snapwebsites/qstring_stream.h>

#include <QtCassandra/QCassandraQuery.h>
#include <QtCassandra/QCassandraSchema.h>
#include <QtCassandra/QCassandraSession.h>

#include <QtCore>

#include "poison.h"

using namespace QtCassandra;


namespace
{
    const int g_row_count = 100;
}

void RowModel::clear()
{
    f_query.reset();
    f_session.reset();
    f_keyspaceName.clear();
    f_tableName.clear();
    f_rowKey.clear();
    f_columns.clear();
    reset();
}


void RowModel::setSession
        ( QCassandraSession::pointer_t session
        , const QString& keyspace_name
        , const QString& table_name
        , const QByteArray& row_key
        , const QRegExp& filter
        )
{
    f_columns.clear();

    f_session      = session;
    f_keyspaceName = keyspace_name;
    f_tableName    = table_name;
    f_rowKey       = row_key;
    f_filter	   = filter;

    f_query = std::make_shared<QCassandraQuery>(f_session);
    f_query->query(
        QString("SELECT column1 FROM %1.%2 WHERE key = ?")
            .arg(f_keyspaceName)
            .arg(f_tableName)
        , 1
        );
    f_query->setPagingSize( g_row_count );
    f_query->bindByteArray( 0, f_rowKey );
    f_query->start( false /*don't block*/ );

    fireQueryTimer();

    reset();
}


/** \brief Single shot timer.
 */
void RowModel::fireQueryTimer()
{
    QTimer::singleShot( 500, this, SLOT(onQueryTimer()) );
}


void RowModel::firePageTimer()
{
    QTimer::singleShot( 500, this, SLOT(onPageTimer()) );
}


void RowModel::onQueryTimer()
{
    if( f_query->isReady() )
    {
        f_query->getQueryResult();
        firePageTimer();
        return;
    }

    fireQueryTimer();
}


void RowModel::onPageTimer()
{
    const int start_pos = static_cast<int>(f_columns.size()-1);
    int count = 0;
    while( f_query->nextRow() )
    {
        bool add = true;
        const QByteArray key( f_query->getByteArrayColumn("column1") );
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
            f_columns.push_back( key );
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

    // Otherwise, done.
}


Qt::ItemFlags RowModel::flags( const QModelIndex & idx ) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if( idx.column() == 0 )
    {
        f |= Qt::ItemIsEditable;
    }
    return f;
}


void RowModel::displayError( std::exception const& except, QString const& message ) const
{
    emit exceptionCaught( except.what(), message );
}


QVariant RowModel::data( QModelIndex const & idx, int role ) const
{
    if( role != Qt::DisplayRole && role != Qt::EditRole ) //&& role != Qt::UserRole )
    {
        return QVariant();
    }

    if( idx.column() < 0 || idx.column() > 1 )
    {
        Q_ASSERT(false);
        return QVariant();
    }

    QSettings settings;
    auto const column_name( *(f_columns.begin() + idx.row()) );
    const QString snap_keyspace( settings.value("snap_keyspace","snap_websites").toString() );
    if( f_keyspaceName == snap_keyspace )
    {
        snap::dbutils du( f_tableName, QString::fromUtf8(f_rowKey.data()) );
        du.set_display_len( 24 );
        return du.get_column_name( column_name );
    }

    return column_name;
}


QVariant RowModel::headerData( int /*section*/, Qt::Orientation orientation, int role ) const
{
    if( role != Qt::DisplayRole || orientation != Qt::Horizontal )
    {
        return QVariant();
    }

    return "Row Name";
}


int RowModel::rowCount( QModelIndex const & /*parent*/ ) const
{
    return f_columns.size();
}


int RowModel::columnCount( const QModelIndex & /*parent*/ ) const
{
    return 1;
}


bool RowModel::setData( const QModelIndex & idx, const QVariant & value, int role )
{
    if( role != Qt::EditRole )
    {
        return false;
    }

    try
    {
        QByteArray key( data( idx, Qt::UserRole ).toByteArray() );
        QByteArray save_value;

        QSettings settings;
        const QString snap_keyspace( settings.value("snap_keyspace","snap_websites").toString() );
        if( f_keyspaceName == snap_keyspace )
        {
            snap::dbutils du( f_tableName, QString::fromUtf8(f_rowKey.data()) );
            du.set_column_value( key, save_value, value.toString() );
        }
        else
        {
            QtCassandra::setStringValue( save_value, value.toString() );
        }

        QCassandraQuery q( f_session );
        q.query(
                    QString("UPDATE %1.%2 SET column1 = ? WHERE key = ?")
                        .arg(f_keyspaceName)
                        .arg(f_tableName)
                    , 2
                    );
        q.bindByteArray( 0, save_value );
        q.bindByteArray( 1, f_rowKey   );
        q.start();
        q.end();

        Q_EMIT dataChanged( idx, idx );

        return true;
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot write data to database.") );
    }

    return false;
}


bool RowModel::setHeaderData( int /*section*/, Qt::Orientation /*orientation*/, const QVariant & /*value*/, int /*role*/ )
{
    return false;
}


bool RowModel::insertRows ( int row, int count, const QModelIndex & parent_index )
{
    beginInsertRows( parent_index, row, row+count );
    for( int i = 0; i < count; ++i )
    {
        const QByteArray newcol( QString("New column %1").arg(i).toUtf8() );
        f_columns.push_back(newcol);

        // TODO: this might be pretty slow. I need to utilize the "prepared query" API.
        //
        QCassandraQuery q( f_session );
        q.query(
                    QString("INSERT INTO %1.%2 VALUES (key,column1) = (?,?)")
                    .arg(f_keyspaceName)
                    .arg(f_tableName)
                    , 2
                    );
        q.bindByteArray( 0, f_rowKey );
        q.bindByteArray( 1, newcol   );
        q.start();
        q.end();
    }
    endInsertRows();

    return true;
}


bool RowModel::removeRows( int row, int count, const QModelIndex & )
{
    // Make a list of the keys we will drop
    //
    QList<QByteArray> key_list;
    for( int idx = 0; idx < count; ++idx )
    {
        const QByteArray key(f_columns[idx + row]);
        key_list << key;
    }

    // Drop each key
    //
    for( auto key : key_list )
    {
        // TODO: this might be pretty slow. I need to utilize the "prepared query" API.
        //
        QCassandraQuery q( f_session );
        q.query(
                    QString("DELETE FROM %1.%2 WHERE key = ? AND column1 = ?")
                    .arg(f_keyspaceName)
                    .arg(f_tableName)
                    , 2
                    );
        q.bindByteArray( 0, f_rowKey );
        q.bindByteArray( 1, key 	 );
        q.start();
        q.end();
    }

    // Remove the columns from the model
    //
    beginRemoveRows( QModelIndex(), row, row+count );
    f_columns.erase( f_columns.begin()+row, f_columns.begin()+row+count );
    endRemoveRows();

    return true;
}


// vim: ts=4 sw=4 et
