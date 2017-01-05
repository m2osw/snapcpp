#ifndef AS2JS_EXCEPTIONS_H
#define AS2JS_EXCEPTIONS_H
/* exceptions.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    <stdexcept>


namespace as2js
{

class exception_internal_error : public std::logic_error
{
public:
    exception_internal_error(char const *msg)        : logic_error(msg) {}
    exception_internal_error(std::string const& msg) : logic_error(msg) {}
};


class exception_exit : public std::runtime_error
{
public:
    exception_exit(int exit_code, char const *msg)        : runtime_error(msg), f_exit_code(exit_code) {}
    exception_exit(int exit_code, std::string const& msg) : runtime_error(msg), f_exit_code(exit_code) {}

    int         get_exit_code() const { return f_exit_code; }

private:
    int const   f_exit_code;
};


class exception_locked_node : public std::runtime_error
{
public:
    exception_locked_node(char const *msg)        : runtime_error(msg) {}
    exception_locked_node(std::string const& msg) : runtime_error(msg) {}
};


class exception_invalid_float : public std::runtime_error
{
public:
    exception_invalid_float(char const *msg)        : runtime_error(msg) {}
    exception_invalid_float(std::string const& msg) : runtime_error(msg) {}
};


class exception_invalid_index : public std::runtime_error
{
public:
    exception_invalid_index(char const *msg)        : runtime_error(msg) {}
    exception_invalid_index(std::string const& msg) : runtime_error(msg) {}
};


class exception_incompatible_node_type : public std::runtime_error
{
public:
    exception_incompatible_node_type(char const *msg)        : runtime_error(msg) {}
    exception_incompatible_node_type(std::string const& msg) : runtime_error(msg) {}
};


class exception_incompatible_node_data : public std::runtime_error
{
public:
    exception_incompatible_node_data(char const *msg)        : runtime_error(msg) {}
    exception_incompatible_node_data(std::string const& msg) : runtime_error(msg) {}
};


class exception_invalid_data : public std::runtime_error
{
public:
    exception_invalid_data(char const *msg)        : runtime_error(msg) {}
    exception_invalid_data(std::string const& msg) : runtime_error(msg) {}
};


class exception_already_defined : public std::runtime_error
{
public:
    exception_already_defined(char const *msg)        : runtime_error(msg) {}
    exception_already_defined(std::string const& msg) : runtime_error(msg) {}
};


class exception_no_parent : public std::runtime_error
{
public:
    exception_no_parent(char const *msg)        : runtime_error(msg) {}
    exception_no_parent(std::string const& msg) : runtime_error(msg) {}
};


class exception_index_out_of_range : public std::out_of_range
{
public:
    exception_index_out_of_range(char const *msg)        : out_of_range(msg) {}
    exception_index_out_of_range(std::string const& msg) : out_of_range(msg) {}
};


class exception_cannot_open_file : public std::runtime_error
{
public:
    exception_cannot_open_file(char const *msg)        : runtime_error(msg) {}
    exception_cannot_open_file(std::string const& msg) : runtime_error(msg) {}
};


class exception_file_already_open : public std::logic_error
{
public:
    exception_file_already_open(char const *msg)        : logic_error(msg) {}
    exception_file_already_open(std::string const& msg) : logic_error(msg) {}
};


class exception_cyclical_structure : public std::logic_error
{
public:
    exception_cyclical_structure(char const *msg)        : logic_error(msg) {}
    exception_cyclical_structure(std::string const& msg) : logic_error(msg) {}
};


}
// namespace as2js
#endif
// #ifndef AS2JS_AS_H

// vim: ts=4 sw=4 et
