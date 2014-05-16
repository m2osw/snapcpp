#ifndef AS2JS_OPTIMIZER_H
#define AS2JS_OPTIMIZER_H
/* optimizer.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/options.h"
#include    "as2js/node.h"


namespace as2js
{




// Finally, once the program was parsed and then compiled
// one usually wants to optimize it. This means removing
// all the possible expressions and statements which can
// be removed to make the code more efficient. The
// optimizations applied can be tweaked using the options.
//
// The code, after you ran the compiler looks like this:
//
//    Optimizer *optimizer = Optimizer::CreateOptimizer();
//    // this is the same options as for the parser
//    optimize->SetOptions(options);
//    optimize->Optimize(root);
//
// The Optimize() function goes through the list of
// nodes defined in the root parameter and it tries to
// remove all possible expressions and functions which
// will have no effect in the final output (certain things,
// such as x + 0, are not removed since it has an effect!).
// The root parameter is what was returned by the Parse()
// function of the Parser object.
//
// Note that it is expected that you first Compile()
// the nodes, but it is possible to call the optimizer
// without first running any compilation.
class Optimizer
{
public:
    typedef std::shared_ptr<Optimizer>      pointer_t;
    typedef int32_t                         label_t;

                            Optimizer();

    void                    set_options(Options::pointer_t& options);
    int                     optimize(Node::pointer_t& root);
    label_t                 get_last_label() const;
    void                    set_first_label(label_t label);

private:
    void                    run(Node::pointer_t& root);
    compare_t               compare(Node::pointer_t& relational);
    void                    label(String& new_label);

    void                    add(Node::pointer_t& add_node);
    void                    assignment(Node::pointer_t& assignment_node);
    void                    assignment_add(Node::pointer_t& assignment_node);
    void                    assignment_divide(Node::pointer_t& assignment_node);
    void                    assignment_multiply(Node::pointer_t& assignment_node);
    void                    assignment_modulo(Node::pointer_t& assignment_node);
    void                    bitwise_and(Node::pointer_t& bitwise_and_node);
    void                    bitwise_not(Node::pointer_t& bitwise_not_node);
    void                    bitwise_or(Node::pointer_t& bitwise_or_node);
    void                    bitwise_xor(Node::pointer_t& bitwise_xor_node);
    void                    conditional(Node::pointer_t& conditional_node);
    void                    condition_double_logical_not(Node::pointer_t& condition);
    void                    directive_list(Node::pointer_t& list);
    void                    divide(Node::pointer_t& divide_node);
    void                    do_directive(Node::pointer_t& do_node);
    void                    equality(Node::pointer_t& equality_node, bool const strict, bool const inverse);
    void                    greater(Node::pointer_t& greater_node);
    void                    greater_equal(Node::pointer_t& greate_equal_node);
    void                    if_directive(Node::pointer_t& if_node);
    void                    less(Node::pointer_t& less_node);
    void                    less_equal(Node::pointer_t& less_equal_node);
    void                    logical_and(Node::pointer_t& logical_and_node);
    void                    logical_not(Node::pointer_t& logical_not_node);
    void                    logical_or(Node::pointer_t& logical_or_node);
    void                    logical_xor(Node::pointer_t& logical_xor_node);
    void                    maximum(Node::pointer_t& maximum_node);
    void                    minimum(Node::pointer_t& minimum_node);
    void                    modulo(Node::pointer_t& modulo_node);
    void                    multiply(Node::pointer_t& multiply_noe);
    void                    power(Node::pointer_t& power_node);
    void                    rotate_left(Node::pointer_t& rotate_left_node);
    void                    rotate_right(Node::pointer_t& rotate_right_node);
    void                    shift_left(Node::pointer_t& shift_left_node);
    void                    shift_right(Node::pointer_t& shift_right_node);
    void                    shift_right_unsigned(Node::pointer_t& shift_right_unsigned_node);
    void                    subtract(Node::pointer_t& subtract_node);
    void                    while_directive(Node::pointer_t& while_node);

    Options::pointer_t                      f_options;
    controlled_vars::auto_init<label_t, 0>  f_label;    // for auto-label naming
};





}
// namespace as2js

#endif
// #ifndef AS2JS_OPTIMIZER_H

// vim: ts=4 sw=4 et
