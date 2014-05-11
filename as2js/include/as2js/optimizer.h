#ifndef AS2JS_OPTIMIZER_H
#define AS2JS_OPTIMIZER_H
/* optimizer.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

//#include    "as2js/stream.h"
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
    typedef int32_t         label_t;

                            Optimizer();

    void                    set_options(Options::pointer_t& options);
    int                     optimize(Node::pointer_t& root);
    label_t                 get_last_label() const;
    void                    set_first_label(label_t label);

private:
    void                    run(Node::pointer_t& root);
    int                     compare(Node::pointer_t& relational);
    void                    label(String& new_label);

    void                    add(Node::pointer_t& add);
    void                    assignment(Node::pointer_t& add);
    void                    assignment_add(Node::pointer_t& assignment);
    void                    assignment_divide(Node::pointer_t& assignment);
    void                    assignment_multiply(Node::pointer_t& assignment);
    void                    assignment_modulo(Node::pointer_t& assignment);
    void                    bitwise_and(Node::pointer_t& bitwise_and);
    void                    bitwise_not(Node::pointer_t& bitwise_not);
    void                    bitwise_or(Node::pointer_t& bitwise_or);
    void                    bitwise_xor(Node::pointer_t& bitwise_xor);
    void                    conditional(Node::pointer_t& conditional);
    void                    decrement(Node::pointer_t& decrement);
    void                    directive_list(Node::pointer_t& id);
    void                    divide(Node::pointer_t& divide);
    void                    do_directive(Node::pointer_t& do_node);
    void                    equality(Node::pointer_t& equality, bool strict, bool logical_not);
    void                    greater(Node::pointer_t& logical_and);
    void                    greater_equal(Node::pointer_t& logical_and);
    void                    if_directive(Node::pointer_t& if_node);
    void                    increment(Node::pointer_t& increment);
    void                    less(Node::pointer_t& logical_and);
    void                    less_equal(Node::pointer_t& logical_and);
    void                    logical_and(Node::pointer_t& logical_and);
    void                    logical_not(Node::pointer_t& logical_not);
    void                    logical_or(Node::pointer_t& logical_or);
    void                    logical_xor(Node::pointer_t& logical_xor);
    void                    maximum(Node::pointer_t& minmax);
    void                    minimum(Node::pointer_t& minmax);
    void                    modulo(Node::pointer_t& divide);
    void                    multiply(Node::pointer_t& multiply);
    void                    power(Node::pointer_t& multiply);
    void                    rotate_left(Node::pointer_t& rotate_left);
    void                    rotate_right(Node::pointer_t& rotate_right);
    void                    shift_left(Node::pointer_t& shift_left);
    void                    shift_right(Node::pointer_t& shift_right);
    void                    shift_right_unsigned(Node::pointer_t& shift_right_unsigned);
    void                    subtract(Node::pointer_t& subtract);
    void                    while_directive(Node::pointer_t& while_node);

    Options::pointer_t                      f_options;
    controlled_vars::auto_init<label_t, 0>  f_label;    // for auto-label naming
};





}
// namespace as2js

#endif
// #ifndef AS2JS_OPTIMIZER_H

// vim: ts=4 sw=4 et
