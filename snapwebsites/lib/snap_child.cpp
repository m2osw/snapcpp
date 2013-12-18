// Snap Websites Server -- snap websites serving children
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

#include "snap_child.h"
#include "snap_uri.h"
#include "not_reached.h"
#include "snapwebsites.h"
#include "plugins.h"
#include "log.h"
#include "qlockfile.h"
#include "http_strings.h"
#include "compression.h"
#include <memory>
#include <wait.h>
#include <errno.h>
#include <libtld/tld.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <QDateTime>
#include <QtSerialization/QSerialization.h>
#include <QCoreApplication>
#include "poison.h"

namespace snap
{




/** \class snap_child
 * \brief Child process class.
 *
 * This class handles child objects that process queries from the Snap
 * CGI tool.
 *
 * The children appear in the Snap Server and themselves. The server is
 * the parent that handles the lifetime of the child. The parent also
 * holds the child process identifier and it waits on the child for its
 * death.
 *
 * The child itself has its f_child_pid set to zero.
 *
 * Some of the functions will react with an error if called from the
 * wrong process (i.e. parent calling a child process function and vice
 * versa.)
 */



/** \brief Initialize a child process.
 *
 * This function initializes a child process object. At this point it
 * marked as a parent process child instance (i.e. calling child process
 * functions will generate an error.)
 *
 * Whenever the parent Snap Server receives a connect from the Snap CGI
 * tool, it calls the process() function which creates the child and starts
 * processing the TCP request.
 *
 * Note that at this point there is not communication between the parent
 * and child processes other than the child process death that the parent
 * acknowledge at some point.
 */
snap_child::snap_child(server_pointer_t s)
    : f_start_date(0)
    , f_server(s)
    //, f_cassandra() -- auto-init
    //, f_context() -- auto-init
    //, f_site_table() -- auto-init
    //, f_new_content(false) -- auto-init
    //, f_is_child(false) -- auto-init
    , f_child_pid(0)
    , f_socket(-1)
    //, f_env() -- auto-init
    //, f_post() -- auto-init
    //, f_browser_cookies() -- auto-init
    //, f_has_post(false) -- auto-init
    //, f_fixed_server_protocol() -- auto-init
    //, f_uri() -- auto-init
    //, f_domain_key() -- auto-init
    //, f_website_key() -- auto-init
    //, f_site_key() -- auto-init
    //, f_site_key_with_slash() -- auto-init
    //, f_original_site_key() -- auto-init
    //, f_output() -- auto-init
    //, f_header() -- auto-init
    //, f_cookies() -- auto-init
{
}

/** \brief Clean up a child process.
 *
 * This function cleans up a child process. For the parent this means waiting
 * on the child, assuming that the child process was successfully started.
 * This also means that the function may block until the child dies...
 */
snap_child::~snap_child()
{
    // detach or wait till it dies?
    if(f_child_pid > 0)
    {
        int status;
        wait(&status);
    }
    //else
    //{ // this is the child process deleting itself
    //    ...
    //    if(f_socket != -1)
    //    {
    //        // this is automatic anyway (we're in Unix)
    //        // and if not already cleared, we've got more serious problems
    //        // (see the process() function for more info)
    //        close(f_socket);
    //    }
    //}
}

/** \brief Reset the start date to now.
 *
 * This function is called by the processing functions to reset the
 * start date. This is important because child objects may be reused
 * multiple times instead of allocated and deallocated by the server.
 */
void snap_child::init_start_date()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    f_start_date = static_cast<int64_t>(tv.tv_sec) * static_cast<int64_t>(1000000)
                 + static_cast<int64_t>(tv.tv_usec);
}

/** \brief Process a request from the Snap CGI tool.
 *
 * The process function accepts a socket that was just connected.
 * Only the parent (Snap Server) can call this function. Assuming
 * that (1) the parent is calling and (2) that this snap_child
 * object is not already in use, then the function forks a new
 * process (the child).
 *
 * The parent acknowledge by saving the new process identifier
 * and closing its copy of the TCP socket.
 *
 * If the fork() call fails (returning -1) then the parent process
 * writes an HTTP error in the socket (503 Service Unavailable).
 *
 * \param[in] socket  The socket connecting this child to the client.
 *
 * \return true if the child process was successfully created.
 */
bool snap_child::process(int socket)
{
    if(f_is_child)
    {
        // this is a bug! die() on the spot
        // (here we ARE in the child process!)
        die(HTTP_CODE_SERVICE_UNAVAILABLE, "Server Bug", "Your Snap! server detected a serious problem. Please check your logs for more information.", "snap_child::process() was called from the child process.");
        return false;
    }

    if(f_child_pid != 0)
    {
        // this is a bug!
        // WARNING: At this point we CANNOT call the die() function
        //          (we're not the child and have the wrong socket)
        SNAP_LOG_FATAL("BUG: snap_child::process() called when the process is still in use.");
        return false;
    }

// to avoid the fork use 1 on the next line
// (much easier to debug a crashing problem in a snap child!)
#if 0
    pid_t p = 0;
#else
    pid_t p = fork();
#endif
    if(p != 0)
    {
        // parent process
        if(p == -1)
        {
            // WARNING: At this point we CANNOT call the die() function
            //          (we're not the child and have the wrong socket)
            SNAP_LOG_FATAL("snap_child::process() could not create child process, dropping connection.");
            return false;
        }

        // save the process identifier since it worked
        f_child_pid = p;

        // socket is now the responsibility of the child process
        // the accept() call in the parent will close it though
        return true;
    }

    // on fork() we lose the configuration so we have to reload it
    logging::reconfigure();

    init_start_date();

    // child process
    f_is_child = true;
    f_socket = socket;

    read_environment();         // environment to QMap<>
    setup_uri();                // the raw URI

    // now we connect to the DB
    // move all possible work that does not required the DB before
    // this line so we avoid a network connection altogether
    connect_cassandra();

    canonicalize_domain();      // using the URI, find the domain core::rules and start the canonalization process
    canonicalize_website();     // using the canonicalized domain, find the website core::rules and continue the canonalization process

    // check whether this website has a redirect and apply it if necessary
    // (not a full 301, just show site B instead of site A)
    site_redirect();

    // save the start date as a variable so all the plugins have access
    // to it as any other variable
    f_uri.set_option("start_date", QString("%1").arg(f_start_date));

    // start the plugins and there initialization
    init_plugins();

    // finally, "execute" the page being accessed
    execute();

    // we could delete ourselves but really only the socket is an
    // object that needs to get cleaned up properly and it is done
    // in the exit() function.
    //delete this;
    exit(0);
    NOTREACHED();
    return false;
}


/** \brief Execute the backend processes after initialization.
 *
 * This function is somewhat similar to the process() function. It is used
 * to ready the server and then run the backend processes by sending a
 * signal.
 */
void snap_child::backend()
{
    init_start_date();

    f_is_child = true;
    f_child_pid = getpid();
    f_socket = -1;

    connect_cassandra();

    QString uri(f_server->get_parameter("__BACKEND_URI"));
    if(!uri.isEmpty())
    {
        process_backend_uri(uri);
    }
    else
    {
        QString table_name(get_name(SNAP_NAME_SITES));
        QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
        if(table.isNull())
        {
            // the whole table is still empty
            return;
        }

        // if a site exists then it has a "core::last_updated" entry
        QSharedPointer<QtCassandra::QCassandraColumnNamePredicate> column_predicate(new QtCassandra::QCassandraColumnNamePredicate);
        column_predicate->addColumnName(get_name(SNAP_NAME_CORE_LAST_UPDATED));
        QtCassandra::QCassandraRowPredicate row_predicate;
        row_predicate.setColumnPredicate(column_predicate);
        for(;;)
        {
            table->clearCache();
            uint32_t count(table->readRows(row_predicate));
            if(count == 0)
            {
                // we reached the end of the whole table
                break;
            }
            const QtCassandra::QCassandraRows& r(table->rows());
            for(QtCassandra::QCassandraRows::const_iterator o(r.begin());
                    o != r.end(); ++o)
            {
                QString key(QString::fromUtf8(o.key().data()));
                process_backend_uri(key);
            }
        }
    }
}


/** \brief Process a backend request on the specified URI.
 *
 * This function is called with each URI that needs to be processed by
 * the backend processes. It creates a child process that will allow
 * the Cassandra data to not be shared between all instances. Instead
 * each instance reads data and then drops it as the process ends.
 * Since the parent blocks until the child is done, the Cassandra library
 * is still only used by a single process at a time thus we avoid
 * potential conflicts reading/writing on the same network connection
 * (since the child inherits the parents Cassandra connection.)
 *
 * \note
 * Note that the child is created from Cassandra, the plugins, the
 * f_uri and all the resulting keys... so we gain an environment
 * very similar to what we get in the server with Apache.
 *
 * \note
 * If that site has an internal redirect then no processing is
 * performed because otherwise the destination would be processed
 * twice in the end.
 *
 * \param[in] uri  The URI of the site to be checked.
 */
void snap_child::process_backend_uri(const QString& uri)
{
    // create a child process so the data between sites doesn't get
    // shared (also the Cassandra data would remain in memory increasing
    // the foot print each time we run a new website,) but the worst
    // are the plugins; we can request a plugin to be unloaded but
    // frankly the system is not very well written to handle that case.
    pid_t p = fork();
    if(p != 0)
    {
        // parent process
        if(p == -1)
        {
            SNAP_LOG_FATAL("snap_child::process_backend_uri() could not create a child process.");
            // we don't try again, we just abandon the whole process
            exit(1);
        }
        // block until child is done
        int status;
        wait(&status);
        // TODO: check status?
        return;
    }

    f_uri.set_uri(uri);

    // child process initialization
    //connect_cassandra(); -- this is already done in backend()...

    // process the f_uri parameter
    canonicalize_domain();
    canonicalize_website();
    site_redirect();
    if(f_site_key != f_original_site_key)
    {
        return;
    }
    // same as in normal server process -- should it change for each iteration?
    // (i.e. we're likely to run the backend process for each website of this
    // Cassandra instance!)
    f_uri.set_option("start_date", QString("%1").arg(f_start_date));

    init_plugins();

    QString action(f_server->get_parameter("__BACKEND_ACTION"));
    if(!action.isEmpty())
    {
        server::backend_action_map_t actions;
        f_server->register_backend_action(actions);
        if(actions.contains(action))
        {
            // this is a valid action, execute the corresponding function!
            actions[action]->on_backend_action(action);
        }
        else if(action == "list")
        {
            // the user wants to know what's supported
            // we add a "list" entry so it appears in the right place
            class fake : public server::backend_action
            {
                virtual void on_backend_action(const QString& /*action*/)
                {
                }
            };
            fake foo;
            actions["list"] = &foo;
            for(server::backend_action_map_t::const_iterator it(actions.begin()); it != actions.end(); ++it)
            {
                std::cout << it.key().toUtf8().data() << std::endl;
            }
        }
        else
        {
            std::cerr << "error: unknown action \"" << action.toUtf8().data() << "\"" << std::endl;
            exit(1);
        }
    }
    else
    {
        f_server->backend_process();
    }
}


/** \brief Check the status of the child process.
 *
 * This function checks whether the child is still running or not.
 * The function returns the current status such as running,
 * or ready (to process a request.)
 *
 * The child process is not expected to call this function. It knows
 * it is running if it can anyway.
 *
 * The parent uses the wait() instruction to check the current status
 * if the process is running (f_child_pid is not zero.)
 *
 * \return The status of the child.
 */
snap_child::status_t snap_child::check_status()
{
    if(f_is_child)
    {
        // XXX -- call die() instead
        SNAP_LOG_FATAL("snap_child::check_status() was called from the child process.");
        return SNAP_CHILD_STATUS_RUNNING;
    }

    if(f_child_pid != 0)
    {
        int status;
        pid_t r = waitpid(f_child_pid, &status, WNOHANG);
        if(r == static_cast<pid_t>(-1))
        {
            int e(errno);
            SNAP_LOG_FATAL("a waitpid() returned an error (")(e)(")");
        }
        else if(r == f_child_pid)
        {
            // the status of our child changed
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            if(WIFEXITED(status))
            {
                // stopped with exit() or return in main()
                f_child_pid = 0;
            }
            else if(WIFSIGNALED(status))
            {
                // stopped because of a signal
                SNAP_LOG_FATAL("child process ")(f_child_pid)(" exited after it received signal #")(WTERMSIG(status));
                f_child_pid = 0;
            }
#pragma GCC diagnostic pop
            // other statuses are ignored
        }
    }

    return f_child_pid == 0 ? SNAP_CHILD_STATUS_READY : SNAP_CHILD_STATUS_RUNNING;
}


/** \brief Read the command and eventually the environment sent by snap.cgi
 *
 * The socket starts with a one line command. The command may be followed
 * by additional entries such as the Apache environment when the Snap CGI
 * connects to us.
 *
 * When the environment is defined, it is saved in a map so all the other
 * functions can later retrieve those values from the child. Note that at
 * this point the script does not tweak that data.
 *
 * To make sure that the entire environment is sent, snap.cgi starts the
 * feed with "#START\n" and terminate it with "#END\n".
 *
 * Note that unless we are receiving the Apache environment from the snap.cgi
 * tool, we do NOT return. This is important because when returning we start
 * generating a web page which is not what we want for the other instructions
 * such as the #INFO.
 *
 * \section commands Understood Commands
 *
 * \li #START
 *
 * Start passing the environment to the server.
 *
 * \li #INFO
 *
 * Request for information about the server. The result is an environment
 * like variable/value pairs. Mainly versions are returned in that buffer.
 * Use the #STATS for statistics information.
 *
 * \li #STATS
 *
 * Request for statistics about this server instance. The result is an
 * environment like variable/value pairs. This command generates values
 * such as the total number of requests received, the number of children
 * currently running, etc.
 */
