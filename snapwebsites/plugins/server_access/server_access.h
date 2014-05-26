// Snap Websites Server -- AJAX response management
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
#pragma once

#include "../content/content.h"

namespace snap
{
namespace server_access
{


class server_access_exception : public snap_exception
{
public:
    server_access_exception(char const *       what_msg) : snap_exception("server-access", what_msg) {}
    server_access_exception(std::string const& what_msg) : snap_exception("server-access", what_msg) {}
    server_access_exception(QString const&     what_msg) : snap_exception("server-access", what_msg) {}
};

class server_access_exception_create_called_twice : public server_access_exception
{
public:
    server_access_exception_create_called_twice(char const *       what_msg) : server_access_exception(what_msg) {}
    server_access_exception_create_called_twice(std::string const& what_msg) : server_access_exception(what_msg) {}
    server_access_exception_create_called_twice(QString const&     what_msg) : server_access_exception(what_msg) {}
};







class server_access : public plugins::plugin
{
public:
                                server_access();
                                ~server_access();

    static server_access *      instance();
    virtual QString             description() const;
    virtual int64_t             do_update(int64_t last_updated);

    void                        on_bootstrap(snap_child *snap);
    void                        on_output_result(QString const& uri_path, QByteArray& result);

    bool                        is_ajax_request() const;
    void                        create_ajax_result(content::path_info_t& ipath, bool const success);
    void                        ajax_output();

    void                        ajax_redirect(QString const& uri, QString const& target = "");
    void                        ajax_append_data(QString const& name, QByteArray const& data);

    SNAP_SIGNAL(process_ajax_result, (content::path_info_t& ipath, bool const succeeded), (ipath, succeeded));

private:
    typedef QMap<QString, QByteArray>   data_map_t;

    void                        content_update(int64_t variables_timestamp);

    zpsnap_child_t              f_snap;
    QDomDocument                f_ajax;
    controlled_vars::fbool_t    f_ajax_initialized;
    controlled_vars::fbool_t    f_success;
    QString                     f_ajax_redirect;
    QString                     f_ajax_target;
    data_map_t                  f_ajax_data;
};


} // namespace server_access
} // namespace snap
// vim: ts=4 sw=4 et
