/*
 * Text:
 *      snapwebsites/snapmanagercgi/lib/installer.cpp
 *
 * Description:
 *      The implementation of the INSTALL function.
 *
 * License:
 *      Copyright (c) 2016 Made to Order Software Corp.
 *
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


// ourselves
//
#include "manager.h"

// snapwebsites lib
//
#include "file_content.h"
#include "lockfile.h"
#include "log.h"
#include "not_used.h"
#include "process.h"
#include "tokenize_string.h"

// Qt lib
//
#include <QFile>

// C lib
//
#include <fcntl.h>
#include <sys/wait.h>

namespace snap_manager
{


namespace
{

void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}

} // no name namespace



/** \brief Check whether a package is installed.
 *
 * This function runs a query to determine whether a named page
 * is installed or not.
 *
 * The output of the dpkg-query command we expect includes the
 * following four words:
 *
 * \code
 *      <version> install ok installed
 * \endcode
 *
 * The \<version> part will be the current version of that package.
 * The "install ok installed" part is the current status dpkg considered
 * the package in. When exactly that, it is considered that the package
 * is properly installed.
 *
 * \param[in] package_name  The name of the package to install.
 * \param[out] output  The output of the dpkg-query commmand.
 *
 * \return The exit code of the dpkg-query install command.
 */
int manager::package_status(std::string const & package_name, std::string & output)
{
    output.clear();

    snap::process p("query package status");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
    p.set_command("dpkg-query");
    p.add_argument("--showformat='${Version} ${Status}\\n'");
    p.add_argument("--show");
    p.add_argument(QString::fromUtf8(package_name.c_str()));
    int const r(p.run());

    // the output is saved so we can send it to the user and log it...
    if(r == 0)
    {
        output = p.get_output(true).toUtf8().data();
    }

    return r;
}


