/* rc.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// this is private
#include    "rc.h"

#include    "as2js/message.h"

#include    <cstring>


namespace as2js
{

namespace
{

char const *g_rc_directories[] =
{
    // try locally first (assuming you are a heavy JS developer, you'd
    // probably start with your local files)
    "as2js",
    // try your user "global" installation directory
    "~/.config/as2js",
    // try the system directory
    "/usr/share/as2js",
    nullptr
};

controlled_vars::zbool_t    g_home_initialized;
String                      g_home;

}
// no name namespace


/** \brief Find the resource file.
 *
 * This function tries to find a resource file.
 *
 * The resource file defines two paths where we can find the system
 * definitions and user imports.
 *
 * \param[in] accept_if_missing  Whether an error is generated (false)
 *                               if the file cannot be found.
 */
void rc_t::find_rc(bool const accept_if_missing)
{
    // first try to find a place with a .rc file
    const char *d;

    for(const char **dir = g_rc_directories; *dir != nullptr; ++dir)
    {
        std::stringstream buffer;
        d = *dir;
        if(*d == '~')
        {
            String home(get_home());
            if(home.empty())
            {
                // no valid $HOME variable
                continue;
            }
            buffer << home << "/" << (dir + 1) << "/as2js.rc";
        }
        else
        {
            buffer << *dir << "/as2js.rc";
        }
        f_rcfilename.from_utf8(buffer.str().c_str());
        if(!f_rcfilename.empty())
        {
            f_rcfile.open(f_rcfilename.to_utf8().c_str());
            if(f_rcfile.is_open())
            {
                // it worked, we are done
                return;
            }
        }
    }

    if(!accept_if_missing)
    {
        // no position in this case...
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INSTALLATION);
        msg << "cannot find the as2js.rc file; it is usually put in /usr/share/as2js/scripts/as2js.rc";
        exit(1);
    }

    // if we want everything internal, we will just use working defaults
    f_path = "as2js/scripts";
    f_db = "/tmp/as2js_packages.db";
    f_rcfilename = "internal.rc";
}


/** \brief Read data from the resource file.
 *
 * This function reads the compiler information from the resource file.
 * It is interested in two parameters:
 *
 * \li f_path
 *
 * The path to the JavaScript files that declare the global and other
 * environment.
 *
 * \li f_db
 *
 * The name of the file used to save our database information (to avoid
 * having to recompile everything each time.) This has to be a writable
 * file.
 */
void rc_t::read_rc()
{
    // if f_f is null, we already have the defaults
    // and f_input_retriever is not NULL
    if(!f_rcfile.is_open())
    {
        return;
    }

    Position pos;
    pos.set_filename(f_rcfilename);
    for(char line_buf[256]; f_rcfile.getline(line_buf, sizeof(line_buf)); pos.new_line())
    {
        char *s(line_buf);
        while(isspace(*s))
        {
            s++;
        }
        if(*s == '#' || *s == '\n' || *s == '\0')
        {
            // empty line or commented out
            continue;
        }
        // name of this variable
        char *name(s);
        while(*s != '\0' && *s != '=' && !isspace(*s))
        {
            s++;
        }
        // length of the name
        size_t l(s - name);
        while(isspace(*s))
        {
            s++;
        }
        // all variables are expected to be assigned a value
        if(*s != '=')
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INVALID_VARIABLE, pos);
            msg << "syntax error; expected an equal sign after the variable name.";
            continue;
        }
        // skip the = and following spaces
        s++;
        while(isspace(*s))
        {
            s++;
        }
        // parameter defined within quotes?
        char *param(s);
        if(*s == '"' || *s == '\'')
        {
            ++param; // skip quote in param too
            char quote(*s++);
            while(*s != '\0' && *s != quote && *s != '\n')
            {
                s++;
            }
        }
        else
        {
            param = s;
            while(*s != '\0' && *s != '\n')
            {
                s++;
            }
        }
        // end param
        *s = '\0';

        if(l == 7 && std::strncmp(name, "version", 7) == 0)
        {
            // TODO: check that we understand this version
        }
        else if(l == 10 && std::strncmp(name, "as2js_path", 10) == 0)
        {
            f_path.from_utf8(param);
        }
        else if(l == 8 && std::strncmp(name, "as2js_db", 8) == 0)
        {
            f_db.from_utf8(param);
        }
        else
        {
            Message msg(MESSAGE_LEVEL_WARNING, AS_ERR_INVALID_VARIABLE, pos);
            name[l] = '\0';
            msg << "unknown parameter \"" << name << "\" ignored.";
        }
    }
}


void rc_t::close()
{
    if(f_rcfile.is_open())
    {
        f_rcfile.close();
    }
}


String const& rc_t::get_path() const
{
    return f_path;
}


String const& rc_t::get_db() const
{
    return f_db;
}


String const& rc_t::get_home()
{
    if(!g_home_initialized)
    {
        g_home_initialized = true;
        g_home.from_utf8(getenv("HOME"));

        // TODO: add test for tainted getenv() result
    }

    return g_home;
}

}
// namespace as2js

// vim: ts=4 sw=4 et
