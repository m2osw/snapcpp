#include "AboutDialog.h"
#include <QCoreApplication>
#include <QMessageBox>

AboutDialog::AboutDialog(QWidget *p)
    : QDialog(p)
{
    setupUi(this);

    textBrowser->setHtml(
        QCoreApplication::translate
        (
            "AboutDialog"
            , "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
             "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
             "p, li { white-space: pre-wrap; }\n"
             "</style></head><body style=\" font-family:'Ubuntu'; font-size:11pt; font-weight:400; font-style:normal;\">\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:22pt; font-weight:600;\">CassView</span></p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-style:italic;\">v"
             CASSVIEW_VERSION
             "</span></p>\n"
             "<p align=\"center\" style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><br /></p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:"
             "0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">To view your</span></p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Cassandra Cluster</p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Optimized for</p>\n"
             "<p align=\"center\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Snap! Server<br /><br />Made to Order Software Corporation</span></p></body></html>"
            , 0
            //, QApplication::UnicodeUTF8
        )
    );
}

AboutDialog::~AboutDialog()
{
}

// vim: ts=4 sw=4 et
