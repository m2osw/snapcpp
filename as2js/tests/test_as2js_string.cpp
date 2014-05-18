/* test_as2js_string.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/string.h"

#include    "test_as2js_string.h"


namespace as2js_test
{


#if 0
                            String();
                            String(String const& str);
                            String(char const *str, int len = -1);
                            String(wchar_t const *str, int len = -1);
                            String(as_char_t const *str, int len = -1);
                            String(std::string const& str);
                            String(std::wstring const& str);
                            String(std::basic_string<as_char_t> const& str);

    String&                 operator = (String const& str);
    String&                 operator = (char const *str);
    String&                 operator = (wchar_t const *str);
    String&                 operator = (as_char_t const *str);
    String&                 operator = (std::string const& str);
    String&                 operator = (std::wstring const& str);
    String&                 operator = (std::basic_string<as_char_t> const& str);

    bool                    operator == (char const *str) const;
    friend bool             operator == (char const *str, String const& string);

    String&                 operator += (String const& str);
    String&                 operator += (char const *str);
    String&                 operator += (wchar_t const *str);
    String&                 operator += (as_char_t const *str);
    String&                 operator += (std::string const& str);
    String&                 operator += (std::wstring const& str);
    String&                 operator += (std::basic_string<as_char_t> const& str);

    String&                 operator += (as_char_t const c);
    String&                 operator += (char const c);
    String&                 operator += (wchar_t const c);

    bool                    valid() const;
    static bool             valid_character(as_char_t c);

    bool                    is_int64() const;
    bool                    is_float64() const;
    bool                    is_number() const;
    Int64::int64_type       to_int64() const;
    Float64::float64_type   to_float64() const;
    bool                    is_true() const;

    conversion_result_t     from_char(char const *str, int len = -1);
    conversion_result_t     from_wchar(wchar_t const *str, int len = -1);
    conversion_result_t     from_as_char(as_char_t const *str, int len = -1);
    conversion_result_t     from_utf8(char const *str, int len = -1);

    ssize_t                 utf8_length() const;
    std::string             to_utf8() const;
#endif

}
// namespace as2js_test

// vim: ts=4 sw=4 et
