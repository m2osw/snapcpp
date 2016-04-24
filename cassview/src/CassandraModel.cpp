#include "CassandraModel.h"

#include <iostream>

using namespace QtCassandra;
using namespace QCassandraSchema;

void CassandraModel::setCassandra( QCassandraSession::pointer_t c )
{
    f_sessionMeta = SessionMeta::create( c );
    f_sessionMeta->loadSchema();
    reset();
}


Qt::ItemFlags CassandraModel::flags( const QModelIndex & /*idx*/ ) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant CassandraModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_sessionMeta )
    {
        return QVariant();
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    try
    {
        const auto& keyspace_list( f_sessionMeta->getKeyspaces() );
        const QString& keyspace_name( (keyspace_list.begin()+idx.row())->first );
        return keyspace_name;
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return QVariant();
}


QVariant CassandraModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
	// TODO
	return "Row Name";
}


int CassandraModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    if( !f_sessionMeta )
    {
        return 0;
    }

    try
    {
        const auto& keyspace_list( f_sessionMeta->getKeyspaces() );
        return keyspace_list.size();
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return 0;
}


