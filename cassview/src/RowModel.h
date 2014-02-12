#pragma once

#include <QtCassandra/QCassandraRow.h>
#include <QtGui>

#include <memory>

class RowModel
    : public QAbstractTableModel
{
    Q_OBJECT

    public:
        RowModel() {}

        QtCassandra::QCassandraRow::pointer_t   getRow() const;
        void                                    setRow( QtCassandra::QCassandraRow::pointer_t c );

        Qt::ItemFlags   flags       ( const QModelIndex & index ) const;
        QVariant        data        ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        QVariant        headerData  ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        int             rowCount    ( const QModelIndex & parent = QModelIndex() ) const;
        int             columnCount ( const QModelIndex & parent = QModelIndex() ) const;

        // Write access
        //
        bool            setData       ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
        bool            setHeaderData ( int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole );

        // Resizable methods
        //
        bool insertRows ( int row, int count, const QModelIndex & parent = QModelIndex() );
        bool removeRows ( int row, int count, const QModelIndex & parent = QModelIndex() );

    private:
        QtCassandra::QCassandraRow::pointer_t   f_row;
};


// vim: ts=4 sw=4 et syntax=cpp.doxygen
