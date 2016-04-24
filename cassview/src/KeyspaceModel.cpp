#include "KeyspaceModel.h"

#include <iostream>

using namespace QtCassandra;
using namespace QCassandraSchema;


void KeyspaceModel::setTableNames( const string_list_t& list )
{
	f_tableNames = list;
    reset();
}


Qt::ItemFlags KeyspaceModel::flags( const QModelIndex & /*idx*/ ) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant KeyspaceModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_meta )
    {
        return QVariant();
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    try
    {
        return f_tableNames[idx.row()];
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return QVariant();
}


QVariant KeyspaceModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
	// TODO
	return "Row Name";
}


int KeyspaceModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    if( !f_meta )
    {
        return 0;
    }

    try
    {
        return static_cast<int>(f_tableNames.size());
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return 0;
}


