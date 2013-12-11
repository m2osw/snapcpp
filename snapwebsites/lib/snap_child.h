// Snap Websites Servers -- snap websites child
// Copyright (C) 2011-2013  Made to Order Software Corp.
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
#ifndef SNAP_SNAPCHILD_H
#define SNAP_SNAPCHILD_H

#include "snap_uri.h"
#include "snap_signals.h"
#include "snap_exception.h"
#include "http_cookie.h"
#include "udp_client_server.h"
#include <stdlib.h>
#include <controlled_vars/controlled_vars_need_init.h>
#include <QPointer>
#include <QDomDocument>
#include <QBuffer>
#include <QtCassandra/QCassandra.h>
#include <QtCassandra/QCassandraContext.h>

namespace snap
{

class snap_child_exception : public snap_exception {};
class snap_child_exception_unique_number_error : public snap_child_exception {};
class snap_child_exception_invalid_header_value : public snap_child_exception {};
class snap_child_exception_invalid_header_field_name : public snap_child_exception {};


class server;


class snap_child
{
public:
    enum http_code_t
    {
        // a couple of internal codes used here and there (never sent to user)
        HTTP_CODE_INVALID = -2,
        HTTP_CODE_UNDEFINED = -1,

        HTTP_CODE_CONTINUE = 100,
        HTTP_CODE_SWITCHING_PROTOCOLS = 101,
        HTTP_CODE_PROCESSING = 102,

        HTTP_CODE_OK = 200,
        HTTP_CODE_CREATED = 201,
        HTTP_CODE_ACCEPTED = 202,
        HTTP_CODE_NON_AUTHORITATIVE_INFORMATION = 203,
        HTTP_CODE_NO_CONTENT = 204,
        HTTP_CODE_RESET_CONTENT = 205,
        HTTP_CODE_PARTIAL_CONTENT = 206,
        HTTP_CODE_MULTI_STATUS = 207,
        HTTP_CODE_ALREADY_REPORTED = 208,
        HTTP_CODE_IM_USED = 226, // Instance Manipulation Used

        HTTP_CODE_MULTIPLE_CHOICE = 300,
        HTTP_CODE_MOVED_PERMANENTLY = 301,
        HTTP_CODE_FOUND = 302,
        HTTP_CODE_SEE_OTHER = 303,
        HTTP_CODE_NOT_MODIFIED = 304,
        HTTP_CODE_USE_PROXY = 305,
        HTTP_CODE_SWITCH_PROXY = 306,
        HTTP_CODE_TEMPORARY_REDIRECT = 307,
        HTTP_CODE_PERMANENT_REDIRECT = 308,

        HTTP_CODE_BAD_REQUEST = 400,
        HTTP_CODE_UNAUTHORIZED = 401,
        HTTP_CODE_PAYMENT_REQUIRED = 402,
        HTTP_CODE_FORBIDDEN = 403,
        HTTP_CODE_NOT_FOUND = 404,
        HTTP_CODE_METHOD_NOT_ALLOWED = 405,
        HTTP_CODE_NOT_ACCEPTABLE = 406,
        HTTP_CODE_PROXY_AUTHENTICATION_REQUIRED = 407,
        HTTP_CODE_REQUEST_TIMEOUT = 408,
        HTTP_CODE_CONFLICT = 409,
        HTTP_CODE_GONE = 410,
        HTTP_CODE_LENGTH_REQUIRED = 411,
        HTTP_CODE_PRECONDITION_FAILED = 412,
        HTTP_CODE_REQUEST_ENTITY_TOO_LARGE = 413,
        HTTP_CODE_REQUEST_URI_TOO_LONG = 414,
        HTTP_CODE_UNSUPPORTED_MEDIA_TYPE = 415,
        HTTP_CODE_REQUESTED_RANGE_NOT_SATISFIABLE = 416,
        HTTP_CODE_EXPECTATION_FAILED = 417,
        HTTP_CODE_I_AM_A_TEAPOT = 418,
        HTTP_CODE_ENHANCE_YOUR_CALM = 420,
        HTTP_CODE_UNPROCESSABLE_ENTITY = 422,
        HTTP_CODE_LOCKED = 423,
        HTTP_CODE_FAILED_DEPENDENCY = 424,
        HTTP_CODE_METHOD_FAILURE = 424, /* WARNING: same as Failed Dependency */
        HTTP_CODE_UNORDERED_COLLECTION = 425,
        HTTP_CODE_UPGRADE_REQUIRED = 426,
        HTTP_CODE_PRECONDITION_REQUIRED = 428,
        HTTP_CODE_TOO_MANY_REQUESTS = 429,
        HTTP_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
        HTTP_CODE_NO_RESPONSE = 444,
        HTTP_CODE_RETRY_WITH = 449,
        HTTP_CODE_BLOCKED_BY_WINDOWS_PARENTAL_CONTROLS = 450,
        HTTP_CODE_UNAVAILABLE_FOR_LEGAL_REASONS = 451,
        HTTP_CODE_REDIRECT = 451, /* WARNING: same as Unavailable For Legal Reasons */
        HTTP_CODE_REQUEST_HEADER_TOO_LARGE = 494,
        HTTP_CODE_CERT_ERROR = 495,
        HTTP_CODE_NO_CERT = 496,
        HTTP_CODE_HTTP_TO_HTTPS = 497,
        HTTP_CODE_CLIENT_CLOSED_REQUEST = 499,

