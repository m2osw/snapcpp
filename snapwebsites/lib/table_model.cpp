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

#include "table_model.h"
#include "dbutils.h"
#include "log.h"
#include "not_used.h"

#include <QtCassandra/QCassandraContext.h>
#include <QVariant>

#include <iostream>
#include <exception>

#include "poison.h"



namespace snap
{

QtCassandra::QCassandraTable::pointer_t table_model::getTable() const
{
    return f_table;
}


void table_model::setTable( QtCassandra::QCassandraTable::pointer_t t, QRegExp const & re )
{
    f_table = t;

    if( f_table )
    {
        // add a filter and add the start/end column names (see snapdb with '%')
        // this would be important for some rows that have a very large number of columns
        QtCassandra::QCassandraColumnRangePredicate::pointer_t columnp( new QtCassandra::QCassandraColumnRangePredicate );
        columnp->setCount(f_rowCount); // TODO: define a column count too

        // we cannot use the start and end row names to filter the rows
        // because they are not in order in the tables (by default Cassandra
        // only sorts columns)
        //
        // so instead we use the match feature which requires a regex
        //
        f_rowp.setStartRowName("");
        f_rowp.setEndRowName("");
        f_rowp.setCount(f_rowCount); // 1000 is the default for now
        f_rowp.setColumnPredicate(columnp);
        f_rowp.setRowNameMatch(re);
        f_rowsRemaining = f_table->readRows( f_rowp );
        f_pos = 0;
    }

    reset();
}


bool table_model::canFetchMore(QModelIndex const & model_index) const
{
    NOTUSED(model_index);

    return f_rowsRemaining >= f_rowCount;
}


void table_model::fetchMore(QModelIndex const & model_index)
{
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


Qt::ItemFlags table_model::flags( QModelIndex const & idx ) const
{
    NOTUSED(idx);
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant table_model::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( role != Qt::DisplayRole )
    {
        return QVariant();
    }

    if( !f_table )
    {
        return QVariant();
    }

    try
    {
        // the rows array is used immediately, so we can keep a temporary
        // reference here
        auto const & rows( f_table->rows() );
        if( rows.size() <= section )
        {
            return QVariant();
        }

        if( orientation == Qt::Horizontal )
        {
            auto const & row( *(rows.begin()) );
            Q_ASSERT(row);

            auto const & cell( *(row->cells().begin() + section) );
            return cell->columnName();
        }
    }
    catch( std::exception const & x )
    {
        SNAP_LOG_ERROR() << "Exception caught! [" << x.what() << "]";
    }

    return QVariant();
}


QVariant table_model::data( QModelIndex const & idx, int role ) const
{
    if( !f_table )
    {
        return QVariant();
    }

    try
    {
        if( role == Qt::DisplayRole || role == Qt::EditRole )
        {
            QtCassandra::QCassandraContext::pointer_t context( f_table->parentContext() );
            auto const & rows = f_table->rows();
            if( rows.size() <= idx.row() )
            {
                return QVariant();
            }
            auto const row( (rows.begin() + idx.row()).value() );
            QString ret_name;
            if( context->contextName() == "snap_websites" )
            {
                snap::dbutils du( f_table->tableName(), "" );
                ret_name = du.get_row_name( row );
            }
            else
            {
                ret_name = row->rowName();
            }
            //
            return ret_name;
        }

        if( role == Qt::UserRole )
        {
            auto const & rows = f_table->rows();
            if( rows.size() <= idx.row() )
            {
                return QVariant();
            }
            auto const row( (rows.begin() + idx.row()).value() );
            return row->rowKey();
        }
    }
    catch( std::exception const & x )
    {
        SNAP_LOG_ERROR() << "Exception caught! [" << x.what() << "]";
    }

    return QVariant();
}


int table_model::rowCount( QModelIndex const & prnt ) const
{
    if( !f_table )
    {
        return 0;
    }

    if( prnt.isValid() )
    {
        return 1;
    }

    try
    {
        auto const & rows = f_table->rows();
        return rows.size();
    }
    catch( std::exception const & x )
    {
        SNAP_LOG_ERROR() << "Exception caught! [" << x.what() << "]";
    }

    return 0;
}

}
// namespace snap


// vim: ts=4 sw=4 et
