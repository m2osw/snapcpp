/* optimizer.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/optimizer.h"
#include    "as2js/message.h"
//#include    "as2js/exceptions.h"
#include    "optimizer_tables.h"

//#include    <controlled_vars/controlled_vars_no_enum_init.h>

//#include    <math.h>


namespace as2js
{


/** \class class Optimizer
 * \brief The as2js optimizer.
 *
 * Finally, once the program was parsed and then compiled
 * one usually wants to optimize it. This means removing
 * all the possible expressions and statements which can
 * be removed to make the code more efficient. The
 * optimizations applied can be tweaked using the options,
 * however, the options have already been parsed and thus
 * they are found directly in the nodes.
 *
 * The code, after you ran the compiler looks like this:
 *
 *    Optimizer::pointer_t optimizer(Optimizer::CreateOptimizer());
 *    optimize->Optimize(root);
 *
 * The optimize() function goes through the list of
 * nodes defined in the root parameter and it tries to
 * remove all possible expressions and functions which
 * will have no effect in the final output (by default,
 * certain things such as x + 0, are not removed since it
 * has an effect!).
 *
 * The root parameter may be what was returned by the parse()
 * function of the Parser object. However, in most cases, the
 * compiler only optimizes part of the tree as required (because
 * many parts cannot be optimized and it will make things
 * generally faster.)
 *
 * Note that it is expected that you first Compile()
 * the nodes, but it is possible to call the optimizer
 * without first running any compilation.
 */


/** \brief Optimize the specified node.
 *
 * This function goes through all the available optimizations and
 * processes them whenever they apply to your code.
 *
 * Errors may be generated whenever a problem is found. For example,
 * a division or modulo by zero can legally occur in your input
 * program. In that case the optimizer generates an error to let you
 * know that such a division is not legal and the compiler cannot
 * generate the output.
 *
 * The optimizer reports the total number of errors that were generated
 * while optimizing.
 *
 * \important
 * It is important to note that this function is not unlikely going
 * to modify your tree (even if you do not think there is a possible
 * optimization). This means the caller should not expect the node to
 * still be the same pointer and possibly not at the same location in
 * the parent node (many nodes get deleted.)
 *
 * \param[in] node  The node to optimize.
 *
 * \return The number of errors generated while optimizing.
 */
int Optimizer::optimize(Node::pointer_t& node)
{
    int const errcnt(Message::error_count());

    optimizer_details::optimize_tree(node);

    // This may not be at the right place because the caller may be
    // looping through a list of children too... (although we have
    // an important not in the documentation... that does not mean
    // much, does it?)
    if(node)
    {
        node->clean_tree();
    }

    return Message::error_count() - errcnt;
}



#if 0
void Optimizer::run(Node::pointer_t& node)
{
    // accept empty nodes, just ignore them
    if(!node
    || node->get_type() == Node::node_t::NODE_UNKNOWN)
    {
        return;
    }

    // we need to optimize the child most nodes first
    size_t const max(node->get_children_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(node->get_child(idx));
        run(child); // recurse
    }

    switch(node->get_type())
    {
    case Node::node_t::NODE_DIRECTIVE_LIST:
        directive_list(node);
        break;

    case Node::node_t::NODE_IF:
        if_directive(node);
        break;

    case Node::node_t::NODE_WHILE:
        while_directive(node);
        break;

    case Node::node_t::NODE_DO:
        do_directive(node);
        break;

    case Node::node_t::NODE_ASSIGNMENT:
        assignment(node);
        break;

    case Node::node_t::NODE_ASSIGNMENT_ADD:
    case Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
        assignment_add(node);
        break;

    case Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
        assignment_multiply(node);
        break;

    case Node::node_t::NODE_ASSIGNMENT_DIVIDE:
        assignment_divide(node);
        break;

    case Node::node_t::NODE_ASSIGNMENT_MODULO:
        assignment_modulo(node);
        break;

    case Node::node_t::NODE_BITWISE_NOT:
        bitwise_not(node);
        break;

    case Node::node_t::NODE_LOGICAL_NOT:
        logical_not(node);
        break;

    case Node::node_t::NODE_POWER:
        power(node);
        break;

    case Node::node_t::NODE_MULTIPLY:
        multiply(node);
        break;

    case Node::node_t::NODE_DIVIDE:
        divide(node);
        break;

    case Node::node_t::NODE_MODULO:
        modulo(node);
        break;

    case Node::node_t::NODE_ADD:
        add(node);
        break;

    case Node::node_t::NODE_SUBTRACT:
        subtract(node);
        break;

    case Node::node_t::NODE_SHIFT_LEFT:
        shift_left(node);
        break;

    case Node::node_t::NODE_SHIFT_RIGHT:
        shift_right(node);
        break;

    case Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
        shift_right_unsigned(node);
        break;

    case Node::node_t::NODE_ROTATE_LEFT:
        rotate_left(node);
        break;

    case Node::node_t::NODE_ROTATE_RIGHT:
        rotate_right(node);
        break;

    case Node::node_t::NODE_LESS:
        less(node);
        break;

    case Node::node_t::NODE_LESS_EQUAL:
        less_equal(node);
        break;

    case Node::node_t::NODE_GREATER:
        greater(node);
        break;

    case Node::node_t::NODE_GREATER_EQUAL:
        greater_equal(node);
        break;

    case Node::node_t::NODE_EQUAL:
        equality(node, false, false);
        break;

    case Node::node_t::NODE_STRICTLY_EQUAL:
        equality(node, true, false);
        break;

    case Node::node_t::NODE_NOT_EQUAL:
        equality(node, false, true);
        break;

    case Node::node_t::NODE_STRICTLY_NOT_EQUAL:
        equality(node, true, true);
        break;

    case Node::node_t::NODE_BITWISE_AND:
        bitwise_and(node);
        break;

    case Node::node_t::NODE_BITWISE_XOR:
        bitwise_xor(node);
        break;

    case Node::node_t::NODE_BITWISE_OR:
        bitwise_or(node);
        break;

    case Node::node_t::NODE_LOGICAL_AND:
        logical_and(node);
        break;

    case Node::node_t::NODE_LOGICAL_XOR:
        logical_xor(node);
        break;

    case Node::node_t::NODE_LOGICAL_OR:
        logical_or(node);
        break;

    case Node::node_t::NODE_MAXIMUM:
        maximum(node);
        break;

    case Node::node_t::NODE_MINIMUM:
        minimum(node);
        break;

    case Node::node_t::NODE_CONDITIONAL:
        conditional(node);
        break;

    default:
        break;

    }
}



void Optimizer::directive_list(Node::pointer_t& list)
{
    size_t const max(list->get_children_size());
    for(size_t idx(0); idx < max; ++idx)
    {
        Node::pointer_t child(list->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_IDENTIFIER
        && child->get_link(Node::link_t::LINK_INSTANCE))
        {
            // TBD: At this point I do not recall what this represents...
            //      (I think child would be an unused type)
            //
            // Source:
            //   ??
            // Destination:
            //   ??
            child->to_unknown();
        }
    }
}


void Optimizer::condition_double_logical_not(Node::pointer_t& condition)
{
    // Reduce double '!'
    //   !!a, !!!!a, !!!!!!a, etc.
    //
    while(condition->get_type() == Node::node_t::NODE_LOGICAL_NOT
       && condition->get_children_size() == 1)
    {
        Node::pointer_t sub_expr(condition->get_child(0));
        if(sub_expr->get_type() == Node::node_t::NODE_LOGICAL_NOT
        && sub_expr->get_children_size() == 1)
        {
            // Source:
            //   !!a
            // Destination:
            //   a
            //
            condition->replace_with(sub_expr->get_child(0));
        }
    }
}


void Optimizer::if_directive(Node::pointer_t& if_node)
{
    size_t max(if_node->get_children_size());
    if(max != 2 && max != 3)
    {
        return;
    }

    Node::pointer_t condition(if_node->get_child(0));
    condition_double_logical_not(condition);
    if(condition->to_boolean())
    {
        if(condition->is_true())
        {
            // Source:
            //   if(true) A; [else B;]
            // Destination:
            //   A;
            if_node->replace_with(if_node->get_child(1));
        }
        else if(max == 3)
        {
            // Source:
            //   if(false) A; else B;
            // Destination:
            //   B;
            if_node->replace_with(if_node->get_child(2));
        }
        else
        {
            // Source:
            //   if(false) A;
            // Destination:
            //   ;
            //
            // we want to delete the if_node, it serves
            // no purpose! (another function is in charge
            // of deleting; we just mark the node as unknown)
            if_node->to_unknown();
        }
    }
}


void Optimizer::while_directive(Node::pointer_t& while_node)
{
    size_t const max(while_node->get_children_size());
    if(max != 2)
    {
        return;
    }

// TODO:
//
// if we detect something such as:
//
//    cnt = 3
//    while(cnt > 0) {
//        ...
//    }
//
// we can reduce to:
//
//    cnt = 3;
//    do {
//        ...
//    } while(cnt > 0);
//
// since the first time cnt > 0 will always be true.
//

    Node::pointer_t condition(while_node->get_child(0));
    condition_double_logical_not(condition);
    if(condition->to_boolean())
    {
        if(condition->is_true())
        {
            // Source:
            //    while(true) A;
            // Destination:
            //    for(;;) A;
            //
            // TBD: is a for(;;) ... faster than a while(true) ... ?
            //
            // TODO: this is wrong if the user has labels and
            //       uses them with a break or a continue.
            //
            // TODO:
            // Whenever we detect a forever loop we
            // need to check whether it includes some
            // break, continue, or return statement.
            //
            // if(while_node->check_labels(BREAK?)) ...
            //
            // If there are none, we should err about it.
            // It is likely a bad one!
            // [except if the function returns Never]
            //
            // create a forever loop
            //
            Node::pointer_t forever(while_node->create_replacement(Node::node_t::NODE_FOR));
            forever->append_child(while_node->create_replacement(Node::node_t::NODE_EMPTY));
            forever->append_child(while_node->create_replacement(Node::node_t::NODE_EMPTY));
            forever->append_child(while_node->create_replacement(Node::node_t::NODE_EMPTY));
            forever->append_child(while_node->get_child(1));
            while_node->replace_with(forever);
        }
        else
        {
            // Source:
            //    while(false) A;
            // Destination:
            //    ;
            //
            // we want to delete the while_node, it serves
            // no purpose! (another function is in charge
            // of deleting; we just mark the node as unknown)
            while_node->to_unknown();
        }
    }
}


