//
// File:        snapmanagercgi.cpp
// Object:      Allow for managing a Snap! Cluster.
//
// Copyright:   Copyright (c) 2016 Made to Order Software Corp.
//              All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "snapmanagercgi.h"

#include "server_status.h"

// C lib
//
#include <fcntl.h>
#include <glob.h>
#include <sys/file.h>



namespace snap_manager
{


/** \brief Initialize the manager_cgi.
 *
 * The manager_cgi gets initialized with the argc and argv in case it
 * gets started from the command line. That way one can use --version
 * and --help, especially.
 */
manager_cgi::manager_cgi()
    : manager(false)
    , f_communicator_port(4040)
    , f_communicator_address("127.0.0.1")
{
}


manager_cgi::~manager_cgi()
{
}


int manager_cgi::error(char const * code, char const * msg, char const * details)
{
    if(details == nullptr)
    {
        details = "No details.";
    }

    SNAP_LOG_FATAL("error(\"")(code)("\", \"")(msg)("\", \"")(details)("\")");

    std::string body("<h1>");
    body += code;
    body += "</h1><p>";
    body += (msg == nullptr ? "Sorry! We found an invalid server configuration or some other error occurred." : msg);
    body += "</p>";

    std::cout   << "Status: " << code                       << std::endl
                << "Expires: Sun, 19 Nov 1978 05:00:00 GMT" << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << body.length()      << std::endl
                << "X-Powered-By: snapmanager.cgi"          << std::endl
                << std::endl
                << body
                ;

    return 1;
}


/** \brief Verify that the request is acceptable.
 *
 * This function makes sure that the request corresponds to what we
 * generally expect.
 *
 * \return true if the request is accepted, false otherwise.
 */
bool manager_cgi::verify()
{
    if(!f_config.contains("stylesheet"))
    {
        error("503 Service Unavailable",
              "The snapmanager.cgi service is not currently available.",
              "The stylesheet parameter is not defined.");
        return false;
    }

    // If not defined, keep the default of localhost:4040
    if(f_config.contains("snapcommunicator"))
    {
        snap_addr::addr const a(f_config["snapcommunicator"].toUtf8().data(), "127.0.0.1", 4040, "tcp");
        f_communicator_address = a.get_ipv4or6_string(false, false);
        f_communicator_port = a.get_port();
    }

    // catch "invalid" methods early so we do not waste
    // any time with methods we do not support at all
    //
    // later we want to add support for PUT, PATCH and DELETE though
    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * request_method(getenv("REQUEST_METHOD"));
        if(request_method == nullptr)
        {
            SNAP_LOG_FATAL("Request method is not defined.");
            std::string body("<html><head><title>Method Not Defined</title></head><body><p>Sorry. We only support GET and POST.</p></body></html>");
            std::cout   << "Status: 405 Method Not Defined"         << std::endl
                        << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, POST"                       << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snapmanager.cgi"          << std::endl
                        << std::endl
                        << body;
            return false;
        }
        if(strcmp(request_method, "GET") != 0
        && strcmp(request_method, "POST") != 0)
        {
            SNAP_LOG_FATAL("Request method is \"")(request_method)("\", which we currently refuse.");
            if(strcmp(request_method, "BREW") == 0)
            {
                // see http://tools.ietf.org/html/rfc2324
                std::cout << "Status: 418 I'm a teapot" << std::endl;
            }
            else
            {
                std::cout << "Status: 405 Method Not Allowed" << std::endl;
            }
            //
            std::string const body("<html><head><title>Method Not Allowed</title></head><body><p>Sorry. We only support GET and POST.</p></body></html>");
            std::cout   << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                        << "Allow: GET, POST"                       << std::endl
                        << "Connection: close"                      << std::endl
                        << "Content-Type: text/html; charset=utf-8" << std::endl
                        << "Content-Length: " << body.length()      << std::endl
                        << "X-Powered-By: snapamanager.cgi"         << std::endl
                        << std::endl
                        << body;
            return false;
        }
    }

    // get the client IP address
    //
    char const * remote_addr(getenv("REMOTE_ADDR"));
    if(remote_addr == nullptr)
    {
        error("400 Bad Request", nullptr, "The REMOTE_ADDR parameter is not available.");
        return false;
    }

