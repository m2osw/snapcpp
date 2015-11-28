// Snap Websites Server -- Snap Software Description handling
// Copyright (C) 2012-2015  Made to Order Software Corp.
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

#include "../robotstxt/robotstxt.h"

namespace snap
{
namespace snap_software_description
{

enum class name_t
{
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_ENABLE,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_MAX_FILES,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_PATH,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_END_MARKER,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_TAGS,
    SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_WORDS
};
char const * get_name(name_t name) __attribute__ ((const));



class snap_software_description_exception : public snap_exception
{
public:
    snap_software_description_exception(char const *        what_msg) : snap_exception("snap_software_description", what_msg) {}
    snap_software_description_exception(std::string const & what_msg) : snap_exception("snap_software_description", what_msg) {}
    snap_software_description_exception(QString const &     what_msg) : snap_exception("snap_software_description", what_msg) {}
};







class snap_software_description
        : public plugins::plugin
{
public:
                                            snap_software_description();
    virtual                                 ~snap_software_description();

    // plugins::plugin implementation
    static snap_software_description *      instance();
    virtual QString                         description() const;
    virtual QString                         dependencies() const;
    virtual int64_t                         do_update(int64_t last_updated);
    virtual void                            bootstrap(::snap::snap_child * snap);

    // server signal
    void                                    on_backend_process();

    // robotstxt signal
    void                                    on_generate_robotstxt(robotstxt::robotstxt * r);

    // shorturl signal
    void                                    on_allow_shorturl(content::path_info_t & ipath, QString const & owner, QString const & type, bool & allow);

private:
    void                                    content_update(int64_t variables_timestamp);
    void                                    create_catalog(content::path_info_t & ipath, int const depth);

    zpsnap_child_t                          f_snap;
    QtCassandra::QCassandraRow::pointer_t   f_snap_software_description_settings_row;
    QString                                 f_snap_software_description_parser_xsl;
};

} // namespace snap_software_description
} // namespace snap
// vim: ts=4 sw=4 et