void snap_child::read_environment()
{
    class read_env
    {
    public:
        read_env(snap_child *snap, int socket, environment_map_t& env, environment_map_t& browser_cookies, environment_map_t& post, post_file_map_t& files)
            : f_snap(snap)
            , f_socket(socket)
            //, f_unget('\0') -- auto-init
            //, f_running(true) -- auto-init
            //, f_started(false) -- auto-init
            , f_env(env)
            , f_browser_cookies(browser_cookies)
            //, f_name("") -- auto-init
            //, f_value("") -- auto-init
            //, f_has_post(false) -- auto-init
            , f_post(post)
            , f_files(files)
            //, f_post_first() -- auto-init
            //, f_post_header() -- auto-init
            //, f_post_line() -- auto-init
            //, f_post_content() -- auto-init
            //, f_boundary() -- auto-init
            //, f_end_boundary() -- auto-init
            //, f_post_environment() -- auto-init
            //, f_post_index(0) -- auto-init
        {
        }

        void die(QString const& details) const
        {
            f_snap->die(HTTP_CODE_SERVICE_UNAVAILABLE, "",
                "Unstable network connection",
                QString("an error occured while reading the environment from the socket in the server child process (%1).").arg(details));
            NOTREACHED();
        }

        char getc() const
        {
            char c;

            //if(f_unget != '\0')
            //{
            //    c = f_unget;
            //    f_unget = '\0';
            //    return c;
            //}

            // this read blocks, so we read just 1 char. because we
            // want to stop calling read() as soon as possible (otherwise
            // we'd be blocked here forever)
            if(read(f_socket, &c, 1) != 1)
            {
                int e(errno);
                die(QString("I/O error, errno: %1").arg(e));
                NOTREACHED();
            }
            return c;
        }

        //void ungetc(char c)
        //{
        //    if(f_unget != '\0')
        //    {
        //        die("too many unget() called");
        //        NOTREACHED();
        //    }
        //    f_unget = c;
        //}

        void start_process()
        {
            // #INFO
            if(f_name == "#INFO")
            {
                f_snap->snap_info();
                NOTREACHED();
            }

            // #STATS
            if(f_name == "#STATS")
            {
                f_snap->snap_statistics();
                NOTREACHED();
            }

            // #START
            if(f_name != "#START")
            {
                die("#START or other supported command missing.");
                NOTREACHED();
            }
            // TODO add support for a version: #START=1.2
            //      so that way the server can cleanly "break" if the
            //      snap.cgi version is not compatible

            f_started = true;
            f_name.clear();
            f_value.clear();
        }

        void process_post_variable()
        {
            // here we have a set of post environment variables (header)
            // and the f_post_content which represents the value of the field
            //
            // Content-Disposition: form-data; name="field-name"
            // Content-Type: image/gif
            // Content-Transfer-Encoding: binary
            //
            // The Content-Type cannot be used with plain variables. We
            // distinguish plain variables from files as the
            // Content-Disposition includes a filename="..." parameter.
            //
            if(!f_post_environment.contains("CONTENT-DISPOSITION"))
            {
                die("multipart posts must have a Content-Disposition header to be considered valid.");
                NOTREACHED();
            }
            // TODO: verify and if necessary fix this as the ';' could I think
            //       appear in a string; looking at the docs, I'm not too sure
            //       but it looks like we would need to support the
            //       extended-value and extended-other-values as defined in
            //       http://tools.ietf.org/html/rfc2184
            QStringList disposition(f_post_environment["CONTENT-DISPOSITION"].split(";"));
            if(disposition.size() < 2)
            {
                die(QString("multipart posts Content-Disposition must at least include \"form-data\" and a name parameter, \"%1\" is not valid.")
                            .arg(f_post_environment["CONTENT-DISPOSITION"]));
                NOTREACHED();
            }
            if(disposition[0].trimmed() != "form-data")
            {
                // not happy if we don't get form-data parts
                die(QString("multipart posts Content-Disposition must be a \"form-data\", \"%1\" is not valid.")
                            .arg(f_post_environment["CONTENT-DISPOSITION"]));
                NOTREACHED();
            }
            // retrieve all the parameters, then keep those we want to keep
            int const max(disposition.size());
            environment_map_t params;
            for(int i(1); i < max; ++i)
            {
                // each parameter is name=<value>
                QStringList nv(disposition[i].split('='));
                if(nv.size() != 2)
                {
                    die(QString("parameter %1 in this multipart posts Content-Disposition does not include an equal character so \"%1\" is not valid.")
                                .arg(i)
                                .arg(f_post_environment["CONTENT-DISPOSITION"]));
                    NOTREACHED();
                }
                nv[0] = nv[0].trimmed().toLower(); // case insensitive
                nv[1] = nv[1].trimmed();
                if(nv[1].startsWith("\"")
                && nv[1].endsWith("\""))
                {
                    nv[1] = nv[1].mid(1, nv[1].length() - 2);
                }
                params[nv[0]] = nv[1];
            }
            if(!params.contains("name"))
            {
                die(QString("multipart posts Content-Disposition must include a name=\"...\" parameter, \"%1\" is not valid.")
                            .arg(f_post_environment["CONTENT-DISPOSITION"]));
                NOTREACHED();
            }
            f_name = params["name"];
            if(params.contains("filename"))
            {
                // make sure the filename is unique otherwise we'd overwrite
                // the previous file with the same name...
                QString filename(params["filename"]);

                // this is a file so we want to save it in the f_file and
                // not in the f_post although we do create an f_post entry
                // with the filename
                if(f_post.contains(f_name))
                {
                    die(QString("multipart post variable \"%1\" defined twice")
                                .arg(f_name));
                    NOTREACHED();
                }
                f_post[f_name] = filename;

                post_file_t& file(f_files[f_name]);
                file.set_name(f_name);
                file.set_filename(filename);
                ++f_post_index; // 1-based
                file.set_index(f_post_index);
                file.set_data(f_post_content);
                if(params.contains("creation-date"))
                {
                    file.set_creation_time(string822_to_date(params["creation-date"]));
                }
                if(params.contains("modification-date"))
                {
                    file.set_modification_time(string822_to_date(params["modification-date"]));
                }
                // Content-Type is actually expected on this side
                if(f_post_environment.contains("CONTENT-TYPE"))
                {
                    file.set_mime_type(f_post_environment["CONTENT-TYPE"]);
                }
                if(params.contains("modification-date"))
                {
                    file.set_modification_time(string822_to_date(params["modification-date"]));
                }
//fprintf(stderr, " f_files[\"%s\"] = \"...\" (MIME: %s)\n", filename.toUtf8().data(), file.get_mime_type().toUtf8().data());
            }
            else
            {
                // this is a simple parameter
                if(f_post_environment.contains("CONTENT-TYPE"))
                {
                    // XXX accept a few valid types? it should not be necessary...
                    // the character encoding is defined as the form, page,
                    // or UTF-8 encoding; Content-Type not permitted here!
                    die(QString("multipart posts Content-Type is not allowed with simple parameters."));
                    NOTREACHED();
                }
                // TODO verify that the content of a post just needs to be
                //      decoded or whether it already is UTF-8 as required
                //      to be saved in f_post
                if(f_post_content.right(2) == "\r\n")
                {
                    f_post_content.resize(f_post_content.size() - 2);
                }
                else if(f_post_content.right(1) == "\n"
                     || f_post_content.right(1) == "\r")
                {
                    f_post_content.resize(f_post_content.size() - 1);
                }
                if(f_post.contains(f_name))
                {
                    die(QString("multipart post variable \"%1\" defined twice")
                                .arg(f_name));
                    NOTREACHED();
                }
                f_post[f_name] = f_post_content;//snap_uri::urldecode(f_post_content, true);
//fprintf(stderr, " f_post[\"%s\"] = \"%s\"\n", f_name.toUtf8().data(), f_post_content.data());
            }
        }

        bool process_post_line()
        {
            // found a marker?
            if(f_post_line.length() >= f_boundary.length())
            {
                if(f_post_line == f_end_boundary)
                {
                    if(f_post_first)
                    {
                        die("got end boundary without a start");
                        NOTREACHED();
                    }
                    process_post_variable();
                    return true;
                }

                if(f_post_line == f_boundary)
                {
                    // got the first boundary yet?
                    if(f_post_first)
                    {
                        // we got the first boundary
                        f_post_first = false;
                        return false;
                    }
                    process_post_variable();

                    // on next line, we're reading a new header
                    f_post_header = true;

                    // we're done with those in this iteration
                    f_post_environment.clear();
                    f_post_content.clear();
                    return false;
                }
            }

            if(f_post_first)
            {
                die("the first POST boundary is missing.");
                NOTREACHED();
            }

            if(f_post_header)
            {
                if(f_post_line.isEmpty() || (f_post_line.size() == 1 && f_post_line.at(0) == '\r'))
                {
                    // end of the header
                    f_post_header = false;
                    return false;
                }

                // we got a header (Blah: value)
                QString line(f_post_line);
//printf(" ++ header line [\n%s\n] %d\n", line.trimmed().toUtf8().data(), line.size());
                if(isspace(f_post_line[0]))
                {
                    // continuation of the previous header, concatenate
                    f_post_environment[f_name] += " " + line.trimmed();
                }
                else
                {
                    // new header
                    int p(line.indexOf(':'));
                    if(p == -1)
                    {
                        die("invalid header variable name/value pair, no ':' found.");
                        NOTREACHED();
                    }
                    // render name case insensitive
                    f_name = line.mid(0, p).trimmed().toUpper();
                    // TODO: verify that f_name is a valid header name
                    f_post_environment[f_name] = line.mid(p + 1).trimmed();
                }
            }
            else
            {
                // this is content for the current variable
                f_post_content += f_post_line;
                f_post_content += '\n'; // the '\n' was not added to f_post_line
            }

            return false;
        }

        void process_post()
        {
            // one POST per request!
            if(f_has_post)
            {
                die("at most 1 #POST is accepted in the environment.");
                NOTREACHED();
            }
            f_has_post = true;

            if(!f_env.contains("CONTENT_TYPE")
            || !f_env["CONTENT_TYPE"].startsWith("multipart/form-data"))
            {
                // standard post, just return and let the main loop
                // handle the name/value pairs
                return;
            }

            // multi-part posts require special handling
            // (i.e. these are not simple VAR=VALUE)
            //
            // the POST is going to be multiple lines with
            // \r characters included! We read then all
            // up to the closing boundary
            //
            // Example of such a variable:
            // CONTENT_TYPE=multipart/form-data; boundary=---------5767747
            //
            // IMPORTANT NOTE:
            // Sub-parts are NOT supported in HTML POST messages. This is
            // clearly mentioned in HTML5 documentations:
            // http://www.w3.org/html/wg/drafts/html/master/forms.html#multipart-form-data

            // 1. Get the main boundary from the CONTENT_TYPE
            QStringList content_info(f_env["CONTENT_TYPE"].split(';'));
            QString boundary;
            int const max(content_info.size());
            for(int i(1); i < max; ++i)
            {
                QString param(content_info[i].trimmed());
                if(param.startsWith("boundary="))
                {
                    boundary = param.mid(9).trimmed();
                    break;
                }
            }
            if(boundary.isEmpty())
            {
                die("multipart POST does not include a valid boundary.");
                NOTREACHED();
            }
            f_boundary.append(("--" + boundary).toAscii());
            f_end_boundary = f_boundary;
            f_end_boundary.append("--\r", 3);
            f_boundary += '\r';

            for(;;)
            {
                char c(getc());
                if(c == '\n')
                {
                    if(process_post_line())
                    {
                        return;
                    }
                    f_post_line.clear();
                }
                else
                {
                    f_post_line += c;
                }
            }
        }

        void process_line()
        {
            // not started yet? check low level commands then
            if(!f_started)
            {
                start_process();
                return;
            }

            // got to the end?
            if(f_name == "#END")
            {
                f_running = false;
                return;
            }

            // got a POST?
            if(f_name == "#POST")
            {
                process_post();
                return;
            }

            if(f_name.isEmpty())
            {
                die("empty lines are not accepted in the child environment.");
                NOTREACHED();
            }
            if(f_has_post)
            {
                f_post[f_name] = snap_uri::urldecode(f_value, true);
//fprintf(stderr, "(simple) f_post[\"%s\"] = \"%s\" (\"%s\");\n", f_name.toUtf8().data(), f_value.toUtf8().data(), f_post[f_name].toUtf8().data());
            }
            else
            {
                if(f_name == "HTTP_COOKIE")
                {
                    // special case
                    QStringList cookies(f_value.split(';', QString::SkipEmptyParts));
                    int max(cookies.size());
                    for(int i(0); i < max; ++i)
                    {
                        QString name_value(cookies[i]);
                        QStringList nv(name_value.trimmed().split('=', QString::SkipEmptyParts));
                        if(nv.size() == 2)
                        {
                            // XXX check with other systems to see
                            //     whether urldecode() is indeed
                            //     necessary here
                            QString cookie_name(snap_uri::urldecode(nv[0], true));
                            QString cookie_value(snap_uri::urldecode(nv[1], true));
                            if(f_browser_cookies.contains(cookie_name))
                            {
                                die(QString("cookie \"%1\" defined twice")
                                            .arg(cookie_name));
                                NOTREACHED();
                            }
                            f_browser_cookies[cookie_name] = cookie_value;
//fprintf(stderr, "f_browser_cookies[\"%s\"] = \"%s\";\n", cookie_name.toUtf8().data(), cookie_value.toUtf8().data());
                        }
                    }
                }
                else
                {
                    // TODO: verify that f_name is a valid header name
                    f_env[f_name] = f_value;
//fprintf(stderr, " f_env[\"%s\"] = \"%s\"\n", f_name.toUtf8().data(), f_value.toUtf8().data());
                }
            }
        }

        void run()
        {
            bool reading_name(true);
            do
            {
                char const c = getc();
                if(c == '=' && reading_name)
                {
                    reading_name = false;
                }
                else if(c == '\n')
                {
                    process_line();

                    // clear for next line
                    f_name.clear();
                    f_value.clear();
                    reading_name = true;
                }
                else if(c == '\r')
                {
                    die("got a \\r character in the environment (not in a multi-part POST)");
                    NOTREACHED();
                }
                else if(reading_name)
                {
                    if(isspace(c))
                    {
                        die("spaces are not allowed in environment variable names");
                        NOTREACHED();
                    }
                    f_name += c;
                }
                else
                {
                    f_value += c;
                }
            }
            while(f_running);
        }

        bool has_post() const
        {
            return f_has_post;
        }

    private:
        mutable zpsnap_child_t      f_snap;
        controlled_vars::zint32_t   f_socket;
        //controlled_vars::zchar_t    f_unget;
        controlled_vars::tbool_t    f_running;
        controlled_vars::fbool_t    f_started;

        environment_map_t&          f_env;
        environment_map_t&          f_browser_cookies;
        QString                     f_name;
        QString                     f_value;

        controlled_vars::fbool_t    f_has_post;
        environment_map_t&          f_post;
        post_file_map_t&            f_files;
        controlled_vars::tbool_t    f_post_first;
        controlled_vars::tbool_t    f_post_header;
        QByteArray                  f_post_line;
        QByteArray                  f_post_content;
        QByteArray                  f_boundary;
        QByteArray                  f_end_boundary;
        environment_map_t           f_post_environment;
        controlled_vars::zuint32_t  f_post_index;
    };

    // reset the old environment
    f_env.clear();
    f_post.clear();
    f_files.clear();

    read_env r(this, f_socket, f_env, f_browser_cookies, f_post, f_files);
    r.run();
    f_has_post = r.has_post();
}


/** \brief Write data to the output socket.
 *
 * This function writes data to the output socket.
 *
 * \exception runtime_error
 * This exception is thrown if the write() fails writing all the bytes. This
 * generally means the client closed the socket early.
 *
 * \param[in] data  The data pointer.
 * \param[in] size  The number of bytes to write from data.
 */
void snap_child::write(const char *data, ssize_t size)
{
    if(f_socket == -1)
    {
        // this happens from backends that do not have snap.cgi running
        return;
    }

    if(::write(f_socket, data, size) != size)
    {
        SNAP_LOG_FATAL("error while sending data to a client.");
        // XXX throw? we cannot call die() because die() calls write()!
        throw std::runtime_error("error while sending data to the client");
    }
}


/** \brief Write a string to the socket.
 *
 * This function is an overload of the write() data buffer. It writes the
 * specified string using strlen() to obtain the length. The string must be
 * a valid null terminated C string.
 *
 * \param[in] str  The string to write to the socket.
 */
void snap_child::write(const char *str)
{
    write(str, strlen(str));
}


/** \brief Write a QString to the socket.
 *
 * This function is an overload that writes the QString to the socket. This
 * QString is transformed to UTF-8 before being processed.
 *
 * \param[in] str  The QString to write to the socket.
 */
void snap_child::write(const QString& str)
{
    QByteArray a(str.toUtf8());
    write(a.data(), a.size());
}


/** \brief Generate the Snap information buffer and return it.
 *
 * This function prints out information about the Snap! Server.
 * This means writing information about all the different libraries
 * in use such as their version, name, etc.
 */
