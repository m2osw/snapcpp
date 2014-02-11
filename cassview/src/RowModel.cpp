#include "RowModel.h"
#include "InputDialog.h"
#include <snapwebsites/dbutils.h>
#include <snapwebsites/qstring_stream.h>
#include <QtCassandra/QCassandra.h>
#include <QtCassandra/QCassandraContext.h>
#include <QMessageBox>
#include <QSettings>
#include <iostream>
#include <sstream>

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


namespace
{
    void displayError( const std::exception& except )
    {
        std::stringstream ss;
        ss << QObject::tr("QtCassandra exception caught!") << "\n[" << except.what() << "]";
        std::cerr << ss.str() << std::endl;
        QMessageBox::critical( QApplication::activeWindow()
                , QObject::tr("Error")
                , ss.str().c_str()
                );
    }
}


QVariant RowModel::data( const QModelIndex & idx, int role ) const
{
    if( !f_row )
    {
        return QVariant();
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::UserRole )
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
        const QCassandraCells& cell_list = f_row->cells();
        const auto cell( (cell_list.begin()+idx.row()).value() );

        if( role == Qt::UserRole )
        {
            return cell->columnKey();
        }

        QCassandraContext::pointer_t context( f_row->parentTable()->parentContext() );
        if( context->contextName() == "snap_websites" )
        {
            snap::dbutils du( f_row->parentTable()->tableName(), f_row->rowName() );
            du.set_display_len( 24 );
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
    catch( const std::exception& except )
    {
        displayError( except );
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
    catch( const std::exception& except )
    {
        displayError( except );
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
        QByteArray key( data( idx, Qt::UserRole ).toByteArray() );
        const auto cell( f_row->findCell(key) );

        QCassandraContext::pointer_t context( f_row->parentTable()->parentContext() );
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
    catch( const std::exception& except )
    {
        displayError( except );
    }

    return false;
}


bool RowModel::setHeaderData( int /*section*/, Qt::Orientation /*orientation*/, const QVariant & /*value*/, int /*role*/ )
{
    return false;
}


bool RowModel::insertRows ( int /*row*/, int /*count*/, const QModelIndex & parent_index )
{
    bool retval( true );
    try
    {
        QSettings settings;
        const QString edit_key( "InputDialog/EditValue" );
        //
        InputDialog dlg;
        dlg.f_inputLabel->setText( tr("Enter Column Name:") );
        dlg.f_inputEdit->setText( settings.value( edit_key, tr("New Column") ).toString() );
        dlg.f_inputEdit->selectAll();
        //
        if( dlg.exec() == QDialog::Accepted )
        {
            settings.setValue( edit_key, dlg.f_inputEdit->text() );

            beginInsertRows( parent_index, rowCount(), 1 );
            const QString col_name( dlg.f_inputEdit->text() );
            auto key( (*f_row)[col_name].columnKey() );
            auto cell( f_row->findCell( key ) );
            cell->setTimestamp( QCassandraValue::TIMESTAMP_MODE_AUTO );

            QCassandraContext::pointer_t context( f_row->parentTable()->parentContext() );
            Q_ASSERT(context);
            if( context->contextName() == "snap_websites" )
            {
                snap::dbutils du( f_row->parentTable()->tableName(), f_row->rowName() );
                du.set_column_value( cell, "New Value" );
            }
            else
            {
                QCassandraValue v;
                v.setStringValue( "New Value" );
                cell->setValue( v );
            }
            endInsertRows();
            reset();
        }
    }
    catch( const std::exception& except )
    {
        endInsertRows();
        displayError( except );
        retval = false;
    }
    return retval;
}


bool RowModel::removeRows ( int row, int count, const QModelIndex & )
{
    QMessageBox::StandardButton result
            = QMessageBox::warning( QApplication::activeWindow()
            , tr("Warning!")
            , tr("Warning!\nYou are about to remove %1 columns from row '%2', in table '%3'.\nThis cannot be undone!")
              .arg(count)
              .arg(f_row->rowName())
              .arg(f_row->parentTable()->tableName())
            , QMessageBox::Ok | QMessageBox::Cancel
            );

    if( result != QMessageBox::Ok )
    {
        return false;
    }

    try
    {
        // Make a list of the keys we will drop
        //
        QList<QByteArray> key_list;
        for( int idx = 0; idx < count; ++idx )
        {
            const QByteArray key( data( index(idx + row, 0), Qt::UserRole ).toByteArray() );
            key_list << key;
        }

        // Drop each key
        //
        for( auto key : key_list )
        {
            f_row->dropCell( key );
        }

        reset();

        return true;
    }
    catch( const std::exception& except )
    {
        displayError( except );
    }

    return false;
}


// vim: ts=4 sw=4 et syntax=cpp.doxygen
