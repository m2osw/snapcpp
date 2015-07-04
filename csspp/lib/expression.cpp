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

/** \file
 * \brief Implementation of the CSS Preprocessor expression.
 *
 * The CSS Preprocessor expression class is used to reduce a list
 * of nodes by applying expressions to the various values.
 *
 * \sa \ref expression_rules
 */

#include "csspp/expression.h"

#include "csspp/exceptions.h"
#include "csspp/parser.h"

#include <cmath>
#include <iostream>

namespace csspp
{

expression::expression(node::pointer_t n, bool skip_whitespace)
    : f_node(n)
    , f_skip_whitespace(skip_whitespace)
{
    if(!f_node)
    {
        throw csspp_exception_logic("expression.cpp:expression(): contructor called with a null pointer.");
    }
}

void expression::set_variable_handler(expression_variables_interface * handler)
{
    f_variable_handler = handler;
}

void expression::compile_args(bool divide_font_metrics)
{
    size_t const max_children(f_node->size());
    for(size_t a(0); a < max_children; ++a)
    {
        if(a + 1 < max_children
        || !f_node->get_last_child()->is(node_type_t::OPEN_CURLYBRACKET))
        {
            expression arg_expr(f_node->get_child(a), true);
            arg_expr.f_divide_font_metrics = divide_font_metrics;
            arg_expr.compile_list(f_node);
        }
    }
}

node::pointer_t expression::compile_list(node::pointer_t parent)
{
    mark_start();
    next();
    node::pointer_t result;

    // result is a list: a b c ...
    for(;;)
    {
        // we have one special case here: !important cannot be
        // compiled as an expression...
        if(f_current->is(node_type_t::EXCLAMATION))
        {
            // this is viewed as !important and only such
            // can appear now so we can just return immediately
            if(!end_of_nodes())
            {
                error::instance() << f_current->get_position()
                        << "A special flag, !"
                        << f_current->get_string()
                        << " in this case, must only appear at the end of a declaration."
                        << error_mode_t::ERROR_WARNING;
            }

            // we remove the !<word> from the declaration and
            // setup a flag instead
            f_node->remove_child(f_current);
            if(f_pos > 0)
            {
                --f_pos;
            }
            parent->set_flag(f_current->get_string(), true);
        }
        else
        {
            result = conditional();
            replace_with_result(result);
        }
        if(end_of_nodes())
        {
            break;
        }
        next();
        // keep one whitespace between expressions if such exists
        if(f_current->is(node_type_t::WHITESPACE))
        {
            if(end_of_nodes())
            {
                // TODO: list of nodes ends with WHITESPACE
                break;    // LCOV_EXCL_LINE
            }
            mark_start();
            next();
        }
    }

    return result;
}

node::pointer_t expression::compile()
{
    mark_start();
    next();
    return replace_with_result(conditional());
}

// basic state handling
bool expression::end_of_nodes()
{
    return f_pos >= f_node->size();
}

void expression::mark_start()
{
    f_start = f_pos;
}

node::pointer_t expression::replace_with_result(node::pointer_t result)
{
    if(result)
    {
        if(f_start == static_cast<size_t>(-1))
        {
            throw csspp_exception_logic("expression.cpp:expression(): replace_with_result() cannot be called if mark_start() was never called.");
        }

        // f_pos may point to a tag right after the end of the previous
        // expression; expressions may be separated by WHITESPACE tokens
        // too so we have to restore them if they appear at the end of
        // the epxression we just worked on (i.e. we cannot eat a WHITESPACE
        // in an expression.)
        if(!f_current->is(node_type_t::EOF_TOKEN) && f_pos > 0)
        {
            while(f_pos > 0)
            {
                --f_pos;
                if(f_node->get_child(f_pos) == f_current)
                {
                    break;
                }
            }
            if(f_pos > 0 && f_node->get_child(f_pos - 1)->is(node_type_t::WHITESPACE))
            {
                --f_pos;
            }
        }

        // this "reduces" the expression with its result
        while(f_pos > f_start)
        {
            --f_pos;
            f_node->remove_child(f_pos);
        }
        f_node->insert_child(f_pos, result);
        ++f_pos;
    }

    mark_start();

    return result;
}

void expression::next()
{
    if(f_pos >= f_node->size())
    {
        if(!f_current || !f_current->is(node_type_t::EOF_TOKEN))
        {
            f_current.reset(new node(node_type_t::EOF_TOKEN, f_node->get_position()));
        }
    }
    else
    {
        f_current = f_node->get_child(f_pos);
        ++f_pos;
        while(f_skip_whitespace
           && f_pos < f_node->size()
           && f_node->get_child(f_pos)->is(node_type_t::WHITESPACE))
        {
            ++f_pos;
        }
    }
}

node::pointer_t expression::look_ahead() const
{
    if(f_pos < f_node->size())
    {
        return f_node->get_child(f_pos);
    }

    return node::pointer_t();
}

node::pointer_t expression::current() const
{
    return f_current;
}

boolean_t expression::boolean(node::pointer_t n)
{
    boolean_t const result(n->to_boolean());
    if(result == boolean_t::BOOLEAN_INVALID)
    {
        error::instance() << n->get_position()
                << "a boolean expression was expected."
                << error_mode_t::ERROR_ERROR;
    }
    return result;
}

// expression parsing
node::pointer_t expression::argument_list()
{
    node::pointer_t result(assignment());
    if(!result)
    {
        return node::pointer_t();
    }

    {
        node::pointer_t item(result);
        result.reset(new node(node_type_t::LIST, item->get_position()));
        result->add_child(item);
    }

    while(f_current->is(node_type_t::COMMA))
    {
        // skip the ','
        next();

        // read the next expression
        result->add_child(assignment());
    }

    return result;
}

bool expression::is_label() const
{
    // we have a label if we have:
    //    <identifier> <ws>* ':'
    size_t pos(f_pos + 1);
    if(!f_current->is(node_type_t::IDENTIFIER)
    || pos >= f_node->size())
    {
        return false;
    }

    node::pointer_t n(f_node->get_child(pos));
    if(n->is(node_type_t::WHITESPACE))
    {
        ++pos;
        if(pos >= f_node->size())
        {
            return false;
        }
        n = f_node->get_child(pos);
    }

    return n->is(node_type_t::COLON);
}

node::pointer_t expression::expression_list()
{
    // expression-list: assignment
    //                | list
    //                | map
    //
    // list: assignment
    //     | list ',' assignment
    //
    // map: IDENTIFIER ':' assignment
    //    | map ',' IDENTIFIER ':' assignment
    //
    safe_bool_t safe_skip_whitespace(f_skip_whitespace);

    if(is_label())
    {
        node::pointer_t map(new node(node_type_t::ARRAY, f_current->get_position()));
        while(is_label())
        {
            map->add_child(f_current);

            // skip the IDENTIFIER (f_current == ':')
            next();

            // skip the ':'
            next();

            node::pointer_t result;
            if(f_current->is(node_type_t::COMMA))
            {
                // empty entries are viewed as valid and set to NULL
                result.reset(new node(node_type_t::NULL_TOKEN, f_current->get_position()));

                // skip the ','
                next();
            }
            else
            {
                result = assignment();
                if(result)
                {
                    // maps need to have an even number of entries, but
                    // the value of an entry does not need to be provided
                    // in which case we want to put NULL in there
                    result.reset(new node(node_type_t::NULL_TOKEN, f_current->get_position()));
                    map->add_child(result);
                    break;
                }
            }

            map->add_child(result);
        }
        return map;
    }
    else
    {
        node::pointer_t result(assignment());

        if(result
        && f_current->is(node_type_t::COMMA))
        {
            node::pointer_t array(new node(node_type_t::ARRAY, f_current->get_position()));
            array->add_child(result);

            // an expression list does NOT return a LIST, it just returns
            // the result of the last expression (as in C/C++)
            while(f_current->is(node_type_t::COMMA))
            {
                // skip the ','
                next();

                if(f_current->is(node_type_t::IDENTIFIER)
                && f_pos + 1 < f_node->size())
                {
                }

                result = assignment();
                if(!result)
                {
                    break;
                }
                array->add_child(result);
            }

            // the result is the array in this case
            return array;
        }

        return result;
    }
}

node::pointer_t expression::assignment()
{
    // assignment: conditional
    //           | IDENTIFIER ':=' conditional

    node::pointer_t result(conditional());
    if(!result)
    {
        return node::pointer_t();
    }

    if(result->is(node_type_t::IDENTIFIER)
    && f_current->is(node_type_t::ASSIGNMENT))
    {
        next();

        node::pointer_t value(conditional());
        f_variables[result->get_string()] = value;

        // the return value of an assignment is the value of
        // the variable
        result = value;
    }

    return result;
}

node::pointer_t expression::conditional()
{
    // conditional: logical_or
    //            | conditional '?' expression_list ':' logical_or

    // note: we also support if(expr, expr, expr)

    node::pointer_t result(logical_or());
    if(!result)
    {
        return node::pointer_t();
    }

    while(f_current->is(node_type_t::CONDITIONAL))
    {
        // skip the '?'
        next();

        // TODO: avoid calculating both sides.
        node::pointer_t result_true(expression_list());
        if(!result_true)
        {
            return node::pointer_t();
        }

        if(!f_current->is(node_type_t::COLON))
        {
            error::instance() << f_current->get_position()
                    << "a mandatory ':' was expected after a '?' first expression."
                    << error_mode_t::ERROR_ERROR;
            return node::pointer_t();
        }

        // skip the ':'
        next();

        node::pointer_t result_false(logical_or());
        if(!result_false)
        {
            return node::pointer_t();
        }

        // select the right result
        int const r(boolean(result));
        if(r < 0)
        {
            return node::pointer_t();
        }
        result = r == 0 ? result_false : result_true;
    }

    return result;
}

node::pointer_t expression::logical_or()
{
    // logical_or: logical_and
    //           | logical_or IDENTIFIER (='or') logical_and
    //           | logical_or '||' logical_and

    node::pointer_t result(logical_and());
    if(!result)
    {
        return false;
    }

    while((f_current->is(node_type_t::IDENTIFIER) && f_current->get_string() == "or")
       || f_current->is(node_type_t::COLUMN))
    {
        position pos(f_current->get_position());

        // skip the OR
        next();

        node::pointer_t rhs(logical_and());
        if(!rhs)
        {
            return false;
        }

        // apply the OR
        int const lr(boolean(result));
        int const rr(boolean(rhs));
        result.reset(new node(node_type_t::BOOLEAN, pos));
        result->set_boolean(lr || rr);
    }

    return result;
}

node::pointer_t expression::logical_and()
{
    // logical_and: equality
    //            | logical_and IDENTIFIER (='and') equality
    //            | logical_and '&&' equality

    node::pointer_t result(equality());
    if(!result)
    {
        return node::pointer_t();
    }

    while((f_current->is(node_type_t::IDENTIFIER) && f_current->get_string() == "and")
       || f_current->is(node_type_t::AND))
    {
        position pos(f_current->get_position());

        // skip the AND
        next();

        node::pointer_t rhs(equality());
        if(!rhs)
        {
            return node::pointer_t();
        }

        // apply the AND
        int const lr(boolean(result));
        int const rr(boolean(rhs));
        result.reset(new node(node_type_t::BOOLEAN, pos));
        result->set_boolean(lr && rr);
    }

    return result;
}

bool is_equal(node::pointer_t lhs, node::pointer_t rhs)
{
    switch(mix_node_types(lhs->get_type(), rhs->get_type()))
    {
    case mix_node_types(node_type_t::BOOLEAN, node_type_t::BOOLEAN):
        return lhs->get_boolean() == rhs->get_boolean();

    case mix_node_types(node_type_t::INTEGER, node_type_t::INTEGER):
        // TBD: should we generate an error if these are not
        //      equivalent dimensions?
        return lhs->get_integer() == rhs->get_integer();

    case mix_node_types(node_type_t::DECIMAL_NUMBER, node_type_t::DECIMAL_NUMBER):
        // TBD: should we generate an error if these are not
        //      equivalent dimensions?
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        return lhs->get_decimal_number() == rhs->get_decimal_number();
#pragma GCC diagnostic pop

    case mix_node_types(node_type_t::PERCENT, node_type_t::PERCENT):
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
        return lhs->get_decimal_number() == rhs->get_decimal_number();
#pragma GCC diagnostic pop

    case mix_node_types(node_type_t::STRING, node_type_t::STRING):
        return lhs->get_string() == rhs->get_string();

    }

    error::instance() << lhs->get_position()
            << "incompatible types between "
            << lhs->get_type()
            << " and "
            << rhs->get_type()
            << " for operator '=', '!=', '<', '<=', '>', or '>='."
            << error_mode_t::ERROR_ERROR;
    return false;
}

bool is_less_than(node::pointer_t lhs, node::pointer_t rhs)
{
    switch(mix_node_types(lhs->get_type(), rhs->get_type()))
    {
    case mix_node_types(node_type_t::BOOLEAN, node_type_t::BOOLEAN):
        return lhs->get_boolean() < rhs->get_boolean();

    case mix_node_types(node_type_t::INTEGER, node_type_t::INTEGER):
        // TBD: should we generate an error if these are not
        //      equivalent dimensions?
        return lhs->get_integer() < rhs->get_integer();

    case mix_node_types(node_type_t::DECIMAL_NUMBER, node_type_t::DECIMAL_NUMBER):
        // TBD: should we generate an error if these are not
        //      equivalent dimensions?
        return lhs->get_decimal_number() < rhs->get_decimal_number();

    case mix_node_types(node_type_t::PERCENT, node_type_t::PERCENT):
        return lhs->get_decimal_number() < rhs->get_decimal_number();

    case mix_node_types(node_type_t::STRING, node_type_t::STRING):
        return lhs->get_string() < rhs->get_string();

    }

    error::instance() << lhs->get_position()
            << "incompatible types between "
            << lhs->get_type()
            << " and "
            << rhs->get_type()
            << " for operator '<', '<=', '>', or '>='."
            << error_mode_t::ERROR_ERROR;
    return false;
}

bool match(node_type_t op, node::pointer_t lhs, node::pointer_t rhs)
{
    std::string s;
    std::string l;

    switch(mix_node_types(lhs->get_type(), rhs->get_type()))
    {
    case mix_node_types(node_type_t::STRING, node_type_t::STRING):
        s = lhs->get_string();
        l = rhs->get_string();
        break;

    default:
        error::instance() << lhs->get_position()
                << "incompatible types between "
                << lhs->get_type()
                << " and "
                << rhs->get_type()
                << " for operator '~='."
                << error_mode_t::ERROR_ERROR;
        return lhs->get_string() == rhs->get_string();

    }

    switch(op)
    {
    case node_type_t::INCLUDE_MATCH:
        l = " " + l + " ";
        s = " " + s + " ";
        break;

    case node_type_t::PREFIX_MATCH:
        if(l.length() < s.length())
        {
            return false;
        }
        return s == l.substr(0, s.length());

    case node_type_t::SUFFIX_MATCH:
        if(l.length() < s.length())
        {
            return false;
        }
        return s == l.substr(l.length() - s.length());

    case node_type_t::SUBSTRING_MATCH:
        break;

    case node_type_t::DASH_MATCH:
        l = "-" + l + "-";
        s = "-" + s + "-";
        break;

    default:
        throw csspp_exception_logic("expression.cpp:include_match(): called with an invalid operator."); // LCOV_EXCL_LINE

    }

    return l.find(s) != std::string::npos;
}

node_type_t expression::equality_operator(node::pointer_t n)
{
    switch(n->get_type())
    {
    // return type as is
    case node_type_t::EQUAL:
        if(f_pos + 1 < f_node->size())
        {
            if(f_node->get_child(f_pos + 1)->is(node_type_t::EQUAL))
            {
                // we accept '==' with a warning (for SASS compatibility
                // skip the first '=' here
                next();
                error::instance() << n->get_position()
                        << "we accepted '==' instead of '=' in an expression, you probably want to change the operator to just '='."
                        << error_mode_t::ERROR_WARNING;
            }
        }
        return node_type_t::EQUAL;

    case node_type_t::NOT_EQUAL:
    case node_type_t::INCLUDE_MATCH:
    case node_type_t::PREFIX_MATCH:
    case node_type_t::SUFFIX_MATCH:
    case node_type_t::SUBSTRING_MATCH:
    case node_type_t::DASH_MATCH:
        return n->get_type();

    case node_type_t::IDENTIFIER:
        {
            if(n->get_string() == "not-equal")
            {
                return node_type_t::NOT_EQUAL;
            }
        }
        /*PASSTHROUGH*/
    default:
        return node_type_t::UNKNOWN;

    }
}

node::pointer_t expression::equality()
{
    // equality: relational
    //         | equality '=' relational
    //         | equality '!=' relational
    //         | equality '~=' relational
    //         | equality '^=' relational
    //         | equality '$=' relational
    //         | equality '*=' relational
    //         | equality '|=' relational

    node::pointer_t result(relational());
    if(!result)
    {
        return node::pointer_t();
    }

    node_type_t op(equality_operator(f_current));
    while(op != node_type_t::UNKNOWN)
    {
        position pos(f_current->get_position());

        // skip the equality operator
        next();

        node::pointer_t rhs(relational());
        if(!rhs)
        {
            return false;
        }

        // apply the equality operation
        bool boolean_result(false);
        switch(op)
        {
        case node_type_t::EQUAL:
            boolean_result = is_equal(result, rhs);
            break;

        case node_type_t::NOT_EQUAL:
            boolean_result = !is_equal(result, rhs);
            break;

        case node_type_t::INCLUDE_MATCH:
        case node_type_t::PREFIX_MATCH:
        case node_type_t::SUFFIX_MATCH:
        case node_type_t::SUBSTRING_MATCH:
        case node_type_t::DASH_MATCH:
            boolean_result = match(op, result, rhs);
            break;

        default:
            throw csspp_exception_logic("expression.cpp:equality(): unexpected operator in 'op'."); // LCOV_EXCL_LINE

        }
        result.reset(new node(node_type_t::BOOLEAN, pos));
        result->set_boolean(boolean_result);

        op = equality_operator(f_current);
    }

    return result;
}

node_type_t relational_operator(node::pointer_t n)
{
    switch(n->get_type())
    {
    case node_type_t::LESS_THAN:
    case node_type_t::LESS_EQUAL:
    case node_type_t::GREATER_THAN:
    case node_type_t::GREATER_EQUAL:
        return n->get_type();

    default:
        return node_type_t::UNKNOWN;

    }
}

node::pointer_t expression::relational()
{
    // relational: additive
    //           | relational '<' additive
    //           | relational '<=' additive
    //           | relational '>' additive
    //           | relational '>=' additive

    node::pointer_t result(additive());
    if(!result)
    {
        return node::pointer_t();
    }

    node_type_t op(relational_operator(f_current));
    while(op != node_type_t::UNKNOWN)
    {
        position pos(f_current->get_position());

        // skip the relational operator
        next();

        node::pointer_t rhs(additive());
        if(!rhs)
        {
            return node::pointer_t();
        }

        // apply the equality operation
        bool boolean_result(false);
        switch(op)
        {
        case node_type_t::LESS_THAN:
            boolean_result = is_less_than(result, rhs);
            break;

        case node_type_t::LESS_EQUAL:
            boolean_result = is_less_than(result, rhs) && is_equal(result, rhs);
            break;

        case node_type_t::GREATER_THAN:
            boolean_result = !is_less_than(result, rhs) && !is_equal(result, rhs);
            break;

        case node_type_t::GREATER_EQUAL:
            boolean_result = !is_less_than(result, rhs);
            break;

        default:
            throw csspp_exception_logic("expression.cpp:relational(): unexpected operator in 'op'."); // LCOV_EXCL_LINE

        }
        result.reset(new node(node_type_t::BOOLEAN, pos));
        result->set_boolean(boolean_result);

        op = relational_operator(f_current);
    }

    return result;
}

node_type_t additive_operator(node::pointer_t n)
{
    switch(n->get_type())
    {
    case node_type_t::ADD:
    case node_type_t::SUBTRACT:
        return n->get_type();

    default:
        return node_type_t::UNKNOWN;

    }
}

node::pointer_t add(node::pointer_t lhs, node::pointer_t rhs, bool subtract)
{
    node_type_t type(node_type_t::UNKNOWN);
    integer_t ai;
    integer_t bi;
    decimal_number_t af;
    decimal_number_t bf;

    switch(mix_node_types(lhs->get_type(), rhs->get_type()))
    {
    case mix_node_types(node_type_t::STRING, node_type_t::STRING):
        {
            if(subtract)
            {
                error::instance() << lhs->get_position()
                        << "incompatible types between "
                        << lhs->get_type()
                        << " and "
                        << rhs->get_type()
                        << " for operator '-'."
                        << error_mode_t::ERROR_ERROR;
                return node::pointer_t();
            }
            // string concatenation
            node::pointer_t result(new node(node_type_t::STRING, lhs->get_position()));
            result->set_string(lhs->get_string() + rhs->get_string());
            return result;
        }

    case mix_node_types(node_type_t::INTEGER, node_type_t::INTEGER):
        // TODO: test that the dimensions are compatible
        ai = lhs->get_integer();
        bi = rhs->get_integer();
        type = node_type_t::INTEGER;
        break;

    case mix_node_types(node_type_t::DECIMAL_NUMBER, node_type_t::DECIMAL_NUMBER):
        // TODO: test that the dimensions are compatible
        af = lhs->get_decimal_number();
        bf = rhs->get_decimal_number();
        type = node_type_t::DECIMAL_NUMBER;
        break;

    case mix_node_types(node_type_t::PERCENT, node_type_t::PERCENT):
        af = lhs->get_decimal_number();
        bf = rhs->get_decimal_number();
        type = node_type_t::PERCENT;
        break;

    default:
        {
            node_type_t lt(lhs->get_type());
            node_type_t rt(rhs->get_type());

            error::instance() << lhs->get_position()
                    << "incompatible types between "
                    << lt
                    << (lt == node_type_t::IDENTIFIER || lt == node_type_t::STRING ? " (" + lhs->get_string() + ")" : "")
                    << " and "
                    << rt
                    << (rt == node_type_t::IDENTIFIER || rt == node_type_t::STRING ? " (" + rhs->get_string() + ")" : "")
                    << " for operator '"
                    << (subtract ? "-" : "+")
                    << "'."
                    << error_mode_t::ERROR_ERROR;
            return node::pointer_t();
        }

    }

    node::pointer_t result(new node(type, lhs->get_position()));
    if(type != node_type_t::PERCENT)
    {
        // do not lose the dimension
        result->set_string(lhs->get_string());
    }

    switch(type)
    {
    case node_type_t::INTEGER:
        if(subtract)
        {
            result->set_integer(ai - bi);
        }
        else
        {
            result->set_integer(ai + bi);
        }
        break;

    case node_type_t::DECIMAL_NUMBER:
    case node_type_t::PERCENT:
        if(subtract)
        {
            result->set_decimal_number(af - bf);
        }
        else
        {
            result->set_decimal_number(af + bf);
        }
        break;

    default:
        {
            node_type_t lt(lhs->get_type());
            node_type_t rt(rhs->get_type());

            error::instance() << lhs->get_position()
                    << "incompatible types between "
                    << lt
                    << (lt == node_type_t::IDENTIFIER || lt == node_type_t::STRING ? " (" + lhs->get_string() + ")" : "")
                    << " and "
                    << rt
                    << (rt == node_type_t::IDENTIFIER || rt == node_type_t::STRING ? " (" + rhs->get_string() + ")" : "")
                    << " for operator '"
                    << (subtract ? "-" : "+")
                    << "'."
                    << error_mode_t::ERROR_ERROR;
            return node::pointer_t();
        }

    }

    return result;
}

node::pointer_t expression::additive()
{
    //  additive: multiplicative
    //          | additive '+' multiplicative
    //          | additive '-' multiplicative

    node::pointer_t result(multiplicative());
    if(!result)
    {
        return node::pointer_t();
    }

    node_type_t op(additive_operator(f_current));
    while(op != node_type_t::UNKNOWN)
    {
        // skip the additive operator
        next();

        node::pointer_t rhs(multiplicative());
        if(!rhs)
        {
            return node::pointer_t();
        }

        // apply the additive operation
        result = add(result, rhs, op == node_type_t::SUBTRACT);
        if(!result)
        {
            return node::pointer_t();
        }

        op = additive_operator(f_current);
    }

    return result;
}

node_type_t expression::multiplicative_operator(node::pointer_t n)
{
    switch(n->get_type())
    {
    case node_type_t::IDENTIFIER:
        if(n->get_string() == "mul")
        {
            return node_type_t::MULTIPLY;
        }
        if(n->get_string() == "div")
        {
            return node_type_t::DIVIDE;
        }
        if(n->get_string() == "mod")
        {
            return node_type_t::MODULO;
        }
        break;

    case node_type_t::MULTIPLY:
    case node_type_t::DIVIDE:
    case node_type_t::MODULO:
        return n->get_type();

    default:
        break;

    }

    return node_type_t::UNKNOWN;
}

node::pointer_t expression::multiply(node_type_t op, node::pointer_t lhs, node::pointer_t rhs)
{
    node_type_t type(node_type_t::INTEGER);
    integer_t ai(0);
    integer_t bi(0);
    decimal_number_t af(0.0);
    decimal_number_t bf(0.0);

    switch(mix_node_types(lhs->get_type(), rhs->get_type()))
    {
    case mix_node_types(node_type_t::INTEGER, node_type_t::STRING):
        std::swap(lhs, rhs);
    case mix_node_types(node_type_t::STRING, node_type_t::INTEGER):
        if(op != node_type_t::MULTIPLY)
        {
            error::instance() << lhs->get_position()
                    << "incompatible types between "
                    << lhs->get_type()
                    << " and "
                    << rhs->get_type()
                    << " for operator '/' or '%'."
                    << error_mode_t::ERROR_ERROR;
            return node::pointer_t();
        }
        else
        {
            integer_t count(rhs->get_integer());
            if(count < 0)
            {
                error::instance() << lhs->get_position()
                        << "string * integer requires that the integer not be negative ("
                        << rhs->get_integer()
                        << ")."
                        << error_mode_t::ERROR_ERROR;
                return node::pointer_t();
            }
            std::string result;
            for(; count > 0; --count)
            {
                result += lhs->get_string();
            }
            lhs->set_string(result);
        }
        return lhs;

    case mix_node_types(node_type_t::INTEGER, node_type_t::INTEGER):
        // TODO: test that the dimensions are compatible
        ai = lhs->get_integer();
        bi = rhs->get_integer();
        type = node_type_t::INTEGER;
        break;

    case mix_node_types(node_type_t::INTEGER, node_type_t::DECIMAL_NUMBER):
        // TODO: test that the dimensions are compatible
        af = static_cast<decimal_number_t>(lhs->get_integer());
        bf = rhs->get_decimal_number();
        type = node_type_t::DECIMAL_NUMBER;
        break;

    case mix_node_types(node_type_t::DECIMAL_NUMBER, node_type_t::INTEGER):
        // TODO: test that the dimensions are compatible
        af = lhs->get_decimal_number();
        bf = static_cast<decimal_number_t>(rhs->get_integer());
        type = node_type_t::DECIMAL_NUMBER;
        break;

    case mix_node_types(node_type_t::DECIMAL_NUMBER, node_type_t::PERCENT):
        // TODO: test that the dimensions are compatible
        af = lhs->get_decimal_number();
        bf = rhs->get_decimal_number();
        type = node_type_t::PERCENT;
        break;

    case mix_node_types(node_type_t::PERCENT, node_type_t::DECIMAL_NUMBER):
        // TODO: test that the dimensions are compatible
        af = lhs->get_decimal_number();
        bf = rhs->get_decimal_number();
        type = node_type_t::PERCENT;
        break;

    case mix_node_types(node_type_t::DECIMAL_NUMBER, node_type_t::DECIMAL_NUMBER):
        // TODO: test that the dimensions are compatible
        af = lhs->get_decimal_number();
        bf = rhs->get_decimal_number();
        type = node_type_t::DECIMAL_NUMBER;
        break;

    case mix_node_types(node_type_t::INTEGER, node_type_t::PERCENT):
        // TODO: test that the dimensions are compatible
        af = static_cast<decimal_number_t>(lhs->get_integer());
        bf = rhs->get_decimal_number();
        type = node_type_t::PERCENT;
        break;

    case mix_node_types(node_type_t::PERCENT, node_type_t::INTEGER):
        // TODO: test that the dimensions are compatible
        af = lhs->get_decimal_number();
        bf = static_cast<decimal_number_t>(rhs->get_integer());
        type = node_type_t::PERCENT;
        break;

    case mix_node_types(node_type_t::PERCENT, node_type_t::PERCENT):
        af = lhs->get_decimal_number();
        bf = rhs->get_decimal_number();
        type = node_type_t::DECIMAL_NUMBER;
        break;

    default:
        error::instance() << f_current->get_position()
                << "incompatible types between "
                << lhs->get_type()
                << " and "
                << rhs->get_type()
                << " for operators '*', '/', or '%'."
                << error_mode_t::ERROR_ERROR;
        return node::pointer_t();

    }

    if(op == node_type_t::DIVIDE && f_divide_font_metrics)
    {
        // this is that special case of a FONT_METRICS node
        node::pointer_t result(new node(node_type_t::FONT_METRICS, lhs->get_position()));

        // convert integers
        // (we don't need to convert to do the set, but I find it cleaner this way)
        if(type == node_type_t::INTEGER)
        {
            af = static_cast<decimal_number_t>(ai);
            bf = static_cast<decimal_number_t>(bi);
        }

        result->set_font_size(af);
        result->set_line_height(bf);
        if(lhs->is(node_type_t::PERCENT))
        {
            result->set_dim1("%");
        }
        else
        {
            result->set_dim1(lhs->get_string());
        }
        if(rhs->is(node_type_t::PERCENT))
        {
            result->set_dim2("%");
        }
        else
        {
            result->set_dim2(rhs->get_string());
        }
        return result;
    }

    node::pointer_t result(new node(type, lhs->get_position()));
    if(type != node_type_t::PERCENT)
    {
        // do not lose the dimension(s)
        result->set_string(lhs->get_string());
    }

    switch(type)
    {
    case node_type_t::INTEGER:
        switch(op)
        {
        case node_type_t::MULTIPLY:
            result->set_integer(ai * bi);
            break;

        case node_type_t::DIVIDE:
            result->set_integer(ai / bi);
            break;

        case node_type_t::MODULO:
            result->set_integer(ai % bi);
            break;

        default:
            throw csspp_exception_logic("expression.cpp:multiply(): unexpected operator."); // LCOV_EXCL_LINE

        }
        break;

    case node_type_t::DECIMAL_NUMBER:
    case node_type_t::PERCENT:
        switch(op)
        {
        case node_type_t::MULTIPLY:
            result->set_decimal_number(af * bf);
            break;

        case node_type_t::DIVIDE:
            result->set_decimal_number(af / bf);
            break;

        case node_type_t::MODULO:
            result->set_decimal_number(fmod(af, bf));
            break;

        default:
            throw csspp_exception_logic("expression.cpp:multiply(): unexpected operator."); // LCOV_EXCL_LINE

        }
        break;

    default:
        error::instance() << f_current->get_position()
                << "incompatible types between "
                << lhs->get_type()
                << " and "
                << rhs->get_type()
                << " for operator "
                << op
                << "."
                << error_mode_t::ERROR_ERROR;
        return node::pointer_t();

    }

    return result;
}

node::pointer_t expression::multiplicative()
{
    // multiplicative: power
    //               | multiplicative '*' power
    //               | multiplicative '/' power
    //               | multiplicative '%' power

    node::pointer_t result(power());
    if(!result)
    {
        return node::pointer_t();
    }

    node_type_t op(multiplicative_operator(f_current));
    while(op != node_type_t::UNKNOWN)
    {
        // skip the multiplicative operator
        next();

        node::pointer_t rhs(power());
        if(!rhs)
        {
            return false;
        }

        // apply the multiplicative operation
        result = multiply(op, result, rhs);
        if(!result)
        {
            return node::pointer_t();
        }

        op = multiplicative_operator(f_current);
    }

    return result;
}

node::pointer_t expression::apply_power(node::pointer_t lhs, node::pointer_t rhs)
{
    node::pointer_t result(new node(node_type_t::DECIMAL_NUMBER, lhs->get_position()));

    switch(mix_node_types(lhs->get_type(), rhs->get_type()))
    {
    case mix_node_types(node_type_t::INTEGER, node_type_t::INTEGER):
        // TODO: how to handle dimension?
        result->set_string(lhs->get_string());
        result->set_decimal_number(pow(lhs->get_integer(), rhs->get_integer()));
        break;

    case mix_node_types(node_type_t::DECIMAL_NUMBER, node_type_t::DECIMAL_NUMBER):
        // TODO: how to handle dimension?
        result->set_string(lhs->get_string());
        result->set_decimal_number(pow(lhs->get_decimal_number(), rhs->get_decimal_number()));
        break;

    case mix_node_types(node_type_t::POWER, node_type_t::POWER):
        result->set_decimal_number(pow(lhs->get_decimal_number(), rhs->get_decimal_number()));
        break;

    default:
        error::instance() << f_current->get_position()
                << "incompatible types between "
                << lhs->get_type()
                << " and "
                << rhs->get_type()
                << " for operator '**'."
                << error_mode_t::ERROR_ERROR;
        return node::pointer_t();

    }

    return result;
}

node::pointer_t expression::power()
{
    // power: post
    //      | post '**' post

    node::pointer_t result(post());
    if(!result)
    {
        return node::pointer_t();
    }

    node_type_t op(multiplicative_operator(f_current));
    while((f_current->is(node_type_t::IDENTIFIER) && f_current->get_string() == "pow")
       || f_current->is(node_type_t::POWER))
    {
        position pos(f_current->get_position());

        // skip the power operator
        next();

        node::pointer_t rhs(post());
        if(!rhs)
        {
            return false;
        }

        // apply the power operation
        result = apply_power(result, rhs);
        if(!result)
        {
            return node::pointer_t();
        }

        op = multiplicative_operator(f_current);
    }

    return result;
}

node::pointer_t expression::post()
{
    // post: unary
    //     | post '[' expression ']'
    //     | post '.' IDENTIFIER

    // TODO: add support to access color members (i.e. $c.red <=> red($c))

    node::pointer_t result(unary());
    if(!result)
    {
        return node::pointer_t();
    }

    node::pointer_t index;
    for(;;)
    {
        if(f_current->is(node_type_t::OPEN_SQUAREBRACKET))
        {
            // compile the index expression
            expression index_expr(f_current, true);
            index_expr.next();
            node::pointer_t i(index_expr.expression_list());
            if(!i)
            {
                return node::pointer_t();
            }

            // skip the '['
            next();

            if(i->is(node_type_t::INTEGER))
            {
                if(result->is(node_type_t::ARRAY)
                || result->is(node_type_t::LIST))
                {
                    // index is 1 based (not like in C/C++)
                    integer_t const idx(i->get_integer() - 1);
                    if(static_cast<size_t>(idx) >= result->size())
                    {
                        error::instance() << f_current->get_position()
                                << "index "
                                << idx
                                << " is out of range. The allowed range is 1 to "
                                << static_cast<int>(result->size())
                                << "."
                                << error_mode_t::ERROR_ERROR;
                        return node::pointer_t();
                    }
                    result = result->get_child(idx);
                }
                else if(result->is(node_type_t::MAP))
                {
                    // index is 1 based (not like in C/C++)
                    // maps are defined as <property name> ':' <property value>
                    // so the numeric index being used to access the property
                    // value it has to be x 2 + 1 (C index: 1, 3, 5...)
                    integer_t const idx((i->get_integer() - 1) * 2 + 1);
                    if(static_cast<size_t>(idx) >= result->size())
                    {
                        error::instance() << f_current->get_position()
                                << "index "
                                << idx
                                << " is out of range. The allowed range is 1 to "
                                << static_cast<int>(result->size())
                                << "."
                                << error_mode_t::ERROR_ERROR;
                        return node::pointer_t();
                    }
                    result = result->get_child(idx);
                }
                else
                {
                    error::instance() << f_current->get_position()
                            << "unsupported type "
                            << result->get_type()
                            << " for the 'list[<index>]' operation."
                            << error_mode_t::ERROR_ERROR;
                    return node::pointer_t();
                }
            }
            else if(i->is(node_type_t::STRING))
            {
                // nothing more to skip, the string is a child in
                // a separate list
                index = i;
                goto field_index;
            }
            else
            {
                return node::pointer_t();
            }
        }
        else if(f_current->is(node_type_t::PERIOD))
        {
            // skip the '.'
            next();

            if(!f_current->is(node_type_t::IDENTIFIER))
            {
                error::instance() << f_current->get_position()
                        << "only an identifier is expected after a '.'."
                        << error_mode_t::ERROR_ERROR;
                return node::pointer_t();
            }
            index = f_current;

            // skip the index (identifier)
            next();

field_index:
            if(result->is(node_type_t::LIST))
            {
                // in this case the index is a string/identifier
                std::string const idx(index->get_string());
                // TODO: what are we indexing against?
                error::instance() << f_current->get_position()
                        << "'map[<string|identifier>]' not yet supported."
                        << error_mode_t::ERROR_ERROR;
                return node::pointer_t();
            }
            else
            {
                error::instance() << f_current->get_position()
                        << "unsupported left handside type "
                        << result->get_type()
                        << " for the 'map[<string|identifier>]' operation."
                        << error_mode_t::ERROR_ERROR;
                return node::pointer_t();
            }
        }
        else
        {
            break;
        }
    }

    return result;
}

node::pointer_t expression::unary()
{
    // unary: IDENTIFIER
    //      | INTEGER
    //      | DECIMAL_NUMBER
    //      | STRING
    //      | PERCENT
    //      | HASH (-> COLOR)
    //      | UNICODE_RANGE
    //      | FUNCTION argument_list ')' -- including url()
    //      | '(' expression_list ')'
    //      | '+' power
    //      | '-' power
    //      | '!' power

    switch(f_current->get_type())
    {
    case node_type_t::ARRAY:
    case node_type_t::DECIMAL_NUMBER:
    case node_type_t::EXCLAMATION:  // this is not a BOOLEAN NOT operator...
    case node_type_t::INTEGER:
    case node_type_t::MAP:
    case node_type_t::NULL_TOKEN:
    case node_type_t::PERCENT:
    case node_type_t::STRING:
    case node_type_t::UNICODE_RANGE:
    case node_type_t::URL:
        {
            node::pointer_t result(f_current);
            // skip that token
            next();
            return result;
        }

    case node_type_t::FUNCTION:
        {
            node::pointer_t func(f_current);

            // skip the '<func>('
            next();

            // calculate the arguments
            parser::argify(func);
            if(func->get_string() != "calc"
            && func->get_string() != "expression")
            {
                expression args_expr(func, true);
                args_expr.compile_args(false);
            }
            //else -- we may want to verify the calculations, but
            //        we cannot compile those

            return excecute_function(func);
        }
        break;

    case node_type_t::OPEN_PARENTHESIS:
        {
            // skip the '('
            next();

            // calculate the result of the sub-expression
            expression group(f_current, true);
            group.next();
            return group.expression_list();
        }

    case node_type_t::ADD:
        // completely ignore the '+' because we assume that
        // '+<anything>' <=> '<anything>'

        // skip the '+'
        next();
        return power();

    case node_type_t::SUBTRACT:
        {
            // skip the '-'
            next();

            node::pointer_t result(power());
            switch(result->get_type())
            {
            case node_type_t::INTEGER:
                result->set_integer(-result->get_integer());
                return result;

            case node_type_t::DECIMAL_NUMBER:
            case node_type_t::PERCENT:
                result->set_decimal_number(-result->get_decimal_number());
                return result;

            default:
                error::instance() << f_current->get_position()
                        << "unsupported type "
                        << result->get_type()
                        << " for operator '-'."
                        << error_mode_t::ERROR_ERROR;
                return node::pointer_t();

            }
        }
        /*NOTREACHED*/

    // This is not too good, we actually transform the !important in
    // one 'EXCLAMATION + string' node; use the not(...) instead
    //case node_type_t::EXCLAMATION:
    //    {
    //        // skip the '!'
    //        next();
    //        node::pointer_t result(power());
    //        int const r(boolean(result));
    //        if(r < 0)
    //        {
    //            return node::pointer_t();
    //        }
    //        // make sure the result is a boolean
    //        if(!result->is(node_type_t::BOOLEAN))
    //        {
    //            result.reset(new node(node_type_t::BOOLEAN, result->get_position()));
    //        }
    //        result->set_boolean(r == 0 ? true : false);
    //        return result;
    //    }
    //    /*NOTREACHED*/

    case node_type_t::HASH:
        // a '#...' in an expression is expected to be a valid color
        {
            color hash;
            if(!hash.set_color(f_current->get_string()))
            {
                error::instance() << f_current->get_position()
                        << "the color in #"
                        << f_current->get_string()
                        << " is not valid."
                        << error_mode_t::ERROR_ERROR;

                // skip the HASH
                next();
                return node::pointer_t();
            }
            node::pointer_t color_node(new node(node_type_t::COLOR, f_current->get_position()));
            color_node->set_color(hash);

            // skip the HASH
            next();
            return color_node;
        }
        /*NOTREACHED*/

    case node_type_t::IDENTIFIER:
        // an identifier may represent a color, null, true, or false
        {
            node::pointer_t result(f_current);
            // skip the IDENTIFIER
            next();

            std::string const identifier(result->get_string());
            if(identifier == "null")
            {
                return node::pointer_t(new node(node_type_t::NULL_TOKEN, result->get_position()));
            }
            if(identifier == "true")
            {
                node::pointer_t b(new node(node_type_t::BOOLEAN, result->get_position()));
                b->set_boolean(true);
                return b;
            }
            if(identifier == "false")
            {
                // a boolean is false by default, so no need to set the value
                return node::pointer_t(new node(node_type_t::BOOLEAN, result->get_position()));
            }
            color col;
            if(!col.set_color(identifier))
            {
                // it is not a color, return as is
                return result;
            }
            node::pointer_t color_node(new node(node_type_t::COLOR, result->get_position()));
            color_node->set_color(col);

            return color_node;
        }
        /*NOTREACHED*/

    default:
        error::instance() << f_current->get_position()
                << "unsupported type "
                << f_current->get_type()
                << " as a unary expression token."
                << error_mode_t::ERROR_ERROR;
        return node::pointer_t();

    }
}

} // namespace csspp

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
