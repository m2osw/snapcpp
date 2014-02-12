#pragma once

#include "ui_MainWindow.h"
#include "CassandraModel.h"
#include "ContextModel.h"

#include <snapwebsites/table_model.h>
#include <snapwebsites/row_model.h>

#include <QtCassandra/QCassandra.h>
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
    void onShowContextMenu( const QPoint& pos );
    void onCellsModelReset();
    void on_action_Settings_triggered();
    void onAboutToQuit();
    void on_f_tables_currentIndexChanged(const QString &table_name);
    void on_f_contextCombo_currentIndexChanged(const QString &arg1);
    void onRowsCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ );
    void on_action_About_triggered();
    void on_action_AboutQt_triggered();
    void onSectionClicked( int section );
    void on_action_InsertColumn_triggered();
    void on_action_DeleteColumns_triggered();

private:
    typedef QtCassandra::QCassandra::pointer_t cassandra_t;
    cassandra_t       f_cassandra;
    CassandraModel    f_cassandraModel;
    ContextModel      f_contextModel;
    snap::table_model f_tableModel;
    snap::row_model   f_rowModel;
    QString           f_context;

    void        connectCassandra();
    void        fillTableList();
    void        changeRow(const QModelIndex &index);
};


// vim: ts=4 sw=4 et syntax=cpp.doxygen
