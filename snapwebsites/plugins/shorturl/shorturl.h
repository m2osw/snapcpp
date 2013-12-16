// Snap Websites Server -- shorturl management (smaller URLs for all pages)
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
#ifndef SNAP_SHORTURL_H
#define SNAP_SHORTURL_H

#include "../layout/layout.h"

namespace snap
{
namespace shorturl
{


enum name_t
{
    SNAP_NAME_SHORTURL_DATE,
    SNAP_NAME_SHORTURL_HTTP_LINK,
    SNAP_NAME_SHORTURL_IDENTIFIER,
    SNAP_NAME_SHORTURL_ID_ROW,
    SNAP_NAME_SHORTURL_INDEX_ROW,
    SNAP_NAME_SHORTURL_NO_SHORTURL,
    SNAP_NAME_SHORTURL_TABLE,
    SNAP_NAME_SHORTURL_URL
};
const char *get_name(name_t name) __attribute__ ((const));


class shorturl_exception : public snap_exception
{
public:
    shorturl_exception(const char *what_msg) : snap_exception("Short URL: " + std::string(what_msg)) {}
    shorturl_exception(const std::string& what_msg) : snap_exception("Short URL: " + what_msg) {}
    shorturl_exception(const QString& what_msg) : snap_exception("Short URL: " + what_msg.toStdString()) {}
};



class shorturl : public plugins::plugin, public path::path_execute, public layout::layout_content
{
public:
                        shorturl();
                        ~shorturl();

    static shorturl *   instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_shorturl_table();

    void                on_bootstrap(snap_child *snap);
    virtual bool        on_path_execute(const QString& url);
    virtual void        on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate);
    void                on_generate_header_content(layout::layout *l, const QString& path, QDomElement& header, QDomElement& metadata, const QString& ctemplate);
    void                on_create_content(const QString& path, const QString& owner, const QString& type);
    void                on_can_handle_dynamic_path(path::path *path_plugin, const QString& cpath);

private:
    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);

    zpsnap_child_t                                  f_snap;
    QSharedPointer<QtCassandra::QCassandraTable>    f_shorturl_table;
};


} // namespace shorturl
} // namespace snap
#endif
// SNAP_SHORTURL_H
// vim: ts=4 sw=4 et