QString manager::count_packages_that_can_be_updated(bool check_cache)
{
    QString const cache_filename(QString("%1/apt-check.output").arg(f_cache_path));

    // check whether we have a cached version of the data, if so, use
    // the cache (which is dead fast in comparison to re-running the
    // apt-check function)
    //
    if(check_cache)
    {
        QFile cache(cache_filename);
        if(cache.open(QIODevice::ReadOnly))
        {
            QByteArray content_buffer(cache.readAll());
            if(content_buffer.size() > 0)
            {
                if(content_buffer.at(content_buffer.size() - 1) == '\n')
                {
                    content_buffer.resize(content_buffer.size() - 1);
                }
            }
            QString const content(QString::fromUtf8(content_buffer));
            snap::snap_string_list counts(content.split(";"));
            if(counts.size() == 1
            && counts[0] == "-1")
            {
                // the function to check that information was not available
                return QString();
            }
            if(counts.size() == 3)
            {
                time_t const now(time(nullptr));
                time_t const cached_on(counts[0].toLongLong());
                if(cached_on + 86400 >= now)
                {
                    // cache is still considered valid
                    //
                    if(counts[1] == "0")
                    {
                        // nothing needs to be upgraded
                        return QString();
                    }
                    // counts[1] packages can be upgraded
                    // counts[2] are security upgrades
                    return QString("%1;%2").arg(counts[1]).arg(counts[2]);
                }
            }
        }
    }

    // check whether we have an apt-check function were we expect it
    //
    QByteArray apt_check(f_apt_check.toUtf8());
    struct stat st;
    if(stat(apt_check.data(), &st) == 0
    && S_ISREG(st.st_mode)
    && (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0)  // make sure it is an executable
    {
        // without a quick apt-get update first the calculations from
        // apt-check are going to be off...
        //
        if(update_packages("update") == 0)
        {
            // apt-check is expected to be a python script and the output
            // will be written in 'stderr'
            //
            snap::process p("apt-check");
            p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
            p.set_command(f_apt_check);
            p.add_argument("2>&1"); // python script sends output to STDERR
            int const r(p.run());
            if(r == 0)
            {
                QString const output(p.get_output(true).toUtf8().data());
                if(!output.isEmpty())
                {
                    QFile cache(cache_filename);
                    if(cache.open(QIODevice::WriteOnly))
                    {
                        time_t const now(time(nullptr));
                        QString const cache_string(QString("%1;%2").arg(now).arg(output));
                        QByteArray const cache_utf8(cache_string.toUtf8());
                        cache.write(cache_utf8.data(), cache_utf8.size());
                        if(output == "0;0")
                        {
                            // again, if we have "0;0" there is nothing to upgrade
                            //
                            return QString();
                        }
                        return output;
                    }
                }
            }
        }
        else
        {
            // this should rarely happen (i.e. generally it would happen
            // whenever the database is in an unknown state)
            //
            SNAP_LOG_ERROR("the \"apt-get update\" command, that we run prior to running the \"apt-check\" command, failed.");
        }
    }

    SNAP_LOG_ERROR("the snapmanagercgi library could not run \"")(f_apt_check)("\" successfully or the output was invalid.");

    {
        QFile cache(cache_filename);
        if(cache.open(QIODevice::WriteOnly))
        {
            // apt-check command is failing... do not try again
            //
            cache.write("-1", 2);
        }
        else
        {
            SNAP_LOG_ERROR("the snapmanagercgi library could not create \"")(cache_filename)("\".");
        }
    }

    // pretend there is nothing to upgrade
    //
    return QString();
}


/** \brief Update the OS packages.
 *
 * This function updates the database of the OS packages.
 *
 * Since snapmanager is already installed, we do not have to do any extra
 * work to get that repository installed.
 *
 * \param[in] command  One of "update", "upgrade", "dist-upgrade", or
 * "autoremove".
 *
 * \return The exit code of the apt-get update command.
 */
int manager::update_packages(std::string const & command)
{
#ifdef _DEBUG
    std::vector<std::string> allowed_commands{"update", "upgrade", "dist-upgrade", "autoremove"};
    if(std::find(allowed_commands.begin(), allowed_commands.end(), command) == allowed_commands.end())
    {
        throw std::logic_error("install_package was called with an invalid command.");
    }
#endif

    snap::process p("update");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
    p.set_command("apt-get");
    p.add_argument("--quiet");
    p.add_argument("--assume-yes");
    if(command == "upgrade"
    || command == "dist-upgrade")
    {
        p.add_argument("--option");
        p.add_argument("Dpkg::Options::=--force-confdef");
        p.add_argument("--option");
        p.add_argument("Dpkg::Options::=--force-confold");
    }
    p.add_argument(QString::fromUtf8(command.c_str()));
    p.add_environ("DEBIAN_FRONTEND", "noninteractive");
    int r(p.run());

    // the output is saved so we can send it to the user and log it...
    QString const output(p.get_output(true));
    SNAP_LOG_INFO(command)(" of packages returned:\n")(output);

    return r;
}



/** \brief Installs one Debian package.
 *
 * This function installs ONE package as specified by \p package_name.
 *
 * \param[in] package_name  The name of the package to install.
 * \param[in] command  One of "install", "remove", or "purge".
 *
 * \return The exit code of the apt-get install command.
 */
int manager::install_package(std::string const & package_name, std::string const & command)
{
#ifdef _DEBUG
    std::vector<std::string> allowed_commands{"install", "remove", "purge"};
    if(std::find(allowed_commands.begin(), allowed_commands.end(), command) == allowed_commands.end())
    {
        throw std::logic_error("install_package was called with an invalid command.");
    }
#endif

    snap::process p("install");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
    p.set_command("apt-get");
    p.add_argument("--quiet");
    p.add_argument("--assume-yes");
    if(command == "install")
    {
        p.add_argument("--option");
        p.add_argument("Dpkg::Options::=--force-confdef");
        p.add_argument("--option");
        p.add_argument("Dpkg::Options::=--force-confold");
        p.add_argument("--no-install-recommends");
    }
    p.add_argument(QString::fromUtf8(command.c_str()));
    p.add_argument(QString::fromUtf8(package_name.c_str()));
    p.add_environ("DEBIAN_FRONTEND", "noninteractive");
    int r(p.run());

    // the output is saved so we can send it to the user and log it...
    QString const output(p.get_output(true));
    SNAP_LOG_INFO(command)(" of package named \"")(package_name)("\" output:\n")(output);

    return r;
}



void manager::reset_aptcheck()
{
    // cache is not unlikely wrong after that
    //
    QString const cache_filename(QString("%1/apt-check.output").arg(f_cache_path));
    unlink(cache_filename.toUtf8().data());

    // also make sure that the bundles.status get regenerated (i.e. the
    // dpkg-query calls)
    //
    QString const bundles_status_filename(QString("%1/bundles.status").arg(f_bundles_path));
    unlink(bundles_status_filename.toUtf8().data());
}


bool manager::upgrader()
{
    // TODO: add command path/name to the configuration file?
    //
    snap::process p("upgrader");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
    p.set_command("snapupgrader");
    p.add_argument("--config");
    p.add_argument( QString::fromUtf8( f_opt->get_string("config").c_str() ) );
    p.add_argument("--data-path");
    p.add_argument(f_data_path);
    if(f_debug)
    {
        p.add_argument("--debug");
    }
    p.add_argument("--log-config");
    p.add_argument(f_log_conf);
    p.add_argument("--server-name");
    p.add_argument(f_server_name);
    int const r(p.run());
    if(r != 0)
    {
        // TODO: get errors to front end...
        //
        // TODO: move the error handling to the snap::process code instead?
        //
        if(r < 0)
        {
            // could not even start the process
            //
            int const e(errno);
            SNAP_LOG_ERROR("could not properly start snapupgrader (errno: ")(e)(", ")(strerror(e))(").");
        }
        else
        {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
            // process started but returned with an error
            //
            if(WIFEXITED(r))
            {
                int const exit_code(WEXITSTATUS(r));
                SNAP_LOG_ERROR("could not properly start snapupgrader (exit code: ")(exit_code)(").");
            }
            else if(WIFSIGNALED(r))
            {
                int const signal_code(WTERMSIG(r));
                bool const has_code_dump(!!WCOREDUMP(r));

                SNAP_LOG_ERROR("snapupgrader terminated because of OS signal \"")
                              (strsignal(signal_code))
                              ("\" (")
                              (signal_code)
                              (")")
                              (has_code_dump ? " and a core dump was generated" : "")
                              (".");
            }
            else
            {
                // I do not think we can reach here...
                //
                SNAP_LOG_ERROR("snapupgrader terminated abnormally in an unknown way.");
            }
#pragma GCC diagnostic pop

        }
        return false;
    }

#if 0
    // if the parent dies, then we generally receive a SIGHUP which is
    // a big problem if we want to go on with the update... so here I
    // make sure we ignore the HUP signal.
    //
    // (this should really only happen if you have a terminal attached
    // to the process, but just in case...)
    //
    signal(SIGHUP, SIG_IGN);

    // setsid() moves us in the group instead of being viewed as a child
    //
    setsid();

    //pid_t const sub_pid(fork());
    //if(sub_pid != 0)
    //{
    //    // we are the sub-parent, by leaving now the child has
    //    // a parent PID equal to 1 meaning that it cannot be
    //    // killed just because snapmanagerdaemon gets killed
    //    //
    //    exit(0);
    //}

    bool const success(update_packages("update")       == 0
                    && update_packages("upgrade")      == 0
                    && update_packages("dist-upgrade") == 0
                    && update_packages("autoremove")   == 0);

    // we have to do this one here now
    //
    reset_aptcheck();

    exit(success ? 0 : 1);

    snap::NOTREACHED();
#endif

    return true;
}


std::string manager::lock_filename() const
{
    return (f_lock_path + "/upgrading.lock").toUtf8().data();
}


bool manager::installer(QString const & bundle_name, std::string const & command, std::string const & install_values)
{
    bool success(true);

    SNAP_LOG_INFO("Installing bundle \"")(bundle_name)("\" on host \"")(f_server_name)("\"");

    // make sure we do not start an installation while an upgrade is
    // still going (and vice versa)
    //
    snap::lockfile lf(lock_filename(), snap::lockfile::mode_t::LOCKFILE_EXCLUSIVE);
    if(!lf.try_lock())
    {
        return false;
    }

    // for installation we first do an update of the packages,
    // otherwise it could fail the installation because of
    // outdated data
    //
    if(command == "install")
    {
        // we cannot "just upgrade" now because the upgrader() function
        // calls fork() and this the call would return early. Instead
        // we check the number of packages that are left to upgrade
        // and if not zero, emit an error and return...
        //success = upgrader();

        QString const count_packages(count_packages_that_can_be_updated(false));
        if(!count_packages.isEmpty())
        {
            // TODO: how do we tell the end user about that one?
            //
            SNAP_LOG_ERROR("Installation of bundle \"")
                          (bundle_name)
                          ("\" on host \"")
                          (f_server_name)
                          ("\" did not proceed because some packages first need to be upgraded.");
            return false;
        }
    }

    // load the XML file
    //
    QDomDocument bundle_xml;
    QString const filename(QString("%1/bundle-%2.xml").arg(f_bundles_path).arg(bundle_name));
    QFile input(filename);
    if(!input.open(QIODevice::ReadOnly)
    || !bundle_xml.setContent(&input, false))
    {
        SNAP_LOG_ERROR("bundle \"")(filename)("\" could not be opened or has invalid XML data. Skipping.");
        return false;
    }

    // install_values is a string of variables that come from the list
    // of fields defined in the bundle file
    //
    std::vector<std::string> variables;
    snap::NOTUSED(snap::tokenize_string(variables, install_values, "\n", true, " "));
    std::string vars;
    std::for_each(variables.begin(), variables.end(),
                [&vars](auto const & v)
                {
                    // TODO: move to a function, this is just too long for a lambda
                    //
                    vars += "BUNDLE_INSTALLATION_";
                    bool found_equal(false);
                    // make sure that double quotes get escaped within
                    // the string
                    //
                    for(auto const c : v)
                    //for(size_t p(0); p < v.length(); ++p)
                    {
                        if(c == '\r'
                        || c == '\n')
                        {
                            // these characters should not happen in those
                            // strings, but just in case...
                            //
                            continue;
                        }

                        if(found_equal)
                        {
                            if(c == '"')
                            {
                                vars += '\\';
                            }
                            vars += c;
                        }
                        else if(c == '=')
                        {
                            found_equal = true;
                            vars += "=\"";
                        }
                        else
                        {
                            if(c >= 'a' && c <= 'z')
                            {
                                // force ASCII uppercase for the name
                                //
                                vars += c & 0x5f;
                            }
                            else
                            {
                                vars += c;
                            }
                        }
                    }
                    vars += "\"\n"; // always add a new line at the end
                });

    // there may be some pre-installation instructions
    //
    QDomNodeList bundle_preinst(bundle_xml.elementsByTagName("preinst"));
    if(bundle_preinst.size() == 1)
    {
        // create a <name>.preinst script that we can run
        //
        std::string path(f_data_path.toUtf8().data());
        path += "/bundle-scripts/"; 
        path += bundle_name.toUtf8().data();
        path += ".preinst";
        snap::file_content script(path);
        QDomElement preinst(bundle_preinst.at(0).toElement());
        script.set_content("# auto-generated by snapmanagerdaemon\n" + vars + preinst.text().toUtf8().data());
        script.write_all();
        snap::process p("preinst");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command(QString::fromUtf8(path.c_str()));
        int const r(p.run());
        if(r != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("pre-installation script failed with ")(r)(" (errno: ")(e)(", ")(strerror(e));
            // if the pre-installation script fails, we do not attempt to
            // install the packages
            //
            return false;
        }
    }

    // get the list of expected packages, it may be empty/non-existant
    //
    QDomNodeList bundle_packages(bundle_xml.elementsByTagName("packages"));
    if(bundle_packages.size() == 1)
    {
        QDomElement package_list(bundle_packages.at(0).toElement());
        std::string const list_of_packages(package_list.text().toUtf8().data());
        std::vector<std::string> packages;
        snap::NOTUSED(snap::tokenize_string(packages, list_of_packages, ",", true, " "));
        std::for_each(packages.begin(), packages.end(),
                [=, &success](auto const & p)
                {
                    // we want to call all the install even if a
                    // previous one (or the update) failed
                    //
                    success = this->install_package(p, command) && success;
                });
    }

    // there may be some post installation instructions
    //
    QDomNodeList bundle_postinst(bundle_xml.elementsByTagName("postinst"));
    if(bundle_postinst.size() == 1)
    {
        // create a <name>.postinst script that we can run
        //
        std::string path(f_data_path.toUtf8().data());
        path += "/bundle-scripts/"; 
        path += bundle_name.toUtf8().data();
        path += ".postinst";
        snap::file_content script(path);
        QDomElement postinst(bundle_postinst.at(0).toElement());
        script.set_content("# auto-generated by snapmanagerdaemon\n" + vars + postinst.text().toUtf8().data());
        script.write_all();
        snap::process p("postinst");
        p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
        p.set_command(QString::fromUtf8(path.c_str()));
        int const r(p.run());
        if(r != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("post installation script failed with ")(r)(" (errno: ")(e)(", ")(strerror(e));
            // not much we can do if the post installation fails
            // (we could remove the packages, but that could be dangerous too)
            success = false;
        }
    }


    return success;
}


/** \brief Reboot or shutdown a computer.
 *
 * This function sends the OS the necessary command(s) to reboot or
 * shutdown a computer system.
 *
 * In some cases, the shutdown is to be done cleanly, meaning that
 * the machine has to unregister itself first, making sure that all
 * others know that the machine is going to go down. Once that
 * disconnect was accomplished, then the shutdown happens.
 *
 * If the function is set to reboot, it will reconnect as expected
 * once it comes back.
 *
 * Also, if multiple machines (all?) are asked to reboot, then it
 * has to be done one after another and not all at once (all at
 * once would kill the cluster!)
 *
 * \param[in] reboot  Whether to reboot (true) or just shutdown (false).
 */
void manager::reboot(bool reboot)
{
    // TODO: we need many different ways to reboot a machine cleanly;
    //       especially front ends and database machines which need
    //       to first be disconnected by all, then rebooted;
    //       also shutdowns have to be coordinated between computers:
    //       one computer cannot decide by itself whether to it can
    //       go down or now...
    //

    // TODO: we could test whether the installer is busy upgrading or
    //       installing something at least (see lockfile() in those
    //       functions.)

    snap::process p("shutdown");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
    p.set_command("shutdown");
    if(reboot)
    {
        p.add_argument("--reboot");
    }
    else
    {
        p.add_argument("--poweroff");
    }
    p.add_argument("now");
    p.add_argument("Shutdown initiated by Snap! Manager Daemon");
    snap::NOTUSED(p.run());
}


bool manager::replace_configuration_value(QString const & filename, QString const & field_name, QString const & new_value)
{
    QString const line(QString("%1=%2\n").arg(field_name).arg(new_value));
    QByteArray utf8_line(line.toUtf8());

    // make sure to create the file if it does not exist
    // we expect the filename parameter to be something like
    //     /etc/snapwebsites/snapwebsites.d/<filename>
    //
    int fd(open(filename.toUtf8().data(), O_RDWR));
    if(fd == -1)
    {
        fd = open(filename.toUtf8().data(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(fd == -1)
        {
            SNAP_LOG_INFO("could not create file \"")(filename)("\" to save new configuration value.");
            return false;
        }
        std::shared_ptr<int> safe_fd(&fd, file_descriptor_deleter);

        std::string const comment("# This file was auto-generated by snapmanager.cgi\n"
                                  "# Feel free to do additional modifications here as\n"
                                  "# snapmanager.cgi will be aware of them as expected.\n");
        if(::write(fd, comment.c_str(), comment.size()) != static_cast<ssize_t>(comment.size()))
        {
            int const e(errno);
            SNAP_LOG_ERROR("write of header to \"")(filename)("\" failed (errno: ")(e)(", ")(strerror(e))(")");
            return false;
        }
        if(::write(fd, utf8_line.data(), utf8_line.size()) != utf8_line.size())
        {
            int const e(errno);
            SNAP_LOG_ERROR("writing of new line to \"")(filename)("\" failed (errno: ")(e)(", ")(strerror(e))(")");
            return false;
        }
    }
    else
    {
        std::shared_ptr<int> safe_fd(&fd, file_descriptor_deleter);

        // TODO: test all returns and if -1 err!
        //
        off_t const size(lseek(fd, 0, SEEK_END));
        snap::NOTUSED(lseek(fd, 0, SEEK_SET));

        std::unique_ptr<char> buf(new char[size]);
        if(read(fd, buf.get(), size) != size)
        {
            int const e(errno);
            SNAP_LOG_ERROR("writing of new line to \"")(filename)("\" failed (errno: ")(e)(", ")(strerror(e))(")");
            return false;
        }

        // TBD: Often administrators a way to define the backup extension?
        {
            int bak(open((filename + ".bak").toUtf8().data(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
            if(bak == -1)
            {
                SNAP_LOG_INFO("could not create backup file \"")(filename)(".bak\" to save new configuration value.");
                return false;
            }
            std::shared_ptr<int> safe_bak(&bak, file_descriptor_deleter);
            if(::write(bak, buf.get(), size) != size)
            {
                SNAP_LOG_INFO("could not save buffer to backup file \"")(filename)(".bak\" before generating a new version.");
                return false;
            }
        }

        // TODO: we do not need to truncate the whole file, only from
        //       the field if it is found, and even only if the new
        //       value is not the exact same size
        //
        snap::NOTUSED(lseek(fd, 0, SEEK_SET));
        snap::NOTUSED(::ftruncate(fd, 0));

        QByteArray field_name_utf8((field_name + "=").toUtf8());

        bool found(false);
        char const * s(buf.get());
        char const * end(s + size);
        for(char const * start(s); s < end; ++s)
        {
            if(*s == '\n')
            {
                // length without the '\n'
                //
                size_t len(s - start);
                if(len >= static_cast<size_t>(field_name_utf8.size())
                && strncmp(start, field_name_utf8.data(), field_name_utf8.size()) == 0)
                {
                    // we found the field the user is asking to update
                    //
                    found = true;
                    if(::write(fd, utf8_line.data(), utf8_line.size()) != utf8_line.size())
                    {
                        int const e(errno);
                        SNAP_LOG_ERROR("writing of new line to \"")(filename)("\" failed (errno: ")(e)(", ")(strerror(e))(")");
                        return false;
                    }
                }
                else
                {
                    // include the '\n'
                    //
                    ++len;
                    if(::write(fd, start, len) != static_cast<ssize_t>(len))
                    {
                        int const e(errno);
                        SNAP_LOG_ERROR("writing back of line to \"")(filename)("\" failed (errno: ")(e)(", ")(strerror(e))(")");
                        return false;
                    }
                }
                start = s + 1;
            }
        }
        if(!found)
        {
            if(::write(fd, utf8_line.data(), utf8_line.size()) != utf8_line.size())
            {
                int const e(errno);
                SNAP_LOG_ERROR("writing of new line at the end of \"")(filename)("\" failed (errno: ")(e)(", ")(strerror(e))(")");
                return false;
            }
        }
    }

    // successfully done
    //
    return true;
}


/** \brief Search for a parameter in a string.
 *
 * This function searches for a named parameter in a string representing
 * a text file.
 *
 * The search is very lose. The parameter does not have to start in the
 * first column, the line may be commented, the case can be ignored.
 *
 * \param[in] configuration  The file to be searched.
 * \param[in] parameter_name  The name of the parameter to search.
 * \param[in] start_pos  The starting position of the search.
 * \param[in] ignore_case  Whether to ignore (true) case or not (false.)
 *
 * \return The position of the parameter in the string.
 */
std::string::size_type manager::search_parameter(std::string const & configuration, std::string const & parameter_name, std::string::size_type const start_pos, bool const ignore_case)
{
    if(start_pos >= configuration.length())
    {
        return std::string::size_type(-1);
    }

    // search for a string that matches, we use this search mechanism
    // so we can support case sensitive or insensitive
    //
    auto const it(std::search(
            configuration.begin() + start_pos,
            configuration.end(),
            parameter_name.begin(),
            parameter_name.end(),
            [ignore_case](char c1, char c2)
            {
                return ignore_case ? std::tolower(c1) == std::tolower(c2)
                                   : c1 == c2;
            }));

    // return the position if found and -1 otherwise
    //
    return it == configuration.end() ? std::string::size_type(-1) : it - configuration.begin();
}



} // namespace snap_manager
// vim: ts=4 sw=4 et