        HTTP_CODE_INTERNAL_SERVER_ERROR = 500,
        HTTP_CODE_NOT_IMPLEMENTED = 501,
        HTTP_CODE_BAD_GATEWAY = 502,
        HTTP_CODE_SERVICE_UNAVAILABLE = 503,
        HTTP_CODE_GATEWAY_TIMEOUT = 504,
        HTTP_CODE_HTTP_VERSION_NOT_SUPPORTED = 505,
        HTTP_CODE_VARIANTS_ALSO_NEGOTIATES = 506,
        HTTP_CODE_INSUFFICIANT_STORAGE = 507,
        HTTP_CODE_LOOP_DETECTED = 508,
        HTTP_CODE_BANDWIDTH_LIMIT_EXCEEDED = 509,
        HTTP_CODE_NOT_EXTENDED = 510,
        HTTP_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511,
        HTTP_CODE_ACCESS_DENIED = 531,
        HTTP_CODE_NETWORK_READ_TIMEOUT_ERROR = 598,
        HTTP_CODE_NETWORK_CONNECT_TIMEOUT_ERROR = 599
    };
    enum status_t
    {
        SNAP_CHILD_STATUS_READY,
        SNAP_CHILD_STATUS_RUNNING
    };
    typedef std::shared_ptr<server> server_pointer_t;
    typedef QMap<QString, QString>  environment_map_t;

                               snap_child(server_pointer_t s);
                               ~snap_child();

    bool                       process(int socket);
    void                        backend();
    status_t                    check_status();

    const snap_uri&             get_uri() const;

    void                        exit(int code);
    bool                        is_debug() const;
    QtCassandra::QCassandraValue get_site_parameter(const QString& name);
    void                        set_site_parameter(const QString& name, const QtCassandra::QCassandraValue& value);
    QSharedPointer<QtCassandra::QCassandraContext> get_context() { return f_context; }
    const QString&              get_domain_key() const { return f_domain_key; }
    const QString&              get_website_key() const { return f_website_key; }
    const QString&              get_site_key() const { return f_site_key; }
    const QString&              get_site_key_with_slash() const { return f_site_key_with_slash; }
    int64_t                     get_start_date() const { return f_start_date; }
    time_t                      get_start_time() const { return f_start_date / static_cast<int64_t>(1000000); }
    void                        set_header(const QString& name, const QString& value);
    void                        set_cookie(const http_cookie& cookie);
    bool                        has_header(const QString& name) const;
    QString                     get_header(const QString& name) const;
    QString                     get_unique_number();
    QSharedPointer<QtCassandra::QCassandraTable> create_table(const QString& table_name, const QString& comment);
    void                        new_content();
    static void                 canonicalize_path(QString& path);
    static QString              date_to_string(int64_t v, bool long_format = false);

    QString                     snapenv(const QString& name) const;
    bool                        postenv_exists(const QString& name) const;
    QString                     postenv(const QString& name, const QString& default_value = "") const;
    const environment_map_t&    all_postenv() const { return f_post; }
    bool                        cookie_is_defined(const QString& name) const;
    QString                     cookie(const QString& name) const;
    void                        attach_to_session();
    QString                     snap_url(const QString& url) const;
    void                        page_redirect(const QString& path, http_code_t http_code = HTTP_CODE_MOVED_PERMANENTLY);
    void                        die(http_code_t err_code, QString err_name, const QString& err_description, const QString& err_details);
    static void                 define_http_name(http_code_t http_code, QString& http_name);

    void                        output(const QString& data);
    void                        output(const std::string& data);
    void                        output(const char *data);
    bool                        empty_output() const;

    void                        udp_ping(const char *name, const char *message = "PING");
    QSharedPointer<udp_client_server::udp_server> udp_get_server(const char *name);

private:
    typedef QMap<QString, QString>      header_map_t;
    typedef QMap<QString, http_cookie>  cookie_map_t;

    void                        read_environment();
    void                        init_start_date();
    void                        setup_uri();
    void                        snap_info();
    void                        snap_statistics();
    void                        connect_cassandra();
    void                        canonicalize_domain();
    void                        canonicalize_website();
    void                        site_redirect();
    void                        init_plugins();
    void                        update_plugins(const QStringList& list_of_plugins);
    void                        execute();
    void                        process_backend_uri(const QString& uri);
    void                        write(const char *data, ssize_t size);
    void                        write(const char *str);
    void                        write(const QString& str);
    void                        output_cookies();
    void                        verify_permissions();

    controlled_vars::mint64_t           f_start_date; // time request arrived
    server_pointer_t                    f_server;
    QPointer<QtCassandra::QCassandra>   f_cassandra;
    QSharedPointer<QtCassandra::QCassandraContext>  f_context;
    QSharedPointer<QtCassandra::QCassandraTable>    f_site_table;
    controlled_vars::fbool_t            f_new_content;
    controlled_vars::fbool_t            f_is_child;
    pid_t                               f_child_pid;
    int                                 f_socket;
    environment_map_t                   f_env;
    environment_map_t                   f_post;
    environment_map_t                   f_browser_cookies;
    controlled_vars::fbool_t            f_has_post;
    mutable controlled_vars::fbool_t    f_fixed_server_protocol;
    snap_uri                            f_uri;
    QString                             f_domain_key;
    QString                             f_website_key;
    QString                             f_site_key;
    QString                             f_site_key_with_slash;
    QString                             f_original_site_key;
    QBuffer                             f_output;
    header_map_t                        f_header;
    cookie_map_t                        f_cookies;
};

typedef std::vector<snap_child *> snap_child_vector_t;

} // namespace snap
#endif
// SNAP_SNAPCHILD_H
// vim: ts=4 sw=4 et
