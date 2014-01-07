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
#ifndef SNAP_VERSIONED_FILENAME_H
#define SNAP_VERSIONED_FILENAME_H

#include "snap_exception.h"
#include <controlled_vars/controlled_vars_auto_init.h>
#include <QVector>

namespace snap
{

class versioned_filename_exception : public snap_exception
{
public:
    versioned_filename_exception(char const *whatmsg)        : snap_exception("versioned_filename", whatmsg) {}
    versioned_filename_exception(std::string const& whatmsg) : snap_exception("versioned_filename", whatmsg) {}
    versioned_filename_exception(QString const& whatmsg)     : snap_exception("versioned_filename", whatmsg) {}
};

class versioned_filename_exception_invalid_extension : public versioned_filename_exception
{
public:
    versioned_filename_exception_invalid_extension(char const *whatmsg)        : versioned_filename_exception(whatmsg) {}
    versioned_filename_exception_invalid_extension(std::string const& whatmsg) : versioned_filename_exception(whatmsg) {}
    versioned_filename_exception_invalid_extension(QString const& whatmsg)     : versioned_filename_exception(whatmsg) {}
};




class versioned_filename
{
public:
    enum compare_t : signed int
    {
        COMPARE_INVALID = -2, // i.e. unordered
        COMPARE_SMALLER = -1,
        COMPARE_EQUAL = 0,
        COMPARE_LARGER = 1
    };

    typedef QVector<controlled_vars::zuint32_t> version_t;

                                versioned_filename(QString const& extension);

    bool                        set_filename(QString const& filename);
    bool                        set_name(QString const& name);
    bool                        set_version(QString const& version_string);

    bool                        validate_name(QString const& name);
    bool                        validate_version(QString& version_string, version_t& version);

    bool                        get_valid() const { return f_valid; }
    QString                     get_error() const { return f_error; }
    QString                     get_filename(bool extension = false) const;
    QString const&              get_extension() const { return f_extension; }
    QString const&              get_name() const { return f_name; }
    QString const&              get_version_string() const { return f_version_string; } // this was canonicalized
    version_t                   get_version() const { return f_version; }
    QString const&              get_browser() const { return f_browser; }

    compare_t                   compare(versioned_filename const& rhs) const;
    bool                        operator == (versioned_filename const& rhs) const;
    bool                        operator != (versioned_filename const& rhs) const;
    bool                        operator <  (versioned_filename const& rhs) const;
    bool                        operator <= (versioned_filename const& rhs) const;
    bool                        operator >  (versioned_filename const& rhs) const;
    bool                        operator >= (versioned_filename const& rhs) const;

                                operator bool () const { return f_valid; }
    bool                        operator ! () const { return !f_valid; }

private:
    controlled_vars::fbool_t    f_valid;
    QString                     f_error;
    QString                     f_extension;
    QString                     f_name;
    QString                     f_version_string;
    version_t                   f_version;
    QString                     f_browser;
};

} // namespace snap
#endif
// SNAP_VERSIONED_FILENAME_H
// vim: ts=4 sw=4 et
