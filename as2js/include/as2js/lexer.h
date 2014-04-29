#ifndef AS2JS_LEXER_H
#define AS2JS_LEXER_H
/* lexer.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/stream.h"
#include    "as2js/options.h"
#include    "as2js/node.h"


namespace as2js
{


class Lexer
{
public:
                                Lexer();

    void                        set_options(Options::options_pointer_t& options);
    void                        set_input(Input::input_pointer_t& input);
    Input::input_pointer_t      get_input() const;
    void                        set_for_in(bool const for_in);

    Node::node_pointer_t        get_next_token();

private:
    typedef int                 char_type_t;

    static char_type_t const    CHAR_NO_FLAGS        = 0x0000;
    static char_type_t const    CHAR_LETTER          = 0x0001;
    static char_type_t const    CHAR_DIGIT           = 0x0002;
    static char_type_t const    CHAR_PUNCTUATION     = 0x0004;
    static char_type_t const    CHAR_WHITE_SPACE     = 0x0008;
    static char_type_t const    CHAR_LINE_TERMINATOR = 0x0010;
    static char_type_t const    CHAR_HEXDIGIT        = 0x0020;
    static char_type_t const    CHAR_INVALID         = 0x8000;   // such as 0xFFFE & 0xFFFF

    typedef controlled_vars::auto_init<char_type_t, CHAR_NO_FLAGS>  zchar_type_t;

    void                        get_token();
    Input::char_t               input_getc();
    Input::char_t               getc();
    void                        ungetc(Input::char_t c);
    int64_t                     read_hex(long max);
    int64_t                     read_octal(Input::char_t c, long max);
    Input::char_t               escape_sequence();
    char_type_t                 char_type(Input::char_t c);
    Input::char_t               read(Input::char_t c, char_type_t flags, String& str);
    void                        read_identifier(Input::char_t c);
    void                        read_number(Input::char_t c);
    void                        read_string(Input::char_t quote);
    bool                        has_option_set(Options::option_t option) const;

    zchar_type_t                f_char_type;    // type of the last character read
    Input::input_pointer_t      f_input;
    Options::options_pointer_t  f_options;
    controlled_vars::fbool_t    f_for_in;       // IN becomes FOR_IN when this is true

    Node::node_t                f_result_type;
    String                      f_result_string;
    Int64                       f_result_int64;
    Float64                     f_result_float64;
};



}
// namespace as2js
#endif
// #ifndef AS2JS_LEXER_H

// vim: ts=4 sw=4 et