void snap_child::snap_info()
{
    QString version;

    // getting started
    write("#START\n");

    // the library (server) version
    write("VERSION=" SNAPWEBSITES_VERSION_STRING "\n");

    // operating system
    version = "OS=";
#ifdef Q_OS_LINUX
    version += "Linux";
#else
#error "Unsupported operating system."
#endif
    version += "\n";
    write(version);

    // the Qt versions
    write("QT=" QT_VERSION_STR "\n");
    version = "RUNTIME_QT=";
    version += qVersion();
    version += "\n";
    write(version);

    // the libtld version
    write("LIBTLD=" LIBTLD_VERSION "\n");
    version = "RUNTIME_LIBTLD=";
    version += tld_version();
    version += "\n";
    write(version);

    // the libQtCassandra version
    version = "LIBQTCASSANDRA=";
    version += QtCassandra::QT_CASSANDRA_LIBRARY_VERSION_STRING;
    version += "\n";
    write(version);
    version = "RUNTIME_LIBQTCASSANDRA=";
    version += QtCassandra::QCassandra::version();
    version += "\n";
    write(version);

    // the libQtSerialization version
    version = "LIBQTSERIALIZATION=";
    version += QtSerialization::QT_SERIALIZATION_LIBRARY_VERSION_STRING;
    version += "\n";
    write(version);
    version = "RUNTIME_LIBQTSERIALIZATION=";
    version += QtSerialization::QLibraryVersion();
    version += "\n";
    write(version);

    // since we do not have an environment we cannot connect
    // to the Cassandra cluster...

    // done
    write("#END\n");

    exit(1);
}


/** \brief Return the current stats in name/value pairs format.
 *
 * This command returns the server statistics.
 */
void snap_child::snap_statistics()
{
    QString s;

    // getting started
    write("#START\n");

    // the library (server) version
    write("VERSION=" SNAPWEBSITES_VERSION_STRING "\n");

    // the number of connections received by the server up until this child fork()'ed
    s = "CONNECTIONS_COUNT=";
    s += QString::number(f_server->connections_count());
    s += "\n";
    write(s);

    // done
    write("#END\n");

    exit(1);
}


/** \brief Setup the URI from the environment.
 *
 * This function gets the different variables from the environment
 * it just received from the snap.cgi script and builds the corresponding
 * Snap URI object with it. This will then be used to determine the
 * domain and finally the website.
 */
void snap_child::setup_uri()
{
    // PROTOCOL
    if(f_env.count("HTTPS") == 1)
    {
        if(f_env["HTTPS"] == "on")
        {
            f_uri.set_protocol("https");
        }
        else
        {
            f_uri.set_protocol("http");
        }
    }
    else
    {
        f_uri.set_protocol("http");
    }

    // HOST (domain name including all sub-domains)
    if(f_env.count("HTTP_HOST") != 1)
    {
        die(HTTP_CODE_SERVICE_UNAVAILABLE, "", "HTTP_HOST is required but not defined in your request.", "HTTP_HOST was not defined in the user request");
        NOTREACHED();
    }
    QString host(f_env["HTTP_HOST"]);
    int port_pos(host.indexOf(':'));
    if(port_pos != -1)
    {
        // remove the port information
        host = host.left(port_pos);
    }
    if(host.isEmpty())
    {
        die(HTTP_CODE_SERVICE_UNAVAILABLE, "", "HTTP_HOST is required but is empty in your request.", "HTTP_HOST was defined but there was no domain name");
        NOTREACHED();
    }
    f_uri.set_domain(host);

    // PORT
    if(f_env.count("SERVER_PORT") != 1)
    {
        die(HTTP_CODE_SERVICE_UNAVAILABLE, "", "SERVER_PORT is required but not defined in your request.", "SERVER_PORT was not defined in the user request");
        NOTREACHED();
    }
    f_uri.set_port(f_env["SERVER_PORT"]);

    // QUERY STRING
    if(f_env.count("QUERY_STRING") == 1)
    {
        f_uri.set_query_string(f_env["QUERY_STRING"]);
    }

    // REQUEST URI
    // Although we ignore the URI, it MUST be there
    if(f_env.count("REQUEST_URI") != 1)
    {
        die(HTTP_CODE_SERVICE_UNAVAILABLE, "",
                     "REQUEST_URI is required but not defined in your request.",
                     "REQUEST_URI was not defined in the user's request");
        NOTREACHED();
    }
    // This is useless since the URI points to the CGI which
    // we are not interested in
    //int p = f_env["REQUEST_URI"].indexOf('?');
    //if(p == -1)
    //{
    //    f_uri.set_path(f_env["REQUEST_URI"]);
    //}
    //else
    //{
    //    f_uri.set_path(f_env["REQUEST_URI"].mid(0, p));
    //}

    QString qs_path(f_server->get_parameter("qs_path"));
    QString path(f_uri.query_option(qs_path));
    QString extension;
    if(path != "." && path != "..")
    {
        f_uri.set_path(path);
        int limit(path.lastIndexOf('/'));
        if(limit == -1)
        {
            limit = 1;
        }
        int ext(path.lastIndexOf('.'));
        if(ext >= limit)
        {
            extension = path.mid(ext);
            // check for a compression and include that and
            // the previous extension
            if(extension == ".gz"       // gzip
            || extension == ".Z"        // Unix compress
            || extension == ".bz2")     // bzip2
            {
                // we generally expect .gz but we have to take
                // whatever extension the user added to make sure
                // we send the file in the right format
                // we will also need to use the Accept-Encoding
                // and make use of the Content-Encoding
                // TODO: make use of extension instead of Accept-Encoding
                f_uri.set_option("compression", extension);
                int real_ext(path.lastIndexOf('.', ext - 1));
                if(real_ext >= limit)
                {
                    // retrieve the extension without the compression
                    extension = path.mid(real_ext, real_ext - ext);
                }
                else
                {
                    extension = "";
                }
            }
        }
    }
    f_uri.set_option("extension", extension);

//fprintf(stderr, "set path to: [%s]\n", f_uri.query_option(qs_path).toUtf8().data());
//
//fprintf(stderr, "original [%s]\n", f_uri.get_original_uri().toUtf8().data());
//fprintf(stderr, "uri [%s] + #! [%s]\n", f_uri.get_uri().toUtf8().data(), f_uri.get_uri(true).toUtf8().data());
//fprintf(stderr, "protocol [%s]\n", f_uri.protocol().toUtf8().data());
//fprintf(stderr, "full domain [%s]\n", f_uri.full_domain().toUtf8().data());
//fprintf(stderr, "top level domain [%s]\n", f_uri.top_level_domain().toUtf8().data());
//fprintf(stderr, "domain [%s]\n", f_uri.domain().toUtf8().data());
//fprintf(stderr, "sub-domains [%s]\n", f_uri.sub_domains().toUtf8().data());
//fprintf(stderr, "port [%d]\n", f_uri.get_port());
//fprintf(stderr, "path [%s] (%s)\n", f_uri.path().toUtf8().data(), qs_path.toUtf8().data());
//fprintf(stderr, "query string [%s]\n", f_uri.query_string().toUtf8().data());
//fprintf(stderr, "q=[%s]\n", f_uri.query_option("q").toUtf8().data());
}


/** \brief Get a copy of the URI information.
 *
 * This function returns a constant reference (i.e. a read-only "copy")
 * of the URI used to access the server. This is the request we want to
 * answer.
 *
 * \return The URI reference.
 */
const snap_uri& snap_child::get_uri() const
{
    return f_uri;
}


/** \brief Connect to the Cassandra database system.
 *
 * This function connects to the Cassandra database system and
 * returns only if the connection succeeds. If it fails, it logs
 * the fact and sends an error back to the user.
 */
void snap_child::connect_cassandra()
{
    // Cassandra already exists?
    if(!f_cassandra.isNull())
    {
        die(HTTP_CODE_SERVICE_UNAVAILABLE, "", "Our database is being initialized more than once.", "The connect_cassandra() function cannot be called more than once.");
        NOTREACHED();
    }

    // connect to Cassandra
    f_cassandra = new QtCassandra::QCassandra;
    if(!f_cassandra->connect(f_server->cassandra_host(), f_server->cassandra_port()))
    {
        die(HTTP_CODE_SERVICE_UNAVAILABLE, "", "Our database system is temporarilly unavailable.", "Could not connect to Cassandra");
        NOTREACHED();
    }

    // select the Snap! context
    f_cassandra->contexts();
    QString context_name(get_name(SNAP_NAME_CONTEXT));
    f_context = f_cassandra->findContext(context_name);
    if(f_context.isNull())
    {
        // we connected to the database, but it is not properly initialized!?
        die(HTTP_CODE_SERVICE_UNAVAILABLE, "", "Our database system does not seem to be properly installed.", "The child process connected to Cassandra but it could not find the \"" + context_name + "\" context.");
        NOTREACHED();
    }
    f_context->setHostName(f_server->get_parameter("server_name"));
}


/** \brief Create a table.
 *
 * This function is generally used by plugins to create indexes for the data
 * they manage.
 *
 * The function can be called even if the table already exists.
 *
 * \param[in] table_name  The name of the table to create.
 * \param[in] comment  The comment to attach to the table.
 */
QSharedPointer<QtCassandra::QCassandraTable> snap_child::create_table(const QString& table_name, const QString& comment)
{
    return f_server->create_table(f_context, table_name, comment);
}


/** \brief Canonalize the domain information.
 *
 * This function uses the URI to find the domain core::rules and
 * start the canonalization process.
 *
 * The canonicalized domain is a domain name with sub-domains that
 * are required. All the optional sub-domains will be removed.
 *
 * All the variables are saved as options in the f_uri object.
 *
 * \todo
 * The functionality of this function needs to be extracted so it becomes
 * available to others (i.e. probably moved to snap_uri.cpp) that way we
 * can write tools that show the results of this parser.
 */
void snap_child::canonicalize_domain()
{
    // retrieve domain table
    QString table_name(get_name(SNAP_NAME_DOMAINS));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->table(table_name));

    // row for that domain exists?
    f_domain_key = f_uri.domain() + f_uri.top_level_domain();
    if(!table->exists(f_domain_key))
    {
        // this domain doesn't exist; i.e. that's a 404
        die(HTTP_CODE_NOT_FOUND, "Domain Not Found", "This website does not exist. Please check the URI and make corrections as required.", "User attempt to access \"" + f_domain_key + "\" which is not defined as a domain.");
        NOTREACHED();
    }

    // get the core::rules
    QtCassandra::QCassandraValue value(table->row(f_domain_key)->cell(QString(get_name(SNAP_NAME_CORE_RULES)))->value());
    if(value.nullValue())
    {
        // Null value means an empty string or undefined column and either
        // way it's wrong here
        die(HTTP_CODE_NOT_FOUND, "Domain Not Found", "This website does not exist. Please check the URI and make corrections as required.", "User attempt to access domain \"" + f_domain_key + "\" which does not have a valid core::rules entry.");
        NOTREACHED();
    }

    // parse the rules to our domain structures
    domain_rules r;
    // QBuffer takes a non-const QByteArray so we have to create a copy
    QByteArray data(value.binaryValue());
    QBuffer in(&data);
    in.open(QIODevice::ReadOnly);
    QtSerialization::QReader reader(in);
    r.read(reader);

    // we add a dot because the list of variables are expected to
    // end with a dot, but only if sub_domains is not empty
    QString sub_domains(f_uri.sub_domains());
    if(!sub_domains.isEmpty())
    {
        sub_domains += ".";
    }
    int max(r.size());
    for(int i = 0; i < max; ++i)
    {
        QSharedPointer<domain_info> info(r[i]);

        // build the regex (TODO: pre-compile the regex?
        // the problem is the var. name versus data parsed)
        QString re;
        int vmax(info->size());
        for(int v = 0; v < vmax; ++v)
        {
            QSharedPointer<domain_variable> var(info->get_variable(v));

            // put parameters between () so we get the data in
            // variables (options) later
            re += "(" + var->get_value() + ")";
            if(!var->get_required())
            {
                // optional sub-domain
                re += "?";
            }
        }
        QRegExp regex(re);
        if(regex.exactMatch(sub_domains))
        {
            // we found the domain!
            QStringList captured(regex.capturedTexts());
            QString canonicalized;

            // note captured[0] is the full matching pattern, we ignore it
            for(int v = 0; v < vmax; ++v)
            {
                QSharedPointer<domain_variable> var(info->get_variable(v));

                QString sub_domain_value(captured[v + 1]);
                // remove the last dot because in most cases we do not want it
                // in the variable even if it were defined in the regex
                if(!sub_domain_value.isEmpty() && sub_domain_value.right(1) == ".")
                {
                    sub_domain_value = sub_domain_value.left(sub_domain_value.length() - 1);
                }

                if(var->get_required())
                {
                    // required, use default if empty
                    if(sub_domain_value.isEmpty()
                    || var->get_type() == domain_variable::DOMAIN_VARIABLE_TYPE_WEBSITE)
                    {
                        sub_domain_value = var->get_default();
                    }
                    f_uri.set_option(var->get_name(), sub_domain_value);

                    // these make up the final canonicalized domain name
                    canonicalized += snap_uri::urlencode(sub_domain_value, ".");
                }
                else if(!sub_domain_value.isEmpty())
                {
                    // optional sub-domain, set only if not empty
                    if(var->get_type() == domain_variable::DOMAIN_VARIABLE_TYPE_WEBSITE)
                    {
                        sub_domain_value = var->get_default();
                    }
                    f_uri.set_option(var->get_name(), sub_domain_value);
                }
                else
                {
                    // optional with a default, use it
                    sub_domain_value = var->get_default();
                    if(!sub_domain_value.isEmpty())
                    {
                        f_uri.set_option(var->get_name(), sub_domain_value);
                    }
                }
            }

            // now we've got the website key
            if(canonicalized.isEmpty())
            {
                f_website_key = f_domain_key;
            }
            else
            {
                f_website_key = canonicalized + "." + f_domain_key;
            }
            return;
        }
    }

    // no domain match, we're dead meat
    die(HTTP_CODE_NOT_FOUND, "Domain Not Found", "This website does not exist. Please check the URI and make corrections as required.", "The domain \"" + f_uri.full_domain() + "\" did not match any domain name defined in your Snap! system. Should you remove it from your DNS?");
}


/** \brief Finish the canonalization process.
 *
 * The function reads the website core::rules and continue the parsing process
 * of the URI.
 *
 * The sub-domain and domain canonalization was accomplished in the previous
 * process: canonicalize_domain(). This is not done again in the websites.
 *
 * This process includes the following checks:
 *
 * 1. Protocol
 * 2. Port
 * 3. Query String
 * 4. Path
 *
 * The protocol, port, and query strings are checked as they are found in the
 * website variables of the core::rules.
 *
 * The path is checked once all the variables were checked and if the protocol,
 * port, and query strings were all matching as expected. If any one of them
 * does not match then we don't need to check the path.
 *
 * \note
 * As the checks of the protocol, port, and query strings are found, we cannot
 * put them in the options just yet since if the path check fails, then
 * another entry could be the proper match and that other entry may have
 * completely different variables.
 *
 * \todo
 * The functionality of this function needs to be extracted so it becomes
 * available to others (i.e. probably moved to snap_uri.cpp) that way we
 * can write tools that show the results of this parser.
 */
