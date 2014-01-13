#include "MainWindow.h"
#include "SettingsDialog.h"

MainWindow::MainWindow(QWidget *p)
    : QMainWindow(p)
{
    setupUi(this);

    SettingsDialog dlg;
    dlg.exec();
}

MainWindow::~MainWindow()
{
}
