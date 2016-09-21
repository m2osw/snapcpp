#pragma once

#include <QtGui>

#include "ui_SettingsDialog.h"

class SettingsDialog
        : public QDialog
        , Ui::SettingsDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog( QWidget *p = nullptr, const bool first_time = false );
    ~SettingsDialog();

    static bool tryConnection( QWidget* p = nullptr );

protected:
    void closeEvent( QCloseEvent * e );

private slots:
    void on_f_buttonBox_accepted();
    void on_f_buttonBox_rejected();
    void on_f_hostnameEdit_textEdited(const QString &arg1);
    void on_f_portEdit_valueChanged(int arg1);
    void on_f_useSslCB_toggled(bool checked);
    void on_f_promptCB_toggled(bool checked);
    void on_f_contextEdit_textChanged(const QString &arg1);

private:
    QVariant	f_server;
    QVariant	f_port;
    QVariant    f_useSSL;
    QVariant    f_promptBeforeSave;
    QVariant    f_contextName;
};

