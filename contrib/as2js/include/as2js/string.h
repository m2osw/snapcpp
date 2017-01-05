#ifndef AS2JS_STRING_H
#define AS2JS_STRING_H
/* as2js/string.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

/*

Copyright (c) 2005-2017 Made to Order Software Corp.

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

#include    "int64.h"
#include    "float64.h"

#include    <iostream>
#include    <string>

namespace as2js
{

// our character type, yes, it also becomes as String::value_type
// but at least this way we control the size outside of the class
typedef int32_t                                         as_char_t;

// Our String type is a UCS-4 compatible string type
// Unfortunately, under MS-Windows wstring is 16 bits
class String : public std::basic_string<as_char_t>
{
public:
    // Unicode BOM character
    static as_char_t const  STRING_BOM = 0xFEFF;
    // Represents a continuation character (i.e. '\' + LineTerminatorSequence)
    static as_char_t const  STRING_CONTINUATION = -2;

    enum class conversion_result_t
    {
        STRING_GOOD    =  0,   // string conversion succeeded
        STRING_END     = -1,   // not enough data to form a character
        STRING_BAD     = -2,   // input is not valid (bad encoding sequence)
        STRING_INVALID = -3    // invalid character found (character is not between 0 and 0x10FFFF, or is a code point reserved for UTF-16 surrogates)
    };

                            String();
                            String(char const * str, int len = -1);
                            String(wchar_t const * str, int len = -1);
                            String(as_char_t const * str, int len = -1);
                            String(std::string const & str);
                            String(std::wstring const & str);
                            String(std::basic_string<as_char_t> const & str);

    String&                 operator = (char const * str);
    String&                 operator = (wchar_t const * str);
    String&                 operator = (as_char_t const * str);
    String&                 operator = (std::string const & str);
    String&                 operator = (std::wstring const & str);
    String&                 operator = (std::basic_string<as_char_t> const & str);

    bool                    operator == (char const * str) const;
    friend bool             operator == (char const * str, String const & string);
    bool                    operator != (char const * str) const;
    friend bool             operator != (char const * str, String const & string);

    String &                operator += (char const * str);
    String &                operator += (wchar_t const * str);
    String &                operator += (as_char_t const * str);
    String &                operator += (std::string const & str);
    String &                operator += (std::wstring const & str);
    String &                operator += (std::basic_string<as_char_t> const & str);

    String &                operator += (as_char_t const c);
    String &                operator += (char const c);
    String &                operator += (wchar_t const c);

    //String                  operator + (char const * str);
    //String                  operator + (wchar_t const * str);
    //String                  operator + (as_char_t const * str);
    //String                  operator + (std::string const & str);
    //String                  operator + (std::wstring const & str);

    bool                    valid() const;
    static bool             valid_character(as_char_t c);

    bool                    is_int64() const;
    bool                    is_float64() const;
    bool                    is_number() const;
    Int64::int64_type       to_int64() const;
    Float64::float64_type   to_float64() const;
    bool                    is_true() const;

    conversion_result_t     from_char(char const * str, int len = -1);
    conversion_result_t     from_wchar(wchar_t const * str, int len = -1);
    conversion_result_t     from_as_char(as_char_t const * str, int len = -1);
    conversion_result_t     from_utf8(char const * str, int len = -1);

    ssize_t                 utf8_length() const;
    std::string             to_utf8() const;

    String                  simplified() const;
};

std::ostream& operator << (std::ostream& out, String const& str);


}
// namespace as2js

#endif
// #ifndef AS2JS_STRING_H

// vim: ts=4 sw=4 et
