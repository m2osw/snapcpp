#pragma once

#include <QtCassandra/QCassandra.h>
#include <QtGui>

#include <memory>

class TableModel
	: public QAbstractListModel
{
	Q_OBJECT

	public:
		TableModel() {}

		void setTable( QSharedPointer<QCassandraTable> t );

		Qt::ItemFlags	flags 		( const QModelIndex & index ) const;
		QVariant		data 		( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		QVariant		headerData	( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
		int 			rowCount   	( const QModelIndex & parent = QModelIndex() ) const;

	private:
		QSharedPointer<QCassandraTable>	f_table;
};