void snap_child::canonicalize_website()
{
    // retrieve website table
    QString table_name(get_name(SNAP_NAME_WEBSITES));
    QSharedPointer<QtCassandra::QCassandraTable> table(f_context->table(table_name));

    // row for that website exists?
    if(!table->exists(f_website_key))
    {
        // this website doesn't exist; i.e. that's a 404
        die(HTTP_CODE_NOT_FOUND, "Website Not Found", "This website does not exist. Please check the URI and make corrections as required.", "User attempt to access \"" + f_website_key + "\" which was not defined as a website.");
        NOTREACHED();
    }

    // get the core::rules
    QtCassandra::QCassandraValue value(table->row(f_website_key)->cell(QString(get_name(SNAP_NAME_CORE_RULES)))->value());
    if(value.nullValue())
    {
        // Null value means an empty string or undefined column and either
        // way it's wrong here
        die(HTTP_CODE_NOT_FOUND, "Website Not Found", "This website does not exist. Please check the URI and make corrections as required.", "User attempt to access website \"" + f_website_key + "\" which does not have a valid core::rules entry.");
        NOTREACHED();
    }

    // parse the rules to our website structures
    website_rules r;
    QByteArray data(value.binaryValue());
    QBuffer in(&data);
    in.open(QIODevice::ReadOnly);
    QtSerialization::QReader reader(in);
    r.read(reader);

    // we check decoded paths
    QString uri_path(f_uri.path(false));
    int max(r.size());
    for(int i = 0; i < max; ++i)
    {
        QSharedPointer<website_info> info(r[i]);

        // build the regex (TODO: pre-compile the regex?
        // the problem is the var. name versus data parsed)
        QString protocol("http");
        QString port("80");
        QMap<QString, QString> query;
        QString re_path;
        int vmax(info->size());
        bool matching(true);
        for(int v = 0; matching && v < vmax; ++v)
        {
            QSharedPointer<website_variable> var(info->get_variable(v));

            // put parameters between () so we get the data in
            // variables (options) later
            QString param_value("(" + var->get_value() + ")");
            switch(var->get_part())
            {
            case website_variable::WEBSITE_VARIABLE_PART_PATH:
                re_path += param_value;
                if(!var->get_required())
                {
                    // optional sub-domain
                    re_path += "?";
                }
                break;

            case website_variable::WEBSITE_VARIABLE_PART_PORT:
            {
                QRegExp regex(param_value);
                if(!regex.exactMatch(QString("%1").arg(f_uri.get_port())))
                {
                    matching = false;
                    break;
                }
                QStringList captured(regex.capturedTexts());
                port = captured[1];
            }
                break;

            case website_variable::WEBSITE_VARIABLE_PART_PROTOCOL:
            {
                QRegExp regex(param_value);
                // the case of the protocol in the regex doesn't matter
                // TODO (TBD):
                // Although I'm not 100% sure this is correct, we may
                // instead want to use lower case in the source
                regex.setCaseSensitivity(Qt::CaseInsensitive);
                if(!regex.exactMatch(f_uri.protocol()))
                {
                    matching = false;
                    break;
                }
                QStringList captured(regex.capturedTexts());
                protocol = captured[1];
            }
                break;

            case website_variable::WEBSITE_VARIABLE_PART_QUERY:
            {
                // the query string parameters are not ordered...
                // the variable name is 1 to 1 what is expected on the URI
                QString name(var->get_name());
                if(f_uri.has_query_option(name))
                {
                    // make sure it matches first
                    QRegExp regex(param_value);
                    if(!regex.exactMatch(f_uri.query_option(name)))
                    {
                        matching = false;
                        break;
                    }
                    QStringList captured(regex.capturedTexts());
                    query[name] = captured[1];
                }
                else if(var->get_required())
                {
                    // if required then we want to use the default
                    query[name] = var->get_default();
                }
            }
                break;

            default:
                throw std::runtime_error("unknown part specified in website_variable::f_part");

            }
        }
        if(!matching)
        {
            // one of protocol, port, or query string failed
            // (path is checked below)
            continue;
        }
        // now check the path, if empty assume it matches and
        // also we have no extra options
        QString canonicalized_path;
        if(!re_path.isEmpty())
        {
            // match from the start, but it doesn't need to match the whole path
            QRegExp regex("^" + re_path);
            if(regex.indexIn(uri_path) != -1)
            {
                // we found the site including a path!
                // TODO: should we keep the length of the captured data and
                //       remove it from the path sent down the road?
                //       (note: if you have a path such as /blah/foo and
                //       you remove it, then what looks like /robots.txt
                //       is really /blah/foo/robots.txt which is wrong.)
                //       However, if the path is only used for options such
                //       as languages, those options should be removed from
                //       the original path.
                QStringList captured(regex.capturedTexts());

                // note captured[0] is the full matching pattern, we ignore it
                for(int v = 0; v < vmax; ++v)
                {
                    QSharedPointer<website_variable> var(info->get_variable(v));

                    if(var->get_part() == website_variable::WEBSITE_VARIABLE_PART_PATH)
                    {
                        QString path_value(captured[v + 1]);

                        if(var->get_required())
                        {
                            // required, use default if empty
                            if(path_value.isEmpty()
                            || var->get_type() == website_variable::WEBSITE_VARIABLE_TYPE_WEBSITE)
                            {
                                path_value = var->get_default();
                            }
                            f_uri.set_option(var->get_name(), path_value);

                            // these make up the final canonicalized domain name
                            canonicalized_path += "/" + snap_uri::urlencode(path_value, "~");
                        }
                        else if(!path_value.isEmpty())
                        {
                            // optional path, set only if not empty
                            if(var->get_type() == website_variable::WEBSITE_VARIABLE_TYPE_WEBSITE)
                            {
                                path_value = var->get_default();
                            }
                            f_uri.set_option(var->get_name(), path_value);
                        }
                        else
                        {
                            // optional with a default, use it
                            path_value = var->get_default();
                            if(!path_value.isEmpty())
                            {
                                f_uri.set_option(var->get_name(), path_value);
                            }
                        }
                    }
                }
            }
            else
            {
                matching = false;
            }
        }

        if(matching)
        {
            // now we've got the protocol, port, query strings, and paths
            // so we can build the final URI that we'll use as the site key
            QString canonicalized;
            f_uri.set_option("protocol", protocol);
            canonicalized += protocol + "://" + f_website_key;
            f_uri.set_option("port", port);
            if(port.toInt() != 80)
            {
                canonicalized += ":" + port;
            }
            if(canonicalized_path.isEmpty())
            {
                canonicalized += "/";
            }
            else
            {
                canonicalized += canonicalized_path;
            }
            QString canonicalized_query;
            for(QMap<QString, QString>::const_iterator it(query.begin()); it != query.end(); ++it)
            {
                f_uri.set_query_option(it.key(), it.value());
                if(!canonicalized_query.isEmpty())
                {
                    canonicalized_query += "&";
                }
                canonicalized_query += snap_uri::urlencode(it.key()) + "=" + snap_uri::urlencode(it.value());
            }
            if(!canonicalized_query.isEmpty())
            {
                canonicalized += "?" + canonicalized_query;
            }
            // now we've got the site key
            f_site_key = canonicalized;
            f_original_site_key = f_site_key; // in case of a redirect...
            f_site_key_with_slash = f_site_key;
            if(f_site_key.right(1) != "/")
            {
                f_site_key_with_slash += "/";
            }
            return;
        }
    }

    // no website match, we're dead meat
    die(HTTP_CODE_NOT_FOUND, "Website Not Found", "This website does not exist. Please check the URI and make corrections as required.", "The website \"" + f_website_key + "\" did not match any website defined in your Snap! system. Should you remove it from your DNS?");
}


/** \brief Check whether a site needs to be redirected.
 *
 * This function verifies the site we just discovered to see whether
 * the user requested a redirect. If so, then we replace the
 * f_site_key accordingly.
 *
 * Note that this is not a 301 redirect, just an internal remap from
 * site A to site B.
 */
void snap_child::site_redirect()
{
    QtCassandra::QCassandraValue redirect(get_site_parameter(get_name(SNAP_NAME_CORE_REDIRECT)));
    if(redirect.nullValue())
    {
        // no redirect
        return;
    }

    // redirect now
    f_site_key = redirect.stringValue();

    // TBD -- should we also redirect the f_domain_key and f_website_key?

    // the site table is the old one, we want to switch to the new one
    f_site_table.clear();
}


/** \brief Redirect the user to a new page.
 *
 * This function forcibly redirects a user to a new page. If the path
 * includes a protocol (is a full URI) then it is used as is. If
 * the path includes no protocol, the current site key is prepended.
 *
 * The HTTP code can be specified. By default, 301 is assumed because
 * that's the most prominent redirect code used. If a page is used to
 * redirect dynamically, make sure to use 302 or 303 instead. You can
 * safely use one of the following codes:
 *
 * \li HTTP_CODE_MOVED_PERMANENTLY (301)
 * \li HTTP_CODE_FOUND (302)
 * \li HTTP_CODE_SEE_OTHER (303) -- POST becomes GET
 * \li HTTP_CODE_TEMPORARY_REDIRECT (307) -- keep same method
 * \li HTTP_CODE_PERMANENT_REDIRECT (308) -- keep same method
 *
 * The path may include a query string and an anchor.
 *
 * \param[in] path  The full URI or the local website path to redirect the
 *                  user browser to.
 * \param[in] http_code  The code used to send the redirect.
 */
void snap_child::page_redirect(const QString& path, http_code_t http_code)
{
    if(f_site_key_with_slash.isEmpty())
    {
        die(HTTP_CODE_INTERNAL_SERVER_ERROR, "Initialization Mismatch",
                "An internal server error was detected while initializing the process.",
                "The server snap_child::page_redirect() function was called before the website got canonicalized.");
        NOTREACHED();
    }

    if(path.contains('\n') || path.contains('\r'))
    {
        // if the path includes a \n or \r then the user could inject
        // a header which could have all sorts of effects we don't even
        // want to think about! just deny it...
        die(HTTP_CODE_INTERNAL_SERVER_ERROR, "Hack Prevention",
                "Server prevented a potential hack from being applied.",
                "The server snap_child::page_redirect() function was called with a path that includes \n or \r and refused processing it: \"" + path + "\"");
        NOTREACHED();
    }

    snap_uri uri;
    if(!uri.set_uri(path))
    {
        // in most cases it fails because the protocol is missing
        QString local_path(path);
        canonicalize_path(local_path);
        if(!uri.set_uri(get_site_key_with_slash() + local_path))
        {
            die(HTTP_CODE_ACCESS_DENIED, "Invalid URI",
                    "The server prevented a redirect because it could not understand the destination URI.",
                    "The server snap_child::page_redirect() function was called with a path that it did not like: \"" + path + "\"");
            NOTREACHED();
        }
    }

    f_server->attach_to_session();

    // redirect the user to the specified path
    QString http_name;
    define_http_name(http_code, http_name);

    set_header("Status", QString("%1 %2")
                    .arg(static_cast<int>(http_code))
                    .arg(http_name), HEADER_MODE_REDIRECT);

    // TODO the URI MUST be encoded
    set_header("Location", uri.get_uri(), HEADER_MODE_REDIRECT);

    // also the default is already text/html we force it again in case this
    // function is called after someone changed this header
    set_header("Content-Type", "text/html; charset=utf-8", HEADER_MODE_EVERYWHERE);

    // compute the body ahead so we can get its size
    // (should we support getting the content of a page? since 99.9999% of
    // the time this content is ignored, I would say no.)
    QString body("<html><head>"
            "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>"
            "<title>Moved</title>"
            "<meta http-equiv=\"Refresh\" content=\"0; url=" + uri.get_uri() + "\"/>"
            "<meta name=\"ROBOTS\" content=\"NOINDEX\"/>" // no need for the NOFOLLOW on this one
            "</head><body><h1>Moved</h1><p>This page has moved to <a href=\""
                    + uri.get_uri() + "\">"
                    + uri.get_uri() + "</a>.</p></body></html>");

    //write(QString("Content-Length: %1\n").arg(body.length()));
    set_header("Content-Length", QString("%1").arg(body.toUtf8().size()), HEADER_MODE_REDIRECT);

    // in case there are any cookies, send them along too
    output_headers(HEADER_MODE_REDIRECT);

    write("\n"); // header / body separator

    write(body);

    // XXX should we exit with 1 in this case?
    exit(0);
    NOTREACHED();
}


/** \brief Attach variables to this session.
 *
 * Once in a while a plugin creates a form that is intermediary. In this
 * case the session variables need to be saved and this function is called.
 *
 * Note that you may want to look into not detaching the variable(s) if at
 * all possible.
 */
void snap_child::attach_to_session()
{
    f_server->attach_to_session();
}


/** \brief Check whether access is permitted.
 *
 * This function checks whether \p user_path can perform \p action on
 * \p path. If the action can be performed this function returns true,
 * otherwise it returns false.
 *
 * The function makes use of an action such as "view". The action is
 * very important to verify whether a user has permission to do something
 * or not.
 *
 * The anonymous user is represented by an empty path and not "user"
 * as in the session.
 *
 * \param[in] user_path  The path to the user account in the content table.
 * \param[in] path  The path to check for permissions.
 * \param[in] action  The action being checked.
 *
 * \return true if the specified user has permission.
 */
bool snap_child::access_allowed(QString const& user_path, QString const& path, QString const& action)
{
    server::permission_flag result;
    f_server->access_allowed(user_path, path, action, result);
    return result.allowed();
}


/** \brief Retrieve an environment variable.
 *
 * This function can be used to read an environment variable. It will make
 * sure, in most cases, that the variable is not tented.
 *
 * At this point only the variables defined in the HTTP request are available.
 * Any other variable name will return an empty string.
 *
 * The SERVER_PROTOCOL variable can be retrieved at any time, even before we
 * read the environment. This is done so we can call the die() function and
 * return with a valid protocol and version.
 *
 * \param[in] name  The name of the variable to retrieve.
 *
 * \return The value of the specified variable.
 */
QString snap_child::snapenv(const QString& name) const
{
    if(name == "SERVER_PROTOCOL")
    {
        // SERVER PROTOCOL
        if(false == f_fixed_server_protocol)
        {
            f_fixed_server_protocol = true;
            // Drupal does the following, can the SERVER_PROTOCOL really be wrong?
            if(f_env.count("SERVER_PROTOCOL") != 1)
            {
                // if undefined, set a default protocol
                const_cast<snap_child *>(this)->f_env["SERVER_PROTOCOL"] = "HTTP/1.0";
            }
            else
            {
                // note that HTTP/0.9 could be somewhat supported but that's
                // most certainly totally useless
                if("HTTP/1.0" != f_env.value("SERVER_PROTOCOL")
                && "HTTP/1.1" != f_env.value("SERVER_PROTOCOL"))
                {
                    // environment is no good!?
                    const_cast<snap_child *>(this)->f_env["SERVER_PROTOCOL"] = "HTTP/1.0";
                }
            }
        }
        return f_env.value("SERVER_PROTOCOL", "HTTP/1.0");
    }

    return f_env.value(name, "");
}


/** \brief Check whether a POST variable was defined.
 *
 * Check whether the named POST variable was defined. This can be useful if
 * you have some optional fields in a form. Also in some places where the
 * code does not know about all the widgets.
 *
 * Note that the functions that directly access the post environment should
 * not be used by most as the form plugin already does what is necessary.
 *
 * \param[in] name  The name of the POST variable to fetch.
 *
 * \return true if the value is defined, false otherwise.
 */
bool snap_child::postenv_exists(const QString& name) const
{
    return f_post.contains(name);
}


/** \brief Retrieve a POST variable.
 *
 * Return the content of one of the POST variables. Post variables are defined
 * only if the method used to access the site was a POST.
 *
 * \warning
 * This function returns the RAW data from a POST. You should instead use the
 * data returned by your form which will have been validated and fixed up as
 * required (decoded, etc.)
 *
 * \param[in] name  The name of the POST variable to fetch.
 * \param[in] default_value  The value to return if the POST variable is not defined.
 *
 * \return The value of that POST variable or the \p default_value.
 */
QString snap_child::postenv(const QString& name, const QString& default_value) const
{
    return f_post.value(name, default_value);
}


/** \brief Retrieve a file from the POST.
 *
 * This function can be called if this request included a POST with a
 * file attached.
 *
 * Note that the files are saved by widget identifier. This means if
 * you check a post with postenv("file") (which returns the filename),
 * then you can get the actual file with postfile("file").
 *
 * \param[in] name  The id (name) of the Input File widget.
 */
