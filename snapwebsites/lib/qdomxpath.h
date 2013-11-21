// Snap Websites Servers -- generate a DOM from the output of an XML Query
// Copyright (C) 2013  Made to Order Software Corp.
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
#ifndef _QXMLXPATH_H
#define _QXMLXPATH_H
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QDomNode>
#pragma GCC diagnostic pop



class QDomXPathException : public std::runtime_error
{
public:
    QDomXPathException(const std::string& err)
        : runtime_error(err)
    {
    }
};

class QDomXPathException_InvalidCharacter : public QDomXPathException
{
public:
    QDomXPathException_InvalidCharacter(const std::string& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_InvalidString : public QDomXPathException
{
public:
    QDomXPathException_InvalidString(const std::string& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_TooManyUnget : public QDomXPathException
{
public:
    QDomXPathException_InvalidString(const std::string& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_SyntaxError : public QDomXPathException
{
public:
    QDomXPathException_SyntaxError(const std::string& err)
        : QDomXPathException(err)
    {
    }
};



class QDomXPath
{
public:
    bool                setXPath(const QString& xpath);
    QString             getXPath() const;

    QVector<QDomNode>   apply(QDomNode node) const;

private:
    class QDomXPathImpl;

    QString             f_xpath;
    QSharedPointer<QDomXPathImpl> f_impl;
};

#endif
// _QXMLXPATH_H
// vim: ts=4 sw=4 et
