// Snap Websites Server -- anti-virus verifies uploaded files cleanliness
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
#ifndef SNAP_ANTIVIRUS_H
#define SNAP_ANTIVIRUS_H

#include "../content/content.h"
#include "../form/form.h"

namespace snap
{
namespace antivirus
{


enum name_t
{
    SNAP_NAME_ANTIVIRUS_SCAN_ARCHIVE
};
char const *get_name(name_t name) __attribute__ ((const));


class antivirus_exception : public snap_exception
{
public:
    antivirus_exception(char const *what_msg) : snap_exception("Anti-Virus: " + std::string(what_msg)) {}
    antivirus_exception(std::string const& what_msg) : snap_exception("Anti-Virus: " + what_msg) {}
    antivirus_exception(QString const& what_msg) : snap_exception("Anti-Virus: " + what_msg.toStdString()) {}
};



class antivirus : public plugins::plugin, public layout::layout_content
{
public:
    static const sessions::sessions::session_info::session_id_t ANTIVIRUS_SESSION_ID_SETTINGS = 1;      // settings-form.xml

                            antivirus();
                            ~antivirus();

    static antivirus *      instance();
    virtual QString         description() const;
    virtual int64_t         do_update(int64_t last_updated);

    void                    on_bootstrap(snap_child *snap);
    //virtual bool            on_path_execute(QString const& url);
    virtual void            on_generate_main_content(layout::layout *l, content::path_info_t& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    //void                    on_generate_page_content(layout::layout *l, content::path_info_t& cpath, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                    on_check_attachment_security(content::attachment_file const& file, content::permission_flag& secure, bool const fast);

private:
    void                    content_update(int64_t variables_timestamp);

    zpsnap_child_t          f_snap;
};


} // namespace antivirus
} // namespace snap
#endif
// SNAP_FAVICON_H
// vim: ts=4 sw=4 et
