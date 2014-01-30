#include "RowModel.h"
#include <snapwebsites/dbutils.h>
#include <QtCassandra/QCassandraContext.h>

using namespace QtCassandra;


void RowModel::setRow( QCassandraRow::pointer_t c )
{
	f_row = c;
    reset();
}


Qt::ItemFlags RowModel::flags( const QModelIndex & /*idx*/ ) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant RowModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_row )
    {
        return QVariant();
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    QCassandraContext::pointer_t context( f_row->parentTable()->parentContext() );
    const QCassandraCells& cell_list = f_row->cells();
    const auto cell( (cell_list.begin()+idx.row()).value() );

    if( context->contextName() == "snap_websites" )
    {
        snap::dbutils du( f_row->parentTable()->tableName(), f_row->rowName() );
        return du.get_column_value( cell );
    }

    const auto value( cell->value() );
    return value.stringValue();
}


QVariant RowModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
	// TODO
	return "Cell Data";
}


int RowModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    if( !f_row )
    {
        return 0;
    }

	const QCassandraCells& cell_list = f_row->cells();
    return cell_list.size();
}