void Optimizer::do_directive(Node::pointer_t& do_node)
{
    size_t const max(do_node->get_children_size());
    if(max != 2)
    {
        return;
    }

    Node::pointer_t condition(do_node->get_child(1));
    condition_double_logical_not(condition);
    if(condition->to_boolean())
    {
        // TODO: changing loops is wrong if the user has labels and
        //       uses them with a break or a continue from within
        //       that loop.
        //
        //if(!do_node->check_labels(USES?))
        //{
        if(condition->is_true())
        {
            // Source:
            //   do { A; } while(true);
            // Destination:
            //   for(;;) A;
            //
            // TODO: this is wrong if the user has labels and
            //       uses them with a break or a continue.
            //
            // TODO:
            // Whenever we detect a forever loop we
            // need to check whether it includes some
            // break. If there isn't a break, we should
            // err about it. It is likely a bad one!
            // [except if the function returns Never]
            //
            //   if(!do_node->check_labels(BREAKS?)) ... warning/error
            //
            // create a forever loop
            //
            Node::pointer_t forever(do_node->create_replacement(Node::node_t::NODE_FOR));
            forever->append_child(do_node->create_replacement(Node::node_t::NODE_EMPTY));
            forever->append_child(do_node->create_replacement(Node::node_t::NODE_EMPTY));
            forever->append_child(do_node->create_replacement(Node::node_t::NODE_EMPTY));
            forever->append_child(do_node->get_child(0));
            do_node->replace_with(forever);
        }
        else
        {
            // Source:
            //   do { A; } while(false);
            // Destination:
            //   A;
            //
            // in this case, we simply run the
            // directives once
            //
            do_node->replace_with(do_node->get_child(0));
        }
        //}
    }
}


void Optimizer::assignment(Node::pointer_t& assignment_node)
{
    if(assignment_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(assignment_node->get_child(0));
    Node::pointer_t right(assignment_node->get_child(1));

    if(left->get_type() == Node::node_t::NODE_IDENTIFIER
    && right->get_type() == Node::node_t::NODE_IDENTIFIER
    && left->get_string() == right->get_string())
    {
        // Source:
        //   a = a;
        // Destination:
        //   a;
        //
        // Note: Why would this happen?
        //
        //       The user may do something much more complicated such as:
        //          a = (complex test) ? a : b;
        //       and (complex test) ends up being true at compile time.
        //
        assignment_node->replace_with(left);
    }
}


void Optimizer::assignment_add(Node::pointer_t& assignment_node)
{
    // a += 0 -> a
    // a -= 0 -> a
    if(assignment_node->get_children_size() != 2)
    {
        return;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    Node::pointer_t right(assignment_node->get_child(1));

    // WARNING: here we cannot convert strings to numbers because
    //          the plus operator concatenates strings
    if((assignment_node->get_type() == Node::node_t::NODE_ASSIGNMENT_ADD && !right->is_string())
    || assignment_node->get_type() == Node::node_t::NODE_ASSIGNMENT_SUBTRACT)
    {
        if(!right->to_number())
        {
            return;
        }
    }

    if((right->is_int64() && right->get_int64().get() == 0)
    || (right->is_float64() && right->get_float64().get() == 0.0))
    {
        // Source:
        //   a += 0;   or   a += 0.0;
        //   a -= 0;   or   a -= 0.0;
        // Destination:
        //   a;
        //
        assignment_node->replace_with(assignment_node->get_child(0));
    }
#pragma GCC diagnostic pop
}


void Optimizer::assignment_multiply(Node::pointer_t& assignment_node)
{
    // a *= 0 -> 0
    // a *= 1 -> a
    // a *= NaN -> NaN
    if(assignment_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t right(assignment_node->get_child(1));
    if(right->to_number())
    {
        if(right->is_int64())
        {
            Int64::int64_type i(right->get_int64().get());
            if(i == 0)
            {
                // Source:
                //   a *= 0;
                // Destination:
                //   0
                //
                assignment_node->replace_with(right);
            }
            else if(i == 1)
            {
                // Source:
                //   a *= 1;
                // Destination:
                //   a;
                //
                assignment_node->replace_with(assignment_node->get_child(0));
            }
        }
        else
        {
            Float64::float64_type f(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(f == 0.0)
            {
                // Source:
                //   a *= 0.0;
                // Destination:
                //   a;
                //
                assignment_node->replace_with(right);
            }
            else if(f == 1.0)
            {
                // Source:
                //   a *= 1.0;
                // Destination:
                //   a;
                //
                assignment_node->replace_with(assignment_node->get_child(0));
            }
#pragma GCC diagnostic pop
        }
    }
}


void Optimizer::assignment_divide(Node::pointer_t& assignment_node)
{
    // a /= 1 -> a
    // a /= 0 -> ERROR
    if(assignment_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t right(assignment_node->get_child(1));
    if(right->get_type() == Node::node_t::NODE_INT64)
    {
        Int64::int64_type i(right->get_int64().get());
        if(i == 0)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, assignment_node->get_position());
            msg << "dividing by zero is illegal";
        }
        else if(i == 1)
        {
            // Source:
            //   a /= 1;
            // Destination:
            //   a;
            //
            assignment_node->replace_with(assignment_node->get_child(0));
        }
    }
    else if(right->get_type() == Node::node_t::NODE_FLOAT64)
    {
        Float64::float64_type f(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(f == 0.0)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, assignment_node->get_position());
            msg << "dividing by zero is illegal";
        }
        else if(f == 1.0)
        {
            // Source:
            //   a /= 1.0;
            // Destination:
            //   a;
            //
            assignment_node->replace_with(assignment_node->get_child(0));
        }
#pragma GCC diagnostic pop
    }
}


void Optimizer::assignment_modulo(Node::pointer_t& assignment_node)
{
    // a %= 0 -> ERROR
    if(assignment_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t right(assignment_node->get_child(1));
    if(right->get_type() == Node::node_t::NODE_INT64)
    {
        if(right->get_int64().get() == 0)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, assignment_node->get_position());
            msg << "modulo by zero is illegal";
        }
    }
    else if(right->get_type() == Node::node_t::NODE_FLOAT64)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(right->get_float64().get() == 0.0)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, assignment_node->get_position());
            msg << "modulo by zero is illegal";
        }
#pragma GCC diagnostic pop
    }
}



// TODO: all arithmetic operations need to be checked for overflows...
//       to_boolean(NaN) == false

void Optimizer::bitwise_not(Node::pointer_t& bitwise_not_node)
{
    if(bitwise_not_node->get_children_size() != 1)
    {
        return;
    }

    Node::pointer_t child(bitwise_not_node->get_child(0));
    if(child->to_number())
    {
        if(child->get_type() == Node::node_t::NODE_INT64)
        {
            // Source:
            //   ~<int>
            // Destination:
            //   computed ~ToInt32(<int>)
            //
            // ECMAScript documentation says return a 32bit number
            child->set_int64((~child->get_int64().get()) & 0x0FFFFFFFF);
        }
        else
        {
            // Source:
            //   ~<float>
            // Destination:
            //   computed ~ToInt32(<float>)
            //
            // ECMAScript version 4 says we can do this with floats!
            // ECMAScript documentation says return a 32bit number
            Float64 f(child->get_float64());
            child->to_int64();
            child->set_int64((~static_cast<int64_t>(f.get())) & 0x0FFFFFFFF);
        }
        bitwise_not_node->replace_with(child);
    }
}


void Optimizer::logical_not(Node::pointer_t& logical_not_node)
{
    if(logical_not_node->get_children_size() != 1)
    {
        return;
    }

    Node::pointer_t child(logical_not_node->get_child(0));
    if(child->to_boolean())
    {
        // Source:
        //   !true   or   !false
        // Destination:
        //   false   or   true
        //
        child->set_boolean(child->get_type() != Node::node_t::NODE_TRUE);
        logical_not_node->replace_with(child);
    }
    else if(child->get_type() == Node::node_t::NODE_LOGICAL_NOT)
    {
        // IMPORTANT NOTE: We do NOT replace '!!a' with 'a' because
        //                 in reality, '!!a != a' if a is not boolean
        //                 to start with!
        //                 However, if someone was to use '!!!a',
        //                 that we can optimize to '!a'.
        //                 Note also that we can optimize cases such
        //                 as if(!!a) and while(!!a).
        if(child->get_children_size() == 1)
        {
            Node::pointer_t sub_child(child->get_child(0));
            if(sub_child->get_type() == Node::node_t::NODE_LOGICAL_NOT)
            {
                if(sub_child->get_children_size() == 1)
                {
                    // Source:
                    //   !!!a;
                    // Destination:
                    //   !a;
                    //
                    child->replace_with(sub_child->get_child(0));
                }
            }
        }
    }
}


