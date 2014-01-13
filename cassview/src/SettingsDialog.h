#pragma once

#include <QtGui>

#include "ui_SettingsDialog.h"

class SettingsDialog
        : public QDialog
        , Ui::SettingsDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);
    ~SettingsDialog();

protected:
    //void closeEvent( QCloseEvent * e );

private slots:
    void on_lineEdit_editingFinished();

private:
    QStringListModel	f_model;
    QVariant			f_server;
    QVariant			f_port;

    void updateContextList();
};

