#include "TableModel.h"

#include <snapwebsites/dbutils.h>
#include <QtCassandra/QCassandraContext.h>
#include <iostream>

using namespace QtCassandra;


QCassandraTable::pointer_t TableModel::getTable() const
{
	return f_table;
}


void TableModel::setTable( QCassandraTable::pointer_t t )
{
	f_table = t;

    if( f_table )
    {
        f_rowp.setStartRowName("");
        f_rowp.setEndRowName("");
        f_rowp.setCount(f_rowCount); // 100 is the default
        f_rowsRemaining = f_table->readRows( f_rowp );
        f_pos = 0;
    }

    reset();
}


bool TableModel::canFetchMore(const QModelIndex & /* index */) const
{
    return f_rowsRemaining >= f_rowCount;
}


void TableModel::fetchMore(const QModelIndex & /* index */)
{
    if( !f_table ) return;

    try
    {
        f_table->clearCache();
        f_rowsRemaining = f_table->readRows( f_rowp );

        const int itemsToFetch( qMin(f_rowCount, f_rowsRemaining) );

        beginInsertRows( QModelIndex(), f_pos, f_pos+itemsToFetch-1 );
        endInsertRows();

        f_pos += itemsToFetch;
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }
}


Qt::ItemFlags TableModel::flags( const QModelIndex & /*idx*/ ) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant TableModel::headerData( int section, Qt::Orientation orientation, int role ) const
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
        const auto& rows( f_table->rows() );
        if( rows.size() == 0 )
        {
            return QVariant();
        }

        if( orientation == Qt::Horizontal )
        {
            const auto& row( *(rows.begin()) );
            Q_ASSERT(row);

            const auto& cell( *(row->cells().begin() + section) );
            return cell->columnName();
        }
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

	return QVariant();
}


QVariant TableModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_table )
    {
        return QVariant();
    }

    try
    {
        if( role == Qt::DisplayRole || role == Qt::EditRole )
        {
            QCassandraContext::pointer_t context( f_table->parentContext() );
            const auto& rows = f_table->rows();
            const auto row( (rows.begin()+idx.row()).value() );
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
            const auto& rows = f_table->rows();
            const auto row( (rows.begin()+idx.row()).value() );
            return row->rowKey();
        }
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return QVariant();
}


int TableModel::rowCount( const QModelIndex &prnt ) const
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
        const QCassandraRows& rows = f_table->rows();
        return rows.size();
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return 0;
}


// vim: ts=4 sw=4 noet