void Optimizer::power(Node::pointer_t& power_node)
{
    if(power_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(power_node->get_child(0));
    Node::pointer_t right(power_node->get_child(1));

    if(!right->to_number())
    {
        // Reduce the following
        //    0 ** b = 0 (we cannot guarantee that b <> 0, we cannot do it)
        //    1 ** b = 1
        if(!left->to_number())
        {
            return;
        }
        if(left->get_type() == Node::node_t::NODE_INT64
        && left->get_int64().get() == 1)
        {
            if(right->has_side_effects())
            {
                // Source:
                //   1 ** b;
                // Destination:
                //   (b, 1);    // because b has side effects...
                //
                Node::pointer_t list(power_node->create_replacement(Node::node_t::NODE_LIST));
                list->append_child(right);
                list->append_child(left);
                power_node->replace_with(list);
            }
            else
            {
                // Source:
                //   1.0 ** b;
                // Destination:
                //   1.0
                //
                power_node->replace_with(left);
            }
        }
        else
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(left->get_float64().get() == 1.0)
            {
                if(right->has_side_effects())
                {
                    // Source:
                    //   1.0 ** b;
                    // Destination:
                    //   (b, 1.0);    // because b has side effects...
                    //
                    Node::pointer_t list(power_node->create_replacement(Node::node_t::NODE_LIST));
                    list->append_child(right);
                    list->append_child(left);
                    power_node->replace_with(list);
                }
                else
                {
                    // Source:
                    //   1.0 ** b;
                    // Destination:
                    //   1.0;
                    //
                    power_node->replace_with(left);
                }
            }
#pragma GCC diagnostic pop
        }
        return;
    }

    // a ** b   where a and b are numbers
    if(left->to_number())
    {
        // Source:
        //   a ** b;
        // Destination:
        //   (a ** b);   // result of operation
        //
        // left and right are numbers, compute the result now
        // the result is considered to be a floating point no matter
        // what so here we simplify our computation
        left->to_float64();
        right->to_float64();
        left->set_float64(pow(left->get_float64().get(), right->get_float64().get()));
        power_node->replace_with(left);
        return;
    }

    //
    // Reduce the following if possible
    //    a ** 0 = 1;
    //    a ** 1 = a;
    //    a ** -1 = 1 / a;
    //    a ** 2 = a * a;  (TODO: we do not do this one because 'a' can
    //                     be a complex expression which we do not want
    //                     to duplicate; test the complexity if just one
    //                     var do it!)
    //
    if(right->get_type() == Node::node_t::NODE_INT64)
    {
        Node::pointer_t one(power_node->create_replacement(Node::node_t::NODE_INT64));
        one->set_int64(1);
        Int64::int64_type i(right->get_int64().get());
        if(i == 0)
        {
            if(left->has_side_effects())
            {
                // Source:
                //   a ** 0;
                // Destination:
                //   (a, 1);     // because 'a' has side effects
                //
                Node::pointer_t list(power_node->create_replacement(Node::node_t::NODE_LIST));
                list->append_child(left);
                list->append_child(one);
                power_node->replace_with(list);
            }
            else
            {
                // Source:
                //   a ** 0;
                // Destination:
                //   1;
                //
                power_node->replace_with(one);
            }
            return;
        }
        else if(i == 1)
        {
            // Source:
            //   a ** 1;
            // Destination:
            //   a;       // note: if 'a' is not a number, this should be NaN
            //
            power_node->replace_with(left);
            return;
        }
        else if(i == -1)
        {
            // Source:
            //   a ** -1;
            // Destination:
            //   1 / a;
            //
            // This should be better than using Math.pow(a, -1);
            Node::pointer_t inverse(power_node->create_replacement(Node::node_t::NODE_DIVIDE));
            inverse->append_child(one);
            inverse->append_child(left);
            power_node->replace_with(inverse);
            return;
        }
    }
    else
    {
        Node::pointer_t one(power_node->create_replacement(Node::node_t::NODE_FLOAT64));
        one->set_float64(1.0);
        Float64::float64_type f(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(f == 0.0)
        {
            if(left->has_side_effects())
            {
                // Source:
                //   a ** 0.0;
                // Destination:
                //   (a, 1.0);     // because 'a' has side effects
                //
                Node::pointer_t list(power_node->create_replacement(Node::node_t::NODE_LIST));
                list->append_child(left);
                list->append_child(one);
                power_node->replace_with(list);
            }
            else
            {
                // Source:
                //   a ** 0.0;
                // Destination:
                //   1.0;
                //
                power_node->replace_with(one);
            }
            return;
        }
        else if(f == 1.0)
        {
            // Source:
            //   a ** 1.0;
            // Destination:
            //   a;       // note: if 'a' is not a number, this should be NaN
            //
            power_node->replace_with(left);
            return;
        }
        else if(f == -1.0)
        {
            // Source:
            //   a ** -1.0;
            // Destination:
            //   1.0 / a;
            //
            // This should be better than using Math.pow(a, -1);
            Node::pointer_t inverse(power_node->create_replacement(Node::node_t::NODE_DIVIDE));
            inverse->append_child(one);
            inverse->append_child(left);
            power_node->replace_with(inverse);
            return;
        }
#pragma GCC diagnostic pop
    }

}



void Optimizer::multiply(Node::pointer_t& multiply_node)
{
    // Reduce
    //    a * 0 = 0
    //    a * 1 = a
    //    a * -1 = -a

    if(multiply_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(multiply_node->get_child(0));
    Node::pointer_t right(multiply_node->get_child(1));

    if(left->to_number())
    {
        if(right->to_number())
        {
            if(left->get_type() == Node::node_t::NODE_INT64)
            {
                if(right->get_type() == Node::node_t::NODE_INT64)
                {
                    // Source:
                    //   a * b;
                    // Destination:
                    //   (a * b);    // computed as it was immediate numbers
                    //
                    left->set_int64(left->get_int64().get() * right->get_int64().get());
                    multiply_node->replace_with(left);
                }
                else
                {
                    // Source:
                    //   a * b;
                    // Destination:
                    //   (a * b);    // computed as it was immediate numbers
                    //
                    right->set_float64(static_cast<Float64::float64_type>(left->get_int64().get()) * right->get_float64().get());
                    multiply_node->replace_with(right);
                }
            }
            else
            {
                if(right->get_type() == Node::node_t::NODE_INT64)
                {
                    // Source:
                    //   a * b;
                    // Destination:
                    //   (a * b);    // computed as it was immediate numbers
                    //
                    left->set_float64(left->get_float64().get() * right->get_int64().get());
                    multiply_node->replace_with(left);
                }
                else
                {
                    // Source:
                    //   a * b;
                    // Destination:
                    //   (a * b);    // computed as it was immediate numbers
                    //
                    right->set_float64(left->get_float64().get() * right->get_float64().get());
                    multiply_node->replace_with(right);
                }
            }
        }
        else
        {
            if(left->get_type() == Node::node_t::NODE_INT64)
            {
                Int64::int64_type i(left->get_int64().get());
                if(i == 0)
                {
                    // TODO: if right is not a number then it should be NaN
                    if(right->has_side_effects())
                    {
                        // Source:
                        //   0 * b;
                        // Destination:
                        //   (b, 0);    // because b has side effects...
                        //
                        Node::pointer_t list(multiply_node->create_replacement(Node::node_t::NODE_LIST));
                        list->append_child(right);
                        list->append_child(left);
                        multiply_node->replace_with(list);
                    }
                    else
                    {
                        // Source:
                        //   0 * b;
                        // Destination:
                        //   0;
                        //
                        multiply_node->replace_with(left);
                    }
                }
                else if(i == 1)
                {
                    // Source:
                    //   1 * b;
                    // Destination:
                    //   b;
                    //
                    multiply_node->replace_with(right);
                }
                else if(i == -1)
                {
                    // Source:
                    //   -1 * b;
                    // Destination:
                    //   -b;
                    //
                    Node::pointer_t negate(multiply_node->create_replacement(Node::node_t::NODE_SUBTRACT));
                    negate->append_child(right);
                    multiply_node->replace_with(negate);
                }
            }
            else
            {
                Float64::float64_type f(left->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                if(f == 0.0)
                {
                    // TODO: if right is not a number then it should be NaN
                    if(right->has_side_effects())
                    {
                        // Source:
                        //   0.0 * b;
                        // Destination:
                        //   (b, 0.0);    // because b has side effects...
                        //
                        Node::pointer_t list(multiply_node->create_replacement(Node::node_t::NODE_LIST));
                        list->append_child(right);
                        list->append_child(left);
                        multiply_node->replace_with(list);
                    }
                    else
                    {
                        // Source:
                        //   0.0 * b;
                        // Destination:
                        //   0.0;
                        //
                        multiply_node->replace_with(left);
                    }
                }
                else if(f == 1.0)
                {
                    // Source:
                    //   1.0 * b;
                    // Destination:
                    //   b;
                    //
                    multiply_node->replace_with(right);
                }
                else if(f == -1.0)
                {
                    // Source:
                    //   -1 * b;
                    // Destination:
                    //   -b;
                    //
                    Node::pointer_t negate(multiply_node->create_replacement(Node::node_t::NODE_SUBTRACT));
                    negate->append_child(right);
                    multiply_node->replace_with(negate);
                }
#pragma GCC diagnostic pop
            }
        }
    }
    else
    {
        if(right->get_type() == Node::node_t::NODE_INT64)
        {
            Int64::int64_type i(right->get_int64().get());
            if(i == 0)
            {
                // TODO: if left is not a number then it should be NaN
                if(left->has_side_effects())
                {
                    // Source:
                    //   a * 0;
                    // Destination:
                    //   (a, 0);    // because a has side effects...
                    //
                    Node::pointer_t list(multiply_node->create_replacement(Node::node_t::NODE_LIST));
                    list->append_child(left);
                    list->append_child(right);
                    multiply_node->replace_with(list);
                }
                else
                {
                    // Source:
                    //   a * 0;
                    // Destination:
                    //   0;
                    //
                    multiply_node->replace_with(right);
                }
            }
            else if(i == 1)
            {
                // Source:
                //   a * 1;
                // Destination:
                //   a;
                //
                multiply_node->replace_with(left);
            }
            else if(i == -1)
            {
                // Source:
                //   a * -1;
                // Destination:
                //   -a;
                //
                Node::pointer_t negate(multiply_node->create_replacement(Node::node_t::NODE_SUBTRACT));
                negate->append_child(left);
                multiply_node->replace_with(negate);
            }
        }
        else
        {
            Float64::float64_type f(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(f == 0.0)
            {
                // TODO: if right is not a number then it should be NaN
                if(left->has_side_effects())
                {
                    // Source:
                    //   a * 0.0;
                    // Destination:
                    //   (a, 0.0);    // because b has side effects...
                    //
                    Node::pointer_t list(multiply_node->create_replacement(Node::node_t::NODE_LIST));
                    list->append_child(left);
                    list->append_child(right);
                    multiply_node->replace_with(list);
                }
                else
                {
                    // Source:
                    //   a * 0.0;
                    // Destination:
                    //   0.0;
                    //
                    multiply_node->replace_with(right);
                }
            }
            else if(f == 1.0)
            {
                // Source:
                //   a * 1.0;
                // Destination:
                //   a;
                //
                multiply_node->replace_with(left);
            }
            else if(f == -1.0)
            {
                // Source:
                //   a * -1.0;
                // Destination:
                //   -a;
                //
                Node::pointer_t negate(multiply_node->create_replacement(Node::node_t::NODE_SUBTRACT));
                negate->append_child(left);
                multiply_node->replace_with(negate);
            }
#pragma GCC diagnostic pop
        }
    }
}


void Optimizer::divide(Node::pointer_t& divide_node)
{
    // Reduce
    //    a / 1 = a
    //    a / -1 = -a
    //    0 / b = 0
    // Error
    //    a / 0

    if(divide_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(divide_node->get_child(0));
    Node::pointer_t right(divide_node->get_child(1));

    if(left->to_number())
    {
        if(right->to_number())
        {
            if(left->get_type() == Node::node_t::NODE_INT64)
            {
                if(right->get_type() == Node::node_t::NODE_INT64)
                {
                    // Source:
                    //   a / b;
                    // Destination:
                    //   (a / b);    // computed as it was immediate numbers
                    //
                    Int64::int64_type ri(right->get_int64().get());
                    if(ri == 0)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
                        msg << "dividing by zero is illegal";
                        return;
                    }
                    Float64::float64_type f(static_cast<Float64::float64_type>(left->get_int64().get()) / static_cast<Float64::float64_type>(ri));
                    Int64::int64_type i(static_cast<Int64::int64_type>(f));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(f == i) // maybe not such a rare event... (10 / 2 = 5)
                    {
                        left->set_int64(i);
                    }
                    else
                    {
                        left->to_float64();
                        left->set_float64(f);
                    }
#pragma GCC diagnostic pop
                    divide_node->replace_with(left);
                }
                else
                {
                    // Source:
                    //   a / b;
                    // Destination:
                    //   (a / b);    // computed as it was immediate numbers
                    //
                    Float64::float64_type rf(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(rf == 0)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
                        msg << "dividing by zero is illegal";
                        return;
                    }
#pragma GCC diagnostic pop
                    right->set_float64(static_cast<Float64::float64_type>(left->get_int64().get()) / rf);
                    divide_node->replace_with(right);
                }
            }
            else
            {
                if(right->get_type() == Node::node_t::NODE_INT64)
                {
                    // Source:
                    //   a / b;
                    // Destination:
                    //   (a / b);    // computed as it was immediate numbers
                    //
                    Int64::int64_type ri(right->get_int64().get());
                    if(ri == 0)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
                        msg << "dividing by zero is illegal";
                        return;
                    }
                    left->set_float64(left->get_float64().get() / ri);
                    divide_node->replace_with(left);
                }
                else
                {
                    // Source:
                    //   a / b;
                    // Destination:
                    //   (a / b);    // computed as it was immediate numbers
                    //
                    Float64::float64_type rf(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(rf == 0)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
                        msg << "dividing by zero is illegal";
                        return;
                    }
#pragma GCC diagnostic pop
                    right->set_float64(left->get_float64().get() / right->get_float64().get());
                    divide_node->replace_with(right);
                }
            }
        }
        else
        {
            if(left->get_type() == Node::node_t::NODE_INT64)
            {
                Int64::int64_type i(left->get_int64().get());
                if(i == 0)
                {
                    // Note that if b is zero too then we should get an
                    // error instead we'll get zero...
                    //
                    // TODO: if right is not a number then it should be NaN
                    if(right->has_side_effects())
                    {
                        // Source:
                        //   0 / b;
                        // Destination:
                        //   (b, 0);    // because b has side effects...
                        //
                        Node::pointer_t list(divide_node->create_replacement(Node::node_t::NODE_LIST));
                        list->append_child(right);
                        list->append_child(left);
                        divide_node->replace_with(list);
                    }
                    else
                    {
                        // Source:
                        //   0 / b;
                        // Destination:
                        //   0;
                        //
                        divide_node->replace_with(left);
                    }
                }
            }
            else
            {
                Float64::float64_type f(left->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                if(f == 0.0)
                {
                    // Note that if b is zero too then we should get an
                    // error instead we'll get zero...
                    //
                    // TODO: if right is not a number then it should be NaN
                    if(right->has_side_effects())
                    {
                        // Source:
                        //   0.0 * b;
                        // Destination:
                        //   (b, 0.0);    // because b has side effects...
                        //
                        Node::pointer_t list(divide_node->create_replacement(Node::node_t::NODE_LIST));
                        list->append_child(right);
                        list->append_child(left);
                        divide_node->replace_with(list);
                    }
                    else
                    {
                        // Source:
                        //   0.0 * b;
                        // Destination:
                        //   0.0;
                        //
                        divide_node->replace_with(left);
                    }
                }
#pragma GCC diagnostic pop
            }
        }
    }
    else
    {
        if(right->get_type() == Node::node_t::NODE_INT64)
        {
            Int64::int64_type i(right->get_int64().get());
            if(i == 0)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
                msg << "dividing by zero is illegal";
                return;
            }
            else if(i == 1)
            {
                // Source:
                //   a / 1;
                // Destination:
                //   a;
                //
                divide_node->replace_with(left);
            }
            else if(i == -1)
            {
                // Source:
                //   a / -1;
                // Destination:
                //   -a;
                //
                Node::pointer_t negate(divide_node->create_replacement(Node::node_t::NODE_SUBTRACT));
                negate->append_child(left);
                divide_node->replace_with(negate);
            }
        }
        else
        {
            Float64::float64_type f(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(f == 0.0)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
                msg << "dividing by zero is illegal";
                return;
            }
            else if(f == 1.0)
            {
                // Source:
                //   a / 1.0;
                // Destination:
                //   a;
                //
                divide_node->replace_with(left);
            }
            else if(f == -1.0)
            {
                // Source:
                //   a / -1.0;
                // Destination:
                //   -a;
                //
                Node::pointer_t negate(divide_node->create_replacement(Node::node_t::NODE_SUBTRACT));
                negate->append_child(left);
                divide_node->replace_with(negate);
            }
#pragma GCC diagnostic pop
        }
    }
}


void Optimizer::modulo(Node::pointer_t& modulo_node)
{
    // Reduce
    //    a % b       -- when a and b are literals
    //    a % 1 = 0   -- this is true only with integers
    //    a % -1 = 0   -- this is true only with integers
    // Error
    //    a % 0

    if(modulo_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(modulo_node->get_child(0));
    Node::pointer_t right(modulo_node->get_child(1));

    if(left->to_number())
    {
        if(right->to_number())
        {
            if(left->get_type() == Node::node_t::NODE_INT64)
            {
                if(right->get_type() == Node::node_t::NODE_INT64)
                {
                    // Source:
                    //   a % b;
                    // Destination:
                    //   (a % b);    // computed as it was immediate numbers
                    //
                    Int64::int64_type ri(right->get_int64().get());
                    if(ri == 0)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, modulo_node->get_position());
                        msg << "modulo by zero is illegal";
                        return;
                    }
                    Float64::float64_type f(static_cast<Float64::float64_type>(left->get_int64().get()) / static_cast<Float64::float64_type>(ri));
                    Int64::int64_type i(static_cast<Int64::int64_type>(f));
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(f == i) // maybe not such a rare event... (10 / 2 = 5)
                    {
                        left->set_int64(i);
                    }
                    else
                    {
                        left->to_float64();
                        left->set_float64(f);
                    }
#pragma GCC diagnostic pop
                    modulo_node->replace_with(left);
                }
                else
                {
                    // Source:
                    //   a / b;
                    // Destination:
                    //   (a / b);    // computed as it was immediate numbers
                    //
                    Float64::float64_type rf(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(rf == 0)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, modulo_node->get_position());
                        msg << "modulo by zero is illegal";
                        return;
                    }
#pragma GCC diagnostic pop
                    right->set_float64(static_cast<Float64::float64_type>(left->get_int64().get()) / rf);
                    modulo_node->replace_with(right);
                }
            }
            else
            {
                if(right->get_type() == Node::node_t::NODE_INT64)
                {
                    // Source:
                    //   a / b;
                    // Destination:
                    //   (a / b);    // computed as it was immediate numbers
                    //
                    Int64::int64_type ri(right->get_int64().get());
                    if(ri == 0)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, modulo_node->get_position());
                        msg << "modulo by zero is illegal";
                        return;
                    }
                    left->set_float64(left->get_float64().get() / ri);
                    modulo_node->replace_with(left);
                }
                else
                {
                    // Source:
                    //   a / b;
                    // Destination:
                    //   (a / b);    // computed as it was immediate numbers
                    //
                    Float64::float64_type rf(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    if(rf == 0)
                    {
                        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, modulo_node->get_position());
                        msg << "modulo by zero is illegal";
                        return;
                    }
#pragma GCC diagnostic pop
                    right->set_float64(left->get_float64().get() / right->get_float64().get());
                    modulo_node->replace_with(right);
                }
            }
        }
        else
        {
            if(left->get_type() == Node::node_t::NODE_INT64)
            {
                Int64::int64_type i(left->get_int64().get());
                if(i == 0)
                {
                    // Note that if b is zero too then we should get an
                    // error instead we'll get zero...
                    //
                    // TODO: if right is not a number then it should be NaN
                    if(right->has_side_effects())
                    {
                        // Source:
                        //   0 / b;
                        // Destination:
                        //   (b, 0);    // because b has side effects...
                        //
                        Node::pointer_t list(modulo_node->create_replacement(Node::node_t::NODE_LIST));
                        list->append_child(right);
                        list->append_child(left);
                        modulo_node->replace_with(list);
                    }
                    else
                    {
                        // Source:
                        //   0 / b;
                        // Destination:
                        //   0;
                        //
                        modulo_node->replace_with(left);
                    }
                }
            }
            else
            {
                Float64::float64_type f(left->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                if(f == 0.0)
                {
                    // Note that if b is zero too then we should get an
                    // error instead we'll get zero...
                    //
                    // TODO: if right is not a number then it should be NaN
                    if(right->has_side_effects())
                    {
                        // Source:
                        //   0.0 * b;
                        // Destination:
                        //   (b, 0.0);    // because b has side effects...
                        //
                        Node::pointer_t list(modulo_node->create_replacement(Node::node_t::NODE_LIST));
                        list->append_child(right);
                        list->append_child(left);
                        modulo_node->replace_with(list);
                    }
                    else
                    {
                        // Source:
                        //   0.0 * b;
                        // Destination:
                        //   0.0;
                        //
                        modulo_node->replace_with(left);
                    }
                }
#pragma GCC diagnostic pop
            }
        }
    }
    else
    {
        if(right->get_type() == Node::node_t::NODE_INT64)
        {
            Int64::int64_type i(right->get_int64().get());
            if(i == 0)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, modulo_node->get_position());
                msg << "modulo by zero is illegal";
                return;
            }
            else if(i == 1)
            {
                // Source:
                //   a / 1;
                // Destination:
                //   a;
                //
                modulo_node->replace_with(left);
            }
            else if(i == -1)
            {
                // Source:
                //   a / -1;
                // Destination:
                //   -a;
                //
                Node::pointer_t negate(modulo_node->create_replacement(Node::node_t::NODE_SUBTRACT));
                negate->append_child(left);
                modulo_node->replace_with(negate);
            }
        }
        else
        {
            Float64::float64_type f(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(f == 0.0)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_DIVIDE_BY_ZERO, modulo_node->get_position());
                msg << "modulo by zero is illegal";
                return;
            }
            else if(f == 1.0)
            {
                // Source:
                //   a % 1.0;
                // Destination:
                //   0;
                //
                modulo_node->replace_with(left);
            }
            else if(f == -1.0)
            {
                // Source:
                //   a % -1.0;
                // Destination:
                //   0;
                //
                Node::pointer_t negate(modulo_node->create_replacement(Node::node_t::NODE_SUBTRACT));
                negate->append_child(left);
                modulo_node->replace_with(negate);
            }
