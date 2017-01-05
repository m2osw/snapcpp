#ifndef AS2JS_OPTIONS_H
#define AS2JS_OPTIONS_H
/* options.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    <vector>
#include    <memory>


namespace as2js
{


// Options you can tweak so the compiler reacts in a different
// manner in different situations (for instance, the \e escape
// sequence can be used to generate the escape character whenever
// the extended escape sequences is set to 1).
//
// At this time AS_OPTION_STRICT is always set to 1
class Options
{
public:
    typedef std::shared_ptr<Options>    pointer_t;

    enum class option_t
    {
        OPTION_UNKNOWN = 0,

        OPTION_ALLOW_WITH,          // we do NOT allow with() statements by default
        OPTION_BINARY,
        OPTION_COVERAGE,
        OPTION_DEBUG,
        OPTION_EXTENDED_ESCAPE_SEQUENCES,
        OPTION_EXTENDED_OPERATORS,  // 1 support extended, 2 or 3 support extended and prevent '=' (use ':=' instead)
        OPTION_EXTENDED_STATEMENTS, // 1 support extended, 2 or 3 support extended and prevent if()/else/for()/while() ... without the '{' ... '}'
        OPTION_JSON,
        OPTION_OCTAL,
        OPTION_STRICT,
        OPTION_TRACE,
        OPTION_UNSAFE_MATH,         // optimize even what can be considered unsafe (see http://stackoverflow.com/questions/6430448/why-doesnt-gcc-optimize-aaaaaa-to-aaaaaa )

        OPTION_max
    };

    typedef int64_t                 option_value_t;

                                    Options();

    void                            set_option(option_t option, option_value_t value);
    option_value_t                  get_option(option_t option);

private:
    std::vector<option_value_t>     f_options;
};



}
// namespace as2js
#endif
// #ifndef AS2JS_OPTIONS_H

// vim: ts=4 sw=4 et