    // verify that this is a client we allow to use snapmanager.cgi
    //
    if(!f_config.contains("clients"))
    {
        error("403 Forbidden", "You are not allowed on this server.", "The clients=... parameter is undefined.");
        return false;
    }

    {
        snap_addr::addr const remote_address(std::string(remote_addr) + ":80", "tcp");
        std::string const client(f_config.contains("clients") ? f_config["clients"].toUtf8().data() : "");

        snap::snap_string_list const client_list(QString::fromUtf8(client.c_str()).split(',', QString::SkipEmptyParts));
        bool found(false);
        for(auto const & c : client_list)
        {
            snap_addr::addr const client_address((c + ":80").toUtf8().data(), "tcp");
            if(client_address == remote_address)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            error("403 Forbidden", "You are not allowed on this server.", ("Your remote address is " + remote_address.get_ipv4or6_string()).c_str());
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * http_host(getenv("HTTP_HOST"));
        if(http_host == nullptr)
        {
            error("400 Bad Request", "The host you want to connect to must be specified.", nullptr);
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("HTTP_HOST=")(http_host);
#endif
        if(tcp_client_server::is_ipv4(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv4 address.");
            std::cout   << "Status: 444 No Response"        << std::endl
                        << "Connection: close"              << std::endl
                        << "X-Powered-By: snapmanager.cgi"  << std::endl
                        << std::endl
                        ;
            snap::server::block_ip(remote_addr, "week");
            return false;
        }
        if(tcp_client_server::is_ipv6(http_host))
        {
            SNAP_LOG_ERROR("The host cannot be an IPv6 address.");
            std::cout   << "Status: 444 No Response"        << std::endl
                        << "Connection: close"              << std::endl
                        << "X-Powered-By: snapmanager.cgi"  << std::endl
                        << std::endl
                        ;
            snap::server::block_ip(remote_addr, "week");
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * request_uri(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_REQUEST_URI)));
        if(request_uri == nullptr)
        {
            // this should NEVER happen because without a path after the method
            // we probably do not have our snapmanager.cgi run anyway...
            //
            error("400 Bad Request", "The path to the page you want to read must be specified.", nullptr);
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("REQUEST_URI=")(request_uri);
#endif

        // if we do not receive this, somehow someone was able to access
        // snapmanager.cgi without specifying /cgi-bin/... which is not
        // correct
        //
        if(strncasecmp(request_uri, "/cgi-bin/", 9) != 0)
        {
            error("404 Page Not Found", "We could not find the page you were looking for.", "The REQUEST_URI cannot start with \"/cgi-bin/\".");
            snap::server::block_ip(remote_addr);
            return false;
        }

        // TBD: we could test <protocol>:// instead of specifically http
        //
        if(strncasecmp(request_uri, "http://", 7) == 0
        || strncasecmp(request_uri, "https://", 8) == 0)
        {
            // avoid proxy accesses
            error("404 Page Not Found", nullptr, "The REQUEST_URI cannot start with \"http[s]://\".");
            snap::server::block_ip(remote_addr);
            return false;
        }

		// TODO: move to snapserver because this could be the name of a legal page...
        if(strcasestr(request_uri, "phpmyadmin") != nullptr)
        {
            // block myPhpAdmin accessors
            error("410 Gone", "MySQL left.", nullptr);
            snap::server::block_ip(remote_addr, "year");
            return false;
        }
    }

    {
        // WARNING: do not use std::string because nullptr will crash
        //
        char const * user_agent(getenv(snap::get_name(snap::name_t::SNAP_NAME_CORE_HTTP_USER_AGENT)));
        if(user_agent == nullptr)
        {
            // we request an agent specification
            //
            error("400 Bad Request", "The accessing agent must be specified.", nullptr);
            snap::server::block_ip(remote_addr, "month");
            return false;
        }
#ifdef _DEBUG
        //SNAP_LOG_DEBUG("HTTP_USER_AGENT=")(request_uri);
#endif

        // left trim
        while(isspace(*user_agent))
        {
            ++user_agent;
        }

        // if we receive this, someone tried to directly access our
        // snapmanager.cgi, which will not work right so better
        // err immediately
        //
        if(*user_agent == '\0'
        || (*user_agent == '-' && user_agent[1] == '\0')
        || strcasestr(user_agent, "ZmEu") != nullptr)
        {
            // note that we consider "-" as empty for this test
            error("400 Bad Request", nullptr, "The agent string cannot be empty.");
            snap::server::block_ip(remote_addr, "month");
            return false;
        }
    }

    // success
    return true;
}


/** \brief Process one hit to snapmanager.cgi.
 *
 * This is the function that generates the HTML or AJAX reply to
 * the client.
 *
 * \return 0 if the process worked as expected, 1 otherwise.
 */
int manager_cgi::process()
{
    // WARNING: do not use std::string because nullptr will crash
    char const * request_method( getenv("REQUEST_METHOD") );
    if(request_method == nullptr)
    {
        // the method was already checked in verify(), before this
        // call so it should always be defined here...
        //
        SNAP_LOG_FATAL("Method not defined in REQUEST_METHOD.");
        std::string body("<html><head><title>Method Not Defined</title></head><body><p>Sorry. We only support GET and POST.</p></body></html>");
        std::cout   << "Status: 405 Method Not Defined"         << std::endl
                    << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                    << "Connection: close"                      << std::endl
                    << "Allow: GET, POST"                       << std::endl
                    << "Content-Type: text/html; charset=utf-8" << std::endl
                    << "Content-Length: " << body.length()      << std::endl
                    << "X-Powered-By: snapmanager.cgi"          << std::endl
                    << std::endl
                    << body;
        return false;
    }
#ifdef _DEBUG
    SNAP_LOG_DEBUG("processing request_method=")(request_method);
#endif

    // retrieve the query string, that's all we use in this one (i.e.
    // at this point we ignore the path)
    //
    // TODO: add support to make sure the administrator uses HTTPS?
    //       (this can be done in Apache2)
    //
    char const * query_string(getenv("QUERY_STRING"));
    if(query_string != nullptr)
    {
        f_uri.set_query_string(QString::fromUtf8(query_string));
    }

    QDomDocument doc;
    QDomElement root(doc.createElement("manager"));
    doc.appendChild(root);

    generate_content(doc, root);

//SNAP_LOG_WARNING("Doc = [")(doc.toString())("]");

    snap::xslt x;
    x.set_xsl_from_file(f_config["stylesheet"]); // setup the .xsl file
    x.set_document(doc);
    QString const body("<!DOCTYPE html>" + x.evaluate_to_string());

    //std::string body("<html><head><title>Snap Manager</title></head><body><p>...TODO...</p></body></html>");
    std::cout   //<< "Status: 200 OK"                         << std::endl
                << "Expires: Sat, 1 Jan 2000 00:00:00 GMT"  << std::endl
                << "Connection: close"                      << std::endl
                << "Content-Type: text/html; charset=utf-8" << std::endl
                << "Content-Length: " << body.length()      << std::endl
                << "X-Powered-By: snapmanager.cgi"          << std::endl
                << std::endl
                << body;

    return 0;
}


/** \brief Generate the body of the page.
 *
 * This function checks the various query strings passed to the manager_cgi
 * and depending on those, generates a page.
 */
void manager_cgi::generate_content(QDomDocument doc, QDomElement root)
{
    QDomElement output(doc.createElement("output"));
    root.appendChild(output);

    QString const function(f_uri.query_option("function"));

    // is a host name specified?
    // if so then the function / page has to be applied to that specific host
    //
    if(f_uri.has_query_option("host"))
    {
        // the function is to be applied to that specific host
        //
        QString const host(f_uri.query_option("host"));
        if(!function.isEmpty())
        {
            // apply a function on that specific host
            //
        }
        else
        {
            // no function + specific host, show a complete status from
            // that host
            //
            get_host_status(doc, output, host);
        }
    }
    else
    {
        // no host specified, if there is a function it has to be applied
        // to all computers, otherwise show the list of computers and their
        // basic status
        //
        if(!function.isEmpty())
        {
            // execute function on all computers
            //
        }
        else
        {
            // "just" a cluster status...
            //
            get_cluster_status(doc, output);
        }
    }

}


void manager_cgi::get_host_status(QDomDocument doc, QDomElement output, QString const host)
{
    // define the path to the .db file
    //
    QString filename;
    if(!f_config.contains("data_path"))
    {
        filename = host + ".db";
    }
    else
    {
        filename = QString("%1/%2.db")
                    .arg(f_config["data_path"])
                    .arg(host);
    }

    // create, open, read the file
    //
    server_status file(filename);
    if(!file.read_all())
    {
        // TODO: add error info in output
        return;
    }

    // output/table
    QDomElement table(doc.createElement("table"));
    output.appendChild(table);

    table.setAttribute("class", "server-status");

    // output/table/tr
    QDomElement tr(doc.createElement("tr"));
    table.appendChild(tr);

    // output/table/tr/th[1]
    QDomElement th(doc.createElement("th"));
    tr.appendChild(th);

        QDomText text(doc.createTextNode("Name"));
        th.appendChild(text);

    // output/table/tr/th[2]
    th = doc.createElement("th");
    tr.appendChild(th);

        text = doc.createTextNode("Value");
        th.appendChild(text);

    // read each name/value pair
    //
    QString name;
    QString value;
    snap_manager::status_t::map_t const & statuses(file.get_statuses());
    for(auto const & s : statuses)
    {
        if(s.second.get_plugin_name() == "header")
        {
            continue;
        }

        // output/table/tr
        tr = doc.createElement("tr");
        table.appendChild(tr);

        switch(s.second.get_state())
        {
        case snap_manager::status_t::state_t::STATUS_STATE_WARNING:
            tr.setAttribute("class", "warnings");
            break;

        case snap_manager::status_t::state_t::STATUS_STATE_ERROR:
        case snap_manager::status_t::state_t::STATUS_STATE_FATAL_ERROR:
            tr.setAttribute("class", "errors");
            break;

        default:
            // do nothing otherwise
            break;
        }

            // output/table/tr/td[1]
            QDomElement td(doc.createElement("td"));
            tr.appendChild(td);

                //// output/table/td[1]/a
                //QDomElement anchor(doc.createElement("a"));
                //td.appendChild(anchor);

                //QString const path(dir.gl_pathv[idx]);
                //int basename_pos(path.lastIndexOf('/'));
                //if(basename_pos < 0)
                //{
                //    // this should not happen, although it is perfectly
                //    // possible that the administrator used "" as the
                //    // path where statuses should be saved.
                //    //
                //    basename_pos = 0;
                //}
                //QString const host(path.mid(basename_pos + 1, path.length() - basename_pos - 1 - 3));

                //anchor.setAttribute("href", QString("?host=%1").arg(host));

                text = doc.createTextNode(s.second.get_field_name());
                td.appendChild(text);

            // output/table/tr/td[2]
            td = doc.createElement("td");
            tr.appendChild(td);

                text = doc.createTextNode(s.second.get_value());
                td.appendChild(text);
    }
}


void manager_cgi::get_cluster_status(QDomDocument doc, QDomElement output)
{
    // TODO: make use of the list_of_servers() function instead of having
    //       our own copy of the glob() call
    //
    struct err_callback
    {
        static int func(const char * epath, int eerrno)
        {
            SNAP_LOG_ERROR("an error occurred while reading directory under \"")
                          (epath)
                          ("\". Got error: ")
                          (eerrno)
                          (", ")
                          (strerror(eerrno))
                          (".");

            // do not abort on a directory read error...
            return 0;
        }
    };

    std::string pattern;
    if(!f_config.contains("data_path"))
    {
        pattern = "*.db";
    }
    else
    {
        pattern = (f_config["data_path"] + "/*.db").toUtf8().data();
    }
    glob_t dir = glob_t();
    int const r(glob(pattern.c_str(), GLOB_NOESCAPE, err_callback::func, &dir));
    if(r != 0)
    {
        //globfree(&dir); -- needed on error?

        // do nothing when errors occur
        //
        switch(r)
        {
        case GLOB_NOSPACE:
            SNAP_LOG_ERROR("glob() did not have enough memory to alllocate its buffers.");
            break;

        case GLOB_ABORTED:
            SNAP_LOG_ERROR("glob() was aborted after a read error.");
            break;

        case GLOB_NOMATCH:
            SNAP_LOG_ERROR("glob() could not find any status information.");
            break;

        default:
            SNAP_LOG_ERROR("unknown glob() error code: ")(r)(".");
            break;

        }
        QDomText text(doc.createTextNode("An error occurred while reading status data. Please check your snapmanagercgi.log file for more information."));
        output.appendChild(text);
        return;
    }

    // output/table
    QDomElement table(doc.createElement("table"));
    output.appendChild(table);

    table.setAttribute("class", "cluster-status");

    // output/table/tr
    QDomElement tr(doc.createElement("tr"));
    table.appendChild(tr);

    // output/table/tr/th[1]
    QDomElement th(doc.createElement("th"));
    tr.appendChild(th);

        QDomText text(doc.createTextNode("Host"));
        th.appendChild(text);

    // output/table/tr/th[2]
    th = doc.createElement("th");
    tr.appendChild(th);

        text = doc.createTextNode("IP");
        th.appendChild(text);

    // output/table/tr/th[3]
    th = doc.createElement("th");
    tr.appendChild(th);

        text = doc.createTextNode("Status");
        th.appendChild(text);

    // output/table/tr/th[4]
    th = doc.createElement("th");
    tr.appendChild(th);

        text = doc.createTextNode("Err/War");
        th.appendChild(text);

    bool has_error(false);
    for(size_t idx(0); idx < dir.gl_pathc; ++idx)
    {
        server_status file(dir.gl_pathv[idx]);
        if(file.read_header())
        {
            // we got what looks like a valid status file
            //
            QString const status(file.get_field("header", "status"));
            if(!status.isEmpty())
            {
                // get number of errors
                //
                size_t error_count(0);
                if(file.get_field_state("header", "errors") != snap_manager::status_t::state_t::STATUS_STATE_UNDEFINED)
                {
                    QString const errors(file.get_field("header", "errors"));
                    error_count = errors.toLongLong();
                }

                // get number of warnings
                //
                size_t warning_count(0);
                if(file.get_field_state("header", "warnings") != snap_manager::status_t::state_t::STATUS_STATE_UNDEFINED)
                {
                    QString const warnings(file.get_field("header", "warnings"));
                    warning_count = warnings.toLongLong();
                }

                // output/table/tr
                tr = doc.createElement("tr");
                table.appendChild(tr);

                snap::snap_string_list row_class;
                if(error_count != 0)
                {
                    row_class << "errors";
                }
                if(warning_count != 0)
                {
                    row_class << "warnings";
                }
                if(status == "down" || status == "unknown")
                {
                    ++error_count;  // we consider this an error, so do +1 here
                    row_class << "down";
                }
                if(!row_class.isEmpty())
                {
                    tr.setAttribute("class", row_class.join(" "));
                }

                // output/table/tr/td[1]
                QDomElement td(doc.createElement("td"));
                tr.appendChild(td);

                    // output/table/tr/td[1]/a
                    QDomElement anchor(doc.createElement("a"));
                    td.appendChild(anchor);

                    QString const path(dir.gl_pathv[idx]);
                    int basename_pos(path.lastIndexOf('/'));
                    // basename_pos will be -1 which is what you would
                    // expect to get for the mid() call below!
                    //if(basename_pos < 0)
                    //{
                    //    // this should not happen, although it is perfectly
                    //    // possible that the administrator used "" as the
                    //    // path where statuses should be saved.
                    //    //
                    //    basename_pos = 0;
                    //}
                    QString const host(path.mid(basename_pos + 1, path.length() - basename_pos - 1 - 3));

                    anchor.setAttribute("href", QString("?host=%1").arg(host));

                        // output/table/tr/td[1]/<text>
                        text = doc.createTextNode(host);
                        anchor.appendChild(text);

                // output/table/tr/td[2]
                td = doc.createElement("td");
                tr.appendChild(td);

                    // output/table/tr/td[2]/<text>
                    text = doc.createTextNode(file.get_field("header", "ip"));
                    td.appendChild(text);

                // output/table/tr/td[3]
                td = doc.createElement("td");
                tr.appendChild(td);

                    // output/table/tr/td[3]/<text>
                    text = doc.createTextNode(status);
                    td.appendChild(text);

                // output/table/tr/td[4]
                td = doc.createElement("td");
                tr.appendChild(td);

                    // output/table/tr/td[4]/<text>
                    text = doc.createTextNode(QString("%1/%2").arg(error_count).arg(warning_count));
                    td.appendChild(text);
            }

            if(file.has_error())
            {
                has_error = true;
            }
        }
        else
        {
            has_error = true;
        }
    }

    if(has_error)
    {
        // output/p
        QDomElement p(doc.createElement("p"));
        output.appendChild(p);

        p.setAttribute("class", "error");

            text = doc.createTextNode("Errors occurred while reading the status. Please check your snapmanagercgi.log file for details.");
            p.appendChild(text);
    }

    // free that memory (not useful in a CGI script though)
    globfree(&dir);
}



}
// namespace snap_manager
// vim: ts=4 sw=4 et
