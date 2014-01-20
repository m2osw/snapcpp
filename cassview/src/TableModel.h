#pragma once

#include <QtCassandra/QCassandraTable.h>
#include <QtGui>

#include <memory>

class TableModel
    : public QAbstractItemModel
{
	Q_OBJECT

	public:
		TableModel() {}

        void setTable( QSharedPointer<QtCassandra::QCassandraTable> t );

        // Read only access
        //
		Qt::ItemFlags	flags 		( const QModelIndex & index ) const;
        QVariant		data 		( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        QVariant		headerData	( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        int 			rowCount   	( const QModelIndex & parent = QModelIndex() ) const;

        // Write access
        //
        //bool 		    setData     ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

        // Required for items with columns.
        //
        int         columnCount( const QModelIndex & parent = QModelIndex() ) const;
        QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
        QModelIndex parent ( const QModelIndex & index ) const;

	private:
        QSharedPointer<QtCassandra::QCassandraTable>	f_table;
};

// vim: ts=4 sw=4 noet
