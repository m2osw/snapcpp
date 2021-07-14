// Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snap-builder
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// self
//
#include    "about_dialog.h"

#include    "version.h"


// Qt lib
//
#include    <QCoreApplication>
#include    <QMessageBox>


// last include
//
#include    <snapdev/poison.h>



AboutDialog::AboutDialog(QWidget *p)
    : QDialog(p)
{
    setupUi(this);

    setStyleSheet("QTextBrowser{ min-width: 700px; min-height: 450px; }");

    textBrowser->setHtml(
        QCoreApplication::translate
        (
            "AboutDialog"
            , "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
             "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
             "p, li { white-space: pre-wrap; }\n"
             "p { text-align: center; margin: 0; -qt-block-indent:0; text-indent:0px; }\n"
             "</style></head><body style=\"font-family:'Ubuntu'; font-size:11pt; font-weight:400; font-style:normal;\">\n"
             "<p><span style=\"font-size:22pt; font-weight:600;\">Snap! Builder</span></p>\n"
             "<p><span style=\"font-style:italic;\">v" SNAPBUILDER_VERSION_STRING "</span></p>\n"
             "<p style=\"-qt-paragraph-type:empty;\"><br /></p>\n"
             "<p>Helper tool used to build the Snap! Websites packages.</p>\n"
             "<p style=\"-qt-paragraph-type:empty;\"><br /></p>\n"
             "<p><span style=\" font-weight:600;\">Snap! Builder<br />by<br />Made to Order Software Corporation<br />All Rights Reserved</span></p>"
             "<p style=\"-qt-paragraph-type:empty;\"><br /></p>\n"
             "<p><a href=\"/usr/share/common-licenses/GPL-2\">License GPL 2.0</a></p>"
             "</body></html>"
            , 0
            //, QApplication::UnicodeUTF8
        )
    );
}

AboutDialog::~AboutDialog()
{
}

// vim: ts=4 sw=4 et
