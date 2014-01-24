#include "RowModel.h"

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

    const QCassandraCells& cell_list = f_row->cells();
    const QString row_name( (cell_list.begin()+idx.row()).value()->value().stringValue() ); //TODO: rules
    return row_name;
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


