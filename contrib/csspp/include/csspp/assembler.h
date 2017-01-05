#ifndef CSSPP_ASSEMBLER_H
#define CSSPP_ASSEMBLER_H
// CSS Preprocessor
// Copyright (C) 2015-2017  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "csspp/node.h"

namespace csspp
{

enum class output_mode_t
{
    COMPACT,        // one rule per line, spaces around all objects
    COMPRESSED,     // as compressed as possible
    EXPANDED,       // beautified for human consumption
    TIDY            // one rule per line, no spaces
};

class assembler_impl;

class assembler
{
public:
                            assembler(std::ostream & out);

    void                    output(node::pointer_t n, output_mode_t mode);

private:
    std::string             escape_id(std::string const & id);

    void                    output(node::pointer_t n);
    void                    output_component_value(node::pointer_t n);
    void                    output_parenthesis(node::pointer_t n, int flags);
    void                    output_at_keyword(node::pointer_t n);
    void                    output_comment(node::pointer_t n);
    void                    output_string(std::string const & str);
    void                    output_url(std::string const & str);

    std::shared_ptr<assembler_impl> f_impl;
    std::ostream &                  f_out;
    node::pointer_t                 f_root;
};

} // namespace csspp

std::ostream & operator << (std::ostream & out, csspp::output_mode_t const type);

#endif
// #ifndef CSSPP_ASSEMBLER_H

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
