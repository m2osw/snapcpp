// Snap Websites Servers -- generate HTML from the output of an XML Query
// Copyright (C) 2011-2012  Made to Order Software Corp.
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
#ifndef _QHTMLSERIALIZER_H
#define _QHTMLSERIALIZER_H
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QAbstractXmlReceiver>
#include <QXmlNamePool>
#include <QBuffer>
#include <QVector>
#pragma GCC diagnostic pop

class QHtmlSerializer : public QAbstractXmlReceiver
{
public:
                    QHtmlSerializer(QXmlNamePool namepool, QBuffer *output);
    virtual         ~QHtmlSerializer();
    virtual void    atomicValue(const QVariant& value);
    virtual void    attribute(const QXmlName& name, const QStringRef& value);
    virtual void    characters(const QStringRef& value);
    virtual void    comment(const QString& value);
    virtual void    endDocument();
    virtual void    endElement();
    virtual void    endOfSequence();
    virtual void    namespaceBinding(const QXmlName& name);
    virtual void    processingInstruction(const QXmlName& target, const QString& value );
    virtual void    startDocument();
    virtual void    startElement(const QXmlName& name);
    virtual void    startOfSequence();

private:
    enum html_serializer_status_t {
        HTML_SERIALIZER_STATUS_READY,
        HTML_SERIALIZER_STATUS_ELEMENT_OPEN
    };

    void            closeElement();

    QXmlNamePool                f_namepool;
    QBuffer *                   f_output;
    html_serializer_status_t    f_status;
    QVector<QString>            f_element_stack;
};

#endif
// _QHTMLSERIALIZER_H
// vim: ts=4 sw=4 et
