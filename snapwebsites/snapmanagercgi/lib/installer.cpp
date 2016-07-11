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
#include "process.h"

// C lib
//
#include <fcntl.h>

namespace snap_manager
{




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
    // make sure to create the file if it does not exist
    // we expect the filename parameter to be something like
    //     /etc/snapwebsites/snapwebsites.d/<filename>
    //
    int fd(open(filename.toUtf8().data(), O_RDONLY));
    if(fd == -1)
    {
        fd = open(filename.toUtf8().data(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if(fd != -1)
        {
            std::string const comment("# This file was auto-generated by snapmanager.cgi\n"
                                      "# Feel free to do additional modifications here as\n"
                                      "# snapmanager.cgi will be aware of them as expected.\n");
            ::write(fd, comment.c_str(), comment.size());
        }
    }
    if(fd != -1)
    {
        close(fd);
    }

    snap::process p("replace_configuration_value");
    p.set_mode(snap::process::mode_t::PROCESS_MODE_COMMAND);
    p.set_command("sed");
    p.add_argument("--in-place=.bak");
    p.add_argument(QString("--expression='/^%1=.*/ d'").arg(field_name));
    p.add_argument(QString("--expression='$ a \\%1=%2'").arg(field_name).arg(new_value));
    p.add_argument(filename);
    int const r(p.run());

    return r;
}


} // namespace snap_manager
// vim: ts=4 sw=4 et