#pragma GCC diagnostic pop
        }
    }
}


void Optimizer::add(Node::pointer_t& add_node)
{
    // Reduce
    //    +a       -- when a is a literal
    //    a + b    -- when a and b are literals

    size_t const max(add_node->get_children_size());
    if(max == 1)
    {
        // in this case the + expects a number and even strings are
        // always converted here
        Node::pointer_t left(add_node->get_child(0));
        if(left->to_number())
        {
            add_node->replace_with(left);
        }
    }
    else if(max == 2)
    {
        Node::pointer_t left(add_node->get_child(0));
        Node::pointer_t right(add_node->get_child(1));

        // we can concatenate in this case
        if(left->is_string() || right->is_string())
        {
            if(left->to_string()
            && right->to_string())
            {
                // Source:
                //   a + b
                // Destination:
                //   (a + b)   computed
                //
                left->set_string(left->get_string() + right->get_string());
                add_node->replace_with(left);
            }
            return;
        }

        // we cannot really know whether a NaN here is a string at
        // runtime (i.e. a variable is considered a NaN.)
        if(left->is_nan() || right->is_nan()
        || !left->to_number() || !right->to_number())
        {
            // any optimization here would be "dangerous"
            return;
        }

        // if both are integers, do it simple
        if(left->is_int64() && right->is_int64())
        {
            // Source:
            //   a + b
            // Destination:
            //   (a + b)   computed
            //
            left->set_int64(left->get_int64().get() + right->get_int64().get());
            add_node->replace_with(left);
            return;
        }

        // then both are floating points
        if(!left->to_float64() || !right->to_float64())
        {
            return;
        }

        // Source:
        //   a + b
        // Destination:
        //   (a + b)   computed
        //
        left->set_float64(left->get_float64().get() + right->get_float64().get());
        add_node->replace_with(left);
    }
}


