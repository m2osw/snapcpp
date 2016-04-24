#pragma once

#include <QtGui>

#include <memory>
#include <vector>

class KeyspaceModel
	: public QAbstractListModel
{
	Q_OBJECT

	public:
		typedef std::vector<QString> string_list_t;

        KeyspaceModel() {}

        void setTableNames( const string_list_t& list );

		Qt::ItemFlags	flags 		( const QModelIndex & index ) const;
		QVariant		data 		( const QModelIndex & index, int role = Qt::DisplayRole ) const;
		QVariant		headerData	( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
		int 			rowCount   	( const QModelIndex & parent = QModelIndex() ) const;

	private:
		string_list_t	f_tableNames;
};


