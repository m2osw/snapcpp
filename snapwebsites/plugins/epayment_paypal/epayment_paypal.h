// Snap Websites Server -- handle payments via PayPal
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

#include "../path/path.h"

#include "http_client_server.h"


/** \file
 * \brief Header of the epayment_paypal plugin.
 *
 * The file defines the various epayment_paypal plugin classes.
 */

namespace snap
{
namespace epayment_paypal
{


enum name_t
{
    SNAP_NAME_EPAYMENT_PAYPAL_CANCEL_URL,
    SNAP_NAME_EPAYMENT_PAYPAL_CLICKED_POST_FIELD,
    SNAP_NAME_EPAYMENT_PAYPAL_DEBUG,
    SNAP_NAME_EPAYMENT_PAYPAL_RETURN_URL,
    SNAP_NAME_EPAYMENT_PAYPAL_SETTINGS_PATH,
    SNAP_NAME_EPAYMENT_PAYPAL_TABLE,

    // SECURE (saved in "secret" table)
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CLIENT_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_CREATED_PAYMENT_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_EXECUTE_PAYMENT,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_ACCESS_TOKEN,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_APP_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_DATA,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_EXPIRES,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_HEADER,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_SCOPE,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_OAUTH2_TOKEN_TYPE,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYMENT_TOKEN,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_PAYER_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SANDBOX_CLIENT_ID,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SANDBOX_SECRET,
    SNAP_SECURE_NAME_EPAYMENT_PAYPAL_SECRET
};
char const *get_name(name_t name) __attribute__ ((const));


class epayment_paypal_exception : public snap_exception
{
public:
    epayment_paypal_exception(char const *       what_msg) : snap_exception("epayment_paypal", what_msg) {}
    epayment_paypal_exception(std::string const& what_msg) : snap_exception("epayment_paypal", what_msg) {}
    epayment_paypal_exception(QString const&     what_msg) : snap_exception("epayment_paypal", what_msg) {}
};

class epayment_paypal_exception_io_error : public snap_exception
{
public:
    epayment_paypal_exception_io_error(char const *       what_msg) : snap_exception("epayment_paypal", what_msg) {}
    epayment_paypal_exception_io_error(std::string const& what_msg) : snap_exception("epayment_paypal", what_msg) {}
    epayment_paypal_exception_io_error(QString const&     what_msg) : snap_exception("epayment_paypal", what_msg) {}
};








class epayment_paypal : public plugins::plugin
                      //, public path::path_execute
{
public:
                                epayment_paypal();
                                ~epayment_paypal();

    static epayment_paypal *    instance();
    virtual QString             description() const;
    virtual int64_t             do_update(int64_t last_updated);

    QtCassandra::QCassandraTable::pointer_t     get_epayment_paypal_table();

    void                        on_bootstrap(snap_child *snap);
    void                        on_generate_header_content(content::path_info_t& path, QDomElement& header, QDomElement& metadata, QString const& ctemplate);
    void                        on_process_post(QString const& uri_path);
    void                        on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info);
    //virtual bool                on_path_execute(content::path_info_t& ipath);

private:
    void                        initial_update(int64_t variables_timestamp);
    void                        content_update(int64_t variables_timestamp);
    bool                        get_oauth2_token(http_client_server::http_client& http, std::string& token_type, std::string& access_token);
    bool                        get_debug();

    zpsnap_child_t                              f_snap;
    QtCassandra::QCassandraTable::pointer_t     f_epayment_paypal_table;
    controlled_vars::flbool_t                   f_debug_defined;
    controlled_vars::flbool_t                   f_debug;
};


} // namespace epayment_paypal
} // namespace snap
// vim: ts=4 sw=4 et
