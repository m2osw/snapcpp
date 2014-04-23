/* message.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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

#include "as2js/message.h"

namespace as2js
{

namespace
{
    MessageCallback *   g_message_callback = nullptr;
}
// no name namespace


/** \brief Create a message object with the specified information.
 *
 * This function generates a message object that can be used to generate
 * a message with the << operator and then gets sent to the client using
 * the message callback function on destruction.
 *
 * The level can be set to any one of the message levels available in
 * the message_level_t enumeration. The special MESSAGE_LEVEL_OFF value
 * can be used to avoid the message altogether (can be handy when you
 * support a varying message level.)
 *
 * \param[in] message_level  The level of the message.
 * \param[in] file  The name of the source file that message was generated from.
 * \param[in] func  The name of the function that message was generated from.
 * \param[in] line  The line number that message was generated from.
 */
Message::Message(message_level_t message_level, char const *file, char const *func, int line)
    : f_message_level(message_level)
    , f_file(file)
    , f_func(func)
    , f_line(line)
    //, f_message() -- auto-init
{
}


/** \brief Copy a message in another.
 *
 * In some cases copy messages get from one Message object to another.
 */
Message::Message(Message const& rhs)
    : f_message_level(rhs.f_message_level)
    , f_file(rhs.f_file)
    , f_func(rhs.f_func)
    , f_line(rhs.f_line)
    //, f_message() -- auto-init
{
    f_message << rhs.f_message.str();
}


/** \brief Output the message created with the << operators.
 *
 * The destructor of the message object is where things happen. This function
 * prints out the message that was built using the different << operators
 * and the parameters specified in the constructor.
 *
 * The result is then passed to the message callback. If you did not setup
 * that function, the message is lost.
 *
 * If the level of the message was set to MESSAGE_LEVEL_OFF (usualy via
 * a command line option) then the message callback does not get called.
 */
Message::~Message()
{
    // actually emit the message
    if(g_message_callback && f_message_level != MESSAGE_LEVEL_OFF)
    {
        if(f_file == nullptr)
        {
            f_file = "unknown-file";
        }
        if(f_func == nullptr)
        {
            f_func = "unknown-func";
        }

        g_message_callback->output(f_message_level, f_file, f_func, f_line, f_message.str());
    }
}

Message& Message::operator << (char const *s)
{
    // we assume UTF-8 because in our Snap environment most everything is
    f_message << s;
    return *this;
}

Message& Message::operator << (wchar_t const *s)
{
    String str;
    str.from_wchar(s);
    f_message << str.to_utf8();
    return *this;
}

Message& Message::operator << (std::string const& s)
{
    f_message << s.c_str();
    return *this;
}

Message& Message::operator << (std::wstring const& s)
{
    String str;
    str.from_wchar(s.c_str(), s.length());
    f_message << str.to_utf8();
    return *this;
}

Message& Message::operator << (String const& s)
{
    f_message << s.to_utf8();
    return *this;
}

Message& Message::operator << (char const v)
{
    f_message << static_cast<int>(v);
    return *this;
}

Message& Message::operator << (signed char const v)
{
    f_message << static_cast<int>(v);
    return *this;
}

Message& Message::operator << (unsigned char const v)
{
    f_message << static_cast<int>(v);
    return *this;
}

Message& Message::operator << (signed short const v)
{
    f_message << static_cast<int>(v);
    return *this;
}

Message& Message::operator << (unsigned short const v)
{
    f_message << static_cast<int>(v);
    return *this;
}

Message& Message::operator << (signed int const v)
{
    f_message << v;
    return *this;
}

Message& Message::operator << (unsigned int const v)
{
    f_message << v;
    return *this;
}

Message& Message::operator << (signed long const v)
{
    f_message << v;
    return *this;
}

Message& Message::operator << (unsigned long const v)
{
    f_message << v;
    return *this;
}

Message& Message::operator << (signed long long const v)
{
    f_message << v;
    return *this;
}

Message& Message::operator << (unsigned long long const v)
{
    f_message << v;
    return *this;
}

Message& Message::operator << (float const v)
{
    f_message << v;
    return *this;
}

Message& Message::operator << (double const v)
{
    f_message << v;
    return *this;
}

Message& Message::operator << (bool const v)
{
    f_message << static_cast<int>(v);
    return *this;
}


/** \brief Setup the callback so tools can receive error messages.
 *
 * This function is used by external processes to setup a callback. The
 * callback receives the message output as generated by the Message
 * class.
 *
 * \sa configure()
 */
void Message::set_message_callback(MessageCallback *callback)
{
    g_message_callback = callback;
}


Message fatal(char const *file, char const *func, int line)
{
    Message l(MESSAGE_LEVEL_FATAL, file, func, line);
    l.operator << ("fatal: ");
    return l;
}

Message error(char const *file, char const *func, int line)
{
    Message l(MESSAGE_LEVEL_ERROR, file, func, line);
    l.operator << ("error: ");
    return l;
}

Message warning(char const *file, char const *func, int line)
{
    Message l(MESSAGE_LEVEL_WARNING, file, func, line);
    l.operator << ("warning: ");
    return l;
}

Message info(char const *file, char const *func, int line)
{
    Message l(MESSAGE_LEVEL_INFO, file, func, line);
    l.operator << ("info: ");
    return l;
}

Message debug(char const *file, char const *func, int line)
{
    Message l(MESSAGE_LEVEL_DEBUG, file, func, line);
    l.operator << ("debug: ");
    return l;
}

Message trace(char const *file, char const *func, int line)
{
    Message l(MESSAGE_LEVEL_INFO, file, func, line);
    l.operator << ("trace: ");
    return l;
}



}
// namespace as2js

// vim: ts=4 sw=4 et
