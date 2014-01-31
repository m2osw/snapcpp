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

    f_table->clearCache();
    f_rowsRemaining = f_table->readRows( f_rowp );

    const int itemsToFetch( qMin(f_rowCount, f_rowsRemaining) );

    beginInsertRows( QModelIndex(), f_pos, f_pos+itemsToFetch-1 );
    endInsertRows();

    f_pos += itemsToFetch;
}


Qt::ItemFlags TableModel::flags( const QModelIndex & /*idx*/ ) const
{
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable ; //| Qt::ItemIsEditable;
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

#if 0
    const auto& row( *(rows.begin() + section) );
    Q_ASSERT(row);

    return row->rowName();
#endif
	return QVariant();
}


QVariant TableModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_table )
    {
        return QVariant();
    }

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

    const QCassandraRows& rows = f_table->rows();
    return rows.size();
}


#if 0
int TableModel::columnCount( const QModelIndex &/*prnt*/ ) const
{
    if( !f_table )
    {
        return 0;
    }

#if 0
    if( !prnt.isValid() )
    {
        std::cout << "prnt is not valid!" << std::endl;
        return 1;
    }
#endif

    const auto& rows( f_table->rows() );
    if( rows.size() == 0 )
    {
        return 0;
    }

    const auto& row( *(rows.begin()) );
    Q_ASSERT(row);
    return row->cells().size();
}


QModelIndex TableModel::index( int row, int column, const QModelIndex &prnt ) const
{
    //const auto& the_row(( f_table->rows().begin()+row ).value());
    //
    if( prnt.isValid() )
    {
        //const auto& the_column(( the_row->cells().begin()+column  ).value());
        return createIndex( row, column, 1 ); //the_column.get() );
    }

    return createIndex( row, column, 0 ); //the_row.get() );
}


QModelIndex TableModel::parent( const QModelIndex &idx ) const
{
	if( !idx.isValid() )
	{
		return QModelIndex();
	}

    if( idx.internalId() == 0 )
	{
        return QModelIndex();
	}

    //QCassandraCell* cell( static_cast<QCassandraCell*>(idx.internalPointer()) );
    //const auto& the_row    (( f_table->rows() .begin()+row     ).value());
    //const auto& the_column (( the_row->cells().begin()+column  ).value());
    return createIndex( idx.row(), 0, 0 );
}


bool TableModel::hasChildren( const QModelIndex & prnt ) const
{
    if( !prnt.isValid() )
    {
        return true;
    }

    return prnt.internalId() == 0;
}
#endif


// vim: ts=4 sw=4 noet
