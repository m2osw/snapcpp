#include "CassandraModel.h"

using namespace QtCassandra;


void CassandraModel::setCassandra( QSharedPointer<QCassandra> c )
{
	f_cassandra = c;
    reset();
}


Qt::ItemFlags CassandraModel::flags( const QModelIndex & /*idx*/ ) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant CassandraModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_cassandra )
    {
        return QVariant();
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    const QCassandraContexts& context_list = f_cassandra->contexts();
    const QString context_name( (context_list.begin()+idx.row()).value()->contextName() );
    return context_name;
}


QVariant CassandraModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
	// TODO
	return "Row Name";
}


int CassandraModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    if( !f_cassandra )
    {
        return 0;
    }

    const QCassandraContexts& context_list = f_cassandra->contexts();
    return context_list.size();
}


