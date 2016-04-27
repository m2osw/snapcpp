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
    reset();
}


void RowModel::setSession
        ( QCassandraSession::pointer_t session
        , const QString& keyspace_name
        , const QString& table_name
        , const QByteArray& row_key
        )
{
    f_session      = session;
    f_keyspaceName = keyspace_name;
    f_tableName    = table_name;
    f_rowKey       = row_key;

    f_query = std::make_shared<QCassandraQuery>(f_session);
    f_query->query(
        QString("SELECT column1 FROM %1.%2 WHERE key = ?")
            .arg(f_keyspaceName)
            .arg(f_tableName)
            );
    f_query->setPagingSize( g_row_count );
    f_query->bindByteArray( 0, f_rowKey );
    f_query->start( false /*don't block*/ );

    reset();
}


/** \brief Single shot timer.
 */
void RowModel::fireTimer()
{
    QTimer::singleShot( 500, this, SLOT(RowModel::onTimer()) );
}


void RowModel::onTimer()
{
    if( !f_query->isReady() )
    {
        fireTimer();
        return;
    }

    const int start_pos = static_cast<int>(f_rows.size()-1);
    beginInsertRows
        ( QModelIndex()
        , start_pos
        , start_pos + g_row_count
        );
    //
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
        }
    }
    //
    endInsertRows();

    if( f_query->nextPage( false /*block*/ ) )
    {
        fireTimer();
    }

    // Otherwise, done.
}


Qt::ItemFlags RowModel::flags( const QModelIndex & idx ) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if( idx.column() == 1 )
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
    if( !f_row )
    {
        return QVariant();
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::UserRole )
    {
        return QVariant();
    }

    if( idx.column() < 0 || idx.column() > 1 )
    {
        Q_ASSERT(false);
        return QVariant();
    }

    try
    {
        QCassandraCells const& cell_list(f_row->cells());
        auto const cell( (cell_list.begin() + idx.row()).value() );

        if( role == Qt::UserRole )
        {
            return cell->columnKey();
        }

        const QString snap_keyspace( settings.value("snap_keyspace","snap_websites") );
        if( f_keyspaceName == snap_keyspace )
        {
            snap::dbutils du( f_tableName, QString::fromUtf8(f_rowKey.data()) );
            switch( idx.column() )
            {
                case 0:
                    du.set_display_len( 24 );
                    return du.get_column_name( cell );

                case 1:
                    du.set_display_len( 64 );
                    return du.get_column_value( cell, role == Qt::DisplayRole /*display_only*/ );
            }

            Q_ASSERT(false);
            return QVariant();
        }

        auto const value( idx.column() == 0 ? cell->columnName(): cell->value() );
        return value.stringValue();
    }
    catch( std::exception const& except )
    {
        displayError( except, tr("Cannot read data from database.") );
    }

    return QVariant();
}


QVariant RowModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( role != Qt::DisplayRole || orientation != Qt::Horizontal )
    {
        return QVariant();
    }

    return "Row Name";
}


int RowModel::rowCount( QModelIndex const & /*parent*/ ) const
{
    if( !f_row )
    {
        return 0;
    }

    try
    {
        QCassandraCells const& cell_list(f_row->cells());
        return cell_list.size();
    }
    catch( std::exception const& except )
    {
        displayError( except, tr("Cannot obtain row count from database.") );
    }

    return 0;
}


int RowModel::columnCount( const QModelIndex & /*parent*/ ) const
{
    return 1;
}


bool RowModel::setData( const QModelIndex & idx, const QVariant & value, int role )
{
    if( !f_row )
    {
        return false;
    }

    if( role != Qt::EditRole )
    {
        return false;
    }

    try
    {
        QByteArray key( data( idx, Qt::UserRole ).toByteArray() );
        QByteArray save_value;

        const QString snap_keyspace( settings.value("snap_keyspace","snap_websites") );
        if( f_keyspaceName == snap_keyspace )
        {
            snap::dbutils du( f_tableName, QString::fromUtf8(f_rowKey.data()) );
            du.set_column_value( key, save_value, value.toString() );
        }
        else
        {
            QtCassandra::setStringValue( save_value, value );
        }

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


bool RowModel::insertNewRow( const QString& new_name, const QString& new_value )
{
    f_newName  = new_name;
    f_newValue = new_value;

    return insertRows( 0, 0 );
}


bool RowModel::insertRows ( int /*row*/, int /*count*/, const QModelIndex & parent_index )
{
    bool retval( true );
    try
    {
        beginInsertRows( parent_index, rowCount(), 1 );
        auto key( (*f_row)[f_newName].columnKey() );
        auto cell( f_row->findCell( key ) );
        cell->setTimestamp( QCassandraValue::TIMESTAMP_MODE_AUTO );

        const QString snap_keyspace( settings.value("snap_keyspace","snap_websites") );
        if( f_keyspaceName == snap_keyspace )
        {
            snap::dbutils du( f_tableName, QString::fromUtf8(f_rowKey.data()) );
            du.set_column_value( cell, f_newValue );
        }
        else
        {
            QCassandraValue v;
            v.setStringValue( f_newValue );
            cell->setValue( v );
        }
        endInsertRows();
        reset();
    }
    catch( const std::exception& except )
    {
        endInsertRows();
        displayError( except, tr("Cannot add rows to database.") );
        retval = false;
    }
    return retval;
}


bool RowModel::removeRows ( int row, int count, const QModelIndex & )
{
    try
    {
        // Make a list of the keys we will drop
        //
        QList<QByteArray> key_list;
        for( int idx = 0; idx < count; ++idx )
        {
            const QByteArray key( data( index(idx + row, 0), Qt::UserRole ).toByteArray() );
            key_list << key;
        }

        // Drop each key
        //
        for( auto key : key_list )
        {
            f_row->dropCell( key );
        }

        reset();

        return true;
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot remove rows to database.") );
    }

    return false;
}


// vim: ts=4 sw=4 et
