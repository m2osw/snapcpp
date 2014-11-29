// Snap Websites Server -- handle various locale information such as timezone and date output, number formatting for display, etc.
// Copyright (C) 2011-2014  Made to Order Software Corp.
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

#include "../layout/layout.h"
#include "../path/path.h"
#include "../editor/editor.h"

/** \file
 * \brief Header of the locale plugin.
 *
 * This header file is named "snap_locale.h" and not "locale.h" because
 * there is a system file "locale.h" and having the same name prevents
 * the system file from being included properly.
 *
 * The file defines the various locale plugin classes.
 */

namespace snap
{
namespace locale
{


enum name_t
{
    SNAP_NAME_LOCALE_TIMEZONE,
    SNAP_NAME_LOCALE_TIMEZONE_CITY,
    SNAP_NAME_LOCALE_TIMEZONE_CONTINENT
};
char const *get_name(name_t name) __attribute__ ((const));


class locale_exception : public snap_exception
{
public:
    locale_exception(char const *       what_msg) : snap_exception("locale", what_msg) {}
    locale_exception(std::string const& what_msg) : snap_exception("locale", what_msg) {}
    locale_exception(QString const&     what_msg) : snap_exception("locale", what_msg) {}
};

class locale_exception_invalid_content_xml : public locale_exception
{
public:
    locale_exception_invalid_content_xml(char const *       what_msg) : locale_exception(what_msg) {}
    locale_exception_invalid_content_xml(std::string const& what_msg) : locale_exception(what_msg) {}
    locale_exception_invalid_content_xml(QString const&     what_msg) : locale_exception(what_msg) {}
};







class locale : public plugins::plugin
             //, public path::path_execute
             //, public layout::layout_content
{
public:
    // the ICU library only gives us the timezone full name,
    // continent and city all the other parameters will be empty
    struct timezone_info_t
    {
        QString         f_2country;         // 2 letter country code
        int64_t         f_longitude;        // city longitude
        int64_t         f_latitude;         // city latitude
        QString         f_timezone_name;    // the full name of the timezone as is
        QString         f_continent;        // one of the 5 continents and a few other locations
        QString         f_country_or_state; // likely empty (Used for Argentina, Kentucky, Indiana...)
        QString         f_city;             // The main city for that timezone
        QString         f_comment;          // likely empty, a comment about this timezone
    };
    typedef QVector<timezone_info_t>    timezone_list_t;

                        locale();
                        ~locale();

    static locale *     instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    void                on_init_editor_widget(content::path_info_t& ipath, QString const& field_id, QString const& field_type, QDomElement& widget, QtCassandra::QCassandraRow::pointer_t row);
    void                on_prepare_editor_form(editor::editor *e);

    timezone_list_t const&      get_timezone_list();

private:
    void                        content_update(int64_t variables_timestamp);

    timezone_list_t                 f_timezone_list;
    zpsnap_child_t                  f_snap;
};


} // namespace locale
} // namespace snap
// vim: ts=4 sw=4 et
