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
 * \brief Implementation of the CSS Preprocessor compiler.
 *
 * The CSS Preprocessor compiler applies the script rules and transform
 * the tree of nodes so it can be output as standard CSS.
 *
 * \sa \ref compiler_reference
 */

#include "csspp/compiler.h"

#include "csspp/exceptions.h"
#include "csspp/nth_child.h"
#include "csspp/parser.h"
//#include "csspp/unicode_range.h"

#include <cmath>
#include <fstream>
#include <iostream>

#include <unistd.h>

namespace csspp
{

namespace
{

class expression_t
{
public:
    // no current on startup
    expression_t(node::pointer_t n, bool skip_whitespace)
        : f_node(n)
        , f_skip_whitespace(skip_whitespace)
    {
        if(!f_node)
        {
            throw csspp_exception_logic("compiler.cpp:expression_t(): contructor called with a null pointer.");
        }
    }

    // basic state handling
    void mark_start()
    {
        f_start = f_pos;
    }

    void replace_with_result(node::pointer_t result)
    {
        if(f_start == static_cast<size_t>(-1))
        {
            throw csspp_exception_logic("compiler.cpp:expression_t(): replace_with_result() cannot be called if mark_start() was never called.");
        }

        // f_pos may point to a tag right after the end of the previous
        // expression; expressions may be separated by WHITESPACE tokens
        // too so we have to restore them if they appear at the end of
        // the epxression we just worked on (i.e. we cannot eat a WHITESPACE
        // in an expression.)
        if(!f_current->is(node_type_t::EOF_TOKEN) && f_pos > 0)
        {
            --f_pos;
            if(f_node->get_child(f_pos)->is(node_type_t::WHITESPACE))
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
        f_start = f_pos;
    }

    void next()
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

    node::pointer_t current() const
    {
        return f_current;
    }

    static boolean_t boolean(node::pointer_t n)
    {
        boolean_t const result(n->to_boolean());
        if(result == boolean_t::INVALID)
        {
            error::instance() << n->get_position()
                    << "a boolean expression was expected."
                    << error_mode_t::ERROR_ERROR;
        }
        return result;
    }

    // expression parsing
    node::pointer_t expression_list()
    {
        node::pointer_t result(assignment());
        if(!result)
        {
            return node::pointer_t();
        }

        if(f_current->is(node_type_t::COMMA))
        {
            // create a list only if we have at least one comma?
            node::pointer_t item(result);
            result.reset(new node(node_type_t::LIST, item->get_position()));
            result->add_child(item);

            while(f_current->is(node_type_t::COMMA))
            {
                // skip the ','
                next();

                item = assignment();
                result->add_child(item);
            }
        }

        return result;
    }

    node::pointer_t assignment()
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

    node::pointer_t conditional()
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

    node::pointer_t logical_or()
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

    node::pointer_t logical_and()
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

    node_type_t equality_operator(node::pointer_t n)
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
                << " for operator '==', '!=', '<=', or '>='."
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
            error::instance() << f_current->get_position()
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
            throw csspp_exception_logic("compiler.cpp:include_match(): called with an invalid operator."); // LCOV_EXCL_LINE

        }

        return l.find(s) != std::string::npos;
    }

    node::pointer_t equality()
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
                throw csspp_exception_logic("compiler.cpp:equality(): unexpected operator in 'op'."); // LCOV_EXCL_LINE

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

    node::pointer_t relational()
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
                throw csspp_exception_logic("compiler.cpp:relational(): unexpected operator in 'op'."); // LCOV_EXCL_LINE

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
            if(subtract)
            {
                error::instance() << f_current->get_position()
                        << "incompatible types between "
                        << lhs->get_type()
                        << " and "
                        << rhs->get_type()
                        << " for operator '-'."
                        << error_mode_t::ERROR_ERROR;
                return node::pointer_t();
            }
            // string concatenation
            lhs->set_string(lhs->get_string() + rhs->get_string());
            return lhs;

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

                error::instance() << f_current->get_position()
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

                error::instance() << f_current->get_position()
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

    node::pointer_t additive()
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

    node_type_t multiplicative_operator(node::pointer_t n)
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

    node::pointer_t multiply(node_type_t op, node::pointer_t lhs, node::pointer_t rhs)
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

        case mix_node_types(node_type_t::DECIMAL_NUMBER, node_type_t::DECIMAL_NUMBER):
            // TODO: test that the dimensions are compatible
            ai = lhs->get_integer();
            bi = rhs->get_integer();
            type = node_type_t::DECIMAL_NUMBER;
            break;

        case mix_node_types(node_type_t::PERCENT, node_type_t::PERCENT):
            ai = lhs->get_integer();
            bi = rhs->get_integer();
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

