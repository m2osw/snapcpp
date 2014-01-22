#pragma once

#include <QtCassandra/QCassandraContext.h>
#include <QtGui>

#include <memory>

class ContextModel
	: public QAbstractListModel
{
	Q_OBJECT

	public:
		ContextModel() {}

        void setContext( QtCassandra::QCassandraContext::pointer_t c );

		Qt::ItemFlags	flags 		( const QModelIndex & index ) const;
		QVariant		data 		( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		QVariant		headerData	( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
		int 			rowCount   	( const QModelIndex & parent = QModelIndex() ) const;

	private:
        QtCassandra::QCassandraContext::pointer_t	f_context;
};


