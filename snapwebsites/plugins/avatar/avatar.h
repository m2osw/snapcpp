// Snap Websites Server -- Internet avatar functionality
// Copyright (C) 2015  Made to Order Software Corp.
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

#include "../filter/filter.h"

namespace snap
{
namespace avatar
{


enum class name_t
{
    SNAP_NAME_AVATAR_ADMIN_SETTINGS,
    SNAP_NAME_AVATAR_AGE,
    SNAP_NAME_AVATAR_ATTACHMENT_TYPE,
    SNAP_NAME_AVATAR_DESCRIPTION,
    SNAP_NAME_AVATAR_EXTENSION,
    SNAP_NAME_AVATAR_MIMETYPE,
    SNAP_NAME_AVATAR_PAGE_LAYOUT,
    SNAP_NAME_AVATAR_TITLE,
    SNAP_NAME_AVATAR_TTL,
    SNAP_NAME_AVATAR_TYPE
};
char const * get_name(name_t name) __attribute__ ((const));


class avatar_exception : public snap_exception
{
public:
    avatar_exception(char const *        what_msg) : snap_exception("Avatar", what_msg) {}
    avatar_exception(std::string const & what_msg) : snap_exception("Avatar", what_msg) {}
    avatar_exception(QString const &     what_msg) : snap_exception("Avatar", what_msg) {}
};



class avatar
        : public plugins::plugin
{
public:
                        avatar();
                        ~avatar();

    // plugins::plugin implementation
    static avatar *     instance();
    virtual QString     settings_path() const;
    virtual QString     icon() const;
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

    // filter signals
    void                on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token);

private:
    void                content_update(int64_t variables_timestamp);
    void                generate_avatars();

    zpsnap_child_t      f_snap;
    QString             f_avatar_parser_xsl;
};


} // namespace avatar
} // namespace snap
// vim: ts=4 sw=4 et
