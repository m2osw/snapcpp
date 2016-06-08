/*
 * Text:
 *      status.cpp
 *
 * Description:
 *      The implementation of the STATUS function.
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
#include "snapmanagerdaemon.h"

// our lib
//
#include "log.h"

// C lib
//
#include <sys/file.h>


namespace snap_manager
{


namespace
{

// TODO: make a common header file...
char const status_file_magic[] = "Snap! Status v1\n";

}
// no name namespace






/** \brief Function called whenever the MANAGERSTATUS message is received.
 *
 * Whenever the status of a snapmanagerdaemon changes, it is sent to all
 * the other snapmanagerdaemon (and this daemon itself.)
 *
 * \param[in] message  The actual MANAGERSTATUS message.
 */
void manager_daemon::set_manager_status(snap::snap_communicator_message const & message)
{
    // WARNING: not using the ofstream class because we want to lock
    //          the file and there is no standard way to access 'fd'
    //          in an ofstream object
    //
    class safe_status_file
    {
    public:
        safe_status_file(QString const & data_path, QString const & server)
            : f_filename(QString("%1/%2.db").arg(data_path).arg(server).toUtf8().data())
            //, f_fd(-1)
            //, f_keep(false)
        {
        }

        ~safe_status_file()
        {
            close();
        }

        void close()
        {
            if(f_fd != -1)
            {
                // Note: there is no need for an explicit unlock, the close()
                //       has the same effect on that file
                //::flock(f_fd, LOCK_UN);
                ::close(f_fd);
            }
            if(!f_keep)
            {
                ::unlink(f_filename.c_str());
            }
        }

        bool open()
        {
            close();

            // open the file
            //
            f_fd = ::open(f_filename.c_str(), O_WRONLY | O_CLOEXEC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if(f_fd < 0)
            {
                SNAP_LOG_ERROR("could not open file \"")
                              (f_filename)
                              ("\" to save snapmanagerdamon status.");
                return false;
            }

            // make sure we are the only one on the case
            //
            if(::flock(f_fd, LOCK_EX) != 0)
            {
                SNAP_LOG_ERROR("could not lock file \"")
                              (f_filename)
                              ("\" to save snapmanagerdamon status.");
                return false;
            }

            return true;
        }

        bool write(void const * buf, size_t size)
        {
            if(::write(f_fd, buf, size) != static_cast<ssize_t>(size))
            {
                SNAP_LOG_ERROR("could not write to file \"")
                              (f_filename)
                              ("\" to save snapmanagerdamon status.");
                return false;
            }

            return true;
        }

        void keep()
        {
            // it worked, make sure the file is kept around
            // (if this does not get called the file gets deleted)
            //
            f_keep = true;
        }

    private:
        std::string f_filename;
        int         f_fd = -1;
        bool        f_keep = false;
    };

    // TBD: should we check that the name of the sending service is one of us?
    //

    QString const server(message.get_sent_from_server());
    QString const status(message.get_parameter("status"));

    {
        QByteArray const status_utf8(status.toUtf8());

        safe_status_file out(f_data_path, server);
        if(!out.open()
        || !out.write(status_file_magic, sizeof(status_file_magic) - 1)
        || !out.write(status_utf8.data(), status_utf8.size()))
        {
            return;
        }
        out.keep();
    }

    // keep a copy of our own information
    //
    if(server == f_server_name)
    {
        f_status = status;
    }
}



} // namespace snap_manager
// vim: ts=4 sw=4 et
