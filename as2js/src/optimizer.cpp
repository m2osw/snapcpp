/* optimizer.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/optimizer.h"
#include    "as2js/message.h"

#include    <math.h>


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  OPTIMIZER  ******************************************************/
/**********************************************************************/
/**********************************************************************/

Optimizer::Optimizer()
    //: f_options(nullptr) -- auto-init
    //, f_label(0) -- auto-init
{
}



void Optimizer::set_options(Options::pointer_t& options)
{
    f_options = options;
}




/**********************************************************************/
/**********************************************************************/
/***  OPTIMIZE  *******************************************************/
/**********************************************************************/
/**********************************************************************/

int Optimizer::optimize(Node::pointer_t& node)
{
    int errcnt(Message::error_count());

    run(node);

    // This may not be at the right place because the caller may be
    // looping through a list of children too...
    node->clean_tree();

    return Message::error_count() - errcnt;
}


void Optimizer::label(String& new_label)
{
    std::stringstream s;
    s << "__optimizer__" << f_label;
    ++f_label;
    new_label = s.str();
}


Optimizer::label_t Optimizer::get_last_label() const
{
    return f_label;
}


void Optimizer::set_first_label(label_t label_id)
{
    f_label = label_id;
}



void Optimizer::run(Node::pointer_t& node)
{
    // accept empty nodes, just ignore them
    if(!node
    || node->get_type() == Node::NODE_UNKNOWN)
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
    case Node::NODE_DIRECTIVE_LIST:
        directive_list(node);
        break;

    case Node::NODE_IF:
        if_directive(node);
        break;

    case Node::NODE_WHILE:
        while_directive(node);
        break;

    case Node::NODE_DO:
        do_directive(node);
        break;

    case Node::NODE_ASSIGNMENT:
        assignment(node);
        break;

    case Node::NODE_ASSIGNMENT_ADD:
    case Node::NODE_ASSIGNMENT_SUBTRACT:
        assignment_add(node);
        break;

    case Node::NODE_ASSIGNMENT_MULTIPLY:
        assignment_multiply(node);
        break;

    case Node::NODE_ASSIGNMENT_DIVIDE:
        assignment_divide(node);
        break;

    case Node::NODE_ASSIGNMENT_MODULO:
        assignment_modulo(node);
        break;

    case Node::NODE_BITWISE_NOT:
        bitwise_not(node);
        break;

    case Node::NODE_LOGICAL_NOT:
        logical_not(node);
        break;

    case Node::NODE_POWER:
        power(node);
        break;

    case Node::NODE_MULTIPLY:
        multiply(node);
        break;

    case Node::NODE_DIVIDE:
        divide(node);
        break;

    case Node::NODE_MODULO:
        modulo(node);
        break;

    case Node::NODE_ADD:
        add(node);
        break;

    case Node::NODE_SUBTRACT:
        subtract(node);
        break;

    case Node::NODE_SHIFT_LEFT:
        shift_left(node);
        break;

    case Node::NODE_SHIFT_RIGHT:
        shift_right(node);
        break;

    case Node::NODE_SHIFT_RIGHT_UNSIGNED:
        shift_right_unsigned(node);
        break;

    case Node::NODE_ROTATE_LEFT:
        rotate_left(node);
        break;

    case Node::NODE_ROTATE_RIGHT:
        rotate_right(node);
        break;

    case Node::NODE_LESS:
        less(node);
        break;

    case Node::NODE_LESS_EQUAL:
        less_equal(node);
        break;

    case Node::NODE_GREATER:
        greater(node);
        break;

    case Node::NODE_GREATER_EQUAL:
        greater_equal(node);
        break;

    case Node::NODE_EQUAL:
        equality(node, false, false);
        break;

    case Node::NODE_STRICTLY_EQUAL:
        equality(node, true, false);
        break;

    case Node::NODE_NOT_EQUAL:
        equality(node, false, true);
        break;

    case Node::NODE_STRICTLY_NOT_EQUAL:
        equality(node, true, true);
        break;

    case Node::NODE_BITWISE_AND:
        bitwise_and(node);
        break;

    case Node::NODE_BITWISE_XOR:
        bitwise_xor(node);
        break;

    case Node::NODE_BITWISE_OR:
        bitwise_or(node);
        break;

    case Node::NODE_LOGICAL_AND:
        logical_and(node);
        break;

    case Node::NODE_LOGICAL_XOR:
        logical_xor(node);
        break;

    case Node::NODE_LOGICAL_OR:
        logical_or(node);
        break;

    case Node::NODE_MAXIMUM:
        maximum(node);
        break;

    case Node::NODE_MINIMUM:
        minimum(node);
        break;

    case Node::NODE_CONDITIONAL:
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
        if(child->get_type() == Node::NODE_IDENTIFIER
        && child->get_link(Node::LINK_INSTANCE))
        {
            // Source:
            //   ??
            // Destination:
            //   ??
            child->to_unknown();
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
    if(condition->to_boolean())
    {
        if(condition->get_type() == Node::NODE_TRUE)
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
    if(condition->to_boolean())
    {
        if(condition->get_type() == Node::NODE_TRUE)
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
            // break or return statement.
            //
            // If there are none, we should err about it.
            // It is likely a bad one!
            // [except if the function returns Never]
            //
            // create a forever loop
            Node::pointer_t forever(while_node->create_replacement(Node::NODE_FOR));
            forever->append_child(while_node->create_replacement(Node::NODE_EMPTY));
            forever->append_child(while_node->create_replacement(Node::NODE_EMPTY));
            forever->append_child(while_node->create_replacement(Node::NODE_EMPTY));
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
            // TODO: this is wrong if the user has labels and
            //       uses them with a break or a continue.
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
    if(condition->to_boolean())
    {
        if(condition->get_type() == Node::NODE_TRUE)
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
            // create a forever loop
            Node::pointer_t forever(do_node->create_replacement(Node::NODE_FOR));
            forever->append_child(do_node->create_replacement(Node::NODE_EMPTY));
            forever->append_child(do_node->create_replacement(Node::NODE_EMPTY));
            forever->append_child(do_node->create_replacement(Node::NODE_EMPTY));
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
            // TODO: this is wrong if the user has labels and
            //       uses them with a break or a continue.
            //
            // in this case, we simply run the
            // directives once
            do_node->replace_with(do_node->get_child(0));
        }
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

    if(left->get_type() == Node::NODE_IDENTIFIER
    && right->get_type() == Node::NODE_IDENTIFIER
    && left->get_string() == right->get_string())
    {
        // Source:
        //   a = a;
        // Destination:
        //   a;
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
    if((right->get_type() == Node::NODE_INT64 && right->get_int64().get() == 0)
    || (right->get_type() == Node::NODE_FLOAT64 && right->get_float64().get() == 0.0))
    {
        // Source:
        //   a += 0;   or   a += 0.0;
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
    if(assignment_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t right(assignment_node->get_child(1));
    if(right->get_type() == Node::NODE_INT64)
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
    else if(right->get_type() == Node::NODE_FLOAT64)
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


void Optimizer::assignment_divide(Node::pointer_t& assignment_node)
{
    // a /= 1 -> a
    // a /= 0 -> ERROR
    if(assignment_node->get_children_size() != 2)
    {
        return;
    }

    Node::pointer_t right(assignment_node->get_child(1));
    if(right->get_type() == Node::NODE_INT64)
    {
        Int64::int64_type i(right->get_int64().get());
        if(i == 0)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, assignment_node->get_position());
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
    else if(right->get_type() == Node::NODE_FLOAT64)
    {
        Float64::float64_type f(right->get_float64().get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(f == 0.0)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, assignment_node->get_position());
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
    if(right->get_type() == Node::NODE_INT64)
    {
        if(right->get_int64().get() == 0)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, assignment_node->get_position());
            msg << "modulo by zero is illegal";
        }
    }
    else if(right->get_type() == Node::NODE_FLOAT64)
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        if(right->get_float64().get() == 0.0)
        {
            Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, assignment_node->get_position());
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
        if(child->get_type() == Node::NODE_INT64)
        {
            // Source:
            //   ~<int>
            // Destination:
            //   computed ~<int>
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
        child->set_boolean(child->get_type() != Node::NODE_TRUE);
        logical_not_node->replace_with(child);
    }
    else if(child->get_type() == Node::NODE_LOGICAL_NOT)
    {
        // IMPORTANT NOTE: We do NOT replace '!!a' with 'a' because
        //                 in reality, '!!a != a' if a is not boolean
        //                 to start with!
        //                 However, if someone was to use '!!!a',
        //                 that we can optimize to '!a'.
        if(child->get_children_size() == 1)
        {
            Node::pointer_t sub_child(child->get_child(0));
            if(sub_child->get_type() == Node::NODE_LOGICAL_NOT)
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
        //    0 ** b = 0 (we can't garantee that b <> 0, we can't do it)
        //    1 ** b = 1
        if(!left->to_number())
        {
            return;
        }
        if(left->get_type() == Node::NODE_INT64
        && left->get_int64().get() == 1)
        {
            if(right->has_side_effects())
            {
                // Source:
                //   1 ** b;
                // Destination:
                //   (b, 1);    // because b has side effects...
                //
                Node::pointer_t list(power_node->create_replacement(Node::NODE_LIST));
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
                    Node::pointer_t list(power_node->create_replacement(Node::NODE_LIST));
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
    if(right->get_type() == Node::NODE_INT64)
    {
        Node::pointer_t one(power_node->create_replacement(Node::NODE_INT64));
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
                Node::pointer_t list(power_node->create_replacement(Node::NODE_LIST));
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
            Node::pointer_t inverse(power_node->create_replacement(Node::NODE_DIVIDE));
            inverse->append_child(one);
            inverse->append_child(left);
            power_node->replace_with(inverse);
            return;
        }
    }
    else
    {
        Node::pointer_t one(power_node->create_replacement(Node::NODE_FLOAT64));
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
                Node::pointer_t list(power_node->create_replacement(Node::NODE_LIST));
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
            Node::pointer_t inverse(power_node->create_replacement(Node::NODE_DIVIDE));
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
            if(left->get_type() == Node::NODE_INT64)
            {
                if(right->get_type() == Node::NODE_INT64)
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
                if(right->get_type() == Node::NODE_INT64)
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
            if(left->get_type() == Node::NODE_INT64)
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
                        Node::pointer_t list(multiply_node->create_replacement(Node::NODE_LIST));
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
                    Node::pointer_t negate(multiply_node->create_replacement(Node::NODE_SUBTRACT));
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
                        Node::pointer_t list(multiply_node->create_replacement(Node::NODE_LIST));
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
                    Node::pointer_t negate(multiply_node->create_replacement(Node::NODE_SUBTRACT));
                    negate->append_child(right);
                    multiply_node->replace_with(negate);
                }
#pragma GCC diagnostic pop
            }
        }
    }
    else
    {
        if(right->get_type() == Node::NODE_INT64)
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
                    Node::pointer_t list(multiply_node->create_replacement(Node::NODE_LIST));
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
                Node::pointer_t negate(multiply_node->create_replacement(Node::NODE_SUBTRACT));
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
                    Node::pointer_t list(multiply_node->create_replacement(Node::NODE_LIST));
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
                Node::pointer_t negate(multiply_node->create_replacement(Node::NODE_SUBTRACT));
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
            if(left->get_type() == Node::NODE_INT64)
            {
                if(right->get_type() == Node::NODE_INT64)
                {
                    // Source:
                    //   a / b;
                    // Destination:
                    //   (a / b);    // computed as it was immediate numbers
                    //
                    Int64::int64_type ri(right->get_int64().get());
                    if(ri == 0)
                    {
                        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
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
                        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
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
                if(right->get_type() == Node::NODE_INT64)
                {
                    // Source:
                    //   a / b;
                    // Destination:
                    //   (a / b);    // computed as it was immediate numbers
                    //
                    Int64::int64_type ri(right->get_int64().get());
                    if(ri == 0)
                    {
                        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
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
                        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
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
            if(left->get_type() == Node::NODE_INT64)
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
                        Node::pointer_t list(divide_node->create_replacement(Node::NODE_LIST));
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
                        Node::pointer_t list(divide_node->create_replacement(Node::NODE_LIST));
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
        if(right->get_type() == Node::NODE_INT64)
        {
            Int64::int64_type i(right->get_int64().get());
            if(i == 0)
            {
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
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
                Node::pointer_t negate(divide_node->create_replacement(Node::NODE_SUBTRACT));
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
                Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_DIVIDE_BY_ZERO, divide_node->get_position());
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
                Node::pointer_t negate(divide_node->create_replacement(Node::NODE_SUBTRACT));
                negate->append_child(left);
                divide_node->replace_with(negate);
            }
#pragma GCC diagnostic pop
        }
    }
}


void Optimizer::modulo(Node::pointer_t& modulo_node)
{
    int64_t        itotal, idiv;
    double        ftotal, fdiv;
    node_t        type;
    int        idx, max;
    bool        div0, constant;

    // Error
    //    a % 0

    type = NODE_UNKNOWN;
    itotal = 0;
    ftotal = 0.0;
    constant = true;
    max = modulo.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        div0 = false;
        NodePtr& child = modulo.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                idiv = data.f_int.Get();
                if(type == NODE_UNKNOWN) {
                    type = NODE_INT64;
                    itotal = idiv;
                }
                else {
                    div0 = idiv == 0;
                    if(!div0) {
                        if(type == NODE_FLOAT64) {
                            ftotal = fmod(ftotal, idiv);
                        }
                        else {
                            itotal %= idiv;
                        }
                    }
                }
            }
            else {
                fdiv = data.f_float.Get();
                if(type == NODE_UNKNOWN) {
                    type = NODE_FLOAT64;
                    ftotal = fdiv;
                }
                else {
                    div0 = fdiv != 0;
                    if(!div0) {
                        if(type == NODE_INT64) {
                            type = NODE_FLOAT64;
                            ftotal = fmod(itotal, fdiv);
                        }
                        else {
                            ftotal = fmod(ftotal, fdiv);
                        }
                    }
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            constant = false;
        }
        if(div0) {
            f_error_stream->ErrMsg(AS_ERR_DIVIDE_BY_ZERO, modulo, "dividing by zero is illegal");
        }
    }

    if(!constant) {
        return;
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = modulo.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        modulo.DeleteChild(max);
    }
}


void Optimizer::Add(NodePtr& add)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max;
    bool        constant;

    // Reduce:
    //    a + 0 = a
    //    0 + a = a

    constant = true;
    type = NODE_INT64;
    itotal = 0;
    ftotal = 0.0;
    max = add.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = add.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(data.f_int.Get() == 0) {
                    add.DeleteChild(idx);
                    --idx;
                    --max;
                }
                else if(type == NODE_FLOAT64) {
                    ftotal += data.f_int.Get();
                }
                else {
                    itotal += data.f_int.Get();
                }
            }
            else {
                // special case where we still want
                // to force the value to a floating
                // point value! (1 + 0.0 -> 1.0)
                if(type == NODE_INT64) {
                    type = NODE_FLOAT64;
                    ftotal = itotal + data.f_float.Get();
                }
                else {
                    ftotal += data.f_float.Get();
                }
                if(data.f_float.Get() == 0.0) {
                    add.DeleteChild(idx);
                    --idx;
                    --max;
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            constant = false;
        }
    }

    // max is 1 when the user used positive or added zero
    if(max == 1) {
        NodePtr expr = add.GetChild(0);
        add.DeleteChild(0);
        add.ReplaceWith(expr);
        return;
    }

    if(!constant) {
        return;
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = add.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        add.DeleteChild(max);
    }
}


void Optimizer::Subtract(NodePtr& subtract)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max, start_max;
    bool        constant;

    // Reduce:
    //    a - 0 = a

    type = NODE_UNKNOWN;
    itotal = 0;
    ftotal = 0.0;
    constant = true;
    start_max = max = subtract.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr& child = subtract.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(idx != 0 && data.f_int.Get() == 0) {
                    subtract.DeleteChild(idx);
                    --idx;
                    --max;
                }
                else if(type == NODE_UNKNOWN) {
                    type = NODE_INT64;
                    itotal = data.f_int.Get();
                }
                else if(type == NODE_FLOAT64) {
                    ftotal -= data.f_int.Get();
                }
                else {
                    itotal -= data.f_int.Get();
                }
            }
            else {
                if(idx != 0 && data.f_int.Get() == 0) {
                    subtract.DeleteChild(idx);
                    --idx;
                    --max;
                }
                else if(type == NODE_UNKNOWN) {
                    type = NODE_FLOAT64;
                    ftotal = data.f_float.Get();
                }
                else if(type == NODE_INT64) {
                    type = NODE_FLOAT64;
                    ftotal = itotal - data.f_float.Get();
                }
                else {
                    ftotal -= data.f_float.Get();
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            constant = false;
        }
    }

// cancellation of the operation? (i.e. a - 0)
    if(start_max > 1 && max == 1) {
        NodePtr expr = subtract.GetChild(0);
        subtract.DeleteChild(0);
        subtract.ReplaceWith(expr);
        return;
    }

    if(!constant) {
        return;
    }

// subtraction or negation?
    if(max == 1) {
        if(type == NODE_INT64) {
            itotal = -itotal;
        }
        else {
            ftotal = -ftotal;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = subtract.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        subtract.DeleteChild(max);
    }
}



void Optimizer::ShiftLeft(NodePtr& shift_left)
{
    int64_t        itotal;
    node_t        type;
    int        idx, max;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = shift_left.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = shift_left.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    itotal <<= data.f_int.Get() & 0x3F;
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    itotal <<= (int32_t) data.f_float.Get() & 0x1F;
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = shift_left.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        shift_left.DeleteChild(max);
    }
}


void Optimizer::ShiftRight(NodePtr& shift_right)
{
    int64_t        itotal;
    node_t        type;
    int        idx, max;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = shift_right.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = shift_right.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    itotal >>= data.f_int.Get() & 0x3F;
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    itotal >>= (int32_t) data.f_float.Get() & 0x1F;
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = shift_right.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        shift_right.DeleteChild(max);
    }
}



void Optimizer::ShiftRightUnsigned(NodePtr& shift_right_unsigned)
{
    uint64_t    itotal;
    node_t        type;
    int        idx, max;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = shift_right_unsigned.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = shift_right_unsigned.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    itotal >>= data.f_int.Get() & 0x3F;
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    itotal >>= (int32_t) data.f_float.Get() & 0x1F;
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = shift_right_unsigned.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        shift_right_unsigned.DeleteChild(max);
    }
}


void Optimizer::RotateLeft(NodePtr& rotate_left)
{
    uint64_t    itotal;
    node_t        type;
    int        idx, max, count;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = rotate_left.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = rotate_left.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    count = data.f_int.Get() & 0x1F;
                    if(count != 0) {
                        itotal = (itotal << count) | (itotal >> (32 - count));
                    }
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    count = (int32_t) data.f_float.Get() & 0x1F;
                    if(count != 0) {
                        itotal = ((itotal << count) | (itotal >> (32 - count)));
                    }
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = rotate_left.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        rotate_left.DeleteChild(max);
    }
}



void Optimizer::RotateRight(NodePtr& rotate_right)
{
    uint64_t    itotal;
    node_t        type;
    int        idx, max, count;

    type = NODE_UNKNOWN;
    itotal = 0;
    max = rotate_right.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = rotate_right.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_UNKNOWN) {
                    itotal = data.f_int.Get();
                }
                else {
                    count = data.f_int.Get() & 0x1F;
                    if(count != 0) {
                        itotal = (itotal >> count) | (itotal << (32 - count));
                    }
                }
                type = NODE_INT64;
            }
            else {
                if(type == NODE_UNKNOWN) {
                    itotal = (int32_t) data.f_float.Get();
                }
                else {
                    count = (int32_t) data.f_float.Get() & 0x1F;
                    if(count != 0) {
                        itotal = ((itotal >> count) | (itotal << (32 - count)));
                    }
                }
                type = NODE_FLOAT64;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = rotate_right.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(itotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        rotate_right.DeleteChild(max);
    }
}



// returns:
//     0 - equal
//    -1 - left < right
//     1 - left > right
//     2 - unordered (left or right is a NaN)
//    -2 - error (cannot compare)
int Optimizer::compare(Node::poiner_t relational)
{
    if(relational->get_children_size() != 2)
    {
        return -2;
    }

    Node::pointer_t left(relational->get_child(0));
    Node::pointer_t right(relational->get_child(1));

    if(left->get_type() == Node::NODE_STRING
    && right->get_type() == Node::NODE_STRING)
    {
        return left->get_string()->compare(right->get_string());
    }

    // TODO: if left or right is a string, then JavaScript may convert
    //       the other side to a string and then do a string compare
    //       and vice versa, a string to a number; and we probably
    //       want to handle boolean values too
    if(!left->to_number())
    {
        return -2;
    }
    if(!right->to_number())
    {
        return -2;
    }

    if(left->get_type() == Node::NODE_INT64)
    {
        Int64::int64_type const li(left->get_int64().get());
        if(right->get_type() == Node::NODE_INT64)
        {
            Int64::int64_type const r(li - right->get_int64().get());
            if(r == 0)
            {
                return 0;
            }
            return r < 0 ? -1 : 1;
        }
        else
        {
            Float64::float64_type rf(right->get_float64().get());
            if(isnan(rf))
            {
                return 2;
            }
            int const inf(isinf(rf));
            if(inf != 0)
            {
                return -inf;
            }
            double const r(li - rf);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(r == 0.0)
            {
                return 0;
            }
#pragma GCC diagnostic pop
            return r < 0.0 ? -1 : 1;
        }
    }
    else
    {
        Float64::float64_type const lf(left->get_float64().get());
        if(isnan(lf))
        {
            return 2;
        }
        if(right->get_type() == Node::NODE_INT64)
        {
            int const inf(isinf(lf));
            if(inf != 0)
            {
                return inf;
            }
            double const r(lf - right.get_int64.get());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(r == 0.0)
            {
                return 0;
            }
#pragma GCC diagnostic pop
            return r < 0.0 ? -1 : 1;
        }
        else
        {
            Float64::float64_type const rf(right->get_float64().get());
            if(isnan())
            {
                return 2;
            }
            int const linf(isinf(lf));
            int const rinf(isinf(rf));
            if(linf != 0 || rinf != 0)
            {
                if(linf == rinf)
                {
                    return 0;
                }
                return linf < rinf ? -1 : 1;
            }
            double const r(lf - rf);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
            if(r == 0.0)
            {
                return 0;
            }
#pragma GCC diagnostic pop
            return r < 0.0 ? -1 : 1;
        }
    }
}



void Optimizer::less(Node::pointer_t less)
{
    int r(compare(less));
    if(r != -2 && r != 2)
    {
        Node::pointer_t result(less->create_replacement(r < 0 ? Node::NODE_TRUE : Node::NODE_FALSE));
        less->replace_with(result);
    }
}


void Optimizer::LessEqual(NodePtr& less_equal)
{
    long r = Compare(less_equal);
    if(r == -2) {
        return;
    }

    if(r == 2) {
        // ???
        return;
    }

    Data& result = less_equal.GetData();
    result.f_type = r <= 0 ? NODE_TRUE : NODE_FALSE;

    less_equal.DeleteChild(1);
    less_equal.DeleteChild(0);
}


void Optimizer::Greater(NodePtr& greater)
{
    long r = Compare(greater);
    if(r == -2) {
        return;
    }

    if(r == 2) {
        // ???
        return;
    }

    Data& result = greater.GetData();
    result.f_type = r > 0 ? NODE_TRUE : NODE_FALSE;

    greater.DeleteChild(1);
    greater.DeleteChild(0);
}


void Optimizer::GreaterEqual(NodePtr& greater_equal)
{
    long r = Compare(greater_equal);
    if(r == -2) {
        return;
    }

    if(r == 2) {
        // ???
        return;
    }

    Data& result = greater_equal.GetData();
    result.f_type = r >= 0 ? NODE_TRUE : NODE_FALSE;

    greater_equal.DeleteChild(1);
    greater_equal.DeleteChild(0);
}


void Optimizer::Equality(NodePtr& equality, bool strict, bool logical_not)
{
    if(equality.GetChildCount() != 2) {
        return;
    }

    bool result = false;

    NodePtr child = equality.GetChild(0);
    Data left = child.GetData();

    child = equality.GetChild(1);
    Data right = child.GetData();

    if(strict) {        // ===
        if((left.f_type  == NODE_TRUE || left.f_type  == NODE_FALSE)
        && (right.f_type == NODE_TRUE || right.f_type == NODE_FALSE)) {
            result = left.f_type == right.f_type;
        }
        else if(left.f_type == NODE_INT64 && right.f_type == NODE_INT64) {
            result = left.f_int.Get() == right.f_int.Get();
        }
        else if(left.f_type == NODE_FLOAT64 && right.f_type == NODE_FLOAT64) {
            result = left.f_float.Get() == right.f_float.Get();
        }
        else if(left.f_type == NODE_INT64 && right.f_type == NODE_FLOAT64) {
            result = left.f_int.Get() == right.f_float.Get();
        }
        else if(left.f_type == NODE_FLOAT64 && right.f_type == NODE_INT64) {
            result = left.f_float.Get() == right.f_int.Get();
        }
        else if(left.f_type == NODE_STRING && right.f_type == NODE_STRING) {
            result = left.f_str == right.f_str;
        }
        else if((left.f_type == NODE_UNDEFINED && right.f_type == NODE_UNDEFINED)
             || (left.f_type == NODE_NULL && right.f_type == NODE_NULL)) {
            result = true;
        }
        else if((left.f_type == NODE_NULL && right.f_type == NODE_UNDEFINED)
             || (left.f_type == NODE_UNDEFINED && right.f_type == NODE_NULL)) {
            result = false;
        }
        else if((left.f_type  == NODE_IDENTIFIER || left.f_type  == NODE_VIDENTIFIER)
             && (right.f_type == NODE_IDENTIFIER || right.f_type == NODE_VIDENTIFIER)) {
            // special case of  a === a
            if(left.f_str != right.f_str) {
                // If not the same, we can't know what the result
                // is at compile time.
                return;
            }
            result = true;
        }
        else {
            switch(left.f_type) {
            case NODE_INT64:
            case NODE_FLOAT64:
            case NODE_STRING:
            case NODE_NULL:
            case NODE_UNDEFINED:
            case NODE_TRUE:
            case NODE_FALSE:
                break;

            default:
                // undefined at compile time
                return;

            }
            switch(right.f_type) {
            case NODE_INT64:
            case NODE_FLOAT64:
            case NODE_STRING:
            case NODE_NULL:
            case NODE_UNDEFINED:
            case NODE_TRUE:
            case NODE_FALSE:
                break;

            default:
                // undefined at compile time
                return;

            }
            // any one of these mix is always false in strict mode
            result = false;
        }
    }
    else {            // ==
        switch(left.f_type) {
        case NODE_UNDEFINED:
        case NODE_NULL:
            switch(right.f_type) {
            case NODE_UNDEFINED:
            case NODE_NULL:
                result = true;
                break;

            case NODE_INT64:
            case NODE_FLOAT64:
            case NODE_STRING:
            case NODE_TRUE:
            case NODE_FALSE:
                result = false;
                break;

            default:
                // undefined at compile time
                return;

            }
            break;

        case NODE_TRUE:
            switch(right.f_type) {
            case NODE_TRUE:
                result = true;
                break;

            case NODE_FALSE:
            case NODE_STRING:
            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            default:
                if(!right.ToNumber()) {
                    // undefined at compile time
                    return;
                }
                if(right.f_type == NODE_INT64) {
                    result = right.f_int.Get() == 1;
                }
                else {
                    result = right.f_float.Get() == 1.0;
                }
                break;

            }
            break;

        case NODE_FALSE:
            switch(right.f_type) {
            case NODE_TRUE:
            case NODE_STRING:
            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            case NODE_FALSE:
                result = true;
                break;

            default:
                if(!right.ToNumber()) {
                    // undefined at compile time
                    return;
                }
                if(right.f_type == NODE_INT64) {
                    result = right.f_int.Get() == 0;
                }
                else {
                    result = right.f_float.Get() == 0.0;
                }
                break;

            }
            break;

        case NODE_INT64:
            switch(right.f_type) {
            case NODE_INT64:
                result = left.f_int.Get() == right.f_int.Get();
                break;

            case NODE_FLOAT64:
                result = left.f_int.Get() == right.f_float.Get();
                break;

            case NODE_TRUE:
                result = left.f_int.Get() == 1;
                break;

            case NODE_FALSE:
                result = left.f_int.Get() == 0;
                break;

            //case NODE_STRING: ...

            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            default:
                return;

            }
            break;

        case NODE_FLOAT64:
            switch(right.f_type) {
            case NODE_FLOAT64:
                result = left.f_float.Get() == right.f_float.Get();
                break;

            case NODE_INT64:
                result = left.f_float.Get() == right.f_int.Get();
                break;

            case NODE_TRUE:
                result = left.f_float.Get() == 1.0;
                break;

            case NODE_FALSE:
                result = left.f_float.Get() == 0.0;
                break;

            //case NODE_STRING: ...

            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            default:
                return;

            }
            break;

        case NODE_STRING:
            switch(right.f_type) {
            case NODE_STRING:
                result = left.f_str == right.f_str;
                break;

            case NODE_NULL:
            case NODE_UNDEFINED:
                result = false;
                break;

            //case NODE_INT64: ...
            //case NODE_FLOAT64: ...

            default:
                return;

            }
            break;

        case NODE_IDENTIFIER:
        case NODE_VIDENTIFIER:
            if((right.f_type != NODE_IDENTIFIER && right.f_type != NODE_VIDENTIFIER)
            || left.f_str != right.f_str) {
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

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = equality.GetData();
    data.f_type = result ^ logical_not ? NODE_TRUE : NODE_FALSE;

// we don't need any of these children anymore
    equality.DeleteChild(1);
    equality.DeleteChild(0);
}



void Optimizer::BitwiseAnd(NodePtr& bitwise_and)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max;
    String        result;

    type = NODE_INT64;
    itotal = -1;
    ftotal = -1.0;
    max = bitwise_and.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = bitwise_and.GetChild(idx);
        Data data = child.GetData();
        if(data.f_type == NODE_STRING || type == NODE_STRING) {
            if(type != NODE_STRING && idx != 0) {
                Data value;
                value.f_type = type;
                if(type == NODE_INT64) {
                    value.f_int.Set(itotal);
                }
                else {
                    value.f_float.Set(ftotal);
                }
                value.ToString();
                result = value.f_str;
            }
            if(!data.ToString()) {
                return;
            }
            type = NODE_STRING;
            result += data.f_str;
        }
        else if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_INT64) {
                    itotal &= data.f_int.Get();
                }
                else {
                    ftotal = (int32_t) ftotal & (int32_t) data.f_int.Get();
                    type = NODE_FLOAT64;
                }
            }
            else {
                if(type == NODE_INT64) {
                    type = NODE_FLOAT64;
                    ftotal = (int32_t) itotal & (int32_t) data.f_float.Get();
                }
                else {
                    ftotal = (int32_t) ftotal & (int32_t) data.f_float.Get();
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = bitwise_and.GetData();
    data.f_type = type;
    if(type == NODE_STRING) {
        data.f_str = result;
    }
    else if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        bitwise_and.DeleteChild(max);
    }
}



void Optimizer::BitwiseXOr(NodePtr& bitwise_xor)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max;

    type = NODE_INT64;
    itotal = 0;
    ftotal = 0.0;
    max = bitwise_xor.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = bitwise_xor.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_INT64) {
                    itotal ^= data.f_int.Get();
                }
                else {
                    ftotal = (int32_t) ftotal ^ (int32_t) data.f_int.Get();
                }
            }
            else {
                if(type == NODE_INT64) {
                    ftotal = (int32_t) itotal ^ (int32_t) data.f_float.Get();
                }
                else {
                    ftotal = (int32_t) ftotal ^ (int32_t) data.f_float.Get();
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = bitwise_xor.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        bitwise_xor.DeleteChild(max);
    }
}



void Optimizer::BitwiseOr(NodePtr& bitwise_or)
{
    int64_t        itotal;
    double        ftotal;
    node_t        type;
    int        idx, max;

    type = NODE_INT64;
    itotal = 0;
    ftotal = 0.0;
    max = bitwise_or.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = bitwise_or.GetChild(idx);
        Data data = child.GetData();
        if(data.ToNumber()) {
            if(data.f_type == NODE_INT64) {
                if(type == NODE_INT64) {
                    itotal |= data.f_int.Get();
                }
                else {
                    ftotal = (int32_t) ftotal | (int32_t) data.f_int.Get();
                }
            }
            else {
                if(type == NODE_INT64) {
                    ftotal = (int32_t) itotal | (int32_t) data.f_float.Get();
                }
                else {
                    ftotal = (int32_t) ftotal | (int32_t) data.f_float.Get();
                }
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = bitwise_or.GetData();
    data.f_type = type;
    if(type == NODE_INT64) {
        data.f_int.Set(itotal);
    }
    else {
        data.f_float.Set(ftotal);
    }

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        bitwise_or.DeleteChild(max);
    }
}



void Optimizer::LogicalAnd(NodePtr& logical_and)
{
    node_t        type;
    int        idx, max;

    type = NODE_TRUE;
    max = logical_and.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = logical_and.GetChild(idx);
        Data data = child.GetData();
        if(data.ToBoolean()) {
            if(data.f_type == NODE_FALSE) {
                // stop testing as soon as it is known to be false
                type = NODE_FALSE;
                break;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = logical_and.GetData();
    data.f_type = type;

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        logical_and.DeleteChild(max);
    }
}



void Optimizer::LogicalXOr(NodePtr& logical_xor)
{
    node_t        type;
    int        idx, max;

    type = NODE_FALSE;
    max = logical_xor.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = logical_xor.GetChild(idx);
        Data data = child.GetData();
        if(data.ToBoolean()) {
            if(data.f_type == NODE_TRUE) {
                type = type == NODE_TRUE ? NODE_FALSE : NODE_TRUE;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = logical_xor.GetData();
    data.f_type = type;

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        logical_xor.DeleteChild(max);
    }
}



void Optimizer::LogicalOr(NodePtr& logical_or)
{
    node_t        type;
    int        idx, max;

    type = NODE_FALSE;
    max = logical_or.GetChildCount();
    for(idx = 0; idx < max; ++idx) {
        NodePtr child = logical_or.GetChild(idx);
        Data data = child.GetData();
        if(data.ToBoolean()) {
            if(data.f_type == NODE_TRUE) {
                type = NODE_TRUE;
                break;
            }
        }
        else {
            // We assume that the expression was already
            // compiled and thus identifiers which were
            // constants have been replaced already.
            return;
        }
    }

// if we reach here, we can change the node type
// to NODE_INT64 or NODE_FLOAT64
    Data& data = logical_or.GetData();
    data.f_type = type;

// we don't need any of these children anymore
    while(max > 0) {
        --max;
        logical_or.DeleteChild(max);
    }
}



void Optimizer::Minimum(NodePtr& minimum)
{
    int r = Compare(minimum);
    if(r == -2 || r == 2) {
        return;
    }

    if(r <= 0) {
        // in this case we want to keep the left node
        minimum = minimum.GetChild(0);
    }
    else {
        minimum = minimum.GetChild(1);
    }
}



void Optimizer::Maximum(NodePtr& maximum)
{
    int r = Compare(maximum);
    if(r == -2 || r == 2) {
        return;
    }

    if(r >= 0) {
        // in this case we want to keep the left node
        maximum = maximum.GetChild(0);
    }
    else {
        maximum = maximum.GetChild(1);
    }
}



void Optimizer::conditional(Node::pointer_t& conditional)
{
    if(conditional.GetChildCount() != 3) {
        return;
    }

    NodePtr child = conditional.GetChild(0);
    Data data = child.GetData();
    if(!data.ToBoolean()) {
        return;
    }

    if(data.f_type == NODE_TRUE) {
        NodePtr expr = conditional.GetChild(1);
        conditional.DeleteChild(1);
        conditional.ReplaceWith(expr);
    }
    else {
        NodePtr expr = conditional.GetChild(2);
        conditional.DeleteChild(2);
        conditional.ReplaceWith(expr);
    }
}





}
// namespace as2js

// vim: ts=4 sw=4 et
