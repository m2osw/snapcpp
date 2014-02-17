#pragma once

#include <QDialog>

#include "ui_InputDialog.h"

class InputDialog
        : public QDialog
        , public Ui::InputDialog
{
    Q_OBJECT

public:
    explicit InputDialog(QWidget *prnt = 0);
    ~InputDialog();

private:
};