const snap_child::post_file_t& snap_child::postfile(QString const& name) const
{
    // the QMap only returns a reference if this is not constant
    return const_cast<snap_child *>(this)->f_files[name];
}


/** \brief Check whether a cookie was sent to us by the browser.
 *
 * This function checks whether a cookie was defined and returned by the
 * browser. This is different from testing whether the value returned by
 * cookie() is an empty string.
 *
 * \note
 * Doing a set_cookie() does not interfere with this list of cookies
 * which represent the list of cookies the browser sent to us.
 *
 * \param[in] name  The name of the cookie to retrieve.
 *
 * \return true if the cookie was sent to us, false otherwise.
 */
bool snap_child::cookie_is_defined(const QString& name) const
{
    return f_browser_cookies.contains(name);
}


/** \brief Return the contents of a cookie.
 *
 * This function returns the contents of the named cookie.
 *
 * Note that this function is not the counterpart of the set_cookie()
 * function. The set_cookie() accepts an http_cookie object, whereas
 * this function only returns a string (because that's all we get
 * from the browser.)
 *
 * \param[in] name  The name of the cookie to retrieve.
 *
 * \return The content of the cookie, an empty string if the cookie is not defined.
 */
QString snap_child::cookie(const QString& name) const
{
    if(f_browser_cookies.contains(name))
    {
        return f_browser_cookies[name];
    }
    return QString();
}


/** \brief Get a proper URL for this access.
 *
 * This function transforms a local URL to a CGI URL if the site
 * was accessed that way.
 *
 * If the site was accessed without the /cgi-bin/snap.cgi then this function
 * returns the URL as is. If it was called with /cgi-bin/snap.cgi then the
 * URL is transformed to also be use the /cgi-bin/snap.cgi syntax.
 *
 * \param[in] url  The URL to transform.
 *
 * \return The URL matching the calling convention.
 */
QString snap_child::snap_url(const QString& url) const
{
    if(snapenv("CLEAN_SNAP_URL") == "1")
    {
        return url;
    }
    // TODO: this should be coming from the database
    if(url[0] == '/')
    {
        QString u(url);
        return u.insert(1, "cgi-bin/snap.cgi?q=");
    }
    return "/cgi-bin/snap.cgi?q=" + url;
}


/** \brief Make sure to clean up then exit the child process.
 *
 * This function cleans up the child and then calls the
 * server::exit() function to give the server a chance to
 * also clean up. Then it exists by calling the exit(3)
 * function of the C library.
 *
 * \param[in] code  The exit code, generally 0 or 1.
 */
void snap_child::exit(int code)
{
    // make sure the socket data is pushed to the caller
    if(f_socket != -1)
    {
        close(f_socket);
        f_socket = -1;
    }
    f_server->exit(code);
    NOTREACHED();
}


/** \brief Check whether the server was started in debug mode.
 *
 * With this function any plugin can determine whether the server was
 * started with the --debug command line option and act accordingly
 * (i.e. show a certain number of debug in stdout or stderr).
 *
 * It should not be used to display debug data in the HTML output.
 * Other parameters can be used for that purpose (specifically, that
 * very plugin can make use of its own debug parameter for that because
 * having the debug turned on for all modules would be a killer.)
 *
 * \return true if the --debug (-d) command line option was used to start
 *         the server
 */
bool snap_child::is_debug() const
{
    return f_server->is_debug();
}


/** \brief Retreive a website wide parameter.
 *
 * This function reads a column from the sites table using the site key as
 * defined by the canonicalization process. The function cannot be called
 * before the canonicalization process ends.
 *
 * The table is opened once and remains opened so calling this function
 * many times is not a problem. Also the libQtCassandra library caches
 * all the data. Reading the same field multiple times is not a concern
 * at all.
 *
 * If the value is undefined, the result is a null value.
 *
 * \param[in] name  The name of the parameter to retrieve.
 *
 * \return The content of the row as a Cassandra value.
 */
QtCassandra::QCassandraValue snap_child::get_site_parameter(const QString& name)
{
    // retrieve site table if not there yet
    if(f_site_table.isNull())
    {
        QString table_name(get_name(SNAP_NAME_SITES));
        QSharedPointer<QtCassandra::QCassandraTable> table(f_context->findTable(table_name));
        if(table.isNull())
        {
            // the whole table is still empty
            QtCassandra::QCassandraValue value;
            return value;
        }
        f_site_table = table;
    }

    if(!f_site_table->exists(f_site_key))
    {
        // an empty value is considered to be a null value
        QtCassandra::QCassandraValue value;
        return value;
    }
    QSharedPointer<QtCassandra::QCassandraRow> row(f_site_table->row(f_site_key));
    if(!row->exists(name))
    {
        // an empty value is considered to be a null value
        QtCassandra::QCassandraValue value;
        return value;
    }

    return row->cell(name)->value();
}


/** \brief Save a website wide parameter.
 *
 * This function writes a column to the sites table using the site key as
 * defined by the canonicalization process. The function cannot be called
 * before the canonicalization process ends.
 *
 * The table is opened once and remains opened so calling this function
 * many times is not a problem.
 *
 * If the value was still undefined, then it is created.
 *
 * \param[in] name  The name of the parameter to save.
 * \param[in] value  The new value for this parameter.
 */
void snap_child::set_site_parameter(const QString& name, const QtCassandra::QCassandraValue& value)
{
    // retrieve site table if not there yet
    if(f_site_table.isNull())
    {
        QString table_name(get_name(SNAP_NAME_SITES));
        QSharedPointer<QtCassandra::QCassandraTable> table(f_context->table(table_name));
        table->setComment("List of sites with their global parameters.");
        table->setColumnType("Standard"); // Standard or Super
        table->setKeyValidationClass("BytesType");
        table->setDefaultValidationClass("BytesType");
        table->setComparatorType("BytesType");
        table->setKeyCacheSavePeriodInSeconds(14400);
        table->setMemtableFlushAfterMins(60);
        //table->setMemtableThroughputInMb(247);
        //table->setMemtableOperationsInMillions(1.1578125);
        table->setGcGraceSeconds(864000);
        table->setMinCompactionThreshold(4);
        table->setMaxCompactionThreshold(22);
        table->setReplicateOnWrite(1);
        table->create();

        f_site_table = table;

        // mandatory fields
        f_site_table->row(f_site_key)->cell(QString(get_name(SNAP_NAME_CORE_SITE_NAME)))->setValue(QString("Website Name"));
    }

//fprintf(stderr, "Setting [%s] parameter\n", name.toUtf8().data());
    f_site_table->row(f_site_key)->cell(name)->setValue(value);
}


/** \brief Write the buffer to the output.
 *
 * This function writes the specified buffer (array of bytes) to
 * the output of the snap child. When the execute function returns
 * from running all the plugins, the data in the buffer is sent to
 * Apache (through snap.cgi).
 *
 * This function is most often used when the process is replying with
 * data other than text (i.e. images, PDF documents, etc.)
 *
 * \param[in] data  The array of byte to append to the buffer.
 */
void snap_child::output(QByteArray const& data)
{
    f_output.write(data);
}


/** \brief Write the string to the output buffer.
 *
 * This function writes the specified string to the output buffer
 * of the snap child. When the execute function returns from running
 * all the plugins, the data in the buffer is sent to Apache.
 *
 * The data is always written in UTF-8.
 *
 * \param[in] data  The string data to append to the buffer.
 */
void snap_child::output(const QString& data)
{
    f_output.write(data.toUtf8());
}


/** \brief Write the string to the output buffer.
 *
 * This function writes the specified string to the output buffer
 * of the snap child. When the execute function returns from running
 * all the plugins, the data in the buffer is sent to Apache.
 *
 * The data is viewed as UTF-8 characters and it is sent as is to the
 * buffer.
 *
 * \param[in] data  The string data to append to the buffer.
 */
void snap_child::output(const std::string& data)
{
    f_output.write(data.c_str(), data.length());
}


/** \brief Write the string to the output buffer.
 *
 * This function writes the specified string to the output buffer
 * of the snap child. When the execute function returns from running
 * all the plugins, the data in the buffer is sent to Apache.
 *
 * The data is viewed as UTF-8 characters and it is sent as is to the
 * buffer.
 *
 * \param[in] data  The string data to append to the buffer.
 */
void snap_child::output(const char *data)
{
    f_output.write(data);
}


/** \brief Write the string to the output buffer.
 *
 * This function writes the specified string to the output buffer
 * of the snap child. When the execute function returns from running
 * all the plugins, the data in the buffer is sent to Apache.
 *
 * The data is viewed as UTF-8 characters and it is sent as is to the
 * buffer.
 *
 * \param[in] data  The string data to append to the buffer.
 */
void snap_child::output(const wchar_t *data)
{
    f_output.write(QString::fromWCharArray(data).toUtf8());
}


/** \brief Check whether someone wrote any output yet.
 *
 * This function checks whether any output was written or not.
 *
 * \return true if the output buffer is still empty.
 */
bool snap_child::empty_output() const
{
    return f_output.buffer().size() == 0;
}


/** \brief Generate an HTTP error and exit the child process.
 *
 * This function kills the child process after sending an HTTP
 * error message to the user and to the logger.
 *
 * The \p err_name parameter is optional in that it can be set to
 * the empty string ("") and let the die() function make use of
 * the default error message for the specified \p err_code.
 *
 * The error description message can include HTML tags to change
 * the basic format of the text (i.e. bold, italic, underline, and
 * other inline tags.) The message is printed inside a paragraph
 * tag (<p>) and thus it should not include block tags.
 * The message is expected to be UTF-8 encoded, although in general
 * it should be in English so only using ASCII.
 *
 * The \p err_details parameter is the message to write to the
 * log. It should be as detailed as possible so it makes it
 * easy to know what's wrong and eventually needs attention.
 *
 * \note
 * You can trick the description paragraph by adding a closing
 * paragraph tag (</p>) at the start and an opening paragraph
 * tag (</p>) at the end of your description.
 *
 * \warning
 * This function does NOT return. It calls exit(1) once done.
 *
 * \param[in] err_code  The error code such as 501 or 503.
 * \param[in] err_name  The name of the error such as "Service Not Available".
 * \param[in] err_description  HTML message about the problem.
 * \param[in] err_details  Server side text message with details that are logged only.
 */
void snap_child::die(http_code_t err_code, QString err_name, const QString& err_description, const QString& err_details)
{
    try
    {
        // define a default error name if undefined
        define_http_name(err_code, err_name);

        // log the error
        SNAP_LOG_FATAL("snap child process: ")(err_details)(" (")(static_cast<int>(err_code))(" ")(err_name)(": ")(err_description)(")");

        // On error we do not return the HTTP protocol, only the Status field
        // it just needs to be first to make sure it works right
        set_header("Status", QString("%1 %2\n")
                .arg(static_cast<int>(err_code))
                .arg(err_name));

        // content type is HTML, we reset this header because it could have
        // been changed to something else and prevent the error from showing
        // up in the browser
        set_header("Content-Type", "text/html; charset=utf8", HEADER_MODE_EVERYWHERE);

        // Generate the signature
        QString signature;
        const QString site_key(get_site_key());
        if(!f_cassandra.isNull())
        {
            // TODO: the description could also come from a user defined page
            //       so that way it can get translated (only for some
            //       4XX errors though)

            QtCassandra::QCassandraValue site_name(get_site_parameter(get_name(SNAP_NAME_CORE_SITE_NAME)));
            signature = "<a href=\"" + get_site_key() + "\">" + site_name.stringValue() + "</a>";
            f_server->improve_signature(f_uri.path(), signature);
        }
        else if(!site_key.isEmpty())
        {
            signature = "<a href=\"" + get_site_key() + "\">" + get_site_key() + "</a>";
            f_server->improve_signature(f_uri.path(), signature);
        }
        // else -- no signature...

        // HTML output
        QString html(QString("<html><head>"
                        "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>"
                        "<meta name=\"ROBOTS\" content=\"NOINDEX,NOFOLLOW\"/>"
                        "<title>Snap Server Error</title>"
                        "</head>"
                        "<body><h1>%1 %2</h1><p>%3</p><p>%4</p></body></html>\n")
                .arg(static_cast<int>(err_code))
                .arg(err_name)
                .arg(err_description)
                .arg(signature));
        set_header("Content-Length", QString("%1").arg(html.toUtf8().size()));

        // in case there are any cookies, send them along too
        output_headers(HEADER_MODE_ERROR);

        // output body
        write(html);
    }
    catch(...)
    {
        // ignore all errors because at this point we must die quickly.
        SNAP_LOG_FATAL("snap_child.cpp:die(): try/catch caught an exception");
    }

    // exit with an error
    exit(1);
}

/** \brief Ensure that the http_name variable is not empty.
 *
 * This function sets the content of the \p http_name variable if empty. It
 * uses the \p http_code value to define a default message in \p http_name.
 *
 * If the \p http_name string is not empty then it is not modified.
 *
 * \param[in] http_code  The code used to determine the http_name value.
 * \param[in,out] http_name  The http request name to set if not already defined.
 */
void snap_child::define_http_name(http_code_t http_code, QString& http_name)
{
    if(http_name.isEmpty())
    {
        switch(http_code)
        {
        case 100: http_name = "Continue"; break;
        case 101: http_name = "Switching Protocols"; break;
        case 102: http_name = "Processing"; break;

        case 200: http_name = "OK"; break;
        case 201: http_name = "Created"; break;
        case 202: http_name = "Accepted"; break;
        case 203: http_name = "Non-Authoritative Information"; break;
        case 204: http_name = "No Content"; break;
        case 205: http_name = "Reset Content"; break;
        case 206: http_name = "Partial Content"; break;
        case 207: http_name = "Multi-Status"; break;
        case 208: http_name = "Already Reported"; break;
        case 226: http_name = "Instance-Manipulation Used"; break;

        case 300: http_name = "Multiple Choice"; break;
        case 301: http_name = "Moved Permanently"; break;
        case 302: http_name = "Found"; break;
        case 303: http_name = "See Other"; break; // POST becomes GET
        case 304: http_name = "Not Modified"; break;
        case 305: http_name = "Use Proxy"; break;
        case 306: http_name = "Switch Proxy"; break;
        case 307: http_name = "Temporary Redirect"; break; // keep same method
        case 308: http_name = "Permanent Redirect"; break; // keep same method

        case 400: http_name = "Bad Request"; break;
        case 401: http_name = "Unauthorized"; break;
        case 402: http_name = "Payment Required"; break;
        case 403: http_name = "Forbidden"; break;
        case 404: http_name = "Not Found"; break;
        case 405: http_name = "Method Not Allowed"; break;
        case 406: http_name = "Not Acceptable"; break;
        case 407: http_name = "Proxy Authentication Required"; break;
        case 408: http_name = "Request Timeout"; break;
        case 409: http_name = "Conflict"; break;
        case 410: http_name = "Gone"; break;
        case 411: http_name = "Length Required"; break;
        case 412: http_name = "Precondition Failed"; break;
        case 413: http_name = "Request Entity Too Large"; break;
        case 414: http_name = "Request-URI Too Long"; break;
        case 415: http_name = "Unsupported Media Type"; break;
        case 416: http_name = "Requested Range Not Satisfiable"; break;
        case 417: http_name = "Expectation Failed"; break;
        case 418: http_name = "I'm a teapot"; break;
        case 420: http_name = "Enhance Your Calm"; break;
        case 422: http_name = "Unprocessable Entity"; break;
        case 423: http_name = "Locked"; break;
        case 424: http_name = "Failed Dependency"; break;
        //case 424: http_name = "Method Failure"; break;
        case 425: http_name = "Unordered Collection"; break;
        case 426: http_name = "Upgrade Required"; break;
        case 428: http_name = "Precondition Required"; break;
        case 429: http_name = "Too Many Requests"; break;
        case 431: http_name = "Request Header Fields Too Large"; break;
        case 444: http_name = "No Response"; break;
        case 449: http_name = "Retry With"; break;
        case 450: http_name = "Blocked by Windows Parental Controls"; break;
        case 451: http_name = "Unavailable For Legal Reasons"; break;
        //case 451: http_name = "Redirect"; break;
        case 494: http_name = "Request Header Too Large"; break;
        case 495: http_name = "Cert Error"; break;
        case 496: http_name = "No Cert"; break;
        case 497: http_name = "HTTP to HTTPS"; break;
        case 499: http_name = "Client Closed Request"; break;

        case 500: http_name = "Internal Server Error"; break;
        case 501: http_name = "Not Implemented"; break;
        case 502: http_name = "Bad Gateway"; break;
        case 503: http_name = "Service Unavailable"; break;
        case 504: http_name = "Gateway Timeout"; break;
        case 505: http_name = "HTTP Version Not Supported"; break;
        case 506: http_name = "Variants Also Negotiates"; break;
        case 507: http_name = "Insufficiant Storage"; break;
        case 508: http_name = "Loop Detected"; break;
        case 509: http_name = "Bandwidth Limit Exceeded"; break;
        case 510: http_name = "Not Extended"; break;
        case 511: http_name = "Network Authentication Required"; break;
        case 531: http_name = "Access Denied"; break;
        case 598: http_name = "Network read timeout error"; break;
        case 599: http_name = "Network connect timeout error"; break;

        default:
            http_name = "Unknown HTTP Code";
            break;

        }
    }
}

