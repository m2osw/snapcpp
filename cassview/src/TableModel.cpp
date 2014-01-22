#include "TableModel.h"

#include <iostream>

using namespace QtCassandra;

void TableModel::setTable( QCassandraTable::pointer_t t )
{
	f_table = t;

    QCassandraRowPredicate rowp;
    //rowp.setStartRowName("");
    //rowp.setEndRowName("");
    //rowp.setCount(10); // 100 is the default
    f_table->readRows( rowp );

    reset();
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

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    if( idx.parent().isValid() )
	{
		const auto& rows( f_table->rows() );
		const auto& row( *(rows.begin() + idx.row()) );
		Q_ASSERT(row);

		const auto cells( row->cells() );
		if( idx.column() < cells.size() )
		{
			const auto& iter( cells.begin() + idx.column() );
			Q_ASSERT(iter != cells.end());
			const auto& cell( *iter );
			return cell->value().stringValue();
		}

		return QVariant();
	}

	const auto& rows = f_table->rows();
	const QString row_name( (rows.begin()+idx.row()).value()->rowName() );
	return row_name;
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


int TableModel::columnCount( const QModelIndex &prnt ) const
{
    if( !f_table )
    {
        return 0;
    }

    if( !prnt.isValid() )
    {
        return 1;
    }

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
    const auto& the_row(( f_table->rows().begin()+row ).value());
    //
    if( prnt.isValid() )
    {
        const auto& the_column(( the_row->cells().begin()+column  ).value());
        return createIndex( row, column, the_column.get() );
    }

    return createIndex( row, 0, the_row.get() );
}


QModelIndex TableModel::parent( const QModelIndex &idx ) const
{
	if( !idx.isValid() )
	{
		return QModelIndex();
	}

    if( idx.internalPointer() == 0 )
	{
        return QModelIndex();
	}

	QCassandraCell* cell( static_cast<QCassandraCell*>(idx.internalPointer()) );
    return createIndex( idx.row(), 0, cell->parent()->data() );
}


// vim: ts=4 sw=4 noet
