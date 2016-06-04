#pragma once

#include "ui_MainWindow.h"
#include "CassandraModel.h"
#include "KeyspaceModel.h"
#include "TableModel.h"
#include "RowModel.h"

#include <QtCassandra/QCassandraSession.h>
#include <QtGui>

class MainWindow
        : public QMainWindow
        , Ui::MainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onShowRowsContextMenu( const QPoint& pos );
    void onShowCellsContextMenu( const QPoint& pos );
    void onCellsModelReset();
    void on_action_Settings_triggered();
    void onAboutToQuit();
    void on_f_contextCombo_currentIndexChanged(const QString &arg1);
    void on_f_tables_currentIndexChanged(const QString &table_name);
    void onRowsCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ );
    void onCellsCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ );
    void on_action_About_triggered();
    void on_action_AboutQt_triggered();
    void onSectionClicked( int section );
    void on_action_InsertRow_triggered();
    void on_action_DeleteRows_triggered();
    void on_action_InsertColumn_triggered();
    void on_action_DeleteColumns_triggered();
    void on_f_connectionBtn_clicked();
    void on_f_applyFilter_clicked();
    void on_f_refreshView_clicked();
    void onExceptionCaught( const QString & what, const QString & message ) const;

private:
    typedef QtCassandra::QCassandraSession::pointer_t cassandra_t;
    cassandra_t       f_session;
    CassandraModel    f_cassandraModel;
    KeyspaceModel     f_contextModel;
    TableModel		  f_tableModel;
    RowModel		  f_rowModel;
    QString           f_context;

    void connectCassandra ();
    void fillTableList    ();
    void saveValue        ();
    void saveValue        ( const QModelIndex &index );
};


// vim: ts=4 sw=4 et
