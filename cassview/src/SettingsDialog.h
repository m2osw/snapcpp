#pragma once

#include <QDialog>

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
    void on_lineEdit_editingFinished();

private:
};