void Optimizer::subtract(Node::pointer_t& subtract_node)
{
    // Reduce:
    //    -a = (-a)    if we can compute it now
    //    a - 0 = a
    //    0 - b = -b

    size_t const max(subtract_node->get_children_size());
    if(max == 1)
    {
        Node::pointer_t left(subtract_node->get_child(0));

        if(!left->to_number())
        {
            return;
        }
        if(left->is_int64())
        {
            // Source:
            //   -a
            // Destination:
            //   (-a)     -- computed
            //
            left->set_int64(-left->get_int64().get());
        }
        else
        {
            // Source:
            //   -b
            // Destination:
            //   (-b)     -- computed
            //
            left->set_float64(-left->get_float64().get());
        }
        subtract_node->replace_with(left);
    }
    else if(max == 2)
    {
        Node::pointer_t left(subtract_node->get_child(0));
        Node::pointer_t right(subtract_node->get_child(1));

        // we can concatenate in this case
        if(!left->to_number() && !right->to_number())
        {
            return;
        }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if((right->is_int64() && right->get_int64().get() == 0)
        || (right->is_float64() && right->get_float64().get() == 0.0))
        {
            // Source:
            //   a - 0;
            // Destination:
            //   a
            //
            subtract_node->replace_with(left);
            return;
        }

        if((left->is_int64() && left->get_int64().get() == 0)
        || (left->is_float64() && left->get_float64().get() == 0.0))
        {
            if(right->is_int64())
            {
                // Source:
                //   0 - b     or    0.0 - b
                // Destination:
                //   -b     -- computed
                //
                right->set_int64(-right->get_int64().get());
            }
            else if(right->is_float64())
            {
                // Source:
                //   0 - b     or    0.0 - b
                // Destination:
                //   -b     -- computed
                //
                right->set_float64(-right->get_float64().get());
            }
            else
            {
                // This is a quite special case since the subtract is
                // also used to negate... so all we need to do in this
                // case is remove the first child
                left->to_unknown();
                return;
            }
            subtract_node->replace_with(right);
        }
#pragma GCC diagnostic pop
    }
}



