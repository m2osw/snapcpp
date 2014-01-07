// Snap Websites Servers -- handle versions and dependencies
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
#ifndef SNAP_VERSION_H
#define SNAP_VERSION_H

#include "snap_exception.h"
#include <controlled_vars/controlled_vars_auto_init.h>
#include <controlled_vars/controlled_vars_limited_auto_init.h>
#include <QVector>
#include <qstring_stream.h>
#include <iostream>

namespace snap
{
namespace snap_version
{

class snap_version_exception : public snap_exception
{
public:
    snap_version_exception(char const *whatmsg)        : snap_exception("snap_version", whatmsg) {}
    snap_version_exception(std::string const& whatmsg) : snap_exception("snap_version", whatmsg) {}
    snap_version_exception(QString const& whatmsg)     : snap_exception("snap_version", whatmsg) {}
};

class snap_version_exception_invalid_extension : public snap_version_exception
{
public:
    snap_version_exception_invalid_extension(char const *whatmsg)        : snap_version_exception(whatmsg) {}
    snap_version_exception_invalid_extension(std::string const& whatmsg) : snap_version_exception(whatmsg) {}
    snap_version_exception_invalid_extension(QString const& whatmsg)     : snap_version_exception(whatmsg) {}
};


enum compare_t : signed int
{
    COMPARE_INVALID = -2, // i.e. unordered
    COMPARE_SMALLER = -1,
    COMPARE_EQUAL = 0,
    COMPARE_LARGER = 1
};

typedef QVector<controlled_vars::zuint32_t> version_numbers_vector_t;

enum operator_t
{
    /* ?? */ OPERATOR_UNORDERED,
    /* == */ OPERATOR_EQUAL,
    /* != */ OPERATOR_EXCEPT,
    /* <  */ OPERATOR_EARLIER,
    /* >  */ OPERATOR_LATER,
    /* <= */ OPERATOR_EARLIER_OR_EQUAL,
    /* >= */ OPERATOR_LATER_OR_EQUAL
};
typedef controlled_vars::limited_auto_init<operator_t, OPERATOR_UNORDERED, OPERATOR_LATER_OR_EQUAL, OPERATOR_UNORDERED> safe_operator_t;


bool validate_name(QString const& name, QString& error);
bool validate_version(QString const& version_string, version_numbers_vector_t& version, QString& error);
bool validate_operator(QString const& operator_string, operator_t& op, QString& error);


class name
{
public:
    void                    clear() { f_name.clear(); f_error.clear(); }
    bool                    set_name(QString const& name_string);
    QString const&          get_name() const { return f_name; }
    bool                    is_valid() const { return f_error.isEmpty(); }
    QString                 get_error() const { return f_error; }

    compare_t               compare(name const& rhs) const;

private:
    QString                 f_name;
    QString                 f_error;
};
typedef QVector<name>       name_vector_t;


class version_operator
{
public:
    bool                    set_operator_string(QString const& operator_string);
    bool                    set_operator(operator_t op);
    char const *            get_operator_string() const;
    operator_t              get_operator() const { return f_operator; }
    bool                    is_valid() const { return f_error.isEmpty(); }
    QString const&          get_error() const { return f_error; }

private:
    safe_operator_t         f_operator;
    QString                 f_error;
};


class version
{
public:
    bool                        set_version_string(QString const& version_string);
    void                        set_version(version_numbers_vector_t const& version);
    void                        set_operator(version_operator const& op);
    version_numbers_vector_t const& get_version() const { return f_version; }
    QString const&              get_version_string() const;
    QString                     get_opversion_string() const;
    version_operator const&     get_operator() const { return f_operator; }
    bool                        is_valid() const { return f_error.isEmpty() && f_operator.is_valid(); }
    QString                     get_error() const { return f_error; }

    compare_t                   compare(version const& rhs) const;

private:
    mutable QString             f_version_string;
    version_numbers_vector_t    f_version;
    QString                     f_error;
    version_operator            f_operator;
};
typedef QVector<version>        version_vector_t;


class versioned_filename
{
public:

                                versioned_filename(QString const& extension);

    bool                        set_filename(QString const& filename);
    bool                        set_name(QString const& name);
    bool                        set_version(QString const& version_string);

    bool                        is_valid() const { return f_error.isEmpty() && f_name.is_valid() && f_version.is_valid() && f_browser.is_valid(); }

    QString                     get_error() const { return f_error; }
    QString                     get_filename(bool extension = false) const;
    QString const&              get_extension() const { return f_extension; }
    QString const&              get_name() const { return f_name.get_name(); }
    QString const&              get_version_string() const { return f_version.get_version_string(); } // this was canonicalized
    version_numbers_vector_t const& get_version() const { return f_version.get_version(); }
    QString const&              get_browser() const { return f_browser.get_name(); }

    compare_t                   compare(versioned_filename const& rhs) const;
    bool                        operator == (versioned_filename const& rhs) const;
    bool                        operator != (versioned_filename const& rhs) const;
    bool                        operator <  (versioned_filename const& rhs) const;
    bool                        operator <= (versioned_filename const& rhs) const;
    bool                        operator >  (versioned_filename const& rhs) const;
    bool                        operator >= (versioned_filename const& rhs) const;

                                operator bool () const { return is_valid(); }
    bool                        operator ! () const { return !is_valid(); }

private:
    QString                     f_error;
    QString                     f_extension;
    name                        f_name;
    version                     f_version;
    name                        f_browser;
};


class dependency
{
public:
    bool                        set_dependency(QString const& dependency_string);
    QString                     get_dependency_string() const;
    QString const&              get_name() const { return f_name.get_name(); }
    version_vector_t const&     get_versions() const { return f_versions; }
    name_vector_t const&        get_browsers() const { return f_browsers; }
    bool                        is_valid() const;
    QString const&              get_error() const { return f_error; }

private:
    QString                     f_error;
    name                        f_name;
    version_vector_t            f_versions;
    name_vector_t               f_browsers;
};


} // namespace snap_version
} // namespace snap
#endif
// SNAP_VERSION_H
// vim: ts=4 sw=4 et
