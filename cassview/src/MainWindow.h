#pragma once

#include "ui_MainWindow.h"

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
    void on_action_Settings_triggered();
    void OnAboutToQuit();

    void on_f_tables_clicked(const QModelIndex &index);

private:
    QtCassandra::QCassandra  	f_cassandra;
    QStringListModel			f_tableModel;
    QStringListModel			f_rowModel;

    void			FillTableList();
};

