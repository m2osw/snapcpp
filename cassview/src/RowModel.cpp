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
#include <snapwebsites/not_used.h>
#include <snapwebsites/qstring_stream.h>

#include <QtCore>

#include "poison.h"

using namespace QtCassandra;


RowModel::RowModel()
{
}


void RowModel::doQuery()
{
    auto q = std::make_shared<QCassandraQuery>(f_session);
    q->query(
        QString("SELECT column1 FROM %1.%2 WHERE key = ?")
            .arg(f_keyspaceName)
            .arg(f_tableName)
        , 1
        );
    q->setPagingSize( 10 );
    q->bindByteArray( 0, f_rowKey );

    QueryModel::doQuery( q );
}


Qt::ItemFlags RowModel::flags( const QModelIndex & idx ) const
{
#if 0
    // Editing is disabled for now.
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if( idx.column() == 0 )
    {
        f |= Qt::ItemIsEditable;
    }
    return f;
#endif
    snap::NOTUSED(idx);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant RowModel::data( QModelIndex const & idx, int role ) const
{
    if( role == Qt::UserRole )
    {
        return QueryModel::data( idx, role );
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    if( idx.column() < 0 || idx.column() > 1 )
    {
        Q_ASSERT(false);
        return QVariant();
    }

    QSettings settings;
    auto const column_name( *(f_rows.begin() + idx.row()) );
    const QString snap_keyspace( settings.value("snap_keyspace","snap_websites").toString() );
    if( f_keyspaceName == snap_keyspace )
    {
        snap::dbutils du( f_tableName, QString::fromUtf8(f_rowKey.data()) );
        du.set_display_len( 24 );
        return du.get_column_name( column_name );
    }

    return QueryModel::data( idx, role );
}


#if 0
bool RowModel::setData( const QModelIndex & idx, const QVariant & value, int role )
{
    if( role != Qt::EditRole )
    {
        return false;
    }

    try
    {
        {
            const QString q_str(
                    QString("SELECT value FROM %1.%2 WHERE key = ? AND column1 = ?")
                    .arg(f_rowModel.keyspaceName())
                    .arg(f_rowModel.tableName())
                    );
            QCassandraQuery q( f_session );
            q.query( q_str, 2 );
            q.bindByteArray( 0, f_rowModel.rowKey() );
            q.bindByteArray( 1, f_rowModel.data(index, Qt::UserRole ).toByteArray() );
            q.start();
            QByteArray value( q.getByteArrayColumn(0) );
            q.end():

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
        }

        {
            QCassandraQuery q( f_session );
            q.query(
                    //QString("UPDATE %1.%2 SET key = ?, column1 = ? WHERE key = ?")
                    QString("INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)")
                    .arg(f_keyspaceName)
                    .arg(f_tableName)
                    , 3
                   );
            q.bindByteArray( 0, f_rowKey   );
            q.bindByteArray( 1, save_value );
            q.start();
            q.end();
        }

        Q_EMIT dataChanged( idx, idx );

        return true;
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot write data to database.") );
        return false;
    }

    return false;
}


bool RowModel::insertRows ( int row, int count, const QModelIndex & parent_index )
{
    try
    {
        beginInsertRows( parent_index, row, row+count );
        for( int i = 0; i < count; ++i )
        {
            const QByteArray newcol( QString("New column %1").arg(i).toUtf8() );
            f_rows.insert( f_rows.begin() + (row+i), newcol);

            // TODO: this might be pretty slow. I need to utilize the "prepared query" API.
            //
            QCassandraQuery q( f_session );
            q.query(
                        QString("INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)")
                        .arg(f_keyspaceName)
                        .arg(f_tableName)
                        , 3
                        );
            q.bindByteArray( 0, f_rowKey    );
            q.bindByteArray( 1, newcol      );
            q.bindByteArray( 2, "New Value" );
            q.start();
            q.end();
        }
        endInsertRows();
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot insert new rows!") );
        return false;
    }

    return true;
}
#endif

bool RowModel::removeRows( int row, int count, const QModelIndex & )
{
    // Make a list of the keys we will drop
    //
    QList<QByteArray> key_list;
    for( int idx = 0; idx < count; ++idx )
    {
        const QByteArray key(f_rows[idx + row]);
        key_list << key;
    }

    try
    {
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
        f_rows.erase( f_rows.begin()+row, f_rows.begin()+row+count );
        endRemoveRows();
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot write data to database.") );
        return false;
    }

    return true;
}


// vim: ts=4 sw=4 et
