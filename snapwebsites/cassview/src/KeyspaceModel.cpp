#include "KeyspaceModel.h"

#include <iostream>

using namespace casswrapper;
using namespace casswrapper::schema;


void KeyspaceModel::reset()
{
    beginResetModel();
    endResetModel();
}

void KeyspaceModel::setCassandra( Session::pointer_t c, const QString& keyspace_name )
{
    auto sessionMeta = SessionMeta::create( c );
    sessionMeta->loadSchema();

    f_tableNames.clear();
    const auto& keyspaces( sessionMeta->getKeyspaces() );
    const auto& keyspace ( keyspaces.find(keyspace_name) );
    if( keyspace != keyspaces.end() )
    {
        for( const auto& pair : keyspace->second->getTables() )
        {
            f_tableNames.push_back( pair.first );
        }
    }

    reset();
}


Qt::ItemFlags KeyspaceModel::flags( const QModelIndex & /*idx*/ ) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant KeyspaceModel::data( const QModelIndex & idx, int role ) const
{
    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    return f_tableNames[idx.row()];
}


QVariant KeyspaceModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
	// TODO
	return "Row Name";
}


int KeyspaceModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    return static_cast<int>(f_tableNames.size());
}


