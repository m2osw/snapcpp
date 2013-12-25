// Snap Websites Server -- favicon management (little icon in tab)
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
#ifndef SNAP_FAVICON_H
#define SNAP_FAVICON_H

#include "../content/content.h"
#include "../form/form.h"

namespace snap
{
namespace favicon
{


enum name_t
{
    SNAP_NAME_FAVICON_ICON,
    SNAP_NAME_FAVICON_ICON_PATH,
    SNAP_NAME_FAVICON_IMAGE,
    SNAP_NAME_FAVICON_SETTINGS
};
char const *get_name(name_t name) __attribute__ ((const));


class favicon_exception : public snap_exception
{
public:
    favicon_exception(char const *what_msg) : snap_exception("Favorite Icon: " + std::string(what_msg)) {}
    favicon_exception(std::string const& what_msg) : snap_exception("Favorite Icon: " + what_msg) {}
    favicon_exception(QString const& what_msg) : snap_exception("Favorite Icon: " + what_msg.toStdString()) {}
};



class favicon : public plugins::plugin, public path::path_execute, public layout::layout_content, public form::form_post
{
public:
    static const sessions::sessions::session_info::session_id_t FAVICON_SESSION_ID_SETTINGS = 1;      // settings-form.xml

                            favicon();
                            ~favicon();

    static favicon *        instance();
    virtual QString         description() const;
    virtual int64_t         do_update(int64_t last_updated);

    void                    on_bootstrap(snap_child *snap);
    virtual bool            on_path_execute(QString const& url);
    virtual void            on_generate_main_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    //void                    on_generate_header_content(layout::layout *l, QString const& path, QDomElement& header, QDomElement& metadata, QString const& ctemplate);
    void                    on_generate_page_content(layout::layout *l, QString const& cpath, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                    on_create_content(QString const& path, QString const& owner, QString const& type);
    void                    on_can_handle_dynamic_path(path::path *path_plugin, QString const& cpath);
    virtual void            on_process_post(QString const& cpath, sessions::sessions::session_info const& info);

private:
    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);
    void output(QString const& cpath);
    void get_icon(QString const& cpath, content::field_search::search_result_t& result, bool image);

    zpsnap_child_t                                  f_snap;
};


} // namespace favicon
} // namespace snap
#endif
// SNAP_FAVICON_H
// vim: ts=4 sw=4 et
