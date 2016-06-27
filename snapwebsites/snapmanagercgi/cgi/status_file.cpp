//
// File:        status_file.cpp
// Object:      Handle the reading of a status file
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

#include "status_file.h"

#include "log.h"
#include "not_reached.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

#include "poison.h"

namespace snap_manager
{

namespace
{

/** \brief The magic at expected on the first line of a status file.
 *
 * This string defines the magic string expected on the first line of
 * the file.
 *
 * \note
 * Note that our reader ignores \r characters so this is not currently
 * a 100% exact match, but since only our application is expected to
 * create / read these files, we are not too concerned.
 */
char const g_status_file_magic[] = "Snap! Status v1";

}
// no name namespace


/** \brief Initializes the status file with the specified filename.
 *
 * This function saves the specified \p filename to this status file
 * object. It does not attempt to open() the file. Use the actual
 * open() function for that and make sure to check whether it
 * succeeds.
 *
 * \param[in] filename  The name of the file to read.
 */
status_file::status_file(char const * filename)
    : f_filename(filename)
    //, f_fd(-1) -- auto-init
    //, f_file(nullptr) -- auto-init
{
}


/** \brief Clean up the status file.
 *
 * This destructor makes sure that the status file is closed before
 * the status_file object goes away.
 */
status_file::~status_file()
{
    close();
}


/** \brief Close the currently opened file if any.
 *
 * This function makes sure that the status file is closed. This
 * automatically unlocks the file so other processes now have
 * access to the data.
 *
 * This function also has the side effect of resetting the
 * has-error flag to false.
 */
void status_file::close()
{
    if(f_fd != -1)
    {
        // Note: there is no need for an explicit unlock, the close()
        //       has the same effect on that file
        //::flock(f_fd, LOCK_UN);
        ::close(f_fd);
    }
    f_has_error = false;
}


/** \brief Open this status file.
 *
 * This function actually tries to open the status file. The function
 * makes sure to lock the file. The lock blocks until it is obtained.
 *
 * If the file does not exist, is not accessible (permissions denied),
 * or cannot be locked, then the function returns false.
 *
 * Also, if the first line is not a valid status file magic string,
 * then the function returns false also.
 *
 * \note
 * On an error, this function already logs information so the caller
 * does not have to do that again.
 *
 * \return true if the file was successfully opened.
 */
bool status_file::open()
{
    close();

    // open the file
    //
    f_fd = ::open(f_filename.c_str(), O_RDONLY | O_CLOEXEC, 0);
    if(f_fd < 0)
    {
        f_fd = -1;
        SNAP_LOG_ERROR("could not open file \"")
                      (f_filename)
                      ("\" to save snapmanagerdamon status.");
        f_has_error = true;
        return false;
    }

    // make sure no write occur while we read the file
    //
    if(::flock(f_fd, LOCK_SH) != 0)
    {
        close();
        SNAP_LOG_ERROR("could not lock file \"")
                      (f_filename)
                      ("\" to read snapmanagerdamon status.");
        f_has_error = true;
        return false;
    }

    // transform to a FILE * so that way we benefit from the
    // caching mechanism without us having to re-implement such
    //
    f_file = fdopen(f_fd, "rb");
    if(f_file == nullptr)
    {
        close();
        SNAP_LOG_ERROR("could not allocate a FILE* for file \"")
                      (f_filename)
                      ("\" to read snapmanagerdamon status.");
        f_has_error = true;
        return false;
    }

    // read the first line, it has to be the proper file magic
    {
        QString line;
        if(!readline(line))
        {
            close();
            SNAP_LOG_ERROR("an error occurred while trying to read the first line of status file \"")
                          (f_filename)
                          ("\".");
            f_has_error = true;
            return false;
        }
        if(line != g_status_file_magic)
        {
            close();
            SNAP_LOG_ERROR("status file \"")
                          (f_filename)
                          ("\" does not start with the expected magic. Found: \"")
                          (line)
                          ("\", expected: \"")
                          (g_status_file_magic)
                          ("\".");
            f_has_error = true;
            return false;
        }
    }

    return true;
}


/** \brief Read one line from the input file.
 *
 * This function reads one line of data from the input file and saves
 * it in \p result.
 *
 * If the end of the file is reached or an error occurs, then the
 * function returns false. Otherwise it returns true.
 *
 * \note
 * If an error occurs, the \p result parameter is set to the empty string.
 *
 * \param[out] result  The resulting line of input read.
 *
 * \return true if a line we read, false otherwise.
 */
bool status_file::readline(QString & result)
{
    std::string line;

    for(;;)
    {
        char buf[2];
        int const r(::fread(buf, sizeof(buf[0]), 1, f_file));
        if(r != 1)
        {
            // reached end of file?
            if(feof(f_file))
            {
                // we reached the EOF
                result = QString::fromUtf8(line.c_str());
                return false;
            }
            // there was an error
            int const e(errno);
            SNAP_LOG_ERROR("an error occurred while reading status file. Error: ")
                  (e)
                  (", ")
                  (strerror(e));
            f_has_error = true;
            result = QString();
            return false; // simulate an EOF so we stop the reading loop
        }
        if(buf[0] == '\n')
        {
            result = QString::fromUtf8(line.c_str());
            return true;
        }
        // ignore any '\r'
        if(buf[0] != '\r')
        {
            buf[1] = '\0';
            line += buf;
        }
    }
    snap::NOTREACHED();
}


/** \brief Read one variable from the status file.
 *
 * This function reads the next variable from the status file.
 *
 * \param[out] name  The name of the newly read parameter.
 * \param[out] value  The value of this parameter.
 *
 * \return true if a variable was found and false otherwise.
 */
bool status_file::readvar(QString & name, QString & value)
{
    // reset the out variables
    //
    name.clear();
    value.clear();

    // read next line of data
    //
    QString line;
    if(!readline(line))
    {
        return false;
    }

    // search for the first equal (between name and value)
    //
    int const pos(line.indexOf('='));
    if(pos < 1)
    {
        SNAP_LOG_ERROR("invalid line in \"")
                      (f_filename)
                      ("\", it has no \"name=...\".");
        f_has_error = true;
        return false;
    }

    name = line.mid(0, pos);
    value = line.mid(pos + 1);

    return true;
}


/** \brief Check whether the file had errors.
 *
 * If an error occurs while reading a file, this flag will be set to
 * true.
 *
 * The flag is false by default and gets reset to false when close()
 * gets called.
 *
 * \return true if an error occurred while reading the file.
 */
bool status_file::has_error() const
{
    return f_has_error;
}


}
// namespace snap_manager
// vim: ts=4 sw=4 et