/** \brief Set an HTTP header.
 *
 * This function sets the specified HTTP header to the specified value.
 * This function overwrites the existing value if any. To append to the
 * existing value, use the append_header() function instead. Note that
 * append only works with fields that supports lists (comma separated
 * values, etc.)
 *
 * The value is trimmed of LWS (SP, HT, CR, LF) characters on both ends.
 * Also, if the value includes CR or LF characters, it must be followed
 * by at least one SP or HT. Note that all CR are transformed to LF and
 * double LFs are replaced by one LF.
 *
 * The definition of an HTTP header is message-header as found
 * in the snippet below:
 *
 * \code
 *     OCTET          = <any 8-bit sequence of data>
 *     CHAR           = <any US-ASCII character (octets 0 - 127)>
 *     CTL            = <any US-ASCII control character
 *                      (octets 0 - 31) and DEL (127)>
 *     CR             = <US-ASCII CR, carriage return (13)>
 *     LF             = <US-ASCII LF, linefeed (10)>
 *     SP             = <US-ASCII SP, space (32)>
 *     HT             = <US-ASCII HT, horizontal-tab (9)>
 *     CRLF           = CR LF
 *     LWS            = [CRLF] 1*( SP | HT )
 *     TEXT           = <any OCTET except CTLs,
 *                      but including LWS>
 *     token          = 1*<any CHAR except CTLs or separators>
 *     separators     = "(" | ")" | "<" | ">" | "@"
 *                    | "," | ";" | ":" | "\" | <">
 *                    | "/" | "[" | "]" | "?" | "="
 *                    | "{" | "}" | SP | HT
 *     message-header = field-name ":" [ field-value ]
 *     field-name     = token
 *     field-value    = *( field-content | LWS )
 *     field-content  = <the OCTETs making up the field-value
 *                      and consisting of either *TEXT or combinations
 *                      of token, separators, and quoted-string>
 * \endcode
 *
 * To remove a header, set the value to the empty string.
 *
 * References: http://www.w3.org/Protocols/rfc2616/rfc2616-sec2.html
 * and http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html
 *
 * When adding a header, it is expected to only be used when no error
 * occurs (HEADER_MODE_NO_ERROR). However, in some circumstances it
 * is useful to send additional headers with errors or redirects.
 * These headers can use a different mode so they appear in those other
 * locations.
 *
 * \note
 * The key of the f_header map is the name in lowercase. For this
 * reason we save the field name as defined by the user in the
 * value as expected in the final result (i.e. "Blah: " + value.)
 *
 * \todo
 * Added a separate function so we can add multiple HTTP Link entries.
 *
 * \param[in] name  The name of the header.
 * \param[in] value  The value to assign to that header.
 * \param[in] modes  Where the header will be used.
 */
void snap_child::set_header(QString const& name, QString const& value, header_mode_t modes)
{
    {
        // name cannot include controls or separators and only CHARs
        std::vector<wchar_t> ws;
        ws.resize(name.length());
        name.toWCharArray(&ws[0]);
        int p(name.length());
        while(p > 0)
        {
            --p;
            wchar_t wc(ws[p]);
            bool valid(true);
            if(wc < 0x21 || wc > 0x7E)
            {
                valid = false;
            }
            else switch(wc)
            {
            case L'(': case L')': case L'<': case L'>': case L'@':
            case L',': case L';': case L':': case L'\\': case L'"':
            case L'/': case L'[': case L']': case L'?': case L'=':
            case L'{': case L'}': // SP & HT are checked in previous if()
                valid = false;
                break;

            default:
                //valid = true; -- default to true
                break;

            }
            if(!valid)
            {
                // more or less ASCII except well defined separators
                throw snap_child_exception_invalid_header_field_name("header field name \"" + name + "\" is not valid, found unwanted character: '" + QChar(wc) + "'");
            }
        }
    }

    QString v;
    {
        // value cannot include controls except LWS (\r, \n and \t)
        std::vector<wchar_t> ws;
        ws.resize(value.length());
        value.toWCharArray(&ws[0]);
        int max(value.length());
        wchar_t lc(L'\0');
        for(int p(0); p < max; ++p)
        {
            wchar_t wc(ws[p]);
            if((wc < 0x20 || wc == 127) && wc != L'\r' && wc != L'\n' && wc != L'\t')
            {
                // refuse controls except \r, \n, \t
                throw snap_child_exception_invalid_header_value("header field value \"" + value + "\" is not valid, found unwanted character: '" + QChar(wc) + "'");
            }
            // we MUST have a space or tab after a newline
            if(wc == L'\r' || wc == L'\n')
            {
                // if p + 1 == max then the user supplied the ending "\r\n"
                if(p + 1 < max)
                {
                    if(ws[p] != L' ' && ws[p] != L'\t' && ws[p] != L'\r' && ws[p] != L'\n')
                    {
                        // missing space or tab after a "\r\n" sequence
                        // (we also accept \r or \n although empty lines are
                        // forbidden but we'll remove them anyway)
                        throw snap_child_exception_invalid_header_value("header field value \"" + value + "\" is not valid, found a \\r pr \\n not followed by a space");
                    }
                }
            }
            if(v.isEmpty() && (wc == L' ' || wc == L'\t' || wc == L'\r' || wc == L'\n'))
            {
                // trim on the left (that's easy and fast to do here)
                continue;
            }
            if(wc == L'\r')
            {
                wc = L'\n';
            }
            if(lc == L'\n' && wc == L'\n')
            {
                // don't double '\n' (happens when user sends us "\r\n")
                continue;
            }
            v += QChar(wc);
            lc = wc;
        }
        while(!v.isEmpty())
        {
            QChar c(v.right(1)[0]);
            // we skip the '\r' because those were removed anyway
            if(c != ' ' || c != '\t' /*|| c != '\r'*/ || c != '\n')
            {
                break;
            }
            v.remove(v.length() - 1, 1);
        }
    }

    if(v.isEmpty())
    {
        f_header.remove(name.toLower());
    }
    else
    {
        // Note that even the Status needs to be a field
        // because we're using Apache and they expect such
        http_header_t header;
        header.f_header = name + ": " + v;
        header.f_modes = modes;
        f_header[name.toLower()] = header;
    }
}


/** \brief Check whether a header is defined.
 *
 * This function searches for the specified name in the list of
 * headers and returns true if it finds it.
 *
 * \warning
 * Cookies are headers, but these are managed using the cookie manager
 * which offers functions such as set_cookie(), cookie_is_defined(),
 * and cookie().
 *
 * \param[in] name  Name of the header to check for.
 *
 * \return false if the header was not defined yet, true otherwise.
 */
bool snap_child::has_header(const QString& name) const
{
    return f_header.find(name.toLower()) != f_header.end();
}

/** \brief Retrieve the current value of the given header.
 *
 * This function returns the value of the specified header, if
 * it exists. You may want to first call has_header() to know
 * whether the header exists. It is not an error to get a header
 * that was not yet defined, you get an empty string as a result.
 *
 * \note
 * We only return the value of the header even though the
 * header field name is included in the f_header value,
 * we simply skip that information.
 *
 * \param[in] name  The name of the header to query.
 *
 * \return The value of this header, "" if undefined.
 */
QString snap_child::get_header(const QString& name) const
{
    header_map_t::const_iterator it(f_header.find(name.toLower()));
    if(it == f_header.end())
    {
        // it's not defined
        return "";
    }

    // return the value without the field
    return it->f_header.mid(name.length() + 2);
}


/** \brief Output the HTTP headers.
 *
 * This function prints the HTTP headers to the output.
 *
 * The headers are defined with a mode (a set of flags really) which
 * can be used to tell the server when such and such header is to
 * be output.
 *
 * Note that the Cookies headers are never printed by this function.
 *
 * \note
 * Headers are NOT encoding in UTF-8, we output them as Latin1, this is
 * VERY important; headers are checked at the time you do the set_header
 * to ensure that only Latin1 characters are used.
 *
 * \todo
 * Any header that a path other than the default (see the die() and
 * page_redirect() functions) uses should not be printed by this
 * function. At this point there is no real protection against that
 * yet it should be protected. An idea is for us to change all those
 * functions to use the set_header() first, then call this function
 * because that way the set_header() will have overwritten whatever
 * other plugins would have defined there.
 *
 * \param[in] modes  Print the headers for these modes.
 */
void snap_child::output_headers(header_mode_t modes)
{
    // Output the status first (we may want to order the HTTP header
    // fields by type and output them ordered by type as defined in
    // the HTTP reference chapter 4.2)
    if(has_header("Status") && (f_header["status"].f_modes & modes) != 0)
    {
        // If status is defined, it should not be 200
        write((f_header["status"].f_header + "\n").toLatin1().data());
//printf("%s", (f_header["status"] + "\n").toLatin1().data());
    }

    // Now output all the other headers except the cookies
    for(header_map_t::const_iterator it(f_header.begin());
                                     it != f_header.end();
                                     ++it)
    {
        if((it.value().f_modes & modes) != 0 && it.key() != "status")
        {
            write((it.value().f_header + "\n").toLatin1().data());
//printf("%s", (it.value().f_header + "\n").toLatin1().data());
        }
    }

    // Finally output the cookies
    output_cookies();

    // Done with the headers
    write("\n");
}


/** \brief Add a cookie.
 *
 * This function adds a cookie to send to the user.
 *
 * Contrary to most other headers, there may be more than one cookie
 * in a reply and the set_header() does not support such. Plus cookies
 * have a few other parameters so this function is used to save those
 * in a separate vector of cookies.
 *
 * The input cookie information is copied in the vector of cookies so
 * you can modify it.
 *
 * The same cookie can be redefined multiple times. Calling the function
 * again overwrites a previous call with the same "name" parameter.
 *
 * \param[in] name  The name of the cookie.
 * \param[in] cookie  The cookie value, expiration, etc.
 *
 * \sa cookie()
 * \sa cookie_is_defined()
 * \sa output_cookies()
 */
void snap_child::set_cookie(const http_cookie& cookie_info)
{
    f_cookies[cookie_info.get_name()] = cookie_info;
}


/** \brief Output the cookies in your header.
 *
 * Since we generate HTTP headers in different places but still want to
 * always generate the cookies if possible (if they are available) we
 * have this function to add the cookies.
 *
 * This function directly outputs the cookies to the socket of the snap.cgi
 * tool.
 *
 * \sa cookie()
 * \sa set_cookie()
 * \sa cookie_is_defined()
 */
void snap_child::output_cookies()
{
    if(!f_cookies.isEmpty())
    {
        for(cookie_map_t::const_iterator it(f_cookies.begin());
                                         it != f_cookies.end();
                                         ++it)
        {
            // the to_http_header() ensures only ASCII characters
            // are used so we can use toLatin1() below
            QString cookie_header(it.value().to_http_header() + "\n");
//printf("snap session = [%s]?\n", cookie_header.toLatin1().data());
            write(cookie_header.toLatin1().data());
        }
    }
}


/** \brief Generate a unique number.
 *
 * This function uses a counter in a text file to generate a unique number.
 * The file is a 64 bit long number (binary) which gets locked to ensure
 * that the number coming out is unique.
 *
 * The resulting number is composed of the server name a dash and the
 * unique number generated from the unique number file.
 *
 * At this point it is not expected that we'd ever run out of unique
 * numbers. 2^64 per server is a really large number. However, you do
 * want to limit calls as much as possible (if you can reuse the same
 * number or check all possibilities that could cause an error before
 * getting the unique numbers so as to avoid wasting too many of them.)
 *
 * The server name is expected to be a unique name defined in the settings.
 * (the .conf file for the server.)
 *
 * \todo
 * All the servers in a given realm should all be given a unique name and
 * information about the other servers (i.e. at least the address of one
 * other server) so that way all the servers can communicate and make sure
 * that their name is indeed unique.
 *
 * \return A string with <server name>-<unique number>
 */
QString snap_child::get_unique_number()
{
    QString lock_path(f_server->get_parameter("data_path"));

    quint64 c(0);
    {
        QString name(lock_path + "/counter.u64");
        QLockFile counter(name);
        if(!counter.open(QIODevice::ReadWrite))
        {
            throw snap_child_exception_unique_number_error("count not open counter file \"" + name + "\"");
        }
        // the very first time the size is zero (empty)
        if(counter.size() != 0)
        {
            if(counter.read(reinterpret_cast<char *>(&c), sizeof(c)) != sizeof(c))
            {
                throw snap_child_exception_unique_number_error("count not read the counter file \"" + name + "\"");
            }
        }
        ++c;
        counter.reset();
        if(counter.write(reinterpret_cast<char *>(&c), sizeof(c)) != sizeof(c))
        {
            throw snap_child_exception_unique_number_error("count not write to the counter file \"" + name + "\"");
        }
        // close the file now; we do not want to hold the file for too long
    }
    return f_server->get_parameter("server_name") + "-" + QString("%1").arg(c);
}

/** \brief Initialize the plugins.
 *
 * Each site may make use of a different set of plugins. This function
 * gathers the list of available plugins and loads them as expected.
 *
 * The bare minimum is hard coded here in order to ensure that minimum
 * functionality of a website. At this time, this list is:
 *
 * \li path
 * \li filter
 * \li robotstxt
 */
