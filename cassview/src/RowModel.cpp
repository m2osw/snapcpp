#include "RowModel.h"
#include <snapwebsites/dbutils.h>
#include <QtCassandra/QCassandraContext.h>
#include <iostream>

using namespace QtCassandra;


void RowModel::setRow( QCassandraRow::pointer_t c )
{
    f_row = c;
    reset();
}


Qt::ItemFlags RowModel::flags( const QModelIndex & idx ) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if( idx.column() == 1 )
    {
        f |= Qt::ItemIsEditable;
    }
    return f;
}


QVariant RowModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_row )
    {
        return QVariant();
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    if( idx.column() < 0 || idx.column() > 1 )
    {
        Q_ASSERT(false);
        return QVariant();
    }

    try
    {
        QCassandraContext::pointer_t context( f_row->parentTable()->parentContext() );
        const QCassandraCells& cell_list = f_row->cells();
        const auto cell( (cell_list.begin()+idx.row()).value() );

        if( context->contextName() == "snap_websites" )
        {
            snap::dbutils du( f_row->parentTable()->tableName(), f_row->rowName() );
            switch( idx.column() )
            {
                case 0: return du.get_column_name ( cell );
                case 1: return du.get_column_value( cell, role == Qt::DisplayRole /*display_only*/ );
            }

            Q_ASSERT(false);
            return QVariant();
        }

        const auto value( idx.column() == 0? cell->columnName(): cell->value() );
        return value.stringValue();
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return QVariant();
}


QVariant RowModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( role != Qt::DisplayRole || orientation != Qt::Horizontal )
    {
        return QVariant();
    }

    switch( section )
    {
        case 0: return tr("Name");
        case 1: return tr("Value");
    }

    return QVariant();
}


int RowModel::rowCount( const QModelIndex & /*parent*/ ) const
{
    if( !f_row )
    {
        return 0;
    }

    try
    {
        const QCassandraCells& cell_list = f_row->cells();
        return cell_list.size();
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return 0;
}


int RowModel::columnCount( const QModelIndex & /*parent*/ ) const
{
    return 2;
}


bool RowModel::setData( const QModelIndex & idx, const QVariant & value, int role )
{
    if( !f_row )
    {
        return false;
    }

    if( role != Qt::EditRole )
    {
        return false;
    }

    try
    {
        QCassandraContext::pointer_t context( f_row->parentTable()->parentContext() );
        const QCassandraCells& cell_list = f_row->cells();
        const auto cell( (cell_list.begin()+idx.row()).value() );

        if( context->contextName() == "snap_websites" )
        {
            snap::dbutils du( f_row->parentTable()->tableName(), f_row->rowName() );
            du.set_column_value( cell, value.toString() );
        }
        else
        {
            QCassandraValue v;
            v.setStringValue( value.toString() );
            cell->setValue( v );
        }

        Q_EMIT dataChanged( idx, idx );

        return true;
    }
    catch( const std::exception& x )
    {
        std::cerr << "Exception caught! [" << x.what() << "]" << std::endl;
    }

    return false;
}


bool RowModel::setHeaderData( int /*section*/, Qt::Orientation /*orientation*/, const QVariant & /*value*/, int /*role*/ )
{
    return false;
}


// vim: ts=4 sw=4 et syntax=cpp.doxygen
