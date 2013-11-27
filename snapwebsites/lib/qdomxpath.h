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
#include <QString>
#include <QSharedPointer>
#include <QMap>
#pragma GCC diagnostic pop
#include <stdexcept>



class QDomXPathException : public std::runtime_error
{
public:
    QDomXPathException(const std::string& err)
        : runtime_error(err)
    {
    }
};

class QDomXPathException_InternalError : public QDomXPathException
{
public:
    QDomXPathException_InternalError(const std::string& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_UndefinedInstructionError : public QDomXPathException
{
public:
    QDomXPathException_UndefinedInstructionError(const std::string& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_InvalidError : public QDomXPathException
{
public:
    QDomXPathException_InvalidError(const std::string& err)
        : QDomXPathException(err)
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
    QDomXPathException_TooManyUnget(const std::string& err)
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

class QDomXPathException_ExecutionTime : public QDomXPathException
{
public:
    QDomXPathException_ExecutionTime(const std::string& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_NotImplemented : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_NotImplemented(const std::string& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};

class QDomXPathException_OutOfRange : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_OutOfRange(const std::string& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};

class QDomXPathException_EmptyStack : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_EmptyStack(const std::string& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};

class QDomXPathException_WrongType : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_WrongType(const std::string& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};

class QDomXPathException_UndefinedVariable : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_UndefinedVariable(const std::string& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};



class QDomXPath
{
public:
    typedef QVector<QDomNode>       node_vector_t;
    typedef QMap<QString, QString>  bind_vector_t;
    typedef uint8_t                 instruction_t;
    typedef QVector<instruction_t>  program_t;

                                    QDomXPath();
                                    ~QDomXPath();

    bool                            setXPath(const QString& xpath, bool show_commands = false);
    QString                         getXPath() const;
    void                            setProgram(const program_t& program);
    program_t                       getProgram() const;

    void                            bindVariable(const QString& name, const QString& value);
    bool                            hasVariable(const QString& name);
    QString                         getVariable(const QString& name);

    node_vector_t                   apply(QDomNode node) const;
    node_vector_t                   apply(node_vector_t node) const;
    void                            disassemble() const;

private:
    class QDomXPathImpl;
    friend QDomXPathImpl;

    QString                         f_xpath;
    QDomXPathImpl *                 f_impl;
    bind_vector_t                   f_variables;
};

#endif
// _QXMLXPATH_H
// vim: ts=4 sw=4 et