void snap_child::init_plugins()
{
    // load the plugins for this website
    QtCassandra::QCassandraValue plugins(get_site_parameter(get_name(SNAP_NAME_CORE_PLUGINS)));
    QString site_plugins(plugins.stringValue());
    if(site_plugins.isEmpty())
    {
        // if the list of plugins is empty in the site parameters
        // then get the default from the server configuration
        site_plugins = f_server->get_parameter("default_plugins");
    }
    QStringList list_of_plugins(site_plugins.split(","));
    for(int i(0); i < list_of_plugins.length(); ++i)
    {
        if(list_of_plugins.at(i).isEmpty())
        {
            list_of_plugins.removeAt(i);
            --i;
        }
    }

    // ensure a certain minimum number of plugins
    static const char *minimum_plugins[] =
    {
        "path",
        "filter",
        "robotstxt",
        NULL
    };
    for(int i(0); minimum_plugins[i] != NULL; ++i)
    {
        if(!list_of_plugins.contains(minimum_plugins[i]))
        {
            list_of_plugins << minimum_plugins[i];
        }
    }

    // load the plugins
    if(!snap::plugins::load(f_server->get_parameter("plugins"), std::static_pointer_cast<snap::plugins::plugin>(f_server), list_of_plugins))
    {
        die(HTTP_CODE_SERVICE_UNAVAILABLE, "Plugin Unavailable", "Server encountered problems with its plugins.", "An error occured loading the server plugins.");
        NOTREACHED();
    }

    // now boot the plugin system
    f_server->bootstrap(this);
    f_server->init();

    // run updates if any
    update_plugins(list_of_plugins);
}

/** \brief Run all the updates as required.
 *
 * This function checks when the updates were last run. If never, then it
 * runs the update immediately. Otherwise, it waits at least 10 minutes
 * between running again to avoid overloading the server. We may increase
 * (i.e. wait more than 10 minutes) that amount of time as we get a better
 * feel of the necessity.
 *
 * The update is done by going through all the modules and checking their
 * modification date and time. If newer than what was registered for
 * them so far, then we call their do_update() function. When it never
 * ran, the modification date and time is always seen as \em newer and
 * thus all updates are run.
 *
 * \todo
 * We may want to look into a way to "install" a plugin which would have
 * the side effect of setting a flag requesting an update instead of
 * relying on the plugin .so file modification date and some of such
 * tricks. A clear signal sent via a command line tool or directly
 * from a website could be a lot more effective.
 *
 * \param[in] list_of_plugins  The list of plugin names that were loaded
 * for this run.
 */
void snap_child::update_plugins(const QStringList& list_of_plugins)
{
    // system updates run at most once every 10 minutes
    QString core_last_updated(get_name(SNAP_NAME_CORE_LAST_UPDATED));
    QString param_name(core_last_updated);
    QtCassandra::QCassandraValue last_updated(get_site_parameter(param_name));
    if(last_updated.nullValue())
    {
        // use an "old" date (631152000)
        last_updated.setInt64Value(SNAP_UNIX_TIMESTAMP(1990, 1, 1, 0, 0, 0) * 1000000LL);
    }
    int64_t last_update_timestamp(last_updated.int64Value());
    // 10 min. elapsed since last update?
    if(is_debug() // force update in debug mode so we don't have to wait 10 min.!
    || f_start_date - static_cast<int64_t>(last_update_timestamp) > static_cast<int64_t>(10 * 60 * 1000000))
    {
        // save that last time we checked for an update
        last_updated.setInt64Value(f_start_date);
        QString core_plugin_threshold(get_name(SNAP_NAME_CORE_PLUGIN_THRESHOLD));
        set_site_parameter(param_name, last_updated);
        QtCassandra::QCassandraValue threshold(get_site_parameter(core_plugin_threshold));
        if(threshold.nullValue())
        {
            // same old date...
            threshold.setInt64Value(SNAP_UNIX_TIMESTAMP(1990, 1, 1, 0, 0, 0) * 1000000LL);
        }
        int64_t plugin_threshold(threshold.int64Value());
        int64_t new_plugin_threshold(plugin_threshold);

        // first run through the plugins to know whether one or more
        // has changed since the last website update
        for(QStringList::const_iterator it(list_of_plugins.begin());
                it != list_of_plugins.end();
                ++it)
        {
            QString plugin_name(*it);
            plugins::plugin *p(plugins::get_plugin(plugin_name));
            if(p != NULL && p->last_modification() > plugin_threshold)
            {
                // the plugin changed, we want to call do_update() on it!
                if(p->last_modification() > new_plugin_threshold)
                {
                    new_plugin_threshold = p->last_modification();
                }
                // run the updates as required
                // we have a date/time for each plugin since each has
                // it's own list of date/time checks
                QString specific_param_name(core_last_updated + "::" + plugin_name);
                QtCassandra::QCassandraValue specific_last_updated(get_site_parameter(specific_param_name));
                if(specific_last_updated.nullValue())
                {
                    // use an "old" date (631152000)
                    specific_last_updated.setInt64Value(SNAP_UNIX_TIMESTAMP(1990, 1, 1, 0, 0, 0) * 1000000LL);
                }
                // IMPORTANT: Note that we save the newest date found in the
                //            do_update() to make 100% sure we catch all the
                //            updates every time (using "now" would often mean
                //            missing many updates!)
                specific_last_updated.setInt64Value(p->do_update(specific_last_updated.int64Value()));
                set_site_parameter(specific_param_name, specific_last_updated);
            }
        }

        // avoid a write to the DB if the value did not change
        // (i.e. most of the time!)
        if(new_plugin_threshold > plugin_threshold)
        {
            set_site_parameter(core_plugin_threshold, new_plugin_threshold);
        }
    }

    // if content was prepared for the database, save it now
    if(f_new_content)
    {
        f_new_content = false;
        f_server->save_content();
    }
}


/** \brief Called whenever a plugin prepares some content to the database.
 *
 * This function is called by the content plugin whenever one of its
 * add_...() function is called. This way the child knows that it has
 * to request the content to save the resulting content.
 *
 * The flag is first checked after the updates are run and the save is
 * called then. The check is done again at the end of the execute function
 * just in case some dynamic data was added while we were running.
 */
void snap_child::new_content()
{
    f_new_content = true;
}


/** \brief Canonalize a path or URL for this plugin.
 *
 * This function is used to canonicalize the paths used to check
 * URLs. This is used against the paths offered by other plugins
 * and the paths arriving from the HTTP server. This way, we know
 * that two paths will match 1 to 1.
 *
 * The canonicalization is done in place.
 *
 * Note that the canonicalization needs to occur before the
 * regular expresions are checked. Also, internal paths that
 * include regular expressions are not getting canonicalized
 * since we may otherwise break the regular expression
 * (i.e. unwillingly remove periods and slashes.)
 * This can explain why one of your paths doesn't work right.
 *
 * The function is really fast if the path is already canonicalized.
 *
 * \note
 * There is one drawback with "fixing" the URL from the user.
 * Two paths that look different will return the same page.
 * Instead we probably want to return an error (505 or 404
 * or 302.) This may be an dynamic setting too.
 *
 * \note
 * The remove() function on a QString is faster than most other
 * options because it is directly applied to the string data.
 *
 * \param[in,out] path  The path to canonicalize.
 */
void snap_child::canonicalize_path(QString& path)
{
    // we get the length on every loop because it could be reduced!
    int i(0);
    while(i < path.length())
    {
        switch(path[i].cell())
        {
        case '\\':
            path[i] = '/';
            break;

        case ' ':
        case '+':
        //case '_': -- this should probably be a flag?
            path[i] = '-';
            break;

        default:
            // other characters are kept as is
            break;

        }
        // here we do not have to check for \ since we just replaced it with /
        if(i == 0 && (path[0].cell() == '.' || path[0].cell() == '/' /*|| path[0].cell() == '\\'*/))
        {
            do
            {
                path.remove(0, 1);
            }
            while(!path.isEmpty() && (path[0].cell() == '.' || path[0].cell() == '/' || path[0].cell() == '\\'));
            // however, in the while we do since later characters were not yet
            // modified to just '/'
        }
        else if(path[i].cell()  == '/' && i + 1 < path.length())
        {
            if(path[i + 1].cell() == '/')
            {
                // remove double '/' in filename
                path.remove(i + 1, 1);
            }
            else if(path[i + 1].cell() == '.')
            {
                // Unix hidden files are forbidden (., .. and .<name>)
                // (here we remove 1 period, on next loop we may remove others)
                path.remove(i + 1, 1);
            }
            else
            {
                ++i;
            }
        }
        else if((path[i].cell() == '.' || path[i].cell() == '-' || path[i].cell() == '/') && i + 1 >= path.length())
        {
            // Filename cannot end with a period, dash (space,) or slash
            path.remove(i, 1);
        }
        else
        {
            ++i;
        }
    }
}


/** \brief We're ready to execute the page, do so.
 *
 * This time we're ready to execute the page the user is trying
 * to access.
 *
 * The function first prepares the HTTP request which includes
 * setting up default headers and the output buffer.
 *
 * Note that by default we expect text/html in the output. If a
 * different type of data is being processed, you are responsible
 * for changing the Content-type field.
 *
 * \todo
 * Take the Origin header in account. If it is not the right
 * origin, especially for log in, registration, and related
 * parges, then we may want to generate an error.
 */
void snap_child::execute()
{
    // prepare the output buffer
    // reserver 64Kb at once to avoid many tiny realloc()
    f_output.buffer().reserve(64 * 1024);
    f_output.open(QIODevice::ReadWrite);

    // TODO if the client says HTTP/1.0 and offers an Upgrade of 1.1, then
    //      we should force switch to 1.1 with a 101 response about here
    // TBD Apache may already take care of such things
    // TBD It may also be used to switch between HTTP and SHTTP
    //if(snapenv("SERVER_PROTOCOL").toUpper() == "HTTP/1.0")
    //{
    //    // if the Upgrade header was specified check whether it includes
    //    // HTTP/1.1 as one of the supported protocols
    //    // Note: we may want to make use of the http_string::WeightedHttpString
    //    //       object to part that client parameter
    //    if(snapenv("?REQUEST_UPGRADE?").simplified().replace(QRegExp("[ \t]?,[ \t]?"), ",").toLower().split(',', QString::SkipEmptyParts).contains("HTTP/1.1"))
    //    {
    //        set_header("Status", "HTTP/1.1 101 Switching Protocols");
    //    }
    //}

    // TODO: Check the cache request status from the client, if not defined
    //       or set to "max-age=0" or some other such value, then check whether
    //       the current page is cached and can safely be resent to the client
    //       (i.e. a public page without form...) if so send the cached version
    //       which will allow us to avoid all the processing.
    //
    // Note: the cached versions are saved really only if the page is
    //       public, mostly non-dynamic, and has no forms other than Search
    //       and similar...

    // prepare the default headers
    // Status is set to HTTP/1.1 or 1.0 depending on the incoming protocol
    // DO NOT PUT A STATUS OF 200 FOR FastCGI TAKES CARE OF IT
    // Sending a status of 200 to Apache results in a status of 500 Internal Server Error
    //set_header("Status", QString("%1 200 OK").arg(snapenv("SERVER_PROTOCOL")));

    // Normally Apache overwrites this information
    set_header("Server", "Snap! C++", HEADER_MODE_EVERYWHERE);

    // By default all pages are to expire in 1 minute (TBD)
    //QDateTime expires(QDateTime().toUTC());
    //expires.setTime_t(f_start_date / 1000000); // micro-seconds
    // TODO:
    // WARNING: the ddd and MMM are localized, we probably need to "fix"
    //          the locale before this call (?)
    //expires.toString("ddd, dd MMM yyyy hh:mm:ss' GMT'"));
    set_header("Expires", "Sat,  1 Jan 2000 00:00:00 GMT", HEADER_MODE_EVERYWHERE);

    // The Date field is added by Apache automatically
    // adding it here generates a 500 Internal Server Error
    //set_header("Date", expires.toString("ddd, dd MMM yyyy hh:mm:ss") + " GMT");

    // XXX it feels like Apache2 adds another no-cache at the end of the list
    set_header("Cache-Control", "no-store, no-cache, must-revalidate, post-check=0, pre-check=0", HEADER_MODE_EVERYWHERE);

    // By default we expect [X]HTML in the output
    set_header("Content-Type", "text/html; charset=utf-8", HEADER_MODE_EVERYWHERE);

    // Let the caches know that the cookie changes all the time
    // (the content is likely to change too, but it could still be cached)
    // TBD -- I'm not entirely sure that this is smart; another default is
    //        to use "Vary: *" so all fields are considered as varying.
    //set_header("Vary", "Cookie");

    if(f_uri.protocol() == "https")
    {
        // this is used by different load balancer as an indication that
        // the request is secure
        set_header("Front-End-Https", "on", HEADER_MODE_EVERYWHERE);
    }

    // give a chance to the system to use cookies such as the
    // cookie used to mark a user as logged in to kick in early
    f_server->process_cookies();

    // let plugins detach whatever data they attached to the user session
    f_server->detach_from_session();

    // get the action, if no action is defined, then use the default
    // which  is "view" unless we are POSTing
    verify_permissions();

    // if the user POSTed something, manage that content first, the
    // effect is often to redirect the user in which case we want to
    // emit an HTTP Location and return; also, with AJAX we may end
    // up stopping early (i.e. not generate a full page but instead
    // return the "form results".)
    if(f_has_post)
    {
        f_server->process_post(f_uri.path());
    }

    // generate the output
    //
    // on_execute() is defined in the path plugin which retrieves the
    // path::primary_owner of the content that match f_uri.path() and
    // then calls the corresponding on_path_execute() function of that
    // primary owner
    f_server->execute(f_uri.path());

    if(f_output.buffer().size() == 0)
    {
        // somehow nothing was output...
        die(HTTP_CODE_NOT_FOUND, "Page Empty",
            "Somehow this page could not be generated.",
            "the execute() command ran but the output is empty (this is never correct with HTML data, it could be with text/plain responses though)");
        NOTREACHED();
    }

    // created a page, output it now

    // Handling the compression has to be done before defining the
    // Content-Length header since that represents the compressed
    // data and not the full length

    // TODO add compression capabilities with bz2, lzma and sdch as
    //      may be supported by the browser
    QByteArray html_output;
    http_strings::WeightedHttpString encodings(snapenv("HTTP_ACCEPT_ENCODING"));

    // it looks like some browsers use that one instead of plain "gzip"
    // try both just in case
    float gzip_level(std::max(std::max(encodings.get_level("gzip"), encodings.get_level("x-gzip")), encodings.get_level("*")));
    float deflate_level(encodings.get_level("deflate"));
    if(gzip_level > 0.0f && gzip_level >= deflate_level)
    {
        // browser asked for gzip with higher preference
        QString compressor("gzip");
        html_output = compression::compress(compressor, f_output.buffer(), 100, true);
        if(compressor == "gzip")
        {
            // compression succeeded
            set_header("Content-Encoding", "gzip");
        }
    }
    else if(deflate_level > 0.0f)
    {
        QString compressor("deflate");
        html_output = compression::compress(compressor, f_output.buffer(), 100, true);
        if(compressor == "deflate")
        {
            // compression succeeded
            set_header("Content-Encoding", "deflate");
        }
    }
    else
    {
        // This 406 is in the spec. (RFC2616) but frankly?!
        float identity_level(encodings.get_level("identity"));
        if(identity_level == 0.0f)
        {
            die(HTTP_CODE_NOT_ACCEPTABLE, "No Acceptable Compression Encoding",
                "Your client requested a compression that we do not offer and it does not accept content without compression.",
                "a client requested content with Accept-Encoding: identify;q=0 and no other compression we understand");
            NOTREACHED();
        }
        html_output = f_output.buffer();
        // The "identity" SHOULD NOT be used with the Content-Encoding
        // (RFC 2616 -- https://tools.ietf.org/html/rfc2616)
        //set_header("Content-Encoding", "identity");
    }

    QString size(QString("%1").arg(html_output.size()));
    set_header("Content-Length", size);

    QString connection(snapenv("HTTP_CONNECTION"));
//printf("HTTP_CONNECTION=%s\n", connection.toUtf8().data());
    if(connection.toLower() == "keep-alive")
    {
        set_header("Connection", "keep-alive");
    }
    else
    {
        set_header("Connection", "close");
    }

    output_headers(HEADER_MODE_NO_ERROR);

    // write the body unless method is HEAD
    if(snapenv("REQUEST_METHOD") != "HEAD")
    {
        write(html_output, html_output.size());
// Warning: don't use this printf() if you allow compression... 8-)
//printf("%s [%d]\n", f_output.buffer().data(), f_output.buffer().size());
    }
}


