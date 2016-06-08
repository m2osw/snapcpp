#pragma once

#include <QDialog>
#include "ui_AboutDialog.h"

class AboutDialog
        : public QDialog
        , public Ui::AboutDialog
{
    Q_OBJECT

public:
    explicit AboutDialog( QWidget *p = 0);
    ~AboutDialog();

private:
};

