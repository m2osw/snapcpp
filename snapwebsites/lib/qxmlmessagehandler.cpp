// Snap Websites Servers -- handle messages for QXmlQuery
// Copyright (C) 2014  Made to Order Software Corp.
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

#include <qxmlmessagehandler.h>

#include <qstring_stream.h>
#include <log.h>

#include <QDomDocument>
#include <QFile>

#include <iostream>

#include "poison.h"

namespace snap
{


QMessageHandler::QMessageHandler(QObject *parent_object)
    : QAbstractMessageHandler(parent_object)
{
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void QMessageHandler::handleMessage(QtMsgType type, QString const & description, QUrl const & identifier, QSourceLocation const & sourceLocation)
{
    QDomDocument doc("description");
    doc.setContent(description, true, nullptr, nullptr, nullptr);
    QDomElement root(doc.documentElement());
    // TODO: note that the description may include <span>, <b>, <i>, ...
    QString const description_string(root.text());

//std::cerr << "URI A = [" << identifier.toString() << "]\n";
//std::cerr << "URI B = [" << sourceLocation.uri().toString() << "]\n";
//std::cerr << "MESSAGE = [" << description_string << "]\n";

    // avoid "variable unused" warnings
    if(type != QtWarningMsg
    || !description_string.startsWith("The variable")
    || !description_string.endsWith("is unused"))
    {
        char const *type_msg(nullptr);
        logging::log_level_t level(logging::LOG_LEVEL_OFF);
        switch(type)
        {
        case QtDebugMsg:
            type_msg = "Debug";
            level = logging::LOG_LEVEL_DEBUG;
            break;

        case QtWarningMsg:
            type_msg = "Warning";
            level = logging::LOG_LEVEL_WARNING;
            break;

        case QtCriticalMsg:
            type_msg = "Critical";
            level = logging::LOG_LEVEL_ERROR;
            break;

        //case QtFatalMsg:
        default:
            type_msg = "Fatal";
            level = logging::LOG_LEVEL_FATAL;
            break;

        }

        {
            logging::logger l(level, __FILE__, __func__, __LINE__);
            l.operator () (type_msg)(":");
            QString const location(sourceLocation.uri().toString());
            if(!location.isEmpty())
            {
                l.operator () (location)(":");
            }
            if(sourceLocation.line() != 0)
            {
                l.operator () ("line #")(sourceLocation.line())(":");
            }
            if(sourceLocation.column() != 0)
            {
                l.operator () ("column #")(sourceLocation.column())(":");
            }
            l.operator () (" ")(description_string);
            if(!f_xsl.isEmpty())
            {
#ifdef DEBUG
                l.operator () (" Script:\n")(f_xsl);
                static int count(0);
                QFile file_xsl(QString("/tmp/error%1-query.xsl").arg(count));
                file_xsl.open(QIODevice::WriteOnly);
                file_xsl.write(f_xsl.toUtf8());
                file_xsl.close();
                QFile file_xml(QString("/tmp/error%1-document.xml").arg(count));
                file_xml.open(QIODevice::WriteOnly);
                file_xml.write(f_doc.toUtf8());
                file_xml.close();
                ++count;
#else
                l.operator () (" Beginning of the script involved:\n")(f_xsl.left(200));
#endif
            }
        } // print log
    }
}
#pragma GCC diagnostic pop


} // namespace snap
// vim: ts=4 sw=4 et
