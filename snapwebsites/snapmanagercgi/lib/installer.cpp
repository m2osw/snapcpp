/*
 * Text:
 *      installer.cpp
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

// our lib
//
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
    p.add_argument(QString::fromUtf8(command.c_str()));
    p.add_argument(QString::fromUtf8(package_name.c_str()));
    p.add_environ("DEBIAN_FRONTEND", "noninteractive");
    int r(p.run());

    // the output is saved so we can send it to the user and log it...
    QString const output(p.get_output(true));
    SNAP_LOG_INFO(command)(" of package named \"")(package_name)("\" output:\n")(output);

    return r;
}




bool manager::upgrader()
{
    return update_packages("update") == 0
        && update_packages("upgrade") == 0
        && update_packages("dist-upgrade") == 0
        && update_packages("autoremove") == 0;
}



bool manager::installer(QString const & bundle_name, std::string const & command)
{
    bool success(true);

    SNAP_LOG_INFO("Installing bundle \"")(bundle_name)("\" on host \"")(f_server_name)("\"");

    // for installation we first do an update of the packages,
    // otherwise it could fail the installation because of
    // outdated data
    //
    if(command == "install")
    {
        success = upgrader();
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

    // get the list of expected packages, it may be empty/non-existant
    //
    QDomNodeList bundle_packages(bundle_xml.elementsByTagName("packages"));
    if(bundle_packages.size() == 1)
    {
        QDomElement package_list(bundle_packages.at(0).toElement());
        std::string const list_of_packages(package_list.text().toUtf8().data());
        std::vector<std::string> packages;
        snap::tokenize_string(packages, list_of_packages, ",", true, " ");
        std::for_each(packages.begin(), packages.end(),
                [=, &success](auto const & p)
                {
                    // we want to call all the install even if a
                    // previous one (or the update) failed
                    //
                    success = this->install_package(p, command) && success;
                });
    }

    return success;
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


} // namespace snap_manager
// vim: ts=4 sw=4 et
