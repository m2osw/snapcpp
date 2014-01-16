#include "ContextModel.h"

using namespace QtCassandra;


void ContextModel::setContext( QSharedPointer<QCassandraContext> c )
{
	f_context = c;
    reset();
}


Qt::ItemFlags ContextModel::flags( const QModelIndex & /*idx*/ ) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}


QVariant ContextModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_context )
    {
        return QVariant();
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    const QCassandraTables& table_list = f_context->tables();
    const QString table_name( (table_list.begin()+idx.row()).value()->tableName() );
    return table_name;
}


QVariant ContextModel::headerData( int /*section*/, Qt::Orientation /*orientation*/, int /*role*/ ) const
{
	// TODO
	return "Row Name";
}


int ContextModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    if( !f_context )
    {
        return 0;
    }

    const QCassandraTables& row_list = f_context->tables();
    return row_list.size();
}


