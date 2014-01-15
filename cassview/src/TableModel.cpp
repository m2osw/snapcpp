#include "TableModel.h"


void TableModel::setTable( QSharedPointer<QCassandraTable> t )
{
	f_table = t;
}


Qt::ItemFlags TableModel::flags( const QModelIndex & index ) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant TableModel::data( const QModelIndex & index, int role = Qt::DisplayRole ) const
{
    QCassandraRowPredicate rowp;
    f_table->readRows( rowp );
    const QCassandraRows& rows = f_table->rows();
	return rows[index->row()];
}


QVariant TableModel::headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const
{
}


int TableModel::rowCount( const QModelIndex & parent = QModelIndex() ) const
{
}



		//QSharedPointer<QCassandraTable>	f_table;


