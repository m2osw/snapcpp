#include "SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget *p)
    : QDialog(p)
{
    setupUi( this );
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::on_lineEdit_editingFinished()
{
    // TODO: Add code which refreshes the context list.
}
