/*    compatibility.h -- Windows compatibility definitions
 *    Copyright (C) 2012-2013  Made to Order Software Corporation
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *    Authors
 *    Alexis Wilke   alexis@m2osw.com
 */
#pragma once

/** \file
 * \brief Compatibility definitions to ease implementation.
 *
 * This is a list of simple definitions using inline functions and
 * macros that replicate the behavior of Unix for Unix like functions
 * that could easily be reimplemented and that we need.
 */

// the following are definitions that are used to make our code compile
// under MS-Windows without having to rewrite these in each .cpp file
// where such are used.
#include    "libdebpackages/debian_export.h"
#include    "libutf8/libutf8.h"
#include    <stdio.h>
#include    <string.h>
#include    <sys/stat.h>
#if defined(MO_WINDOWS)
#include    <io.h>
#include    <wchar.h>
#include    <direct.h>

// MS-Windows does not supply a function that easily parses dates
// We use the FreeBSD version in wpkg, here is the function declaration
extern "C" char * strptime(const char *buf, const char *fmt, struct tm *tm);
#endif
#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
#include    <windows.h>
#endif


#ifdef _MSC_VER
typedef int mode_t;

int snprintf(char *output, size_t size, const char *format, ...);
int strcasecmp(const char *a, const char *b);
int strncasecmp(const char *a, const char *b, size_t c);

#define WEXITSTATUS(code) ((code) == -1 ? (code) : (code) & 0x0FF)

// avoid warnings
#define close   _close
#define getcwd  _wgetcwd
//#define open    _wopen -- cannot do that one because ifstream uses open() too
#else
#define os_open open
#endif

#ifdef MO_WINDOWS
// MS-Windows really uses 32Kb buffers for full paths, but we
// use a limit of MAX_PATH which in most situation works; we
// may grow that number later (because it's only 260 chars.)
#ifdef PATH_MAX
#if PATH_MAX != MAX_PATH
#error "PATH_MAX not equal to MAX_PATH in compatibility.h"
#endif
#else
#define PATH_MAX    MAX_PATH
#endif

#undef os_open
#define os_open _wopen
#define rename  _wrename
#define rmdir   _wrmdir
#define unlink  _wunlink

int mkdir(const wchar_t *name, mode_t mode);
int symlink(const wchar_t * /*destination*/, const wchar_t * /*symoblic_link*/);
int getuid();
int getgid();
int getpid();
#endif


#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
namespace wpkg_compatibility
{


/** \brief RAII handle class.
 *
 * This class is used to create RAII handles. This means when using this
 * class to store a handle (instead of a straight HANDLE type) your
 * handles will always get destroyed when the function that
 * created them exits or the object that holds them is closed.
 */
template<class H, class C>
class raii_handle_t
{
public:
    typedef H handle_type_t;

    raii_handle_t(handle_type_t handle = C::default_value())
    {
        f_handle = handle;
    }
    raii_handle_t(raii_handle_t<H, C>& handle)
    {
        f_handle = handle.release();
    }
    virtual ~raii_handle_t()
    {
        reset();
    }
    raii_handle_t<H, C>& operator = (raii_handle_t<H, C>& rhs)
    {
        reset(rhs.release());
        return *this;
    }
    void operator = (handle_type_t handle)
    {
        reset(handle);
    }
    operator handle_type_t ()
    {
        return f_handle;
    }
    operator handle_type_t () const
    {
        return f_handle;
    }
    handle_type_t *operator & ()
    {
        reset();
        return &f_handle;
    }
    operator bool () const
    {
        return f_handle != C::default_value();
    }
    void reset(handle_type_t handle = C::default_value()) throw()
    {
        if(f_handle != C::default_value())
        {
            C()(f_handle);
        }
        f_handle = handle;
    }
    handle_type_t release()
    {
        handle_type_t handle(f_handle);
        f_handle = C::default_value();
        return handle;
    }

private:
    handle_type_t   f_handle;
};


/** \brief Trait for a standard handle.
 *
 * This class is a trait to standard handles, meaning handles that can be
 * closed by a call to CloseHandle().
 *
 * If you create other handles and need a call other than CloseHandle()
 * to get rid of the handle, you have to create a new trait.
 */
class standard_handle_trait
{
public:
    static HANDLE default_value()
    {
        return INVALID_HANDLE_VALUE;
    }

    void operator () (HANDLE handle)
    {
        CloseHandle(handle);
    }
};


/** \brief Definition of a handle type that makes use of the standard trait.
 *
 * This typedef defines a standard handle type so it is easy to make use of
 * it everywhere it is necessary. This replaces all your HANDLE definitions.
 * However, remember to use the release() function if you want to transfer
 * the HANDLE from one standard_handle_t to another. This happens
 * automatically, but it means that the source is null afterward.
 *
 * \code
 * standard_handle_t a, b;
 * a.reset(CreateFile(...));
 * ...
 * b = a;
 * // here a is INVALID_HANDLE_VALUE
 * if(a)
 * {
 *    // therefore you do not enter here
 * }
 * \endcode
 */
typedef raii_handle_t<HANDLE, standard_handle_trait> standard_handle_t;

} // namespace wpkg_compatibility
#endif


bool same_file(const std::string& a, const std::string& b);
size_t strftime_utf8(char *s, size_t max, const char *format, const struct tm *tm);


#endif
// vim: ts=4 sw=4 et
