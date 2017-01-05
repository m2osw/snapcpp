#ifndef AS2JS_OPTIMIZER_TABLES_H
#define AS2JS_OPTIMIZER_TABLES_H
/* optimizer_tables.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/lexer.h"

/** \file
 * \brief Definition of internal tables of the optimizer.
 *
 * The structures defined here are used to define arrays of optimizations.
 *
 * In general we place a set of optimizations in separate files
 * based on the type of operator, statement, or other feature
 * being optimized.
 *
 * The organization is fairly simple:
 *
 * . optimization_table_t
 *
 * A large set of optimization_table_t structures each one
 * definiting one simple optimization such as:
 *
 * \code
 *     'a + b' => 'sum(a, b)'
 * \endcode
 *
 * when 'a' and 'b' are literal numbers.
 *
 * . 
 */

namespace as2js
{
namespace optimizer_details
{


#define POINTER_AND_COUNT(name)             name, sizeof(name) / sizeof(name[0])
#define NULL_POINTER_AND_COUNT()            nullptr, 0


uint32_t const OPTIMIZATION_MATCH_FLAG_CHILDREN =        0x0001;


struct optimization_match_t
{
    struct optimization_literal_t
    {
        Node::node_t                        f_operator;
        char const *                        f_string;
        Int64::int64_type                   f_int64;
        Float64::float64_type               f_float64;
    };

    uint8_t                         f_depth;        // to build a tree
    uint8_t                         f_match_flags;  // zero by default

    Node::node_t const *            f_node_types;
    size_t                          f_node_types_count;

    optimization_literal_t const *  f_with_value;

    Node::attribute_t const *       f_attributes;   // list of attributes, NODE_ATTR_max is used to separate each list
    size_t                          f_attributes_count;

    Node::flag_t const *            f_flags;        // list of flags, NODE_FLAG_max is used to seperate each list
    size_t                          f_flags_count;
};


enum class optimization_function_t
{
    OPTIMIZATION_FUNCTION_ADD,
    OPTIMIZATION_FUNCTION_BITWISE_AND,
    OPTIMIZATION_FUNCTION_BITWISE_NOT,
    OPTIMIZATION_FUNCTION_BITWISE_OR,
    OPTIMIZATION_FUNCTION_BITWISE_XOR,
    OPTIMIZATION_FUNCTION_COMPARE,
    OPTIMIZATION_FUNCTION_CONCATENATE,
    OPTIMIZATION_FUNCTION_DIVIDE,
    OPTIMIZATION_FUNCTION_EQUAL,
    OPTIMIZATION_FUNCTION_LESS,
    OPTIMIZATION_FUNCTION_LESS_EQUAL,
    OPTIMIZATION_FUNCTION_LOGICAL_NOT,
    OPTIMIZATION_FUNCTION_LOGICAL_XOR,
    OPTIMIZATION_FUNCTION_MATCH,
    OPTIMIZATION_FUNCTION_MAXIMUM,
    OPTIMIZATION_FUNCTION_MINIMUM,
    OPTIMIZATION_FUNCTION_MODULO,
    OPTIMIZATION_FUNCTION_MOVE,
    OPTIMIZATION_FUNCTION_MULTIPLY,
    OPTIMIZATION_FUNCTION_NEGATE,
    OPTIMIZATION_FUNCTION_POWER,
    OPTIMIZATION_FUNCTION_REMOVE,
    OPTIMIZATION_FUNCTION_ROTATE_LEFT,
    OPTIMIZATION_FUNCTION_ROTATE_RIGHT,
    OPTIMIZATION_FUNCTION_SET_INTEGER,
    //OPTIMIZATION_FUNCTION_SET_FLOAT,
    OPTIMIZATION_FUNCTION_SET_NODE_TYPE,
    OPTIMIZATION_FUNCTION_SHIFT_LEFT,
    OPTIMIZATION_FUNCTION_SHIFT_RIGHT,
    OPTIMIZATION_FUNCTION_SHIFT_RIGHT_UNSIGNED,
    OPTIMIZATION_FUNCTION_SMART_MATCH,
    OPTIMIZATION_FUNCTION_STRICTLY_EQUAL,
    OPTIMIZATION_FUNCTION_SUBTRACT,
    OPTIMIZATION_FUNCTION_SWAP,
    OPTIMIZATION_FUNCTION_TO_CONDITIONAL,
    //OPTIMIZATION_FUNCTION_TO_FLOAT64,
    OPTIMIZATION_FUNCTION_TO_INT64,
    OPTIMIZATION_FUNCTION_TO_NUMBER,
    //OPTIMIZATION_FUNCTION_TO_STRING,
    OPTIMIZATION_FUNCTION_WHILE_TRUE_TO_FOREVER
};


typedef uint16_t                    index_t;

struct optimization_optimize_t
{
    optimization_function_t         f_function;
    index_t                         f_indexes[6];   // number of indices used varies depending on the function
};


uint32_t const OPTIMIZATION_ENTRY_FLAG_UNSAFE_MATH =        0x0001;
uint32_t const OPTIMIZATION_ENTRY_FLAG_UNSAFE_OBJECT =      0x0002;     // in most cases because the object may have its own operator(s)

struct optimization_entry_t
{
    char const *                    f_name;
    uint32_t                        f_flags;

    optimization_match_t const *    f_match;
    size_t                          f_match_count;

    optimization_optimize_t const * f_optimize;
    size_t                          f_optimize_count;
};


struct optimization_table_t
{
    optimization_entry_t const *    f_entry;
    size_t                          f_entry_count;
};


struct optimization_tables_t
{
    optimization_table_t const *    f_table;
    size_t                          f_table_count;
};




bool optimize_tree(Node::pointer_t node);
bool match_tree(node_pointer_vector_t& node_array, Node::pointer_t node, optimization_match_t const *match, size_t match_size, uint8_t depth);
void apply_functions(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize, size_t optimize_size);


}
// namespace optimizer_details
}
// namespace as2js
#endif
// #ifndef AS2JS_OPTIMIZER_TABLES_H

// vim: ts=4 sw=4 et
