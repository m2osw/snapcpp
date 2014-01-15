#pragma once

#include <QtCassandra/QCassandraTable.h>
#include <QtGui>

#include <memory>

class TableModel
	: public QAbstractListModel
{
	Q_OBJECT

	public:
		TableModel() {}

        void setTable( QSharedPointer<QtCassandra::QCassandraTable> t );

		Qt::ItemFlags	flags 		( const QModelIndex & index ) const;
		QVariant		data 		( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		QVariant		headerData	( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
		int 			rowCount   	( const QModelIndex & parent = QModelIndex() ) const;

	private:
        QSharedPointer<QtCassandra::QCassandraTable>	f_table;
};