void Optimizer::shift_left(Node::pointer_t& shift_left_node)
{
    // Reduce
    //    a << b       -- when a and b are literals
    //    a << 0       -- this CANNOT be simplified because it is like a = static_cast<int32_t>(a) instead...
    //    a << 1       -- we could replace with a+a or a*2 but it's not really the same...

    if(shift_left_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(shift_left_node->get_child(0));
    Node::pointer_t right(shift_left_node->get_child(1));

    if(left->to_number() && right->to_number())
    {
        left->to_int64();
        right->to_int64();

        // Source:
        //   a << b
        // Destination:
        //   ToInt32(a << (b & 0x1F))   -- computed
        //
        left->set_int64(static_cast<int32_t>(left->get_int64().get() << (right->get_int64().get() & 0x1F)));
        shift_left_node->replace_with(left);
    }
    else if(left->is_float64() && left->get_float64().is_NaN())
    {
        // Source:
        //   NaN << b
        // Destination:
        //   0
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        left->to_int64();
        left->set_int64(0);
        shift_left_node->replace_with(left);
        return;
    }
    else if(right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   a << NaN
        // Destination:
        //   a
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        shift_left_node->replace_with(left);
        return;
    }
    //else if((right->is_int64() && right->get_int64().get() == 0)
    //     || (right->is_float64() && right->get_float64().get() == 0.0))
    //{
    //    // Source:
    //    //   a << 0;    or    a << 0.0;
    //    // Destination:
    //    //   a;
    //    //
    //    // NOTE: This would mean 'a' would NOT be modified when in reality
    //    //       it should be because the final result is computed in an
    //    //       int32_t or something like a = (a & 0x0FFFFFFFF) + repeat bit 31:
    //    //
    //    shift_left_node->replace_with(left);
    //    return;
    //}
}


void Optimizer::shift_right(Node::pointer_t& shift_right_node)
{
    // Reduce
    //    a >> b       -- when a and b are literals
    //    a >> 0       -- this CANNOT be simplified because it is like a = static_cast<int32_t>(a) instead...
    //    a >> 1       -- we could replace with a/2 but it's not really the same in JavaScript...

    if(shift_right_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(shift_right_node->get_child(0));
    Node::pointer_t right(shift_right_node->get_child(1));

    if(left->to_number() && right->to_number())
    {
        left->to_int64();
        right->to_int64();

        // Source:
        //   a >> b
        // Destination:
        //   ToInt32(a >> (b & 0x1F))   -- computed
        //
        left->set_int64(static_cast<int32_t>(left->get_int64().get()) >> (right->get_int64().get() & 0x1F));
        shift_right_node->replace_with(left);
    }
    else if(left->is_float64() && left->get_float64().is_NaN())
    {
        // Source:
        //   NaN >> b
        // Destination:
        //   0
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        left->to_int64();
        left->set_int64(0);
        shift_right_node->replace_with(left);
        return;
    }
    else if(right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   a >> NaN
        // Destination:
        //   a
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        shift_right_node->replace_with(left);
        return;
    }
    //else if((right->is_int64() && right->get_int64().get() == 0)
    //     || (right->is_float64() && right->get_float64().get() == 0.0))
    //{
    //    // Source:
    //    //   a >> 0;    or    a >> 0.0;
    //    // Destination:
    //    //   a;
    //    //
    //    // NOTE: This would mean 'a' would NOT be modified when in reality
    //    //       it should be because the final result is computed in an
    //    //       int32_t or something like a = (a & 0x0FFFFFFFF) + repeat bit 31:
    //    //
    //    shift_right_node->replace_with(left);
    //    return;
    //}
}



void Optimizer::shift_right_unsigned(Node::pointer_t& shift_right_unsigned_node)
{
    // Reduce
    //    a >>> b       -- when a and b are literals
    //    a >>> 0       -- this CANNOT be simplified because it is like a = static_cast<int32_t>(a) instead...
    //    a >>> 1       -- we could replace with a/2 but it's not really the same in JavaScript...

    if(shift_right_unsigned_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(shift_right_unsigned_node->get_child(0));
    Node::pointer_t right(shift_right_unsigned_node->get_child(1));

    if(left->to_number() && right->to_number())
    {
        left->to_int64();
        right->to_int64();

        // Source:
        //   a >>> b
        // Destination:
        //   ToUInt32(a >>> (b & 0x1F))   -- computed
        //
        left->set_int64(static_cast<uint32_t>(left->get_int64().get()) >> (right->get_int64().get() & 0x1F));
        shift_right_unsigned_node->replace_with(left);
    }
    else if(left->is_float64() && left->get_float64().is_NaN())
    {
        // Source:
        //   NaN >>> b
        // Destination:
        //   0
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        left->to_int64();
        left->set_int64(0);
        shift_right_unsigned_node->replace_with(left);
        return;
    }
    else if(right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   a >>> NaN
        // Destination:
        //   a
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        shift_right_unsigned_node->replace_with(left);
        return;
    }
    //else if((right->is_int64() && right->get_int64().get() == 0)
    //     || (right->is_float64() && right->get_float64().get() == 0.0))
    //{
    //    // Source:
    //    //   a >>> 0;    or    a >>> 0.0;
    //    // Destination:
    //    //   a;
    //    //
    //    // NOTE: This would mean 'a' would NOT be modified when in reality
    //    //       it should be because the final result is computed in an
    //    //       int32_t or something like a = (a & 0x0FFFFFFFF)
    //    //
    //    shift_right_unsigned_node->replace_with(left);
    //    return;
    //}
}


void Optimizer::rotate_left(Node::pointer_t& rotate_left_node)
{
    // Reduce
    //    a <! b       -- when a and b are literals
    //    a <! 0       -- this CANNOT be simplified because it is like a = static_cast<uint32_t>(a) instead...

    if(rotate_left_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(rotate_left_node->get_child(0));
    Node::pointer_t right(rotate_left_node->get_child(1));

    if(left->to_number() && right->to_number())
    {
        left->to_int64();
        right->to_int64();

        // Source:
        //   a <! b
        // Destination:
        //   ToUInt32(a <! (b & 0x1F))   -- computed
        //
        int32_t const shift(right->get_int64().get() & 0x1F);
        if(shift != 0)
        {
            left->set_int64(static_cast<uint32_t>(left->get_int64().get()) << shift
                          | static_cast<uint32_t>(left->get_int64().get()) >> (32 - shift));
        }
        rotate_left_node->replace_with(left);
    }
    else if(left->is_float64() && left->get_float64().is_NaN())
    {
        // Source:
        //   NaN <! b
        // Destination:
        //   0
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        left->to_int64();
        left->set_int64(0);
        rotate_left_node->replace_with(left);
        return;
    }
    else if(right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   a <! NaN
        // Destination:
        //   a
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        rotate_left_node->replace_with(left);
        return;
    }
    //else if((right->is_int64() && right->get_int64().get() == 0)
    //     || (right->is_float64() && right->get_float64().get() == 0.0))
    //{
    //    // Source:
    //    //   a <! 0;    or    a <! 0.0;
    //    // Destination:
    //    //   a;
    //    //
    //    // NOTE: This would mean 'a' would NOT be modified when in reality
    //    //       it should be because the final result is computed in an
    //    //       int32_t or something like a = (a & 0x0FFFFFFFF)
    //    //
    //    //       (Obviously, since the <! and >! operators are our
    //    //       additions we could change the rules, but it makes
    //    //       more sense to follow the rules used to compute
    //    //       other bitwise operations.)
    //    //
    //    rotate_left_node->replace_with(left);
    //    return;
    //}
}



void Optimizer::rotate_right(Node::pointer_t& rotate_right_node)
{
    // Reduce
    //    a >! b       -- when a and b are literals
    //    a >! 0       -- this CANNOT be simplified because it is like a = static_cast<uint32_t>(a) instead...

    if(rotate_right_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(rotate_right_node->get_child(0));
    Node::pointer_t right(rotate_right_node->get_child(1));

    if(left->to_number() && right->to_number())
    {
        left->to_int64();
        right->to_int64();

        // Source:
        //   a <! b
        // Destination:
        //   ToUInt32(a <! (b & 0x1F))   -- computed
        //
        int32_t const shift(right->get_int64().get() & 0x1F);
        if(shift != 0)
        {
            left->set_int64(static_cast<uint32_t>(left->get_int64().get()) >> shift
                          | static_cast<uint32_t>(left->get_int64().get()) << (32 - shift));
        }
        rotate_right_node->replace_with(left);
    }
    else if(left->is_float64() && left->get_float64().is_NaN())
    {
        // Source:
        //   NaN >! b
        // Destination:
        //   0
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        left->to_int64();
        left->set_int64(0);
        rotate_right_node->replace_with(left);
        return;
    }
    else if(right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   a >! NaN
        // Destination:
        //   a
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        rotate_right_node->replace_with(left);
        return;
    }
    //else if((right->is_int64() && right->get_int64().get() == 0)
    //     || (right->is_float64() && right->get_float64().get() == 0.0))
    //{
    //    // Source:
    //    //   a >! 0;    or    a >! 0.0;
    //    // Destination:
    //    //   a;
    //    //
    //    // NOTE: This would mean 'a' would NOT be modified when in reality
    //    //       it should be because the final result is computed in an
    //    //       int32_t or something like a = (a & 0x0FFFFFFFF)
    //    //
    //    //       (Obviously, since the <! and >! operators are our
    //    //       additions we could change the rules, but it makes
    //    //       more sense to follow the rules used to compute
    //    //       other bitwise operations.)
    //    //
    //    rotate_right_node->replace_with(left);
    //    return;
    //}
}



// returns (please use the COMPARE_... enumerations):
//     0 - equal
//    -1 - left < right
//     1 - left > right
//     2 - unordered (left or right is a NaN)
//    -2 - error (cannot compare)
compare_t Optimizer::compare(Node::pointer_t& relational)
{
    if(relational->get_children_size() != 2)
    {
        return compare_t::COMPARE_ERROR;
    }

    Node::pointer_t left(relational->get_child(0));
    Node::pointer_t right(relational->get_child(1));

    // Contrary to some other operators, if one of left or right
    // is a number, the string gets converted!
    if(left->get_type() == Node::node_t::NODE_STRING
    && right->get_type() == Node::node_t::NODE_STRING)
    {
        // the std::string::compare() function returns 0 if
        // equal, however, negative or postive when no equal
        // so we have to normalize the return value
        int r(left->get_string().compare(right->get_string()));
        return r == 0 ? compare_t::COMPARE_EQUAL
             : (r < 0 ? compare_t::COMPARE_LESS
                      : compare_t::COMPARE_GREATER);
    }

    // if both are numbers, then convert and test
    // but avoid the conversion if either is not a number
    if(left->is_nan() || right->is_nan()
    || !left->to_number() || !right->to_number())
    {
        return compare_t::COMPARE_ERROR;
    }

    // if both are integers, compare as integers
    // (this is NOT 100% JavaScript compatible since we do have more
    // finite precision)
    if(left->is_int64() && right->is_int64())
    {
        return left->get_int64().compare(right->get_int64());
    }

    left->to_float64();
    right->to_float64();
    return left->get_float64().compare(right->get_float64());
}



void Optimizer::less(Node::pointer_t& less_node)
{
    compare_t r(compare(less_node));
    if(compare_utils::is_ordered(r))
    {
        Node::pointer_t result(less_node->create_replacement(r == compare_t::COMPARE_LESS ? Node::node_t::NODE_TRUE : Node::node_t::NODE_FALSE));
        less_node->replace_with(result);
    }
}


void Optimizer::less_equal(Node::pointer_t& less_equal_node)
{
    compare_t r(compare(less_equal_node));
    if(compare_utils::is_ordered(r))
    {
        Node::pointer_t result(less_equal_node->create_replacement(r != compare_t::COMPARE_GREATER ? Node::node_t::NODE_TRUE : Node::node_t::NODE_FALSE));
        less_equal_node->replace_with(result);
    }
}


void Optimizer::greater(Node::pointer_t& greater_node)
{
    compare_t r(compare(greater_node));
    if(compare_utils::is_ordered(r))
    {
        Node::pointer_t result(greater_node->create_replacement(r == compare_t::COMPARE_GREATER ? Node::node_t::NODE_TRUE : Node::node_t::NODE_FALSE));
        greater_node->replace_with(result);
    }
}


void Optimizer::greater_equal(Node::pointer_t& greater_equal_node)
{
    compare_t r(compare(greater_equal_node));
    if(compare_utils::is_ordered(r))
    {
        Node::pointer_t result(greater_equal_node->create_replacement(r != compare_t::COMPARE_LESS ? Node::node_t::NODE_TRUE : Node::node_t::NODE_FALSE));
        greater_equal_node->replace_with(result);
    }
}


void Optimizer::equality(Node::pointer_t& equality_node, bool const strict, bool const inverse)
{
    if(equality_node->get_children_size() != 2)
    {
        return;
    }

    // result is expected to always be set so we do no initialized it
    // by default; but we use a contolled variable to make sure that
    // if we use it without first setting it, we get a throw
    controlled_vars::rbool_t result;

    Node::pointer_t left(equality_node->get_child(0));
    Node::pointer_t right(equality_node->get_child(1));

    if(strict)          // ===
    {
        // in this case, do not apply any conversion
        if(left->is_boolean() && right->is_boolean())
        {
            result = left->get_type() == right->get_type();
        }
        else if(left->is_int64() && right->is_int64())
        {
            result = left->get_int64().get() == right->get_int64().get();
        }
        else if(left->is_float64() && right->is_float64())
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            result = left->get_float64().get() == right->get_float64().get();
#pragma GCC diagnostic pop
        }
        else if(left->is_int64() && right->is_float64())
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            result = left->get_int64().get() == right->get_float64().get();
#pragma GCC diagnostic pop
        }
        else if(left->is_float64() && right->is_int64())
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            result = left->get_float64().get() == right->get_int64().get();
#pragma GCC diagnostic pop
        }
        else if(left->is_string() && right->is_string())
        {
            result = left->get_string() == right->get_string();
        }
        else if((left->is_undefined() && right->is_undefined())
             || (left->is_null() && right->is_null()))
        {
            result = true;
        }
        else if(left->is_identifier() && right->is_identifier())
        {
            // special case of  a === a
            if(left->get_string() != right->get_string())
            {
                // If not the same variable, we cannot know what the result
                // is at compile time.
                return;
            }
            result = true;
        }
        else
        {
            // compare a === b where a and b are not of a type that
            // can be compared between each others in strict mode
            // (those that can be compared were just before)
            switch(left->get_type())
            {
            case Node::node_t::NODE_INT64:
            case Node::node_t::NODE_FLOAT64:
            case Node::node_t::NODE_STRING:
            case Node::node_t::NODE_NULL:
            case Node::node_t::NODE_UNDEFINED:
            case Node::node_t::NODE_TRUE:
            case Node::node_t::NODE_FALSE:
                break;

            default:
                // undefined at compile time
                return;

            }
            switch(right->get_type())
            {
            case Node::node_t::NODE_INT64:
            case Node::node_t::NODE_FLOAT64:
            case Node::node_t::NODE_STRING:
            case Node::node_t::NODE_NULL:
            case Node::node_t::NODE_UNDEFINED:
            case Node::node_t::NODE_TRUE:
            case Node::node_t::NODE_FALSE:
                break;

            default:
                // undefined at compile time
                return;

            }
            // any one of these mixes is always false in strict mode
            result = false;
        }
    }
    else            // ==
    {
        Node::node_t rt(right->get_type());
        switch(left->get_type())
        {
        case Node::node_t::NODE_UNDEFINED:
        case Node::node_t::NODE_NULL:
            switch(rt)
            {
            case Node::node_t::NODE_UNDEFINED:
            case Node::node_t::NODE_NULL:
                result = true;
                break;

            case Node::node_t::NODE_INT64:
            case Node::node_t::NODE_FLOAT64:
            case Node::node_t::NODE_STRING:
            case Node::node_t::NODE_TRUE:
            case Node::node_t::NODE_FALSE:
                result = false;
                break;

            default:
                // undefined at compile time
                return;

            }
            break;

        case Node::node_t::NODE_TRUE:
            switch(rt)
            {
            case Node::node_t::NODE_TRUE:
                result = true;
                break;

            case Node::node_t::NODE_FALSE:
            case Node::node_t::NODE_NULL:
            case Node::node_t::NODE_UNDEFINED:
                result = false;
                break;

            //case Node::node_t::NODE_STRING:
            //case Node::node_t::NODE_INT64:
            //case Node::node_t::NODE_FLOAT64:
            default:
                if(!right->to_number())
                {
                    // undefined at compile time
                    return;
                }
                if(right->is_int64())
                {
                    result = right->get_int64().get() == 1;
                }
                else
                {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    result = right->get_float64().get() == 1.0;
#pragma GCC diagnostic pop
                }
                break;

            }
            break;

        case Node::node_t::NODE_FALSE:
            switch(rt)
            {
            case Node::node_t::NODE_TRUE:
            case Node::node_t::NODE_NULL:
            case Node::node_t::NODE_UNDEFINED:
                result = false;
                break;

            case Node::node_t::NODE_FALSE:
                result = true;
                break;

            //case Node::node_t::NODE_STRING:
            //case Node::node_t::NODE_INT64:
            //case Node::node_t::NODE_FLOAT64:
            default:
                if(!right->to_number())
                {
                    // undefined at compile time
                    return;
                }
                if(right->is_int64())
                {
                    result = right->get_int64().get() == 0;
                }
                else
                {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    result = right->get_float64().get() == 0.0;
#pragma GCC diagnostic pop
                }
                break;

            }
            break;

        case Node::node_t::NODE_INT64:
            switch(rt)
            {
            case Node::node_t::NODE_INT64:
                result = left->get_int64().get() == right->get_int64().get();
                break;

            case Node::node_t::NODE_FLOAT64:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                result = left->get_int64().get() == right->get_float64().get();
#pragma GCC diagnostic pop
                break;

            case Node::node_t::NODE_TRUE:
                result = left->get_int64().get() == 1;
                break;

            case Node::node_t::NODE_FALSE:
                result = left->get_int64().get() == 0;
                break;

            case Node::node_t::NODE_STRING:
                if(!right->to_number())
                {
                    throw exception_internal_error("somehow a string could not be converted to a number");
                }
                if(right->is_int64()) // rt is the old type
                {
                    result = left->get_int64().get() == right->get_int64().get();
                }
                else
                {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    result = left->get_int64().get() == right->get_float64().get();
#pragma GCC diagnostic pop
                }
                break;

            case Node::node_t::NODE_NULL:
            case Node::node_t::NODE_UNDEFINED:
                result = false;
                break;

            default:
                return;

            }
            break;

        case Node::node_t::NODE_FLOAT64:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            switch(rt)
            {
            case Node::node_t::NODE_FLOAT64:
                result = left->get_float64().get() == right->get_float64().get();
                break;

            case Node::node_t::NODE_INT64:
                result = left->get_float64().get() == right->get_int64().get();
                break;

            case Node::node_t::NODE_TRUE:
                result = left->get_float64().get() == 1.0;
                break;

            case Node::node_t::NODE_FALSE:
                result = left->get_float64().get() == 0.0;
                break;

            case Node::node_t::NODE_STRING:
                if(!right->to_number())
                {
                    throw exception_internal_error("somehow a string could not be converted to a number");
                }
                if(right->is_int64()) // rt is the old type
                {
                    result = left->get_float64().get() == right->get_int64().get();
                }
                else
                {
                    result = left->get_float64().get() == right->get_float64().get();
                }
                break;

            case Node::node_t::NODE_NULL:
            case Node::node_t::NODE_UNDEFINED:
                result = false;
                break;

            default:
                return;

            }
#pragma GCC diagnostic pop
            break;

        case Node::node_t::NODE_STRING:
            switch(rt)
            {
            case Node::node_t::NODE_STRING:
                result = left->get_string() == right->get_string();
                break;

            case Node::node_t::NODE_NULL:
            case Node::node_t::NODE_UNDEFINED:
                result = false;
                break;

            case Node::node_t::NODE_INT64:
            case Node::node_t::NODE_TRUE:
            case Node::node_t::NODE_FALSE:
                if(!left->to_number())
                {
                    throw exception_internal_error("somehow a string could not be converted to a number");
                }
                if(!right->to_number())
                {
                    throw exception_internal_error("somehow a number or a boolean value could not be converted to a number?!");
                }
                if(left->is_int64())
                {
                    result = left->get_int64().get() == right->get_int64().get();
                }
                else
                {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                    result = left->get_float64().get() == right->get_int64().get();
#pragma GCC diagnostic pop
                }
                break;

            case Node::node_t::NODE_FLOAT64:
                if(!left->to_number())
                {
                    throw exception_internal_error("somehow a string could not be converted to a number");
                }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                if(left->is_int64())
                {
                    result = left->get_int64().get() == right->get_float64().get();
                }
                else
                {
                    result = left->get_float64().get() == right->get_float64().get();
                }
#pragma GCC diagnostic pop
                break;

            default:
                return;

            }
            break;

        case Node::node_t::NODE_IDENTIFIER:
        case Node::node_t::NODE_VIDENTIFIER:
            if(!right->is_identifier()
            || left->get_string() != right->get_string())
            {
                return;
            }
            // special case of  a == a
            result = true;
            break;

        default:
            // undefined at compile time
            return;

        }
    }

    // we use !inverse to make 100% sure that it is a valid Boolean value
    // (it can be tainted since it is given to use by the caller)
    Node::pointer_t boolean(equality_node->create_replacement(result ^ !inverse ? Node::node_t::NODE_FALSE : Node::node_t::NODE_TRUE));
    equality_node->replace_with(boolean);
}



void Optimizer::bitwise_and(Node::pointer_t& bitwise_and_node)
{
    // Reduce
    //   a & 0 = 0
    //   0 & b = 0
    //   a & -1 = a  -- NOT TRUE, because they implicitely convert floats to
    //                  integers of 32 bits when using &
    //   a & b       -- when a and b are literals
    //   a & b & c & d ...       -- when several of the parameters are literals TODO

    if(bitwise_and_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(bitwise_and_node->get_child(0));
    Node::pointer_t right(bitwise_and_node->get_child(1));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    if(left->to_number() && right->to_number())
    {
        left->to_int64();
        right->to_int64();

        // Source:
        //   a & b
        // Destination:
        //   ToInt32(a <! (b & 0x1F))   -- computed
        //
        left->set_int64(static_cast<int32_t>(left->get_int64().get())
                      & static_cast<int32_t>(right->get_int64().get()));
        bitwise_and_node->replace_with(left);
    }
    else if(left->is_float64() && left->get_float64().is_NaN()
         || right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   a & NaN     or     NaN & b
        // Destination:
        //   0
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        left->to_int64();
        left->set_int64(0);
        bitwise_and_node->replace_with(left);
        return;
    }
    else if((right->is_int64() && right->get_int64().get() == 0)
         || (right->is_float64() && right->get_float64().get() == 0.0))
    {
        // Source:
        //   a & 0;
        // Destination:
        //   0;
        //
        bitwise_and_node->replace_with(right);
        return;
    }
    else if((left->is_int64() && left->get_int64().get() == 0)
         || (left->is_float64() && left->get_float64().get() == 0.0))
    {
        // Source:
        //   0 & b;
        // Destination:
        //   0;
        //
        bitwise_and_node->replace_with(left);
        return;
    }
    //else if((right->is_int64() && right->get_int64().get() == -1)
    //     || (right->is_float64() && right->get_float64().get() == -1.0))
    //{
    //    // Source:
    //    //   a & -1;    or    -1 & b;
    //    // Destination:
    //    //   a;         or    b
    //    //
    //    // NOTE: This would mean 'a' would NOT be modified when in reality
    //    //       it should be because the final result is computed in an
    //    //       int32_t or something like a = (a & 0x0FFFFFFFF)
    //    //
    //    //       (Obviously, since the <! and >! operators are our
    //    //       additions we could change the rules, but it makes
    //    //       more sense to follow the rules used to compute
    //    //       other bitwise operations.)
    //    //
    //    bitwise_and_node->replace_with(left or right);
    //    return;
    //}
#pragma GCC diagnostic pop
}



void Optimizer::bitwise_xor(Node::pointer_t& bitwise_xor_node)
{
    // Reduce
    //   a ^ 0       -- this cannot be optimized because of the side effects
    //   a ^ -1 = ~a
    //   a ^ b       -- when a and b are literals
    //   a ^ b ^ c ^ d ...       -- when several of the parameters are literals

    if(bitwise_xor_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(bitwise_xor_node->get_child(0));
    Node::pointer_t right(bitwise_xor_node->get_child(1));

    if(left->to_number() && right->to_number())
    {
        left->to_int64();
        right->to_int64();

        // Source:
        //   a ^ b
        // Destination:
        //   ToInt32(a ^ b)   -- computed
        //
        left->set_int64(static_cast<int32_t>(left->get_int64().get())
                      ^ static_cast<int32_t>(right->get_int64().get()));
        bitwise_xor_node->replace_with(left);
    }
    else if(left->is_float64() && left->get_float64().is_NaN()
         && right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   NaN ^ NaN
        // Destination:
        //   0
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        left->to_int64();
        left->set_int64(0);
        bitwise_xor_node->replace_with(left);
        return;
    }
    else if(left->is_float64() && left->get_float64().is_NaN())
    {
        // Source:
        //   NaN ^ b
        // Destination:
        //   b
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        bitwise_xor_node->replace_with(right);
        return;
    }
    else if(right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   a ^ NaN
        // Destination:
        //   a
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        bitwise_xor_node->replace_with(left);
        return;
    }
    else if(left->is_number())
    {
        left->to_int64();
        Int64::int64_type l(left->get_int64().get());
        if((l & 0xFFFFFFFF) == 0xFFFFFFFF)
        {
            // Source:
            //   -1 ^ b;
            // Destination:
            //   ~b;
            //
            Node::pointer_t bitwise_not_node(bitwise_xor_node->create_replacement(Node::node_t::NODE_BITWISE_NOT));
            bitwise_not_node->append_child(right);
            bitwise_xor_node->replace_with(bitwise_not_node);
        }
        return;
    }
    else if(right->is_number())
    {
        right->to_int64();
        Int64::int64_type r(right->get_int64().get());
        if((r & 0xFFFFFFFF) == 0xFFFFFFFF)
        {
            // Source:
            //   a ^ -1;
            // Destination:
            //   ~a;
            //
            Node::pointer_t bitwise_not_node(bitwise_xor_node->create_replacement(Node::node_t::NODE_BITWISE_NOT));
            bitwise_not_node->append_child(left);
            bitwise_xor_node->replace_with(bitwise_not_node);
        }
        return;
    }
}



void Optimizer::bitwise_or(Node::pointer_t& bitwise_or_node)
{
    // Reduce
    //   a | 0       -- this cannot be optimized because of the side effects
    //   a | -1 = -1
    //   a | b       -- when a and b are literals
    //   a | b | c | d ...       -- when several of the parameters are literals

    if(bitwise_or_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(bitwise_or_node->get_child(0));
    Node::pointer_t right(bitwise_or_node->get_child(1));

    if(left->to_number() && right->to_number())
    {
        left->to_int64();
        right->to_int64();

        // Source:
        //   a | b
        // Destination:
        //   ToInt32(a | b)   -- computed
        //
        left->set_int64(static_cast<int32_t>(left->get_int64().get())
                      | static_cast<int32_t>(right->get_int64().get()));
        bitwise_or_node->replace_with(left);
    }
    else if(left->is_float64() && left->get_float64().is_NaN()
         && right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   NaN | NaN
        // Destination:
        //   0
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        left->to_int64();
        left->set_int64(0);
        bitwise_or_node->replace_with(left);
        return;
    }
    else if(left->is_float64() && left->get_float64().is_NaN())
    {
        // Source:
        //   NaN | b
        // Destination:
        //   b
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        bitwise_or_node->replace_with(right);
        return;
    }
    else if(right->is_float64() && right->get_float64().is_NaN())
    {
        // Source:
        //   a | NaN
        // Destination:
        //   a
        //
        // This case may be a bug in Firefox, but that is what Firefox returns
        //
        bitwise_or_node->replace_with(left);
        return;
    }
    else if(left->is_number())
    {
        left->to_int64();
        Int64::int64_type l(left->get_int64().get());
        if((l & 0xFFFFFFFF) == 0xFFFFFFFF)
        {
            // Source:
            //   -1 | b;
            // Destination:
            //   -1;
            //
            left->set_int64(-1);
            bitwise_or_node->replace_with(left);
        }
        return;
    }
    else if(right->is_number())
    {
        right->to_int64();
        Int64::int64_type r(right->get_int64().get());
        if((r & 0xFFFFFFFF) == 0xFFFFFFFF)
        {
            // Source:
            //   a | -1;
            // Destination:
            //   -1;
            //
            right->set_int64(-1);
            bitwise_or_node->replace_with(right);
        }
        return;
    }
}



void Optimizer::logical_and(Node::pointer_t& logical_and_node)
{
    // WARNING: the left hand side of the && operator is converted
    //          to a boolean, but NOT the right hand side!
    //          So if the left hand side is true, replace the &&
    //          with the right hand side as is...

    // Reduce
    //    true && b == b
    //    false && b == false
    //
    //       the following is NOT correct, if a is true, then
    //       we get true, but otherwise we get a (i.e. null,
    //       undefined, '', 0, 0.0, or false)
    //    a && true == !!a     -- we reduce this one because it is smaller
    //                            text wise and we may end up reducing the
    //                            !! to nothing
    //
    //    a && false == false  -- if a has no side effect

    if(logical_and_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(logical_and_node->get_child(0));
    Node::pointer_t right(logical_and_node->get_child(1));

    condition_double_logical_not(left);
    if(left->to_boolean())
    {
        if(left->is_true())
        {
            // Source:
            //   true && b;
            // Destination:
            //   b;
            //
            logical_and_node->replace_with(right);
        }
        else //if(left->is_false())
        {
            // Source:
            //   false && b;
            // Destination:
            //   false;
            //
            logical_and_node->replace_with(left);
        }
    }
    else if(right->is_true())
    {
        // Source:
        //   a && true;
        // Destination:
        //   !!a;
        //
        Node::pointer_t logical_not_one(logical_and_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
        Node::pointer_t logical_not_two(logical_and_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
        logical_not_one->append_child(logical_not_two);
        logical_not_two->append_child(left);
        logical_and_node->replace_with(logical_not_one);
    }
    else if(right->is_false())
    {
        if(!left->has_side_effects())
        {
            // Source:
            //   a && false;
            // Destination:
            //   false;        -- if a has no side effects
            //
            logical_and_node->replace_with(right);
        }
    }
}



void Optimizer::logical_xor(Node::pointer_t& logical_xor_node)
{
    // WARNING: the logical XOR (^^) is another of our additions;
    //          and since we always need both sides to compute the
    //          result (contrary to the && and ||) we presume that
    //          both sides must be Boolean values

    // Reduce
    //    true ^^ b == !b
    //    false ^^ b == !!b

    if(logical_xor_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(logical_xor_node->get_child(0));
    Node::pointer_t right(logical_xor_node->get_child(1));

    condition_double_logical_not(left);
    condition_double_logical_not(right);

    if(left->to_boolean() || right->to_boolean())
    {
        if(left->is_true())
        {
            if(right->is_true())
            {
                // Source:
                //   true ^^ true
                // Destination:
                //   false
                //
                left->set_boolean(false);
                logical_xor_node->replace_with(left);
            }
            else if(right->is_false())
            {
                // Source:
                //   true ^^ false
                // Destination:
                //   true
                //
                logical_xor_node->replace_with(left);
            }
            else
            {
                // Source:
                //   true ^^ b;
                // Destination:
                //   !b;
                //
                Node::pointer_t logical_not_node(logical_xor_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
                logical_not_node->append_child(right);
                logical_xor_node->replace_with(logical_not_node);
            }
        }
        else if(left->is_false())
        {
            if(right->is_true())
            {
                // Source:
                //   false ^^ true
                // Destination:
                //   true
                //
                logical_xor_node->replace_with(right);
            }
            else if(right->is_false())
            {
                // Source:
                //   false ^^ false
                // Destination:
                //   false
                //
                logical_xor_node->replace_with(left);
            }
            else
            {
                // Source:
                //   false ^^ b;
                // Destination:
                //   !!b;
                //
                Node::pointer_t logical_not_one(logical_xor_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
                Node::pointer_t logical_not_two(logical_xor_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
                logical_not_one->append_child(logical_not_two);
                logical_not_two->append_child(right);
                logical_xor_node->replace_with(logical_not_one);
            }
        }
        else if(right->is_true())
        {
            // Source:
            //   a ^^ true;
            // Destination:
            //   !a;
            //
            Node::pointer_t logical_not_node(logical_xor_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
            logical_not_node->append_child(left);
            logical_xor_node->replace_with(logical_not_node);
        }
        else if(right->is_false())
        {
            // Source:
            //   a ^^ false;
            // Destination:
            //   !!a;
            //
            Node::pointer_t logical_not_one(logical_xor_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
            Node::pointer_t logical_not_two(logical_xor_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
            logical_not_one->append_child(logical_not_two);
            logical_not_two->append_child(left);
            logical_xor_node->replace_with(logical_not_one);
        }
    }
}



void Optimizer::logical_or(Node::pointer_t& logical_or_node)
{
    // WARNING: the left hand side of the || operator is converted
    //          to a boolean, but NOT the right hand side!
    //          So if the left hand side is false, replace the ||
    //          with the right hand side as is...

    // Reduce
    //    true || b == true
    //    false || b == b
    //    a || true == true   or (a, true)
    //                         -- we reduce if a has no side effects
    //                         -- this is wrong because if 'a' represents
    //                            true, then 'a' is returned
    //    a || false == a     or !!a
    //                         -- the || does not force Boolean conversions

    if(logical_or_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t left(logical_or_node->get_child(0));
    Node::pointer_t right(logical_or_node->get_child(1));

    condition_double_logical_not(left);
    if(left->to_boolean())
    {
        if(left->is_true())
        {
            // Source:
            //   true || b;
            // Destination:
            //   true;
            //
            logical_or_node->replace_with(left);
        }
        else //if(left->is_false())
        {
            // Source:
            //   false || b;
            // Destination:
            //   b;
            //
            logical_or_node->replace_with(right);
        }
    }
    else if(right->is_true())
    {
        if(!left->has_side_effects())
        {
            // Source:
            //   a || true
            // Destination:
            //   true         -- assuming 'a' has no side effects
            //
            logical_or_node->replace_with(right);
        }
    }
    else if(right->is_false())
    {
        // Source:
        //   a || false;
        // Destination:
        //   !!a;
        //
        // We do this optimization because there are several places where
        // we will remove the double logical not (i.e. 'if(a || false)'
        // will result in 'if(a)'.)
        //
        Node::pointer_t logical_not_one(logical_or_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
        Node::pointer_t logical_not_two(logical_or_node->create_replacement(Node::node_t::NODE_LOGICAL_NOT));
        logical_not_one->append_child(logical_not_two);
        logical_not_two->append_child(left);
        logical_or_node->replace_with(logical_not_one);
    }
}



void Optimizer::minimum(Node::pointer_t& minimum_node)
{
    compare_t r(compare(minimum_node));
    if(compare_utils::is_ordered(r))
    {
        if(r != compare_t::COMPARE_GREATER)
        {
            minimum_node->replace_with(minimum_node->get_child(0));
        }
        else
        {
            minimum_node->replace_with(minimum_node->get_child(1));
        }
    }
}



void Optimizer::maximum(Node::pointer_t& maximum_node)
{
    compare_t r(compare(maximum_node));
    if(compare_utils::is_ordered(r))
    {
        if(r != compare_t::COMPARE_LESS)
        {
            maximum_node->replace_with(maximum_node->get_child(0));
        }
        else
        {
            maximum_node->replace_with(maximum_node->get_child(1));
        }
    }
}



void Optimizer::conditional(Node::pointer_t& conditional_node)
{
    if(conditional_node->get_children_size() != 3)
    {
        return;
    }

    Node::pointer_t condition(conditional_node->get_child(0));
    condition_double_logical_not(condition);
    if(condition->to_boolean())
    {
        if(condition->is_true())
        {
            conditional_node->replace_with(conditional_node->get_child(1));
        }
        else
        {
            conditional_node->replace_with(conditional_node->get_child(2));
        }
    }
}
#endif




}
// namespace as2js

// vim: ts=4 sw=4 et