/** \brief Verify for permissions.
 *
 * This function calculates the permissions of the user to access the
 * specified path with the specified action. If the result is that the
 * current user does not have permission to access the page, then the
 * function checks whether the user is logged in. If not, he gets
 * sent to the log in page after saving the current path as the place
 * to come back after logging in. If the user is already logged in,
 * then an Access Denied error is generated.
 */
void snap_child::verify_permissions()
{
    QString qs_action(f_server->get_parameter("qs_action"));
    QString action("view");
    if(f_uri.has_query_option(qs_action))
    {
        // the user specified an action
        action = f_uri.query_option(qs_action);
        if(action.isEmpty())
        {
            // use the default
            action = "view";
        }
    }

    // only actions that are defined in the permission types are
    // allowed, anything else is funky action from a hacker or
    // whatnot and we can either change it to a "view" action or
    // die with an error; this work is done by the permissions
    // plugin which changes the action parameter
    f_server->validate_action(f_uri.path(), action);

    // save the found action in the URI so that way any plugin can access that
    // information at any point, not just the verify_rights() function
    // XXX -- note that right now plugins cannot know what qs_action is!
    f_uri.set_query_option(qs_action, action);
}


/** \brief Convert a time/date value to a string.
 *
 * This function transform a date such as the content::modified field
 * to a format that is useful to the XSL parser. It supports a short
 * and a long form:
 *
 * \li Short: YYYY-MM-DD
 * \li Long: YYYY-MM-DDTHH:MM:SS
 *
 * The long format includes the time.
 *
 * The date is always output as UTC (opposed to local time.)
 *
 * \param[in] v  A 64 bit time / date value in microseconds, although we
 *               really only use precision to the second.
 * \param[in] long_format  Whether to use the short (default) or long format.
 *
 * \return The formatted date and time.
 */
QString snap_child::date_to_string(int64_t v, bool long_format)
{
    // go to seconds
    time_t seconds(v / 1000000);

    struct tm *time_info(gmtime(&seconds));

    char buf[256];

    strftime(
        buf,
        sizeof(buf),
        (long_format ? "%Y-%m-%dT%H:%M:%S" : "%Y-%m-%d"),
        time_info
    );

    return buf;
}


/** \brief Convert an RFC822 date to a time_t.
 *
 * This function transforms a date received by the client to a Unix
 * time_t value. We programmed our own because several fields are
 * optional and the strptime() function does not support such. Also
 * the strptime() uses the locale() for the day and month check
 * which is not expected for HTTP.
 *
 * \param[in] date  The date to convert to a time_t.
 *
 * \return The date and time as a Unix time_t number, -1 if the convertion fails.
 */
time_t snap_child::string822_to_date(QString const& date)
{
    const char *s(date.trimmed().toLower().toUtf8().data());
    struct tm time_info;

    // date-time   =  [ day "," ] date time
    // date        =  1*2DIGIT month 2DIGIT
    // time        =  hour zone
    // hour        =  2DIGIT ":" 2DIGIT [":" 2DIGIT]

    // clear the info as default
    memset(&time_info, 0, sizeof(time_info));

    // week day? (as far as I know mktime() doesn't use that one)
    //
    // day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
    //             /  "Fri"  / "Sat" /  "Sun"
    if(*s >= 'a' && *s <= 'z')
    {
        if(s[0] == 'm' && s[1] == 'o' && s[2] == 'n')
        {
            time_info.tm_wday = 1;
        }
        else if(s[0] == 't' && s[1] == 'u' && s[2] == 'e')
        {
            time_info.tm_wday = 2;
        }
        else if(s[0] == 'w' && s[1] == 'e' && s[2] == 'd')
        {
            time_info.tm_wday = 3;
        }
        else if(s[0] == 't' && s[1] == 'h' && s[2] == 'u')
        {
            time_info.tm_wday = 4;
        }
        else if(s[0] == 'f' && s[1] == 'r' && s[2] == 'i')
        {
            time_info.tm_wday = 5;
        }
        else if(s[0] == 's' && s[1] == 'a' && s[2] == 't')
        {
            time_info.tm_wday = 6;
        }
        else if(s[0] == 's' && s[1] == 'u' && s[2] == 'n')
        {
            time_info.tm_wday = 0;
        }
        else
        {
            // invalid weekday
            return -1;
        }
        if(s[3] != ',')
        {
            return -1;
        }
        s += 4;
        while(isspace(*s))
        {
            ++s;
        }
    }

    // day of the month (1*2DIGIT)
    if(s[0] < '0' || s[0] > '9')
    {
        return -1;
    }
    if(s[1] >= '0' && s[1] <= '9')
    {
        time_info.tm_mday = (s[0] - '0') * 10 + s[1] - '0';
        s += 2;
    }
    else
    {
        time_info.tm_mday = s[0] - '0';
        ++s;
    }

    if(!isspace(*s))
    {
        return -1;
    }
    do
    {
        ++s;
    }
    while(isspace(*s));

    // month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
    //             /  "May"  /  "Jun" /  "Jul"  /  "Aug"
    //             /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"
    if(s[0] == 'j' && s[1] == 'a' && s[2] == 'n')
    {
        time_info.tm_mon = 0;
    }
    else if(s[0] == 'f' && s[1] == 'e' && s[2] == 'b')
    {
        time_info.tm_mon = 1;
    }
    else if(s[0] == 'm' && s[1] == 'a' && s[2] == 'r')
    {
        time_info.tm_mon = 2;
    }
    else if(s[0] == 'a' && s[1] == 'p' && s[2] == 'r')
    {
        time_info.tm_mon = 3;
    }
    else if(s[0] == 'm' && s[1] == 'a' && s[2] == 'y')
    {
        time_info.tm_mon = 4;
    }
    else if(s[0] == 'j' && s[1] == 'u' && s[2] == 'n')
    {
        time_info.tm_mon = 5;
    }
    else if(s[0] == 'j' && s[1] == 'u' && s[2] == 'l')
    {
        time_info.tm_mon = 6;
    }
    else if(s[0] == 'a' && s[1] == 'u' && s[2] == 'g')
    {
        time_info.tm_mon = 7;
    }
    else if(s[0] == 's' && s[1] == 'e' && s[2] == 'p')
    {
        time_info.tm_mon = 8;
    }
    else if(s[0] == 'o' && s[1] == 'c' && s[2] == 't')
    {
        time_info.tm_mon = 9;
    }
    else if(s[0] == 'n' && s[1] == 'o' && s[2] == 'v')
    {
        time_info.tm_mon = 10;
    }
    else if(s[0] == 'd' && s[1] == 'e' && s[2] == 'c')
    {
        time_info.tm_mon = 11;
    }
    else
    {
        // invalid month
        return -1;
    }

    s += 3;
    if(!isspace(*s))
    {
        return -1;
    }
    do
    {
        ++s;
    }
    while(isspace(*s));

    // year (2DIGIT)
    if(s[0] < '0' || s[0] > '9'
    || s[1] < '0' || s[1] > '9')
    {
        return -1;
    }
    time_info.tm_year = 1900 + (s[0] - '0') * 10 + s[1] - '0';

    // How to handle this one? At this time I do not expect our software
    // to work beyond 2070 which is probably short sighted (ha! ha!)
    // However, that way we avoid calling time() and transform that in
    // a tm structure and check that date
    if(time_info.tm_year < 1970)
    {
        time_info.tm_year += 100;
    }

    // hour (2DIGIT)
    if(s[0] < '0' || s[0] > '9'
    || s[1] < '0' || s[1] > '9'
    || s[2] != ':')
    {
        return -1;
    }
    time_info.tm_hour = 1900 + (s[0] - '0') * 10 + s[1] - '0';
    s += 3;

    // minute (2DIGIT)
    if(s[0] < '0' || s[0] > '9'
    || s[1] < '0' || s[1] > '9')
    {
        return -1;
    }
    time_info.tm_min = (s[0] - '0') * 10 + s[1] - '0';
    s += 2;

    if(*s == ':')
    {
        ++s;

        // second (2DIGIT)
        if(s[0] < '0' || s[0] > '9'
        || s[1] < '0' || s[1] > '9')
        {
            return -1;
        }
        time_info.tm_sec = (s[0] - '0') * 10 + s[1] - '0';
        s += 2;
    }

    while(isspace(*s))
    {
        ++s;
    }

    if(*s != '\0')
    {
        // not too sure that the zone is properly handled at this point, TBD
        // (i.e. should I do += or -=, it may be wrong in many places...)
        //
        // zone        =  "UT"  / "GMT"
        //             /  "EST" / "EDT"
        //             /  "CST" / "CDT"
        //             /  "MST" / "MDT"
        //             /  "PST" / "PDT"
        //             /  1ALPHA
        //             / ( ("+" / "-") 4DIGIT )
        if((s[0] == 'u' && s[1] == 't' && s[2] == '\0')                 // UT
        || (s[0] == 'u' && s[1] == 't' && s[2] == 'c' && s[3] == '\0')  // UTC (not in the spec...)
        || (s[0] == 'g' && s[1] == 'm' && s[2] == 't' && s[3] == '\0')) // GMT
        {
            // no adjustment for UTC (GMT)
        }
        else if(s[0] == 'e' && s[1] == 's' && s[2] == 't' && s[3] == '\0') // EST
        {
            time_info.tm_hour -= 5;
        }
        else if(s[0] == 'e' && s[1] == 'd' && s[2] == 't' && s[3] == '\0') // EDT
        {
            time_info.tm_hour -= 4;
        }
        else if(s[0] == 'c' && s[1] == 's' && s[2] == 't' && s[3] == '\0') // CST
        {
            time_info.tm_hour -= 6;
        }
        else if(s[0] == 'c' && s[1] == 'd' && s[2] == 't' && s[3] == '\0') // CDT
        {
            time_info.tm_hour -= 5;
        }
        else if(s[0] == 'm' && s[1] == 's' && s[2] == 't' && s[3] == '\0') // MST
        {
            time_info.tm_hour -= 7;
        }
        else if(s[0] == 'm' && s[1] == 'd' && s[2] == 't' && s[3] == '\0') // MDT
        {
            time_info.tm_hour -= 6;
        }
        else if(s[0] == 'p' && s[1] == 's' && s[2] == 't' && s[3] == '\0') // PST
        {
            time_info.tm_hour -= 8;
        }
        else if(s[0] == 'p' && s[1] == 'd' && s[2] == 't' && s[3] == '\0') // PDT
        {
            time_info.tm_hour -= 7;
        }
        else if(s[0] >= 'a' && s[0] <= 'z' && s[0] != 'j' && s[1] == '\0')
        {
            signed char adjust[26] = {
                /* A */ -1,
                /* B */ -2,
                /* C */ -3,
                /* D */ -4,
                /* E */ -5,
                /* F */ -6,
                /* G */ -7,
                /* H */ -8,
                /* I */ -9,
                /* J */ 0, // not used
                /* K */ -10,
                /* L */ -11,
                /* M */ -12,
                /* N */ 1,
                /* O */ 2,
                /* P */ 3,
                /* Q */ 4,
                /* R */ 5,
                /* S */ 6,
                /* T */ 7,
                /* U */ 8,
                /* V */ 9,
                /* W */ 10,
                /* X */ 11,
                /* Y */ 12,
                /* Z */ 0,
            };
            time_info.tm_hour += adjust[s[0] - 'a'];
        }
        else if((s[0] == '+' || s[0] == '-')
              && s[1] >= '0' && s[1] <= '9'
              && s[2] >= '0' && s[2] <= '9'
              && s[3] >= '0' && s[3] <= '9'
              && s[4] >= '0' && s[4] <= '9'
              && s[5] == '\0')
        {
            time_info.tm_hour += ((s[1] - '0') * 10 + s[2] - '0') * (s[0] == '+' ? 1 : -1);
            time_info.tm_min  += ((s[3] - '0') * 10 + s[4] - '0') * (s[0] == '+' ? 1 : -1);
        }
        else
        {
            // invalid month
            return -1;
        }
    }

    // now we have a time_info which is fully adjusted except for DST...
    // let's make time
    return mktime(&time_info);
}


/** \brief Send a PING message to the specified UDP server.
 *
 * This function sends a PING message (4 bytes) to the specified
 * UDP server. This is used after you saved data in the Cassandra
 * cluster to wake up a background process which can then "slowly"
 * process the data further.
 *
 * Remember that UDP is not reliable so we do not in any way
 * guarantee that this goes anywhere. The function returns no
 * feedback at all. We do not wait for a reply since at the time
 * we send the message the listening server may be busy. The
 * idea of this ping is just to make sure that if the server is
 * sleeping at that time, it wakes up sooner rather than later
 * so it can immediately start processing the data we just added
 * to Cassandra.
 *
 * The \p message is expected to be a NUL terminated string. The
 * NUL is not sent across. At this point most of our servers
 * accept a PING message to wake up and start working on new
 * data.
 *
 * The \p name parameter is the name of a variable in the server
 * configuration file.
 *
 * \param[in] name  The name of the configuration variable used to read the IP and port
 * \param[in] message  The message to send, "PING" by default.
 */
void snap_child::udp_ping(const char *name, const char *message)
{
    f_server->udp_ping(name, message);
}


/** \brief Create a UDP server that receives udp_ping() messages.
 *
 * This function is used to receive PING messages from the udp_ping()
 * function. Other messages can also be sent such as RSET and STOP.
 *
 * The server is expected to be used with the recv() or timed_recv()
 * functions to wait for a message and act accordingly. A server
 * that makes use of these pings is expected to be waiting for some
 * data which, once available requires additional processing. The
 * server that handles the row data sends the PING to the server.
 * For example, the sendmail plugin just saves the email data in
 * the Cassandra database, then it sends a PING to the sendmail
 * backend process. That backend process wakes up and actually
 * processes the email by sending it to the mail server.
 *
 * \param[in] name  The name of the configuration variable used to read the IP and port
 */
QSharedPointer<udp_client_server::udp_server> snap_child::udp_get_server(const char *name)
{
    // TODO: we should have a common function to read and transform the
    //       parameter to a valid IP/Port pair (see above)
    QString udp_addr_port(f_server->get_parameter(name));
    QString addr, port;
    int bracket(udp_addr_port.lastIndexOf("]"));
    int p(udp_addr_port.lastIndexOf(":"));
    if(bracket != -1 && p != -1)
    {
        if(p > bracket)
        {
            // IPv6 port specification
            addr = udp_addr_port.mid(0, bracket + 1); // include the ']'
            port = udp_addr_port.mid(p + 1); // ignore the ':'
        }
        else
        {
            throw std::runtime_error("invalid [IPv6]:port specification, port missing for UDP ping");
        }
    }
    else if(p != -1)
    {
        // IPv4 port specification
        addr = udp_addr_port.mid(0, p); // ignore the ':'
        port = udp_addr_port.mid(p + 1); // ignore the ':'
    }
    else
    {
        throw std::runtime_error("invalid IPv4:port specification, port missing for UDP ping");
    }
    QSharedPointer<udp_client_server::udp_server> server(new udp_client_server::udp_server(addr.toUtf8().data(), port.toInt()));
    if(server.isNull())
    {
        // this should not happen since std::badalloc is raised when allocation fails
        // and the new operator will rethrow any exception that the constructor throws
        throw std::runtime_error("server could not be allocated");
    }
    return server;
}


} // namespace snap

// vim: ts=4 sw=4 et
