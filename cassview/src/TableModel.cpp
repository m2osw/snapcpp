#include "TableModel.h"

using namespace QtCassandra;

void TableModel::setTable( QSharedPointer<QCassandraTable> t )
{
	f_table = t;
    reset();

    QCassandraRowPredicate rowp;
    //rowp.setStartRowName("");
    //rowp.setEndRowName("");
    //rowp.setCount(10); // 100 is the default
    f_table->readRows( rowp );
}


Qt::ItemFlags TableModel::flags( const QModelIndex & /*idx*/ ) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
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

    const QCassandraRows& rows = f_table->rows();
    const QString row_name( (rows.begin()+idx.row()).value()->rowName() );
    return row_name;
}


QVariant TableModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
	// TODO
	return "Row Name";
}


int TableModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    if( !f_table )
    {
        return 0;
    }

    const QCassandraRows& rows = f_table->rows();
    return rows.size();
}


