// Snap Websites Servers -- snap websites child process hanlding
// Copyright (C) 2011-2015  Made to Order Software Corp.
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

#include "snap_uri.h"
#include "snap_signals.h"
#include "snap_exception.h"
#include "snap_version.h"
#include "http_cookie.h"
#include "udp_client_server.h"

#include <controlled_vars/controlled_vars_need_init.h>

#include <QtCassandra/QCassandra.h>
#include <QtCassandra/QCassandraContext.h>

#include <stdlib.h>

#include <QBuffer>

namespace snap
{

class snap_child_exception : public snap_exception
{
public:
    snap_child_exception(char const *       whatmsg) : snap_exception("snap_child", whatmsg) {}
    snap_child_exception(std::string const& whatmsg) : snap_exception("snap_child", whatmsg) {}
    snap_child_exception(QString const&     whatmsg) : snap_exception("snap_child", whatmsg) {}
};

class snap_child_exception_unique_number_error : public snap_child_exception
{
public:
    snap_child_exception_unique_number_error(char const *       whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_unique_number_error(std::string const& whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_unique_number_error(QString const&     whatmsg) : snap_child_exception(whatmsg) {}
};

class snap_child_exception_invalid_header_value : public snap_child_exception
{
public:
    snap_child_exception_invalid_header_value(char const *       whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_invalid_header_value(std::string const& whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_invalid_header_value(QString const&     whatmsg) : snap_child_exception(whatmsg) {}
};

class snap_child_exception_invalid_header_field_name : public snap_child_exception
{
public:
    snap_child_exception_invalid_header_field_name(char const *       whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_invalid_header_field_name(std::string const& whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_invalid_header_field_name(QString const&     whatmsg) : snap_child_exception(whatmsg) {}
};

class snap_child_exception_no_server : public snap_child_exception
{
public:
    snap_child_exception_no_server(char const *       whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_no_server(std::string const& whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_no_server(QString const&     whatmsg) : snap_child_exception(whatmsg) {}
};

class snap_child_exception_invalid_email : public snap_child_exception
{
public:
    snap_child_exception_invalid_email(char const *       whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_invalid_email(std::string const& whatmsg) : snap_child_exception(whatmsg) {}
    snap_child_exception_invalid_email(QString const&     whatmsg) : snap_child_exception(whatmsg) {}
};



class permission_error_callback;
class server;

typedef controlled_vars::auto_init<pid_t>   zpid_t;
typedef controlled_vars::auto_init<int, -1> zfile_descriptor_t;


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
        HTTP_CODE_METHOD_FAILURE = 420, /* WARNING: same as Enhance Your Calm */
        HTTP_CODE_UNPROCESSABLE_ENTITY = 422,
        HTTP_CODE_LOCKED = 423,
        HTTP_CODE_FAILED_DEPENDENCY = 424,
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
        HTTP_CODE_TOKEN_EXPIRED = 498,
        HTTP_CODE_CLIENT_CLOSED_REQUEST = 499,
        HTTP_CODE_TOKEN_REQUIRED = 499, /* WARNING: same as Client Close Request */

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

    enum date_format_t
    {
        DATE_FORMAT_SHORT,
        DATE_FORMAT_SHORT_US,
        DATE_FORMAT_LONG,
        DATE_FORMAT_TIME,
        DATE_FORMAT_EMAIL,
        DATE_FORMAT_HTTP
    };

    enum status_t
    {
        SNAP_CHILD_STATUS_READY,
        SNAP_CHILD_STATUS_RUNNING
    };

    typedef std::weak_ptr<server>   server_pointer_t;
    typedef QMap<QString, QString>  environment_map_t;

    // Note: the information saved in files come from the POST and
    // is not to be trusted (especially the mime type)
    class post_file_t
    {
    public:
        void                        set_name(QString const& name) { f_name = name; }
        void                        set_filename(QString const& filename) { f_filename = filename; }
        void                        set_mime_type(QString const& mime_type) { f_mime_type = mime_type; }
        void                        set_original_mime_type(QString const& mime_type) { f_original_mime_type = mime_type; }
        void                        set_creation_time(time_t ctime) { f_creation_time = ctime; }
        void                        set_modification_time(time_t mtime) { f_modification_time = mtime; }
        void                        set_data(QByteArray const& data);
        void                        set_size(int size) { f_size = size; }
        void                        set_index(int index) { f_index = index; }
        void                        set_image_width(int width) { f_image_width = width; }
        void                        set_image_height(int height) { f_image_height = height; }

        QString                     get_name() const { return f_name; }
        QString                     get_filename() const { return f_filename; }
        QString                     get_basename() const;
        QString                     get_original_mime_type() const { return f_original_mime_type; }
        QString                     get_mime_type() const { return f_mime_type; }
        time_t                      get_creation_time() const { return f_creation_time; }
        time_t                      get_modification_time() const { return f_modification_time; }
        QByteArray                  get_data() const { return f_data; }
        int                         get_size() const;
        int                         get_index() const { return f_index; }
        int                         get_image_width() const { return f_image_width; }
        int                         get_image_height() const { return f_image_height; }

    private:
        QString                     f_name; // field name
        QString                     f_filename;
        QString                     f_original_mime_type;
        QString                     f_mime_type;
        controlled_vars::zint64_t   f_creation_time;
        controlled_vars::zint64_t   f_modification_time;
        QByteArray                  f_data;
        controlled_vars::zuint32_t  f_size;
        controlled_vars::zuint32_t  f_index;
        controlled_vars::zuint32_t  f_image_width;
        controlled_vars::zuint32_t  f_image_height;
    };
    // map indexed by filename
    typedef QMap<QString, post_file_t> post_file_map_t;

    struct language_name_t
    {
        char const *    f_language;         // full English name of the language
        char const *    f_native;           // full Native name of the language
        char const      f_short_name[3];    // expected name (xx); must be 2 characters
        char const *    f_other_names;      // 3 or 4 letter names separated by commas; if NULL no extras
    };

    struct country_name_t
    {
        char const      f_abbreviation[3];  // must be 2 characters
        char const *    f_name;
    };

    class locale_info_t
    {
    public:
        void            set_language(QString const& language) { f_language = language; }
        void            set_country(QString const& country) { f_country = country; }

        QString const&  get_language() const { return f_language; }
        QString const&  get_country() const { return f_country; }

    private:
        QString         f_language;
        QString         f_country;
    };
    typedef QVector<locale_info_t> locale_info_vector_t;

    typedef int header_mode_t;
    static header_mode_t const HEADER_MODE_NO_ERROR     = 0x0001;
    static header_mode_t const HEADER_MODE_REDIRECT     = 0x0002;
    static header_mode_t const HEADER_MODE_ERROR        = 0x0004;
    static header_mode_t const HEADER_MODE_EVERYWHERE   = 0xFFFF;

    enum compression_t
    {
        COMPRESSION_INVALID = -2,
        COMPRESSION_UNDEFINED = -1,
        COMPRESSION_IDENTITY = 0,   // no compression
        COMPRESSION_GZIP,
        COMPRESSION_DEFLATE,        // zlib without the gzip magic numbers
        COMPRESSION_BZ2,
        COMPRESSION_SDCH
    };
    typedef controlled_vars::limited_auto_init<compression_t, COMPRESSION_INVALID, COMPRESSION_DEFLATE, COMPRESSION_UNDEFINED> safe_compression_t;
    typedef QVector<safe_compression_t> compression_vector_t;

                                snap_child(server_pointer_t s);
                                virtual ~snap_child();

    bool                        process(int socket);
    void                        kill();
    status_t                    check_status();

    snap_uri const &            get_uri() const;
    bool                        has_post() const { return f_has_post; }
    QString                     get_action() const;
    void                        set_action(QString const& action);
    static void                 verify_email(QString const& email, size_t const max = 1);

    void                        exit(int code);
    bool                        is_debug() const;
    static char const *         get_running_server_version();
    QString                     get_server_parameter(QString const& name);
    QtCassandra::QCassandraValue get_site_parameter(QString const& name);
    void                        set_site_parameter(QString const& name, QtCassandra::QCassandraValue const& value);
    void                        improve_signature(QString const& path, QString& signature);
    QtCassandra::QCassandraContext::pointer_t get_context() { return f_context; }
    QString const &             get_domain_key() const { return f_domain_key; }
    QString const &             get_website_key() const { return f_website_key; }
    QString const &             get_site_key() const { return f_site_key; }
    QString const &             get_site_key_with_slash() const { return f_site_key_with_slash; }
    static int64_t              get_current_date();
    void                        init_start_date();
    int64_t                     get_start_date() const { return f_start_date; }
    time_t                      get_start_time() const { return f_start_date / static_cast<int64_t>(1000000); }
    void                        set_header(QString const & name, QString const & value, header_mode_t modes = HEADER_MODE_NO_ERROR);
    void                        set_cookie(http_cookie const & cookie);
    void                        set_ignore_cookies();
    bool                        has_header(QString const & name) const;
    QString                     get_header(QString const & name) const;
    QString                     get_unique_number();
    QtCassandra::QCassandraTable::pointer_t create_table(const QString & table_name, const QString & comment);
    void                        new_content();
    void                        verify_permissions(QString const & path, permission_error_callback & err_callback);
    QString                     default_action(QString uri_path);
    void                        process_post();
    QString                     get_language();
    QString                     get_country() const;
    QString                     get_language_key();
    locale_info_vector_t const& get_plugins_locales();
    locale_info_vector_t const& get_browser_locales() const;
    bool                        get_working_branch() const;
    snap_version::version_number_t get_branch() const;
    snap_version::version_number_t get_revision() const;
    QString                     get_revision_key() const; // <branch>.<revision> as a string (pre-defined)
    compression_vector_t        get_compression() const;
    static void                 canonicalize_path(QString & path);
    static QString              date_to_string(int64_t v, date_format_t date_format = DATE_FORMAT_SHORT);
    static time_t               string_to_date(QString const & date);
    static int                  last_day_of_month(int month, int year);
    bool                        verify_locale(QString & lang, QString & country, bool generate_errors);
    static bool                 verify_language_name(QString & lang);
    static bool                 verify_country_name(QString & country);
    static language_name_t const *get_languages();
    static country_name_t const *get_countries();
    static bool                 tag_is_inline(char const * tag, int length);
    void                        set_timezone(QString const & timezone);
    void                        set_locale(QString const & locale);

    QString                     snapenv(QString const & name) const;
    bool                        postenv_exists(QString const & name) const;
    QString                     postenv(QString const & name, QString const & default_value = "") const;
    void                        replace_postenv(QString const & name, QString const & value);
    environment_map_t const &   all_postenv() const { return f_post; }
    bool                        postfile_exists(QString const & name) const;
    post_file_t const&          postfile(QString const & name) const;
    bool                        cookie_is_defined(QString const & name) const;
    QString                     cookie(QString const & name) const;
    void                        attach_to_session();
    bool                        load_file(post_file_t & file);
    QString                     snap_url(QString const & url) const;
    // TODO translations? (not too important though)
    void                        page_redirect(QString const & path, http_code_t http_code = HTTP_CODE_MOVED_PERMANENTLY, QString const & reason_brief = "Moved", QString const & reason = "This page has moved");
    void                        die(http_code_t err_code, QString err_name, QString const & err_description, QString const & err_details);
    static void                 define_http_name(http_code_t http_code, QString & http_name);
    void                        finish_update();

    QByteArray                  get_output() const;
    void                        output(QByteArray const & data);
    void                        output(QString const & data);
    void                        output(std::string const & data);
    void                        output(char const * data);
    void                        output(wchar_t const * data);
    bool                        empty_output() const;
    void                        output_result(header_mode_t mode, QByteArray output_data);
    void                        trace(QString const & data);
    void                        trace(std::string const & data);
    void                        trace(char const * data);

    void                        udp_ping(char const * name, char const * message = "PING");

    typedef QSharedPointer<udp_client_server::udp_server> udp_server_t;
    udp_server_t                udp_get_server( char const * name );

protected:
    pid_t                       fork_child();
    void                        connect_cassandra();
    void                        canonicalize_domain();
    void                        canonicalize_website();
    void                        canonicalize_options();
    void                        site_redirect();
    QStringList                 init_plugins(bool const add_defaults);

    server_pointer_t                            f_server;
    controlled_vars::flbool_t                   f_is_child;
    zpid_t                                      f_child_pid;
    zfile_descriptor_t                          f_socket;
    QtCassandra::QCassandraContext::pointer_t   f_context;
    controlled_vars::mint64_t                   f_start_date; // time request arrived
    controlled_vars::flbool_t                   f_ready; // becomes true just before the server::execute() call
    environment_map_t                           f_env;
    snap_uri                                    f_uri;
    QString                                     f_site_key;
    QString                                     f_original_site_key;

private:
    struct http_header_t
    {
        QString         f_header;
        header_mode_t   f_modes;
    };
    typedef QMap<QString, http_header_t>    header_map_t;
    typedef QMap<QString, http_cookie>      cookie_map_t;

    void                        read_environment();
    void                        mark_for_initialization();
    void                        setup_uri();
    void                        snap_info();
    void                        snap_statistics();
    void                        update_plugins(QStringList const& list_of_plugins);
    void                        execute();
    void                        process_backend_uri(QString const& uri);
    void                        write(char const *data, ssize_t size);
    void                        write(char const *str);
    void                        write(QString const& str);
    void                        output_headers(header_mode_t modes);
    void                        output_cookies();

    QtCassandra::QCassandra::pointer_t          f_cassandra;
    QtCassandra::QCassandraTable::pointer_t     f_site_table;
    controlled_vars::flbool_t                   f_new_content;
    controlled_vars::flbool_t                   f_is_being_initialized;
    environment_map_t                           f_post;
    post_file_map_t                             f_files;
    environment_map_t                           f_browser_cookies;
    controlled_vars::flbool_t                   f_has_post;
    mutable controlled_vars::flbool_t           f_fixed_server_protocol;
    QString                                     f_domain_key;
    QString                                     f_website_key;
    QString                                     f_site_key_with_slash;
    QBuffer                                     f_output;
    header_map_t                                f_header;
    cookie_map_t                                f_cookies;
    controlled_vars::flbool_t                   f_ignore_cookies;
    QString                                     f_language;
    QString                                     f_country;
    QString                                     f_language_key;
    controlled_vars::flbool_t                   f_original_timezone_defined;
    QString                                     f_original_timezone;
    controlled_vars::flbool_t                   f_plugins_locales_was_not_ready;
    locale_info_vector_t                        f_plugins_locales;
    locale_info_vector_t                        f_browser_locales;
    controlled_vars::flbool_t                   f_working_branch;
    snap_version::version_number_t              f_branch;
    snap_version::version_number_t              f_revision;
    QString                                     f_revision_key;
    compression_vector_t                        f_compressions;
};

typedef std::vector<snap_child *> snap_child_vector_t;

} // namespace snap
// vim: ts=4 sw=4 et
