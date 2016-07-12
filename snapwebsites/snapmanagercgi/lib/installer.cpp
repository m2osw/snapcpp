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



/** \brief Installs one Debian package.
 *
 * This function installs ONE package as specified by \p package_name.
 *
 * \param[in] package_name  The name of the package to install.
 *
 * \return The exit code of the apt-get install command.
 */
int manager::install(QString const & package_name)
{
    snap::process p("install");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_OUTPUT);
    p.set_command("apt-get");
    p.add_argument("-y");
    p.add_argument("install");
    p.add_argument(package_name);
    p.add_environ("DEBIAN_FRONTEND", "noninteractive");
    int r(-1);
    {
        // TODO: switch to "root" while doing the installation
        //make_root root;

        r = p.run();
    }

    // the output is saved so we can send it to the user and log it...
    QString const output(p.get_output(true));
    SNAP_LOG_INFO("installation of package named \"")(package_name)("\" output:\n")(output);

    return r;
}




void manager::installer(QString const & bundle_name)
{
    SNAP_LOG_INFO("Installing bundle \"")(bundle_name)("\" on host \"")(f_server_name)("\"");


    //snap::snap_communicator_message reply;
    //reply.reply_to(message);

    //QString const system(message.get_parameter("system"));
    //if(system.isEmpty())
    //{
    //    reply.set_command("INVALID");
    //    reply.add_parameter("what", "command MANAGE/function=INSTALL must specify a \"system\" parameter.");
    //    f_messenger->send_message(reply);
    //    return;
    //}

    //int r(0);
    //bool installed(false);
    //f_output.clear();

    //switch(system[0].unicode())
    //{
    //case 'a':
    //    if(system == "application")
    //    {
    //        // This is snapserver behind an apache proxy (working through snap.cgi)
    //        //
    //        r = install("snapserver");
    //        installed = true;
    //    }
    //    break;

    //case 'f':
    //    if(system == "frontend")
    //    {
    //        // This is just snap.cgi
    //        //
    //        r = install("snapcgi");
    //        installed = true;
    //    }
    //    else if(system == "firewall")
    //    {
    //        // This is just firewall
    //        //
    //        r = install("snapfirewall");
    //        installed = true;
    //    }
    //    break;

    //case 'm':
    //    if(system == "mailserver")
    //    {
    //        // Install snapbounce which forces a postfix installation
    //        // and allows us to send and receive emails as well as to
    //        // know that some emails do not make it
    //        //
    //        install("snapbounce");
    //        installed = true;
    //    }
    //    break;

    //}

    //if(installed)
    //{
    //    reply.set_command("RESULTS");
    //    reply.add_parameter("exitcode", QString("%1").arg(r));
    //    reply.add_parameter("output", f_output);
    //    f_messenger->send_message(reply);
    //    return;
    //}

    //reply.set_command("INVALID");
    //reply.add_parameter("what", "unknown system parameter \"" + system + "\" in command MANAGE/function=INSTALL.");
    //f_messenger->send_message(reply);
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
