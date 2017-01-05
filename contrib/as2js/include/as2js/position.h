#ifndef AS2JS_POSITION_H
#define AS2JS_POSITION_H
/* position.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "string.h"


namespace as2js
{



class Position
{
public:
    typedef int32_t         counter_t;
    static counter_t const  DEFAULT_COUNTER = 1;

//                        Position();
//                        Position(Position const& rhs);
//
//    Position&           operator = (Position const& rhs);

    void                set_filename(String const& filename);
    void                set_function(String const& function);
    void                reset_counters(counter_t line = DEFAULT_COUNTER);
    void                new_page();
    void                new_paragraph();
    void                new_line();

    String              get_filename() const;
    String              get_function() const;
    counter_t           get_page() const;
    counter_t           get_page_line() const;
    counter_t           get_paragraph() const;
    counter_t           get_line() const;

private:
    String              f_filename;
    String              f_function;
    counter_t           f_page = DEFAULT_COUNTER;
    counter_t           f_page_line = DEFAULT_COUNTER;
    counter_t           f_paragraph = DEFAULT_COUNTER;
    counter_t           f_line = DEFAULT_COUNTER;
};

std::ostream& operator << (std::ostream& out, Position const& pos);


}
// namespace as2js
#endif
// #ifndef AS2JS_POSITION_H

// vim: ts=4 sw=4 et
