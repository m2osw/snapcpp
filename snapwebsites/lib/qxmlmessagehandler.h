// Snap Websites Servers -- handle messages for QXmlQuery
// Copyright (C) 2014-2015  Made to Order Software Corp.
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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

#include <QAbstractMessageHandler>

namespace snap
{

class QMessageHandler : public QAbstractMessageHandler
{
public:
                        QMessageHandler(QObject * parent_object = nullptr);

    void                set_xsl(QString const & xsl) { f_xsl = xsl; }
    void                set_doc(QString const & doc) { f_doc = doc; }
    bool                had_msg() const { return f_had_msg; }

protected:
    virtual void        handleMessage(QtMsgType type, QString const & description, QUrl const & identifier, QSourceLocation const & sourceLocation);

private:
    QString             f_xsl;
    QString             f_doc;
    bool                f_had_msg;
};

} // namespace snap
// vim: ts=4 sw=4 et
