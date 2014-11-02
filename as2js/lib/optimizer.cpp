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
#include    "optimizer_tables.h"


namespace as2js
{
namespace Optimizer
{


/** \brief The as2js optimizer.
 *
 * This function goes through all the available optimizations and
 * processes them whenever they apply to your code.
 *
 * Errors may be generated whenever a problem is found.
 *
 * Also some potential errors such as a division or modulo by
 * zero can legally occur in your input program so in that case the
 * optimizer generates a warning to let you know that such a division
 * was found, but no error to speak of.
 *
 * The function reports the total number of errors that were generated
 * while optimizing.
 *
 * At any point after parsing, the program can be passed through
 * the optimizer. This means removing all the possible expressions and
 * statements which can be removed to make the code smaller in the end.
 * The optimizations applied can be tweaked using options ('use ...;').
 *
 * In most cases the compiler already takes care of calling the optimizer
 * at appropriate times. Since it is a static function, it can directly
 * be called as in:
 *
 * \code
 *    Optimizer::Optimize(root);
 * \endcode
 *
 * Where root is a Node representing the root of the optimization (anything
 * outside of the root does not get optimized.)
 *
 * The optimize() function tries to remove all possible expressions
 * and statements which will have no effect in the final output
 * (by default, certain things such as x + 0, may not be removed since
 * such may have an effect... if x is a string, then x + 0 concatenates
 * zero to that string.)
 *
 * The root parameter may be what was returned by the Parser::parse()
 * function of the. However, in most cases, the compiler only optimizes
 * part of the tree as required (because many parts cannot be optimized
 * and it will make things generally faster.)
 *
 * The optimizations are organized in C++ tables that get linked
 * in the compiler as read-only static data. These are organized
 * in many separate files because of the large amount of possible
 * optimizations. There is a list of the various files in the
 * optimizer:
 *
 * \li optimizer.cpp -- the main Optimizer object implementation;
 * all the other files are considered private.
 *
 * \li optimizer_matches.ci and optimizer_matches.cpp -- the tables
 * (.ci) and the functions (.cpp) used to match a tree of nodes
 * and thus determine whether an optimization can be applied or not.
 *
 * \li optimizer_tables.cpp and optimizer_tables.h -- the top level
 * tables of the optimizer. These are used to search for optimizations
 * that can be applied against your tree of nodes. The header defines
 * private classes, structures, etc.
 *
 * \li optimizer_values.ci -- a set of tables representing literal
 * values as in some cases an optimization can be applied if a
 * literal has a specific value.
 *
 * \li optimizer_optimize.ci and optimizer_optimize.cpp -- a set
 * of optimizations defined using tables and corresponding functions
 * to actually apply the optimizations to a tree of nodes.
 *
 * \li optimizer_additive.ci -- optimizations for '+' and '-', including
 * string concatenations.
 *
 * \li optimizer_assignments.ci -- optimizations for all assignments, '=',
 * '+=', '-=', '*=', etc.
 *
 * \li optimizer_bitwise.ci -- optimizations for '~', '&', '|', '^', '<<', '>>',
 * '>>>', '<!', and '>!'.
 *
 * \li optimizer_compare.ci -- optimizations for '<=>'.
 *
 * \li optimizer_conditional.ci -- optimizations for 'a ? b : c'.
 *
 * \li optimizer_equality.ci -- optimizations for '==', '!=', '===', '!==',
 * and '~~'.
 *
 * \li optimizer_logical.ci -- optimizations for '!', '&&', '||', and '^^'.
 *
 * \li optimizer_match.ci -- optimizations for '~=' and '!~'.
 *
 * \li optimizer_multiplicative.ci -- optimizations for '*', '/', '%',
 * and '**'.
 *
 * \li optimizer_relational.ci -- optimizations for '<', '<=', '>',
 * and '>='.
 *
 * \li optimizer_statments.ci -- optimizations for 'if', 'while', 'do',
 * and "directives" (blocks).
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
int optimize(Node::pointer_t& node)
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

    // a ^^ b is somewhat equivalent to (!a != !b) if just a true/false
    // result is expected (i.e. for the if(), while(), for() statements)
    //
    // the actual full expression in standard JavaScript is:
    //
    //      (a ^^ b) <=> (!a != !b ? a || b : false)
    //
    // and as we can see it returns a Boolean false if (!a != !b)

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
// namespace Optimizer
}
// namespace as2js

// vim: ts=4 sw=4 et
