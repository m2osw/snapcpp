#ifndef AS2JS_STRING_H
#define AS2JS_STRING_H
/* string.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    <controlled_vars/controlled_vars_auto_init.h>

#include    <string>

namespace as2js
{

// our character type, yes, it also becomes as String::value_type
// but at least this way we control the size outside of the class
typedef int32_t         as_char_t;
typedef controlled_vars::auto_init<as_char_t, 0>        zas_char_t;

// Our String type is a UCS-4 compatible string type
// Unfortunately, under MS-Windows wstring is 16 bits
class String : public std::basic_string<as_char_t>
{
public:
    typedef controlled_vars::auto_init<size_type, 0>    zsize_type_t;

                    String();
                    String(String const& str);
                    String(char const *str, int len = -1);
                    String(wchar_t const *str, int len = -1);
                    String(as_char_t const *str, int len = -1);
                    String(std::string const& str);
                    String(std::wstring const& str);

    String&         operator = (String const& str);
    String&         operator = (char const *str);
    String&         operator = (wchar_t const *str);
    String&         operator = (as_char_t const *str);
    String&         operator = (std::string const& str);
    String&         operator = (std::wstring const& str);

    bool            valid() const;
    static bool     valid_character(as_char_t c);

    void            from_char(char const *str, int len = -1);
    void            from_wchar(wchar_t const *str, int len = -1);
    void            from_as_char(as_char_t const *str, int len = -1);
    int             from_utf8(char const *str, int len = -1);

    size_t          utf8_length() const;
    std::string     to_utf8() const;
};

}
// namespace as2js

#endif
// #ifndef AS2JS_STRING_H

// vim: ts=4 sw=4 et
