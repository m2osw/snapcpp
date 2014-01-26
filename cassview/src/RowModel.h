#pragma once

#include <QtCassandra/QCassandraRow.h>
#include <QtGui>

#include <memory>

class RowModel
	: public QAbstractListModel
{
	Q_OBJECT

	public:
		RowModel() {}

        void setRow( QtCassandra::QCassandraRow::pointer_t c );

		Qt::ItemFlags	flags 		( const QModelIndex & index ) const;
		QVariant		data 		( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		QVariant		headerData	( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
		int 			rowCount   	( const QModelIndex & parent = QModelIndex() ) const;

	private:
        QtCassandra::QCassandraRow::pointer_t	f_row;
};


