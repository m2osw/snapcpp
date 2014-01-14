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

private slots:
    void on_f_cassandraServerEdit_lostFocus();
    void on_f_contextList_clicked(const QModelIndex &index);
    void on_SettingsDialog_accepted();

    void on_f_buttonBox_accepted();

    void on_f_buttonBox_rejected();

private:
    QStringListModel	f_model;
    QVariant			f_server;
    QVariant			f_port;
    QVariant			f_context;

    void updateContextList();
};

