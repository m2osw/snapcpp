// Snap Websites Server -- website system info settings
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
#ifndef SNAP_INFO_H
#define SNAP_INFO_H

#include "../content/content.h"
#include "../form/form.h"

namespace snap
{
namespace info
{


enum name_t
{
    SNAP_NAME_INFO_LONG_NAME,
    SNAP_NAME_INFO_NAME,
    SNAP_NAME_INFO_SHORT_NAME
};
char const *get_name(name_t name) __attribute__ ((const));


class info_exception : public snap_exception
{
public:
    info_exception(char const *what_msg) : snap_exception("Info: " + std::string(what_msg)) {}
    info_exception(std::string const& what_msg) : snap_exception("Info: " + what_msg) {}
    info_exception(QString const& what_msg) : snap_exception("Info: " + what_msg.toStdString()) {}
};

class info_exception_invalid_path : public info_exception
{
public:
    info_exception_invalid_path(char const *what_msg) : info_exception(what_msg) {}
    info_exception_invalid_path(std::string const& what_msg) : info_exception(what_msg) {}
    info_exception_invalid_path(QString const& what_msg) : info_exception(what_msg) {}
};



class info : public plugins::plugin, public form::form_post, public path::path_execute, public layout::layout_content
{
public:
    static const sessions::sessions::session_info::session_id_t INFO_SESSION_ID_SETTINGS = 1;      // settings-form.xml

                            info();
                            ~info();

    static info *           instance();
    virtual QString         description() const;
    virtual int64_t         do_update(int64_t last_updated);

    void                    on_bootstrap(snap_child *snap);
    virtual void            on_process_post(QString const& cpath, sessions::sessions::session_info const& session_info);
    virtual bool            on_path_execute(QString const& url);
    virtual void            on_generate_main_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    //void                    on_generate_header_content(layout::layout *l, QString const& path, QDomElement& header, QDomElement& metadata, QString const& ctemplate);
    //void                    on_generate_page_content(layout::layout *l, QString const& cpath, QDomElement& page, QDomElement& body, QString const& ctemplate);
void on_fill_form_widget(form::form *f, QString const& owner, QString const& cpath, QDomDocument xml_form, QDomElement widget, QString const& id);

private:
    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);

    zpsnap_child_t          f_snap;
};


} // namespace info
} // namespace snap
#endif
// SNAP_INFO_H
// vim: ts=4 sw=4 et