        node::pointer_t result(new node(type, lhs->get_position()));
        if(type != node_type_t::PERCENT)
        {
            // do not lose the dimension
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
                throw csspp_exception_logic("compiler.cpp:multiply(): unexpected operator."); // LCOV_EXCL_LINE

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
                throw csspp_exception_logic("compiler.cpp:multiply(): unexpected operator."); // LCOV_EXCL_LINE

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

    node::pointer_t multiplicative()
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

    node::pointer_t apply_power(node::pointer_t lhs, node::pointer_t rhs)
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

    node::pointer_t power()
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

    node::pointer_t post()
    {
        // post: unary
        //     | post '[' expression ']'
        //     | post '.' IDENTIFIER

        node::pointer_t result(unary());
        if(!result)
        {
            return node::pointer_t();
        }

        for(;;)
        {
            if(f_current->is(node_type_t::OPEN_SQUAREBRACKET))
            {
                // compile the index expression
                expression_t index(f_current, true);
                index.next();
                node::pointer_t i(index.expression_list());
                if(!i)
                {
                    return node::pointer_t();
                }

                // skip the '['
                next();

                if(i->is(node_type_t::INTEGER))
                {
                    if(result->is(node_type_t::LIST))
                    {
                        // index is 1 based (not like in C/C++)
                        integer_t const idx(i->get_integer());
                        if(static_cast<size_t>(idx) > result->size())
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
                        result = result->get_child(idx - 1);
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
                    f_current = i;
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

field_index:
                if(result->is(node_type_t::LIST))
                {
                    // index is 1 based (not like in C/C++)
                    std::string idx(f_current->get_string());
                    // TODO: what are we indexing against?
                    error::instance() << f_current->get_position()
                            << "unsupported type "
                            << result->get_type()
                            << " for the 'map[<string>]' operation."
                            << error_mode_t::ERROR_ERROR;
                    return node::pointer_t();
                }
                else
                {
                    error::instance() << f_current->get_position()
                            << "unsupported type "
                            << result->get_type()
                            << " for the 'map[<string>]' operation."
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

    node::pointer_t unary()
    {
        // unary: IDENTIFIER
        //      | INTEGER
        //      | DECIMAL_NUMBER
        //      | STRING
        //      | PERCENT
        //      | FUNCTION expression_list ')'
        //      | '(' expression_list ')'
        //      | '+' power
        //      | '-' power
        //      | '!' power

        switch(f_current->get_type())
        {
        case node_type_t::IDENTIFIER:
        case node_type_t::INTEGER:
        case node_type_t::DECIMAL_NUMBER:
        case node_type_t::STRING:
        case node_type_t::PERCENT:
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
                expression_t args_expr(f_current, true);
                args_expr.next();
                node::pointer_t args(args_expr.expression_list());

                return excecute_function(func, args);
            }
            break;

        case node_type_t::OPEN_PARENTHESIS:
            {
                // skip the '('
                next();

                // calculate the result of the sub-expression
                expression_t group(f_current, true);
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

        case node_type_t::EXCLAMATION:
            {
                // skip the '!'
                next();

                node::pointer_t result(power());
                int const r(boolean(result));
                if(r < 0)
                {
                    return node::pointer_t();
                }
                // make sure the result is a boolean
                if(!result->is(node_type_t::BOOLEAN))
                {
                    result.reset(new node(node_type_t::BOOLEAN, result->get_position()));
                }
                result->set_boolean(r == 0 ? true : false);
                return result;
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

    node::pointer_t excecute_function(node::pointer_t func, node::pointer_t args)
    {
        std::string const function_name(func->get_string());

        if(function_name == "if")
        {
            // if(condition, if-true, if-false)
            if(args->size() == 3)
            {
                int const r(boolean(args->get_child(0)));
                if(r == 0 || r == 1)
                {
                    return args->get_child(r + 1);
                }
            }
            else
            {
                error::instance() << f_current->get_position()
                        << "if() expects exactly 3 arguments."
                        << error_mode_t::ERROR_ERROR;
            }
            return node::pointer_t();
        }

        // "unknown" functions have to be left alone since these maybe
        // CSS functions that we do not want to transform (we already
        // worked on their arguments.)
        return func;
    }

private:
    typedef std::map<std::string, node::pointer_t>  variable_vector_t;

    node::pointer_t     f_node;
    size_t              f_pos = 0;
    size_t              f_start = static_cast<size_t>(-1);
    node::pointer_t     f_current;
    variable_vector_t   f_variables;
    bool                f_skip_whitespace = false;
};

} // no name namespace

class safe_parents_t
{
public:
    safe_parents_t(compiler::compiler_state_t & state, node::pointer_t n)
        : f_state(state)
    {
        f_state.push_parent(n);
    }

    ~safe_parents_t()
    {
        f_state.pop_parent();
    }

private:
    compiler::compiler_state_t &     f_state;
};

class safe_compiler_state_t
{
public:
    safe_compiler_state_t(compiler::compiler_state_t & state)
        : f_state(state)
        , f_state_copy(state)
    {
    }

    ~safe_compiler_state_t()
    {
        f_state = f_state_copy;
    }

private:
    compiler::compiler_state_t &    f_state;
    compiler::compiler_state_t      f_state_copy;
};

void compiler::compiler_state_t::set_root(node::pointer_t root)
{
    f_root = root;
    f_parents.clear();
}

node::pointer_t compiler::compiler_state_t::get_root() const
{
    return f_root;
}

void compiler::compiler_state_t::push_parent(node::pointer_t parent)
{
    f_parents.push_back(parent);
}

void compiler::compiler_state_t::pop_parent()
{
    f_parents.pop_back();
}

bool compiler::compiler_state_t::empty_parents() const
{
    return f_parents.empty();
}

node::pointer_t compiler::compiler_state_t::get_current_parent() const
{
    if(f_parents.empty())
    {
        throw csspp_exception_logic("compiler.cpp:compiler::compiler_state_t::get_current_parent(): no parents available."); // LCOV_EXCL_LINE
    }

    return f_parents.back();
}

node::pointer_t compiler::compiler_state_t::get_previous_parent() const
{
    if(f_parents.size() < 2)
    {
        throw csspp_exception_logic("compiler.cpp:compiler::compiler_state_t::get_current_parent(): no previous parents available."); // LCOV_EXCL_LINE
    }

    // return the parent before last
    return f_parents[f_parents.size() - 2];
}

node::pointer_t compiler::compiler_state_t::find_parent_by_type(node_type_t type) const
{
    size_t pos(f_parents.size());
    while(pos > 0)
    {
        --pos;
        if(f_parents[pos]->is(type))
        {
            return f_parents[pos];
        }
    }

    return node::pointer_t();
}

node::pointer_t compiler::compiler_state_t::find_parent_by_type(node_type_t type, node::pointer_t starting_here) const
{
    size_t pos(f_parents.size());
    while(pos > 0)
    {
        --pos;
        if(f_parents[pos] == starting_here)
        {
            break;
        }
    }

    while(pos > 0)
    {
        --pos;
        if(f_parents[pos]->is(type))
        {
            return f_parents[pos];
        }
    }

    return node::pointer_t();
}

compiler::compiler(bool validating)
    : f_compiler_validating(validating)
{
    f_paths.push_back("/usr/lib/csspp/scripts");
}

node::pointer_t compiler::get_root() const
{
    return f_state.get_root();
}

void compiler::set_root(node::pointer_t root)
{
    f_state.set_root(root);
}

void compiler::clear_paths()
{
    f_paths.clear();
}

void compiler::add_path(std::string const & path)
{
    f_paths.push_back(path);
}

void compiler::compile()
{
    // before we compile anything we want to transform all the variables
    // with their verbatim contents; otherwise the compiler would be way
    // more complex for nothing...
    //
    // also for the variables to work appropriately, we immediately handle
    // the @import and @mixins since both may define additional variables.
    //
    replace_variables(f_state.get_root());
    if(!f_state.empty_parents())
    {
        throw csspp_exception_logic("compiler.cpp: the stack of parents must always be empty before compile() returns"); // LCOV_EXCL_LINE
    }

    compile(f_state.get_root());
    if(!f_state.empty_parents())
    {
        throw csspp_exception_logic("compiler.cpp: the stack of parents must always be empty before compile() returns"); // LCOV_EXCL_LINE
    }
}

void compiler::compile(node::pointer_t n)
{
    safe_parents_t safe_parents(f_state, n);

    switch(n->get_type())
    {
    case node_type_t::LIST:
        // transparent item, just compile all the children
        {
            size_t idx(0);
            while(idx < n->size())
            {
                node::pointer_t child(n->get_child(idx));
                compile(child);

                // the child may replace itself with something else
                // in which case we do not want the ++idx
                if(idx < n->size()
                && n->get_child(idx) == child)
                {
                    ++idx;
                }
            }
            // TODO: remove LIST if it now is empty or has 1 item
        }
        break;

    case node_type_t::COMPONENT_VALUE:
        compile_component_value(n);
        break;

    case node_type_t::COMMENT:
        // passthrough tokens
        break;

    case node_type_t::AT_KEYWORD:
        compile_at_keyword(n);
        break;

    default:
        {
            std::stringstream ss;
            ss << "unexpected token (type: " << n->get_type() << ") in compile().";
            throw csspp_exception_unexpected_token(ss.str());
        }

    }
}

void compiler::compile_component_value(node::pointer_t n)
{
    // there are quite a few cases to handle here:
    //
    //   $variable ':' '{' ... '}'
    //   <field-prefix> ':' '{' ... '}'
    //   <selector-list> '{' ... '}'
    //   $variable ':' ...
    //   <field-name> ':' ...
    //

    if(n->empty())
    {
        // we have a problem, we should already have had an error
        // somewhere? (TBD)
        // I think we can get those if all there was in a component
        // value were variables
        return;
    }

    // $variable ':' '{' ... '}'
    if(parser::is_variable_set(n, true))
    {
        throw csspp_exception_logic("compiler.cpp: somehow a variable definition was found while compiling (1)."); // LCOV_EXCL_LINE
    }

    // <field-prefix> ':' '{' ... '}'
    if(parser::is_nested_declaration(n))
    {
        compile_declaration(n);
        return;
    }

    // <selector-list> '{' ... '}'
    if(n->get_last_child()->is(node_type_t::OPEN_CURLYBRACKET))
    {
        // this is a selector list followed by a block of
        // definitions and sub-blocks
        compile_qualified_rule(n);
        return;
    }

    // $variable ':' ... ';'
    if(parser::is_variable_set(n, false))
    {
        throw csspp_exception_logic("compiler.cpp: somehow a variable definition was found while compiling (1)."); // LCOV_EXCL_LINE
    }

    // <field-name> ':' ...
    compile_declaration(n);
}

void compiler::compile_qualified_rule(node::pointer_t n)
{
    // so here we have a list of selectors, that means we can verify
    // that said list is valid (i.e. binary operators are used properly,
    // only valid operators were used, etc.)

    // any selectors?
    if(n->size() <= 1)
    {
        error::instance() << n->get_position()
                << "a qualified rule without selectors is not valid."
                << error_mode_t::ERROR_ERROR;
        return;
    }

    // compile the selectors using a node parser
    // \ref selectors_rules#grammar
    if(!parse_selector(n))
    {
        // an error occurred, forget this entry and move on
        return;
    }
}

void compiler::compile_declaration(node::pointer_t n)
{
    // first make sure we have a declaration
    // (i.e. IDENTIFIER WHITESPACE ':' ...)
    //
    node::pointer_t identifier(n->get_child(0));
    if(!identifier->is(node_type_t::IDENTIFIER))
    {
        error::instance() << n->get_position()
                << "expected an identifier to start a declaration value; got a: " << n->get_type() << " instead."
                << error_mode_t::ERROR_ERROR;
        return;
    }

    // the WHITESPACE is optional, if present, remove it
    node::pointer_t next(n->get_child(1));
    if(next->is(node_type_t::WHITESPACE))
    {
        n->remove_child(1);
        next = n->get_child(1);
    }

    // now we must have a COLON, also remove that COLON
    if(!next->is(node_type_t::COLON))
    {
        error::instance() << n->get_position()
                << "expected a ':' after the identifier of this declaration value; got a: " << n->get_type() << " instead."
                << error_mode_t::ERROR_ERROR;
        return;
    }
    n->remove_child(1);

    // create a declaration to replace the identifier
    node::pointer_t declaration(new node(node_type_t::DECLARATION, n->get_position()));
    declaration->set_string(identifier->get_string());

    // copy the following children as the children of the declaration
    // (i.e. identifier is element 0, so we copy elements 1 to n)
    size_t const max_children(n->size());
    for(size_t i(1); i < max_children; ++i)
    {
        declaration->add_child(n->get_child(i));
    }

    // now replace that identifier by its declaration in the parent
    f_state.get_current_parent()->replace_child(identifier, declaration);
}

void compiler::compile_at_keyword(node::pointer_t n)
{
    std::string const at(n->get_string());

    // @<id> [expression] '{' ... '}'
    //
    // Note that the expression is optional and we do not want to
    // "compile" if no expression is defined
    node::pointer_t expr;
    if(n->size() > 0
    && !n->get_child(0)->is(node_type_t::OPEN_CURLYBRACKET))
    {
        if(at == "else"
        && n->get_child(0)->is(node_type_t::IDENTIFIER)
        && n->get_child(0)->get_string() == "if")
        {
            // this is a very special case of the:
            //    @else if expr '{' ... '}'
            n->remove_child(0);
            if(!n->empty() && n->get_child(0)->is(node_type_t::WHITESPACE))
            {
                // this should always happen because otherwise we are missing
                // the actual expression!
                n->remove_child(0);
            }
            if(n->size() == 1)
            {
                error::instance() << n->get_position()
                        << "'@else if ...' is missing an expression or a block"
                        << error_mode_t::ERROR_ERROR;
                return;
            }
        }
        expr = compile_expression(n, true);
    }

//std::cerr << "------ RESULT:\n" << *expr;

    //n->replace_child(n->get_child(0), expr);

    if(at == "error")
    {
        error::instance() << n->get_position()
                << (expr ? expr->to_string() : std::string("@error reached"))
                << error_mode_t::ERROR_ERROR;
        return;
    }

    if(n->get_string() == "warning")
    {
        error::instance() << n->get_position()
                << (expr ? expr->to_string() : std::string("@warning reached"))
                << error_mode_t::ERROR_WARNING;
        return;
    }

    if(n->get_string() == "info"
    || n->get_string() == "message")
    {
        error::instance() << n->get_position()
                << (expr ? expr->to_string() : std::string("@message reached"))
                << error_mode_t::ERROR_INFO;
        return;
    }

    if(n->get_string() == "debug")
    {
        error::instance() << n->get_position()
                << (expr ? expr->to_string() : std::string("@debug reached"))
                << error_mode_t::ERROR_DEBUG;
        return;
    }

    if(n->get_string() == "if")
    {
        // find this @-keyword in the parent and remove it
        node::pointer_t parent(f_state.get_previous_parent());
        size_t idx(parent->child_position(n));
        if(idx == node::npos)
        {
            throw csspp_exception_logic("compiler.cpp: somehow a child node was not found in its parent."); // LCOV_EXCL_LINE
        }
        parent->remove_child(idx);

        // make sure that we got a valid syntax
        if(n->size() != 2 || !expr)
        {
            error::instance() << n->get_position()
                    << "@if is expected to have exactly 2 parameters: an expression and a block. This @if has "
                    << static_cast<int>(n->size())
                    << " parameters."
                    << error_mode_t::ERROR_ERROR;
            return;
        }

        boolean_t const r(expression_t::boolean(expr));
        if(r == boolean_t::TRUE)
        {
            // TRUE, we need the data which we put in the stream
            // at the position of the @if as if the @if and
            // expression never existed
            node::pointer_t block(n->get_child(1));
            size_t const max_children(block->size());
            for(size_t j(0); j < max_children; ++j, ++idx)
            {
                parent->insert_child(idx, block->get_child(j));
            }
        }

        // FALSE or INVALID, we remove the block to avoid
        // executing it since we do not know whether it should
        // be executed or not; also we mark the next block as
        // "true" if it is an '@else' or '@else if'
        if(idx < parent->size())
        {
            node::pointer_t next(parent->get_child(idx));
            if(next->is(node_type_t::AT_KEYWORD)
            && next->get_string() == "else")
            {
                // mark that the @else is at the right place
                // (i.e. an @else with integer == 0 is an error)
                next->set_integer(r == boolean_t::TRUE ? 2 : 1);
            }
        }

        return;
    }

    if(n->get_string() == "else")
    {
        // find this @-keyword in the parent and remove it
        node::pointer_t parent(f_state.get_previous_parent());
        size_t idx(parent->child_position(n));
        if(idx == node::npos)
        {
            throw csspp_exception_logic("compiler.cpp: somehow a child node was not found in its parent."); // LCOV_EXCL_LINE
        }
        parent->remove_child(idx);

        // if this @else is not marked with a 1 or 2 then there was no @if
        // or @else if ... before it
        int status(n->get_integer());
        if(status == 0)
        {
            error::instance() << n->get_position()
                    << "a standalone @else is not legal, it has to be preceeded by a @if ... or @else if ..."
                    << error_mode_t::ERROR_ERROR;
            return;
        }

        // when the @if (or last @else if) was FALSE, we are TRUE
        // however, if the status is 2, that means one of the previous
        // @if or @else if was TRUE so everything else is FALSE
        boolean_t r(status == 1 ? boolean_t::TRUE : boolean_t::FALSE);
        if(n->size() != 1)
        {
            if(n->size() != 2 || !expr)
            {
                error::instance() << n->get_position()
                        << "'@else { ... }' is expected to have 1 parameter, '@else if ... { ... }' is expected to have 2 parameters. This @else has "
                        << static_cast<int>(n->size())
                        << " parameters."
                        << error_mode_t::ERROR_ERROR;
                return;
            }

            // as long as status == 1 we have not yet found a match
            // (i.e. the @if was false and any @else if were all false
            // so far) so we check the expression of this very @else if
            // to know whether to go on or not
            if(status == 1)
            {
                r = expression_t::boolean(expr);
            }
        }

        if(r == boolean_t::TRUE)
        {
            status = 2;

            // TRUE, we need the data which we put in the stream
            // at the position of the @if as if the @if and
            // expression never existed
            node::pointer_t block(n->get_child(n->size() == 1 ? 0 : 1));
            size_t const max_children(block->size());
            for(size_t j(0); j < max_children; ++j, ++idx)
            {
                parent->insert_child(idx, block->get_child(j));
            }
        }

        // FALSE or INVALID, we remove the block to avoid
        // executing it since we do not know whether it should
        // be executed or not; also we mark the next block as
        // "true" if it is an '@else' or '@else if'
        if(idx < parent->size())
        {
            node::pointer_t next(parent->get_child(idx));
            if(next->is(node_type_t::AT_KEYWORD)
            && next->get_string() == "else")
            {
                if(n->size() == 1)
                {
                    error::instance() << n->get_position()
                            << "'@else { ... }' cannot follow another '@else { ... }'. Maybe you are missing an 'if expr'?"
                            << error_mode_t::ERROR_ERROR;
                    return;
                }

                // mark that the @else is at the right place and whether
                // it may be TRUE (1) or not (2); our status already shows
                // what it can be:
                next->set_integer(status);
            }
        }

        return;
    }
}

node::pointer_t compiler::compile_expression(node::pointer_t n, bool skip_whitespace)
{
    // expression: conditional
    expression_t expr(n, skip_whitespace);
    expr.mark_start();
    expr.next();
    node::pointer_t result(expr.conditional());
    if(result)
    {
        expr.replace_with_result(result);
    }
    return result;
}

void compiler::replace_import(node::pointer_t n, size_t & idx)
{
    // node 'n' is the @import itself
    //
    // the name of the script is whatever follows the @import which
    // has to be exactly one string
    //
    // (TBD--would an identifier be valid here?)
    //

    // an @import may have a list of filenames to import
    // so we have to argify() and then manage each ARG one by one

    node::pointer_t import(n->get_child(0));

    argify(import);

    node::pointer_t list(new node(node_type_t::LIST, n->get_position()));

    size_t const max_children(import->size());
    for(size_t i(0); i < max_children; ++i)
    {
        node::pointer_t arg(import->get_child(i));

        // we only support arguments with one string
        if(arg->size() != 1)
        {
            // leave it in the final CSS; definitly not for us
            continue;
        }

        // make sure we indeed have a string
        node::pointer_t child(n->get_child(0));
        if(!child->is(node_type_t::STRING))
        {
            // TODO: is this an error?
            continue;
        }

        // TODO: add code to avoid testing with filenames that represent URIs

        // search the corresponding file
        std::string const script_name(child->get_string());
        std::string filename(find_file(script_name));
        if(filename.empty())
        {
            if(script_name.substr(script_name.size() - 5) != ".scss")
            {
                // try again with the "scss" extension
                filename = find_file(script_name + ".scss");
            }
        }

        // still not found, ignore
        if(filename.empty())
        {
            continue;
        }

        // remove that entry since we are managing it
        import->remove_child(i);
        --i;

        // position object for this file
        position pos(filename);

        // TODO: do the necessary to avoid multiple @import

        // we found a file, load it and return it
        std::ifstream in;
        in.open(filename);
        if(!in)
        {
            // the script may not really allow reading even though
            // access() just told us otherwise
            error::instance() << pos
                    << "validation script \""
                    << script_name
                    << "\" could not be opened."
                    << error_mode_t::ERROR_FATAL;
        }
        else
        {
            // the file got loaded, parse it and return the root node
            lexer::pointer_t l(new lexer(in, pos));
            parser p(l);
            list->add_child(p.stylesheet());
        }
    }

    if(import->empty())
    {
        // this was CSS Preprocessor @import files only so we want to
        // remove it from the parent node
        n->remove_child(import);
    }
    else
    {
        // skip the import, it was managed
        ++idx;
    }

    if(!list->empty())
    {
        size_t const max_results(list->size());
        for(size_t i(0), j(idx); i < max_results; ++i, ++j)
        {
            n->insert_child(j, list->get_child(i));
        }
    }
}

void compiler::handle_mixin(node::pointer_t n)
{
    if(n->size() != 2)
    {
        error::instance() << n->get_position()
                << "a @mixin definition expects exactly two parameters: an identifier or function and a {}-block."
                << error_mode_t::ERROR_ERROR;
        return;
    }

    node::pointer_t block(n->get_child(1));
    if(block->is(node_type_t::OPEN_CURLYBRACKET))
    {
        error::instance() << n->get_position()
                << "a @mixin definition expects a {}-block as its second parameter."
                << error_mode_t::ERROR_ERROR;
        return;
    }

    node::pointer_t name(n->get_child(0));
    if(name->is(node_type_t::IDENTIFIER))
    {
        // this is just like a variable
        //
        // search the parents for the node where the variable will be set
        node::pointer_t var_holder(f_state.find_parent_by_type(node_type_t::OPEN_CURLYBRACKET));
        if(!var_holder)
        {
            var_holder = f_state.get_root();
        }

        // save the variable
        var_holder->set_variable(name->get_string(), block);
    }
    else if(name->is(node_type_t::FUNCTION))
    {
        // this is a function declaration, it includes a list of arguments
        // which we want to check, and if valid we save it in the root node
        argify(name);

        size_t const max_children(name->size());
        for(size_t idx(0); idx < max_children; ++idx)
        {
            node::pointer_t arg(name->get_child(idx));
            if(!arg->is(node_type_t::ARG))
            {
                error::instance() << n->get_position()
                        << "a @mixin list of arguments is expected to be only ARG objects."
                        << error_mode_t::ERROR_ERROR;
                return;
            }
            if(arg->size() != 1)
            {
                error::instance() << n->get_position()
                        << "a @mixin list of arguments is expected to be composed of exactly one identifier per argument."
                        << error_mode_t::ERROR_ERROR;
                return;
            }
            node::pointer_t a(arg->get_child(0));
            if(!a->is(node_type_t::IDENTIFIER))
            {
                error::instance() << n->get_position()
                        << "a @mixin list of arguments is expected to be composed of identifiers only."
                        << error_mode_t::ERROR_ERROR;
                return;
            }
            std::string const arg_name(a->get_string());
            if(arg_name.length() > 3
            && arg_name.substr(arg_name.length() - 3) == "..."
            && idx + 1 != max_children)
            {
                error::instance() << n->get_position()
                        << "only the last identifier of a @mixin list of arguments may end with '...'."
                        << error_mode_t::ERROR_ERROR;
                return;
            }
        }
    }
    else
    {
        error::instance() << n->get_position()
                << "a @mixin expects either an identifier or a function as its first parameter."
                << error_mode_t::ERROR_ERROR;
    }
}

void compiler::replace_variables(node::pointer_t n)
{
    safe_parents_t safe_parents(f_state, n);

    switch(n->get_type())
    {
    case node_type_t::AT_KEYWORD:
    case node_type_t::ARG:
    case node_type_t::COMPONENT_VALUE:
    case node_type_t::DECLARATION:
    case node_type_t::FUNCTION:
    case node_type_t::LIST:
    case node_type_t::OPEN_CURLYBRACKET:
    case node_type_t::OPEN_PARENTHESIS:
    case node_type_t::OPEN_SQUAREBRACKET:
        {
            // handle a special case which SETs a variable and cannot
            // get the first $<var> replaced
            bool is_variable_set(n->get_type() == node_type_t::COMPONENT_VALUE
                              && parser::is_variable_set(n, false));

            // replace all $<var> references with the corresponding value
            size_t idx(is_variable_set ? 1 : 0);
            while(idx < n->size())
            {
                node::pointer_t child(n->get_child(idx));
                if(child->is(node_type_t::VARIABLE))
                {
                    n->remove_child(idx);

                    // search for the variable and replace this 'child' with
                    // the contents of the variable
                    node::pointer_t value(get_variable(child));
                    switch(value->get_type())
                    {
                    case node_type_t::LIST:
                    case node_type_t::OPEN_CURLYBRACKET:
                    case node_type_t::OPEN_PARENTHESIS:
                    case node_type_t::OPEN_SQUAREBRACKET:
                        // in this case we insert the children of 'value'
                        // instead of the value itself
                        {
                            size_t const max_children(value->size());
                            for(size_t j(0), i(idx); j < max_children; ++j, ++i)
                            {
                                n->insert_child(i, value->get_child(j));
                            }
                        }
                        break;

                    case node_type_t::WHITESPACE:
                        // whitespaces by themselves do not get re-included,
                        // which may be a big mistake but at this point
                        // it seems wise to do so
                        break;

                    default:
                        n->insert_child(idx, value);
                        break;

                    }
                }
                else
                {
                    // recursive call to handle all children in the
                    // entire tree
                    switch(child->get_type())
                    {
                    case node_type_t::AT_KEYWORD:
                    case node_type_t::ARG:
                    case node_type_t::COMPONENT_VALUE:
                    case node_type_t::DECLARATION:
                    case node_type_t::FUNCTION:
                    case node_type_t::LIST:
                    case node_type_t::OPEN_CURLYBRACKET:
                    case node_type_t::OPEN_PARENTHESIS:
                    case node_type_t::OPEN_SQUAREBRACKET:
                        replace_variables(child);

                        // handle @import and @mixins from the parent node
                        if(child->is(node_type_t::AT_KEYWORD))
                        {
                            std::string const at_keyword(child->get_string());
                            if(at_keyword == "import")
                            {
                                replace_import(n, idx);
                            }
                            else if(at_keyword == "mixin")
                            {
                                // mixins are handled like variables or
                                // function declarations, so we always
                                // remove them
                                //
                                n->remove_child(child);

                                handle_mixin(child);
                            }
                            else
                            {
                                ++idx;
                            }
                        }
                        else
                        {
                            ++idx;
                        }
                        break;

                    default:
                        ++idx;
                        break;

                    }
                }
            }
            // TODO: remove lists that become empty?

            // handle the special case of a variable assignment
            if(is_variable_set)
            {
                // this is enough to get the variable removed
                // from COMPONENT_VALUE
                set_variable(n->get_child(0));
            }
        }
        break;

    default:
        // other nodes are not of interest here
        break;

    }
}

void compiler::set_variable(node::pointer_t n)
{
    // WARNING: 'n' is still the COMPONENT_VALUE and not the $var

    // a variable gets removed from the tree and its current value
    // saved in a parent node that is an OPEN_CURLYBRACKET or the
    // root node if no OPEN_CURLYBRACKET is found in the parents

    f_state.get_previous_parent()->remove_child(n);

    node::pointer_t var(n->get_child(0));
    std::string const & variable_name(var->get_string());

    n->remove_child(0);     // remove the VARIABLE
    if(n->get_child(0)->is(node_type_t::WHITESPACE))
    {
        n->remove_child(0); // remove the WHITESPACE
    }
    if(!n->get_child(0)->is(node_type_t::COLON))
    {
        throw csspp_exception_logic("compiler.cpp: somehow a variable set is not exactly IDENTIFIER WHITESPACE* ':'."); // LCOV_EXCL_LINE
    }
    n->remove_child(0);     // remove the COLON

    // rename the node from COMPONENT_VALUE to a plain LIST
    node::pointer_t list(new node(node_type_t::LIST, n->get_position()));
    list->take_over_children_of(n);

    // now the value of the variable is 'list'; it will get compiled once in
    // context (i.e. not here)

    // search the parents for the node where the variable will be set
    node::pointer_t var_holder(f_state.find_parent_by_type(node_type_t::OPEN_CURLYBRACKET));
    if(!var_holder)
    {
        var_holder = f_state.get_root();
    }

    // save the variable
    var_holder->set_variable(variable_name, list);
}

node::pointer_t compiler::get_variable(node::pointer_t n)
{
    std::string const & variable_name(n->get_string());

    // search the parents for the node where the variable will be set
    node::pointer_t var_holder(f_state.find_parent_by_type(node_type_t::OPEN_CURLYBRACKET));
    while(var_holder)
    {
        node::pointer_t value(var_holder->get_variable(variable_name));
        if(value)
        {
            return value;
        }
        var_holder = f_state.find_parent_by_type(node_type_t::OPEN_CURLYBRACKET, var_holder);
    }

    // if not found yet, check the root node too
    {
        node::pointer_t value(f_state.get_root()->get_variable(variable_name));
        if(value)
        {
            return value;
        }
    }

    if(f_empty_on_undefined_variable)
    {
        // returning "empty"...
        return node::pointer_t(new node(node_type_t::WHITESPACE, n->get_position()));
    }

    error::instance() << n->get_position()
            << "variable named \""
            << variable_name
            << "\" is not set."
            << error_mode_t::ERROR_ERROR;

    node::pointer_t fake(new node(node_type_t::IDENTIFIER, n->get_position()));
    fake->set_string("<undefined-variable(\"" + variable_name + "\")>");
    return fake;
}

bool compiler::argify(node::pointer_t n)
{
    size_t const max_children(n->size());
    if(max_children > 0)
    {
        node::pointer_t temp(new node(node_type_t::LIST, n->get_position()));
        temp->take_over_children_of(n);

        node::pointer_t arg(new node(node_type_t::ARG, n->get_position()));
        n->add_child(arg);

        for(size_t i(0); i < max_children; ++i)
        {
            node::pointer_t child(temp->get_child(i));
            if(child->is(node_type_t::OPEN_CURLYBRACKET))
            {
                if(i + 1 != max_children)
                {
                    throw csspp_exception_logic("compiler.cpp:compiler::argify(): list that has an OPEN_CURLYBRACKET that is not the last child."); // LCOV_EXCL_LINE
                }
                n->add_child(child);
                break;
            }
            if(child->is(node_type_t::COMMA))
            {
                // make sure to remove any WHITESPACE appearing just
                // before a comma
                if(!arg->empty() && arg->get_last_child()->is(node_type_t::WHITESPACE))
                {
                    arg->remove_child(arg->get_last_child());
                }
                if(arg->empty())
                {
                    if(n->size() == 1)
                    {
                        error::instance() << n->get_position()
                                << "dangling comma at the beginning of a list of arguments or selectors."
                                << error_mode_t::ERROR_ERROR;
                    }
                    else
                    {
                        error::instance() << n->get_position()
                                << "two commas in a row are invalid in a list of arguments or selectors."
                                << error_mode_t::ERROR_ERROR;
                    }
                    return false;
                }
                if(i + 1 == max_children
                || temp->get_child(i + 1)->is(node_type_t::OPEN_CURLYBRACKET))
                {
                    error::instance() << n->get_position()
                            << "dangling comma at the end of a list of arguments or selectors."
                            << error_mode_t::ERROR_ERROR;
                    return false;
                }
                // move to the next 'arg'
                arg.reset(new node(node_type_t::ARG, n->get_position()));
                n->add_child(arg);
            }
            else if(!child->is(node_type_t::WHITESPACE) || !arg->empty())
            {
                arg->add_child(child);
            }
        }
    }

    return true;
}

bool compiler::selector_attribute_check(node::pointer_t n)
{
    // use a for() as a 'goto exit;' on a 'break'
    for(;;)
    {
        size_t pos(0);
        node::pointer_t term(n->get_child(pos));
        if(term->is(node_type_t::WHITESPACE))
        {
            // I'm keeping this here, although there should be no WHITESPACE
            // at the start of a '[' block
            n->remove_child(term);          // LCOV_EXCL_LINE
            if(pos >= n->size())            // LCOV_EXCL_LINE
            {
                break;                      // LCOV_EXCL_LINE
            }
            term = n->get_child(pos);       // LCOV_EXCL_LINE
        }

        if(!term->is(node_type_t::IDENTIFIER))
        {
            error::instance() << n->get_position()
                    << "an attribute selector expects to first find an identifier."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }

        ++pos;
        if(pos >= n->size())
        {
            // just IDENTIFIER is valid
            return true;
        }

        term = n->get_child(pos);
        if(term->is(node_type_t::WHITESPACE))
        {
            n->remove_child(pos);
            if(pos >= n->size())
            {
                // just IDENTIFIER is valid, although we should never
                // reach this line because WHITESPACE are removed from
                // the end of lists
                return true;  // LCOV_EXCL_LINE
            }
            term = n->get_child(pos);
        }

        if(!term->is(node_type_t::EQUAL)                // '='
        && !term->is(node_type_t::INCLUDE_MATCH)        // '~='
        && !term->is(node_type_t::PREFIX_MATCH)         // '^='
        && !term->is(node_type_t::SUFFIX_MATCH)         // '$='
        && !term->is(node_type_t::SUBSTRING_MATCH)      // '*='
        && !term->is(node_type_t::DASH_MATCH))          // '|='
        {
            error::instance() << n->get_position()
                    << "expected attribute operator missing, supported operators are '=', '~=', '^=', '$=', '*=', and '|='."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }

        ++pos;
        if(pos >= n->size())
        {
            break;
        }

        term = n->get_child(pos);
        if(term->is(node_type_t::WHITESPACE))
        {
            n->remove_child(pos);
            if(pos >= n->size())
            {
                break;
            }
            term = n->get_child(pos);
        }

        if(!term->is(node_type_t::IDENTIFIER)
        && !term->is(node_type_t::STRING)
        && !term->is(node_type_t::INTEGER)
        && !term->is(node_type_t::DECIMAL_NUMBER))
        {
            error::instance() << n->get_position()
                    << "attribute selector value must be an identifier, a string, an integer, or a decimal number, a "
                    << term->get_type() << " is not acceptable."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }

        ++pos;
        if(pos < n->size())  // <<-- inverted test!
        {
            error::instance() << n->get_position()
                    << "attribute selector cannot be followed by more than one value, found "
                    << n->get_child(pos)->get_type() << " after the value, missing quotes?"
                    << error_mode_t::ERROR_ERROR;
            return false;
        }

        return true;
    }

    error::instance() << n->get_position()
            << "the attribute selector is expected to be an IDENTIFIER optionally followed by an operator and a value."
            << error_mode_t::ERROR_ERROR;
    return false;
}

bool compiler::selector_simple_term(node::pointer_t n, size_t & pos)
{
    // test with `pos + 1` since the last item in the list is not a selector,
    // it is the curly bracket block
    if(pos >= n->size())
    {
        throw csspp_exception_logic("compiler.cpp:compiler::selector_term(): selector_simple_term() called when not enough selectors are available."); // LCOV_EXCL_LINE
    }

    node::pointer_t term(n->get_child(pos));
    switch(term->get_type())
    {
    case node_type_t::HASH:
        // valid term as is
        break;

    case node_type_t::IDENTIFIER:
    case node_type_t::MULTIPLY:
        // IDENTIFIER
        // IDENTIFIER '|' IDENTIFIER
        // IDENTIFIER '|' '*'
        // '*'
        // '*' '|' IDENTIFIER
        // '*' '|' '*'
        if(pos + 1 < n->size())
        {
            if(n->get_child(pos + 1)->is(node_type_t::SCOPE))
            {
                if(pos + 2 >= n->size())
                {
                    error::instance() << n->get_position()
                            << "the scope operator (|) requires a right hand side identifier or '*'."
                            << error_mode_t::ERROR_ERROR;
                    return false;
                }
                pos += 2;
                term = n->get_child(pos);
                if(!term->is(node_type_t::IDENTIFIER)
                && !term->is(node_type_t::MULTIPLY))
                {
                    error::instance() << n->get_position()
                            << "the right hand side of a scope operator (|) must be an identifier or '*'."
                            << error_mode_t::ERROR_ERROR;
                    return false;
                }
            }
            else if(term->is(node_type_t::MULTIPLY)
                 && (n->get_child(pos + 1)->is(node_type_t::OPEN_SQUAREBRACKET)
                  || n->get_child(pos + 1)->is(node_type_t::PERIOD)))
            {
                // this asterisk is not required, get rid of it
                n->remove_child(term);
                return true; // return immediately to avoid the ++pos
            }
        }
        break;

    case node_type_t::SCOPE:
        ++pos;
        if(pos >= n->size())
        {
            error::instance() << n->get_position()
                    << "a scope selector (|) must be followed by an identifier or '*'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        term = n->get_child(pos);
        if(!term->is(node_type_t::IDENTIFIER)
        && !term->is(node_type_t::MULTIPLY))
        {
            error::instance() << n->get_position()
                    << "the right hand side of a scope operator (|) must be an identifier or '*'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        break;

    case node_type_t::COLON:
        ++pos;
        if(pos >= n->size())
        {
            error::instance() << n->get_position()
                    << "a selector list cannot end with a standalone ':'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        term = n->get_child(pos);
        switch(term->get_type())
        {
        case node_type_t::IDENTIFIER:
            {
                // ':' IDENTIFIER
                // validate the identifier as only a small number can be used
                set_validation_script("pseudo-classes");
                node::pointer_t str(new node(node_type_t::STRING, term->get_position()));
                str->set_string(term->get_string());
                add_validation_variable("pseudo_name", str);
                if(!run_validation(false))
                {
                    return false;
                }
            }
            break;

        case node_type_t::FUNCTION:
            {
                // ':' FUNCTION component-value-list ')'
                //
                // create a temporary identifier to run the validation
                // checks, because the FUNCTION is a list of nodes!
                node::pointer_t function_name(new node(node_type_t::STRING, term->get_position()));
                function_name->set_string(term->get_string());
                set_validation_script("pseudo-nth-functions");
                add_validation_variable("pseudo_name", function_name);
                if(run_validation(true))
                {
                    // this is a valid nth function, print out its parameters
                    // and reparse as 'An+B'
                    size_t const max_children(term->size());
                    std::string an_b;
                    for(size_t idx(0); idx < max_children; ++idx)
                    {
                        an_b += term->get_child(idx)->to_string();
                    }
                    // TODO...
                    nth_child nc;
                    if(nc.parse(an_b))
                    {
                        // success, save the compiled An+B in this object
                        node::pointer_t an_b_node(new node(node_type_t::AN_PLUS_B, term->get_position()));
                        an_b_node->set_integer(nc.get_nth());
                        term->clear();
                        term->add_child(an_b_node);
                    }
                    else
                    {
                        // get the error and display it
                        error::instance() << term->get_position()
                                << nc.get_error()
                                << error_mode_t::ERROR_ERROR;
                        return false;
                    }
                }
                else
                {
                    set_validation_script("pseudo-functions");
                    add_validation_variable("pseudo_name", function_name);
                    if(!run_validation(false))
                    {
                        return false;
                    }
                    // this is a standard function, check the parameters
                    if(term->get_string() == "not")
                    {
                        // :not(:not(...)) is illegal
                        error::instance() << n->get_position()
                                << "the :not() selector does not accept an inner :not()."
                                << error_mode_t::ERROR_ERROR;
                        return false;
                    }
                    else if(term->get_string() == "lang")
                    {
                        // the language must be an identifier with no dashes
                        if(term->size() != 1)
                        {
                            error::instance() << term->get_position()
                                    << "a lang() function selector must have exactly one identifier as its parameter."
                                    << error_mode_t::ERROR_ERROR;
                            return false;
                        }
                        term = term->get_child(0);
                        if(term->is(node_type_t::IDENTIFIER))
                        {
                            if(term->get_string().find('-') == std::string::npos)
                            {
                                error::instance() << term->get_position()
                                        << "a lang() function selector expects an identifier without a '-'."
                                        << error_mode_t::ERROR_ERROR;
                                return false;
                            }
                        }
                        else
                        {
                            error::instance() << term->get_position()
                                    << "a lang() function selector expects an identifier as its parameter."
                                    << error_mode_t::ERROR_ERROR;
                            return false;
                        }
                    }
                }
            }
            break;

        default:
            // invalid selector list
            error::instance() << n->get_position()
                    << "a ':' selector must be followed by an identifier or a function, a " << n->get_type() << " was found instead."
                    << error_mode_t::ERROR_ERROR;
            return false;

        }
        break;

    case node_type_t::PERIOD:
        // '.' IDENTIFIER -- class (special attribute check)
        ++pos;
        if(pos >= n->size())
        {
            error::instance() << n->get_position()
                    << "a selector list cannot end with a standalone '.'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        term = n->get_child(pos);
        if(!term->is(node_type_t::IDENTIFIER))
        {
            error::instance() << n->get_position()
                    << "a class selector (after a period: '.') must be an identifier."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        break;

    case node_type_t::OPEN_SQUAREBRACKET:
        // '[' WHITESPACE attribute-check WHITESPACE ']' -- attributes check
        ++pos;
        return selector_attribute_check(term);

    case node_type_t::GREATER_THAN:
    case node_type_t::ADD:
    case node_type_t::PRECEDED:
        error::instance() << n->get_position()
                << "found token " << term->get_type() << ", which cannot be used to start a selector expression."
                << error_mode_t::ERROR_ERROR;
        return false;

    default:
        error::instance() << n->get_position()
                << "found token " << term->get_type() << ", which is not a valid selector token."
                << error_mode_t::ERROR_ERROR;
        return false;

    }

    // move on to the next term
    ++pos;

    return true;
}

bool compiler::selector_term(node::pointer_t n, size_t & pos)
{
    if(pos >= n->size())
    {
        throw csspp_exception_logic("compiler.cpp:compiler::selector_term(): selector_term() called when not enough selectors are available."); // LCOV_EXCL_LINE
    }

    node::pointer_t term(n->get_child(pos));
    switch(term->get_type())
    {
    case node_type_t::PLACEHOLDER:
    case node_type_t::REFERENCE:
        // valid complex term as is
        break;

    case node_type_t::COLON:
        ++pos;
        if(pos >= n->size())
        {
            error::instance() << n->get_position()
                    << "a selector list cannot end with a standalone ':'."
                    << error_mode_t::ERROR_ERROR;
            return false;
        }
        term = n->get_child(pos);
        switch(term->get_type())
        {
        case node_type_t::IDENTIFIER:
            --pos;
            return selector_simple_term(n, pos);

        case node_type_t::FUNCTION:
            // ':' FUNCTION component-value-list ')'
            if(term->get_string() == "not")
            {
                // special handling, the :not() is considered to be
                // a complex selector and as such has to be handled
                // right here; the parameters must represent one valid
                // simple term
                //
                // TODO: got to take care of WHITESPACE, plus the
                //       end of the list of children is NOT a {}-block
                //       (argh!)
                size_t sub_pos(0);
                return selector_simple_term(term, sub_pos);
            }
            else
            {
                --pos;
                return selector_simple_term(n, pos);
            }
            break;

        case node_type_t::COLON:
            // '::' IDENTIFIER -- pseudo elements
            ++pos;
            if(pos >= n->size())
            {
                error::instance() << n->get_position()
                        << "a selector list cannot end with a '::'."
                        << error_mode_t::ERROR_ERROR;
                return false;
            }
            term = n->get_child(pos);
            if(!term->is(node_type_t::IDENTIFIER))
            {
                error::instance() << n->get_position()
                        << "a pseudo element name (defined after a '::' in a list of selectors) must be defined using an identifier."
                        << error_mode_t::ERROR_ERROR;
                return false;
            }
            // only a few pseudo element names exist, do a validation
            set_validation_script("pseudo-elements");
            add_validation_variable("pseudo_name", term);
            if(!run_validation(false))
            {
                return false;
            }
            break;

        default:
            // invalid selector list
            error::instance() << n->get_position()
                    << "a ':' selector must be followed by an identifier or a function, a " << n->get_type() << " was found instead."
                    << error_mode_t::ERROR_ERROR;
            return false;

        }
        break;

    case node_type_t::HASH:
    case node_type_t::IDENTIFIER:
    case node_type_t::MULTIPLY:
    case node_type_t::OPEN_SQUAREBRACKET:
    case node_type_t::PERIOD:
    case node_type_t::SCOPE:
        return selector_simple_term(n, pos);

    case node_type_t::GREATER_THAN:
    case node_type_t::ADD:
    case node_type_t::PRECEDED:
        error::instance() << n->get_position()
                << "found token " << term->get_type() << ", which cannot be used to start a selector expression."
                << error_mode_t::ERROR_ERROR;
        return false;

    default:
        error::instance() << n->get_position()
                << "found token " << term->get_type() << ", which is not a valid selector token."
                << error_mode_t::ERROR_ERROR;
        return false;

    }

    // move on to the next term
    ++pos;

    return true;
}

bool compiler::selector_list(node::pointer_t n, size_t & pos)
{
    // we must have a term first
    if(!selector_term(n, pos))
    {
        return false;
    }

    for(;;)
    {
        if(pos >= n->size())
        {
            return true;
        }

        // skip whitespaces between terms
        // this also works for binary operators
        node::pointer_t term(n->get_child(pos));
        if(term->is(node_type_t::WHITESPACE))
        {
            ++pos;

            // end of list too soon?
            if(pos >= n->size())
            {
                // this should not happen since we remove leading/trailing
                // white space tokens
                throw csspp_exception_logic("compiler.cpp: a component value has a WHITESPACE token before the OPEN_CURLYBRACKET."); // LCOV_EXCL_LINE
            }
            term = n->get_child(pos);
        }

        if(term->is(node_type_t::GREATER_THAN)
        || term->is(node_type_t::ADD)
        || term->is(node_type_t::PRECEDED))
        {
            // if we had a WHITESPACE just before the binary operator,
            // remove it as it is not necessary
            if(n->get_child(pos - 1)->is(node_type_t::WHITESPACE))
            {
                n->remove_child(pos - 1);
            }
            else
            {
                ++pos;
            }

            // it is mandatory for these tokens to be followed by another
            // term (i.e. binary operators)
            if(pos >= n->size())
            {
                error::instance() << n->get_position()
                        << "found token " << term->get_type() << ", which is expected to be followed by another selector term."
                        << error_mode_t::ERROR_ERROR;
                return false;
            }

            // we may have a WHITESPACE first, if so skip it
            term = n->get_child(pos);
            if(term->is(node_type_t::WHITESPACE))
            {
                // no need before/after binary operators
                n->remove_child(term);

                // end of list too soon?
                if(pos >= n->size())
                {
                    // this should not happen since we remove leading/trailing
                    // white space tokens
                    throw csspp_exception_logic("compiler.cpp: a component value has a WHITESPACE token before the OPEN_CURLYBRACKET."); // LCOV_EXCL_LINE
                }
            }
        }

        if(!selector_term(n, pos))
        {
            return false;
        }
    }
}

bool compiler::parse_selector(node::pointer_t n)
{
    if(!argify(n))
    {
        return false;
    }

    size_t const max_children(n->size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node::pointer_t arg(n->get_child(idx));
        if(arg->is(node_type_t::OPEN_CURLYBRACKET))
        {
            // this is at the end of the list, so we're done
            break;
        }
        if(!arg->is(node_type_t::ARG))
        {
            throw csspp_exception_logic("compiler.cpp: parse_selector() just called argify() and yet a child is not an ARG."); // LCOV_EXCL_LINE
        }
        size_t pos(0);
        if(!selector_list(arg, pos))
        {
            return false;
        }
    }

    return true;
}

std::string compiler::find_file(std::string const & script_name)
{
    if(f_paths.empty())
    {
        // should this be "." here instead of the default?
        f_paths.push_back("/usr/lib/csspp/scripts");
    }

    for(auto it : f_paths)
    {
        std::string const name(it == "" ? script_name : it + "/" + script_name);
        if(access(name.c_str(), R_OK) == 0)
        {
            return name;
        }
    }

    // in case we cannot find a file
    return std::string();
}

void compiler::set_validation_script(std::string const & script_name)
{
    // try the filename as is first
    std::string filename(find_file(script_name));
    if(filename.empty())
    {
        if(script_name.substr(script_name.size() - 5) != ".scss")
        {
            // try again with the "scss" extension
            filename = find_file(script_name + ".scss");
        }
    }

    if(filename.empty())
    {
        // a validation script should always be available, right?
        position pos(script_name);
        error::instance() << pos
                << "validation script \""
                << script_name
                << "\" was not found."
                << error_mode_t::ERROR_FATAL;
        throw csspp_exception_exit(1);
    }

    node::pointer_t script;

    auto cache(f_validator_scripts.find(filename));
    if(cache == f_validator_scripts.end())
    {
        position pos(filename);

        // the file exists, read it now
        std::ifstream in;
        in.open(filename);
        if(!in)
        {
            // a validation script should always be available, right?
            error::instance() << pos
                    << "validation script \""
                    << script_name
                    << "\" could not be opened."
                    << error_mode_t::ERROR_FATAL;
            throw csspp_exception_exit(1);
        }

        lexer::pointer_t l(new lexer(in, pos));
        parser p(l);
        script = p.stylesheet();

        // TODO: test whether errors occurred while reading the script, if
        //       so then we have to generate a FATAL error here

        // cache the script
        f_validator_scripts[filename] = script;
//std::cerr << "script " << filename << " is:\n" << *script;
    }
    else
    {
        script = cache->second;
    }

    f_current_validation_script = script;
    script->reset_variables();
}

void compiler::add_validation_variable(std::string const & variable_name, node::pointer_t value)
{
    if(!f_current_validation_script)
    {
        throw csspp_exception_logic("compiler.cpp: somehow add_validation_variable() was called without a current validation script set."); // LCOV_EXCL_LINE
    }

    f_current_validation_script->set_variable(variable_name, value);
}

bool compiler::run_validation(bool check_only)
{
    // avoid validation from within a validation script (we probably would
    // get infinite loops anyway)
    if(!f_compiler_validating)
    {
        // save the number of errors so we can test after we ran
        // the compile() function
        error_happened_t old_count;

        safe_compiler_state_t safe_state(f_state);
        f_state.set_root(f_current_validation_script);
        if(check_only)
        {
            // save the current error/warning counters so they do not change
            // on this run
            safe_error_t safe_error;

            // replace the output stream with a memory buffer so the user
            // does not see any of it
            std::stringstream ignore;
            safe_error_stream_t safe_output(ignore);

            // now compile that true/false check
            compile();

            // WARNING: this MUST be here (before the closing curly bracket)
            //          and not after the if() since we restore the error
            //          state from before the compile() call.
            //
            bool const result(!old_count.error_happened());

            // now restore the stream and error counters
            return result;
        }
        else
        {
            compile();

            return !old_count.error_happened();
        }
    }

    return true;
}

} // namespace csspp

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
