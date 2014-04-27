#ifndef AS2JS_EXCEPTIONS_H
#define AS2JS_EXCEPTIONS_H
/* as.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    <stdexcept>


namespace as2js
{

class exception_internal_error : public std::runtime_error
{
public:
    exception_internal_error(char const *msg)        : runtime_error(msg) {}
    exception_internal_error(std::string const& msg) : runtime_error(msg) {}
};


class exception_locked_node : public std::runtime_error
{
public:
    exception_locked_node(char const *msg)        : runtime_error(msg) {}
    exception_locked_node(std::string const& msg) : runtime_error(msg) {}
};


class exception_incompatible_node_type : public std::runtime_error
{
public:
    exception_incompatible_node_type(char const *msg)        : runtime_error(msg) {}
    exception_incompatible_node_type(std::string const& msg) : runtime_error(msg) {}
};


class exception_index_out_of_range : public std::out_of_range
{
public:
    exception_index_out_of_range(char const *msg)        : out_of_range(msg) {}
    exception_index_out_of_range(std::string const& msg) : out_of_range(msg) {}
};


}
// namespace as2js
#endif
// #ifndef AS2JS_AS_H

// vim: ts=4 sw=4 et
