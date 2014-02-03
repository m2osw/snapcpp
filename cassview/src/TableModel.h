#pragma once

#include <QtCassandra/QCassandraTable.h>
#include <QtCassandra/QCassandraRowPredicate.h>
#include <QtGui>

#include <memory>

class TableModel
    : public QAbstractListModel
{
    Q_OBJECT

    public:
        TableModel() : f_rowCount(100), f_rowsRemaining(0), f_pos(0) {}

        QtCassandra::QCassandraTable::pointer_t getTable() const;
        void setTable( QtCassandra::QCassandraTable::pointer_t t );

        // Read only access
        //
        Qt::ItemFlags   flags       ( const QModelIndex & index ) const;
        QVariant        data        ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
        QVariant        headerData  ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        int             rowCount    ( const QModelIndex & parent = QModelIndex() ) const;

#if 0
        // Required for items with columns.
        //
        int         columnCount( const QModelIndex & parent = QModelIndex() ) const;
        QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
        QModelIndex parent ( const QModelIndex & index ) const;
        bool            hasChildren ( const QModelIndex & parent = QModelIndex() ) const;
#endif

        // Fecth more
        bool canFetchMore ( const QModelIndex & /* index */ ) const;
        void fetchMore    ( const QModelIndex & /* index */ );

    private:
        QtCassandra::QCassandraTable::pointer_t f_table;
        QtCassandra::QCassandraRowPredicate     f_rowp;
        int f_rowCount;
        int f_rowsRemaining;
        int f_pos;
};

// vim: ts=4 sw=4 et
