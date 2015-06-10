#ifndef CSSPP_COMPILER_H
#define CSSPP_COMPILER_H
// CSS Preprocessor
// Copyright (C) 2015  Made to Order Software Corp.
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

#include <istream>

namespace csspp
{

class compiler
{
public:
                            compiler();

    node::pointer_t         get_root() const;
    void                    set_root(node::pointer_t root);

    void                    clear_paths();
    void                    add_path(std::string const & path);

    void                    compile();

private:
    typedef std::vector<std::string>        string_vector_t;

    void                    compile(node::pointer_t n);
    void                    compile_component_value(node::pointer_t n);
    void                    compile_qualified_rule(node::pointer_t n);
    void                    compile_variable(node::pointer_t n);
    void                    compile_declaration(node::pointer_t n);

    node::pointer_t         f_root;
    string_vector_t         f_paths;
    node_vector_t           f_parents;
};

} // namespace csspp
#endif
// #ifndef CSSPP_COMPILER_H

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
