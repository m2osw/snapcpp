//===============================================================================
// Copyright (c) 2011-2017 by Made to Order Software Corporation
// 
// http://snapwebsites.org/
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//===============================================================================

// ourselves
//
#include "TableModel.h"

// snapwebsites lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>

// Qt lib
//
#include <QSettings>
#include <QVariant>

// C++ lib
//
#include <iostream>
#include <exception>

// included last
//
#include <snapwebsites/poison.h>

using namespace casswrapper;


TableModel::TableModel(QObject *p, QSqlDatabase db)
    : QSqlTableModel( p, db )
{
}


QString TableModel::selectStatement() const
{
    f_dbutils = std::make_shared<snap::dbutils>( tableName(), "" );
    return QString("SELECT key, column1, value FROM %1").arg(tableName());
}


#if 0
void TableModel::doQuery()
{
    f_dbutils = std::make_shared<snap::dbutils>( f_tableName, "" );

    auto q = Query::create(f_session);
    q->query(
        QString("SELECT DISTINCT key FROM %1.%2")
            .arg(f_keyspaceName)
            .arg(f_tableName)
            );
    q->setPagingSize( 10 );

    query_model::doQuery( q );
}


bool TableModel::fetchFilter( const QByteArray& key )
{
    QString const row_name( f_dbutils->get_row_name( key ) );

    if( !f_filter.isEmpty() )
    {
        if( f_filter.indexIn( row_name ) == -1 )
        {
            return false;
        }
    }
    //
    return true;
}
#endif


QVariant TableModel::data( QModelIndex const & idx, int role ) const
{
    if( role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::UserRole )
    {
        return QVariant();
    }

    if( f_sortModel )
    {
        if( static_cast<int>(f_sortMap.size()) <= idx.row() )
        {
            return QVariant();
        }

        auto iter = f_sortMap.begin();
        for( int i = 0; i < idx.row(); ++i) iter++;
        if( role == Qt::UserRole )
        {
            return iter->second;
        }
        else
        {
            return iter->first;
        }
    }
    else
    {
        auto data_entry( QSqlTableModel::data( idx, role ) );
        if( role == Qt::UserRole )
        {
            return data_entry;
        }
        else
        {
            return f_dbutils->get_row_name( data_entry.toByteArray() );
        }
    }
}


#if 0
void TableModel::fetchCustomData( Query::pointer_t q )
{
    if( !f_sortModel )
    {
        return;
    }

    const QByteArray value(q->getByteArrayColumn(0));
    f_sortMap[f_dbutils->get_row_name(value)] = value;
}
#endif


// vim: ts=4 sw=4 et
