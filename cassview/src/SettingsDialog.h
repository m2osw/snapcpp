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
    void closeEvent( QCloseEvent * e );

private slots:
    void on_f_buttonBox_accepted();
    void on_f_buttonBox_rejected();
    void on_f_hostnameEdit_textEdited(const QString &arg1);
    void on_f_portEdit_valueChanged(int arg1);

private:
    QVariant			f_server;
    QVariant			f_port;
};

