#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#include <QtCassandra/QCassandra.h>
#include <QtGui>
#pragma GCC diagnostic pop

#include <memory>

class CassandraModel
	: public QAbstractListModel
{
	Q_OBJECT

	public:
		CassandraModel() {}

        void setCassandra( QtCassandra::QCassandra::pointer_t c );

		Qt::ItemFlags	flags 		( const QModelIndex & index ) const;
		QVariant		data 		( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		QVariant		headerData	( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
		int 			rowCount   	( const QModelIndex & parent = QModelIndex() ) const;

	private:
        QtCassandra::QCassandra::pointer_t	f_cassandra;
};


