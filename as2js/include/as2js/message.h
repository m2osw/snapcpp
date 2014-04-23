#ifndef AS2JS_MESSAGE_H
#define AS2JS_MESSAGE_H
/* message.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include "string.h"
#include "int64.h"
#include "float64.h"

#include <sstream>

namespace as2js
{

enum message_level_t
{
    MESSAGE_LEVEL_OFF,
    MESSAGE_LEVEL_FATAL,
    MESSAGE_LEVEL_ERROR,
    MESSAGE_LEVEL_WARNING,
    MESSAGE_LEVEL_INFO,
    MESSAGE_LEVEL_DEBUG,
    MESSAGE_LEVEL_TRACE
};

class MessageCallback
{
public:
    virtual void        output(message_level_t message_level, char const *file, char const *func, int line, std::string const& message) = 0;
};

class Message
{
public:
                        Message(message_level_t message_level, char const *file = nullptr, char const *func = nullptr, int line = -1);
                        Message(Message const& rhs);
                        ~Message();

    // internal types; you can add your own types with
    // Message& operator << (Message& os, <my-type>);
    Message&            operator << (char const *s);
    Message&            operator << (wchar_t const *s);
    Message&            operator << (std::string const& s);
    Message&            operator << (std::wstring const& s);
    Message&            operator << (String const& s);
    Message&            operator << (char const v);
    Message&            operator << (signed char const v);
    Message&            operator << (unsigned char const v);
    Message&            operator << (signed short const v);
    Message&            operator << (unsigned short const v);
    Message&            operator << (signed int const v);
    Message&            operator << (unsigned int const v);
    Message&            operator << (signed long const v);
    Message&            operator << (unsigned long const v);
    Message&            operator << (signed long long const v);
    Message&            operator << (unsigned long long const v);
    Message&            operator << (Int64 const v);
    Message&            operator << (float const v);
    Message&            operator << (double const v);
    Message&            operator << (Float64 const v);
    Message&            operator << (bool const v);

    static void         set_message_callback(MessageCallback *callback);

private:
    message_level_t     f_message_level;
    char const *        f_file;
    char const *        f_func;
    int                 f_line;
    std::stringstream   f_message;
};

Message fatal  (char const *file = nullptr, char const *func = nullptr, int line = -1);
Message error  (char const *file = nullptr, char const *func = nullptr, int line = -1);
Message warning(char const *file = nullptr, char const *func = nullptr, int line = -1);
Message info   (char const *file = nullptr, char const *func = nullptr, int line = -1);
Message debug  (char const *file = nullptr, char const *func = nullptr, int line = -1);
Message trace  (char const *file = nullptr, char const *func = nullptr, int line = -1);

#define    AS2JS_MESSAGE_FATAL       as2js::fatal  (__FILE__, __func__, __LINE__)
#define    AS2JS_MESSAGE_ERROR       as2js::error  (__FILE__, __func__, __LINE__)
#define    AS2JS_MESSAGE_WARNING     as2js::warning(__FILE__, __func__, __LINE__)
#define    AS2JS_MESSAGE_INFO        as2js::info   (__FILE__, __func__, __LINE__)
#define    AS2JS_MESSAGE_DEBUG       as2js::debug  (__FILE__, __func__, __LINE__)
#define    AS2JS_MESSAGE_TRACE       as2js::trace  (__FILE__, __func__, __LINE__)

}
// namespace as2js
#endif
//#ifndef AS2JS_MESSAGE_H

// vim: ts=4 sw=4 et
