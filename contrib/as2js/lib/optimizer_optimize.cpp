/* optimizer_optimize.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "optimizer_tables.h"

#include    "as2js/message.h"
#include    "as2js/exceptions.h"

#include    <regex>


namespace as2js
{
namespace optimizer_details
{


/** \brief Hide all optimizer "optimize function" implementation details.
 *
 * This unnamed namespace is used to further hide all the optimizer
 * details.
 *
 * This namespace includes the functions used to optimize the tree of
 * nodes when a match was found earlier.
 */
namespace
{


/** \brief Apply an ADD function.
 *
 * This function adds two numbers and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \exception exception_internal_error
 * The function may attempt to convert the input to floating point numbers.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_ADD(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    Node::node_t type1(node_array[src1]->get_type());
    Node::node_t type2(node_array[src2]->get_type());

    // add the numbers together
    if(type1 == Node::node_t::NODE_INT64 && type2 == Node::node_t::NODE_INT64)
    {
        // a + b when a and b are integers
        Int64 i1(node_array[src1]->get_int64());
        Int64 i2(node_array[src2]->get_int64());
        // TODO: err on overflows?
        i1.set(i1.get() + i2.get());
        node_array[src1]->set_int64(i1);
    }
    else
    {
        // make sure a and b are floats, then do a + b as floats
        // TODO: check for NaN and other fun things?
        if(!node_array[src1]->to_float64()
        || !node_array[src2]->to_float64())
        {
            throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a float64."); // LCOV_EXCL_LINE
        }
        Float64 f1(node_array[src1]->get_float64());
        Float64 f2(node_array[src2]->get_float64());
        // TODO: err on overflow?
        f1.set(f1.get() + f2.get());
        node_array[src1]->set_float64(f1);
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a BITWISE_AND function.
 *
 * This function AND two numbers and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * Although the AND could be computed using 64 bits when handling integer
 * we do 32 bits to make sure that we get a result as JavaScript would.
 *
 * \exception exception_internal_error
 * The function may attempt to convert the input to floating point numbers.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_BITWISE_AND(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    if(!node_array[src1]->to_int64()
    || !node_array[src2]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an int64."); // LCOV_EXCL_LINE
    }

    // compute the result
    // a & b
    Int64 i1(node_array[src1]->get_int64());
    Int64 i2(node_array[src2]->get_int64());
    i1.set((i1.get() & i2.get()) & 0xFFFFFFFF);
    node_array[src1]->set_int64(i1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a BITWISE_NOT function.
 *
 * This function AND two numbers and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * Although the AND could be computed using 64 bits when handling integer
 * we do 32 bits to make sure that we get a result as JavaScript would.
 *
 * \exception exception_internal_error
 * The function may attempt to convert the input to floating point numbers.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_BITWISE_NOT(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src(optimize->f_indexes[0]),
             dst(optimize->f_indexes[1]);

    if(!node_array[src]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an int64."); // LCOV_EXCL_LINE
    }

    // compute the result
    // ~a
    Int64 i1(node_array[src]->get_int64());
    i1.set(~i1.get() & 0xFFFFFFFF);
    node_array[src]->set_int64(i1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src]);
    node_array[dst] = node_array[src];
}


/** \brief Apply a BITWISE_OR function.
 *
 * This function OR two numbers and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * Although the OR could be computed using 64 bits when handling integer
 * we do 32 bits to make sure that we get a result as JavaScript would.
 *
 * \exception exception_internal_error
 * The function may attempt to convert the input to floating point numbers.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_BITWISE_OR(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    if(!node_array[src1]->to_int64()
    || !node_array[src2]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an int64."); // LCOV_EXCL_LINE
    }

    // compute the result
    // a | b
    Int64 i1(node_array[src1]->get_int64());
    Int64 i2(node_array[src2]->get_int64());
    i1.set((i1.get() | i2.get()) & 0xFFFFFFFF);
    node_array[src1]->set_int64(i1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a BITWISE_XOR function.
 *
 * This function XOR two numbers and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * Although the XOR could be computed using 64 bits when handling integer
 * we do 32 bits to make sure that we get a result as JavaScript would.
 *
 * \exception exception_internal_error
 * The function may attempt to convert the input to floating point numbers.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_BITWISE_XOR(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    if(!node_array[src1]->to_int64()
    || !node_array[src2]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an int64."); // LCOV_EXCL_LINE
    }

    // compute the result
    // a ^ b
    Int64 i1(node_array[src1]->get_int64());
    Int64 i2(node_array[src2]->get_int64());
    i1.set((i1.get() ^ i2.get()) & 0xFFFFFFFF);
    node_array[src1]->set_int64(i1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a COMPARE function.
 *
 * This function compares two literals and saves the result.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \exception exception_internal_error
 * The function does not check whether the parameters are literals. They
 * are assumed to be or can be converted as required. If a conversion
 * fails (returns false) then this exception is raised.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_COMPARE(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);
    Node::pointer_t result;

    compare_t c(Node::compare(node_array[src1], node_array[src2], Node::compare_mode_t::COMPARE_LOOSE));
    switch(c)
    {
	case compare_t::COMPARE_LESS:       // -1
    case compare_t::COMPARE_EQUAL:      // 0
	case compare_t::COMPARE_GREATER:    // +1
        result.reset(new Node(Node::node_t::NODE_INT64));
        {
            Int64 i;
            i.set(static_cast<Int64::int64_type>(c));
            result->set_int64(i);
        }
        break;

	case compare_t::COMPARE_UNORDERED:
	case compare_t::COMPARE_ERROR:
	case compare_t::COMPARE_UNDEFINED:
        // any invalid answer, included unordered becomes undefined
        result.reset(new Node(Node::node_t::NODE_UNDEFINED));
        break;

    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(result);
    node_array[dst] = result;
}


/** \brief Apply a CONCATENATE function.
 *
 * This function concatenates two strings and save the result.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \exception exception_internal_error
 * The function does not check whether the parameters are strings. They
 * are assumed to be or can be converted to a string. The function uses
 * the to_string() just before the concatenation and if the conversion
 * fails (returns false) then this exception is raised.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_CONCATENATE(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    if(!node_array[src1]->to_string()
    || !node_array[src2]->to_string())
    {
        throw exception_internal_error("a concatenate instruction can only be used with nodes that can be converted to strings."); // LCOV_EXCL_LINE
    }

    String s1(node_array[src1]->get_string());
    String s2(node_array[src2]->get_string());
    node_array[src1]->set_string(s1 + s2);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a DIVIDE function.
 *
 * This function divides source 1 by source 2 and saves the result in
 * the destination. Source 1 and 2 are expected to be literals.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \todo
 * Should we always return a floating point number when dividing?
 * At this point two integers return an integer unless the divisor
 * is zero in which case +/-Infinity is returned.
 *
 * \exception exception_internal_error
 * The function does not check whether the parameters are numbers. They
 * are assumed to be or can be converted to a number. The function uses
 * the to_float64() just before the operation and if the conversion
 * fails (returns false) then this exception is raised.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_DIVIDE(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    // if both are integers, keep it as an integer
    // (unless src2 is zero)
    if(node_array[src1]->is_int64()
    && node_array[src2]->is_int64())
    {
        Int64 i1(node_array[src1]->get_int64());
        Int64 i2(node_array[src2]->get_int64());
        if(i2.get() == 0)
        {
            // generate an warning about divisions by zero because it is not
            // unlikely an error
            Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
            msg << "division by zero of integers returning +Infinity or -Infinity.";

            // dividing by zero gives infinity
            Float64 f;
            f.set_infinity(); // +Infinity
            if(i1.get() < 0)
            {
                // -Infinity
                f.set(-f.get());
            }
            if(!node_array[src1]->to_float64())
            {
                throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a float64."); // LCOV_EXCL_LINE
            }
            node_array[src1]->set_float64(f);
        }
        else
        {
            // TBD: should this return a float?
            i1.set(i1.get() / i2.get());
            node_array[src1]->set_int64(i1);
        }
    }
    else
    {
        if(!node_array[src1]->to_float64()
        || !node_array[src2]->to_float64())
        {
            throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a float64."); // LCOV_EXCL_LINE
        }
        // make sure we keep NaN numbers as expected
        Float64 f1(node_array[src1]->get_float64());
        Float64 f2(node_array[src2]->get_float64());
        if(f1.is_NaN()
        || f2.is_NaN())
        {
            f1.set_NaN();
        }
        else
        {
            f1.set(f1.get() / f2.get());
        }
        node_array[src1]->set_float64(f1);
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply an EQUAL function.
 *
 * This function checks source 1 against source 2. If source 1 is equal to
 * source 2, then the function saves true in the destination, otherwise
 * it saves false.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_EQUAL(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    compare_t c(Node::compare(node_array[src1], node_array[src2], Node::compare_mode_t::COMPARE_LOOSE));
    Node::pointer_t result(new Node(c == compare_t::COMPARE_EQUAL
                                    ? Node::node_t::NODE_TRUE
                                    : Node::node_t::NODE_FALSE));

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(result);
    node_array[dst] = result;
}


/** \brief Apply a LESS function.
 *
 * This function checks source 1 against source 2. If source 1 is smaller
 * than source 2, then the function saves true in the destination, otherwise
 * it saves false.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_LESS(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    compare_t c(Node::compare(node_array[src1], node_array[src2], Node::compare_mode_t::COMPARE_LOOSE));
    Node::pointer_t result(new Node(c == compare_t::COMPARE_LESS
                                    ? Node::node_t::NODE_TRUE
                                    : Node::node_t::NODE_FALSE));

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(result);
    node_array[dst] = result;
}


/** \brief Apply a LESS_EQUAL function.
 *
 * This function checks source 1 against source 2. If source 1 is smaller
 * than source 2, then the function saves true in the destination, otherwise
 * it saves false.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_LESS_EQUAL(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    compare_t const c(Node::compare(node_array[src1], node_array[src2], Node::compare_mode_t::COMPARE_LOOSE));
    Node::pointer_t result(new Node(c == compare_t::COMPARE_LESS
                                 || c == compare_t::COMPARE_EQUAL
                                        ? Node::node_t::NODE_TRUE
                                        : Node::node_t::NODE_FALSE));

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(result);
    node_array[dst] = result;
}


/** \brief Apply a LOGICAL_NOT function.
 *
 * This function applies a logical NOT and saves the result in the
 * 2nd position.
 *
 * The logical NOT is applied whatever the input literal after a conversion
 * to Boolean. If the conversion fails, then an exception is raised.
 *
 * \li 0 -- source
 * \li 1 -- destination
 *
 * \exception exception_internal_error
 * The function may attempt to convert the input to a Boolean value.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_LOGICAL_NOT(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src(optimize->f_indexes[0]),
             dst(optimize->f_indexes[1]);

    if(!node_array[src]->to_boolean())
    {
        throw exception_internal_error("optimizer used function to_boolean() against a node that cannot be converted to a Boolean."); // LCOV_EXCL_LINE
    }
    bool b(node_array[src]->get_boolean());
    node_array[src]->set_boolean(!b);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src]);
    node_array[dst] = node_array[src];
}


/** \brief Apply a LOGICAL_XOR function.
 *
 * This function applies a logical XOR between the two sources and saves
 * the result in the 2nd position.
 *
 * The logical XOR is applied whatever the literals after a conversion
 * to Boolean. If the conversion fails, then an exception is raised.
 *
 * \li 0 -- first source
 * \li 1 -- second source
 * \li 2 -- destination
 *
 * \warning
 * The first source will be modified before replacing destination.
 *
 * \exception exception_internal_error
 * The function may attempt to convert the inputs to Boolean values.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_LOGICAL_XOR(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    Node::node_t n1(node_array[src1]->to_boolean_type_only());
    Node::node_t n2(node_array[src2]->to_boolean_type_only());
    if((n1 != Node::node_t::NODE_TRUE && n1 != Node::node_t::NODE_FALSE)
    || (n2 != Node::node_t::NODE_TRUE && n2 != Node::node_t::NODE_FALSE))
    {
        throw exception_internal_error("optimizer used function to_boolean_type_only() against a node that cannot be converted to a Boolean."); // LCOV_EXCL_LINE
    }
    if(n1 == n2)
    {
        // if false, return the Boolean false
        node_array[src1]->to_boolean();
        node_array[src1]->set_boolean(false);
    }
    else
    {
        // if true, return the input as is
        if(n1 == Node::node_t::NODE_FALSE)
        {
            // src2 is the result if src1 represents false
            src1 = src2;
        }
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a MODULO function.
 *
 * This function divides source 1 by source 2 and saves the rest in
 * the destination. Source 1 and 2 are expected to be literals.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * If the divisor is zero, the function returns NaN.
 *
 * \exception exception_internal_error
 * The function does not check whether the parameters are numbers. They
 * are assumed to be or can be converted to a number. The function uses
 * the to_float64() just before the operation and if the conversion
 * fails (returns false) then this exception is raised.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_MODULO(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    // if both are integers, keep it as an integer
    // (unless src2 is zero)
    if(node_array[src1]->is_int64()
    && node_array[src2]->is_int64())
    {
        Int64 i1(node_array[src1]->get_int64());
        Int64 i2(node_array[src2]->get_int64());
        if(i2.get() == 0)
        {
            // generate an warning about divisions by zero because it is not
            // unlikely an error
            Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
            msg << "division by zero for a modulo of integers returning NaN.";

            // dividing by zero gives infinity
            Float64 f;
            f.set_NaN();
            if(!node_array[src1]->to_float64())
            {
                throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a float64."); // LCOV_EXCL_LINE
            }
            node_array[src1]->set_float64(f);
        }
        else
        {
            // TBD: should this return a float?
            i1.set(i1.get() % i2.get());
            node_array[src1]->set_int64(i1);
        }
    }
    else
    {
        if(!node_array[src1]->to_float64()
        || !node_array[src2]->to_float64())
        {
            throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a float64."); // LCOV_EXCL_LINE
        }
        // make sure we keep NaN numbers as expected
        Float64 f1(node_array[src1]->get_float64());
        Float64 f2(node_array[src2]->get_float64());
        if(f1.is_NaN()
        || f2.is_NaN())
        {
            f1.set_NaN();
        }
        else
        {
            f1.set(fmod(f1.get(), f2.get()));
        }
        node_array[src1]->set_float64(f1);
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a MOVE function.
 *
 * This function moves a node to another. In most cases, you move a child
 * to the parent. For example in
 *
 * \code
 *      a := b + 0;
 * \endcode
 *
 * You could move b in the position of the '+' operator so the expression
 * now looks like:
 *
 * \code
 *      a := b;
 * \endcode
 *
 * (note that in this case we were optimizing 'b + 0' at this point)
 *
 * \li 0 -- source
 * \li 1 -- destination
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_MOVE(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src(optimize->f_indexes[0]),
             dst(optimize->f_indexes[1]);

    // move the source in place of the destination
    node_array[dst]->replace_with(node_array[src]);
    node_array[dst] = node_array[src];
}


/** \brief Apply a MATCH function.
 *
 * This function checks whether the left handside matches the regular
 * expression in the right handside and returns true if it does and
 * false if it does not.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \exception exception_internal_error
 * The function does not check whether the parameters are numbers. They
 * are assumed to be or can be converted to a number. The function uses
 * the to_float64() just before the operation and if the conversion
 * fails (returns false) then this exception is raised.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_MATCH(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    // if both are integers, keep it as an integer
    std::regex::flag_type regex_flags(std::regex_constants::ECMAScript | std::regex_constants::nosubs);
    String regex(node_array[src2]->get_string());
    if(regex[0] == '/')
    {
        String::size_type pos(regex.find_last_of('/'));
        String flags(regex.substr(pos + 1));
        regex = regex.substr(1, pos - 1); // remove the /.../
        if(flags.find('i') != String::npos)
        {
            regex_flags |= std::regex_constants::icase;
        }
    }
    // g++ implementation of regex is still quite limited in g++ 4.8.x
    // if you have 4.9.0, the following should work nicely for you, otherwise
    // it does nothing...
    // http://stackoverflow.com/questions/12530406/is-gcc4-7-buggy-about-regular-expressions
    int match_result(-1);
    try
    {
        std::basic_regex<as_char_t> js_regex(regex, regex_flags);
        std::match_results<String::const_iterator> js_match;
        std::regex_search(node_array[src1]->get_string(), js_match, js_regex);
        match_result = js_match.empty() ? 1 : 0;
    }
    catch(std::regex_error const&)
    {
    }
    catch(std::bad_cast const&)
    {
    }

    Node::pointer_t result;
    if(match_result == -1)
    {
        // the regular expression is not valid, so we cannot optimize it
        // to true or false, instead we generate an error now and transform
        // the code to a throw
        //
        //    throw new SyntaxError(errmsg, fileName, lineNumber);
        //
        // important note: any optimization has to do something or the
        //                 optimizer tries again indefinitely...
        //
        result.reset(new Node(Node::node_t::NODE_THROW));
        // TODO: we need to create a SyntaxError object

        Node::pointer_t call(new Node(Node::node_t::NODE_CALL));
        result->append_child(call);

        Node::pointer_t syntax_error(new Node(Node::node_t::NODE_IDENTIFIER));
        syntax_error->set_string("SyntaxError");
        call->append_child(syntax_error);

        Node::pointer_t params(new Node(Node::node_t::NODE_LIST));
        call->append_child(params);

        Node::pointer_t message(new Node(Node::node_t::NODE_STRING));
        String errmsg("regular expression \"");
        errmsg += regex;
        errmsg += "\" could not be compiled by std::regex.";
        message->set_string(errmsg);
        params->append_child(message);

        Position const& pos(node_array[src2]->get_position());

        Node::pointer_t filename(new Node(Node::node_t::NODE_STRING));
        filename->set_string(pos.get_filename());
        params->append_child(filename);

        Node::pointer_t line_number(new Node(Node::node_t::NODE_INT64));
        Int64 ln;
        ln.set(pos.get_line());
        line_number->set_int64(ln);
        params->append_child(line_number);

        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, pos);
        msg << errmsg;
    }
    else
    {
        result.reset(new Node(match_result == 0 ? Node::node_t::NODE_FALSE : Node::node_t::NODE_TRUE));
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(result);
    node_array[dst] = result;
}


/** \brief Apply a MAXIMUM function.
 *
 * This function compares two values and keep the largest one.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_MAXIMUM(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    Node::pointer_t result;

    Node::pointer_t n1(node_array[src1]);
    Node::pointer_t n2(node_array[src2]);
    if(n1->is_float64() && n1->get_float64().is_NaN())
    {
        result = n2;
    }
    else if(n2->is_float64() && n2->get_float64().is_NaN())
    {
        result = n1;
    }
    else
    {
        compare_t const c(Node::compare(n1, n2, Node::compare_mode_t::COMPARE_LOOSE));
        result = c == compare_t::COMPARE_GREATER ? n1 : n2;
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(result);
    node_array[dst] = result;
}


/** \brief Apply a MINIMUM function.
 *
 * This function compares two values and keep the smallest one.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_MINIMUM(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    Node::pointer_t result;

    Node::pointer_t n1(node_array[src1]);
    Node::pointer_t n2(node_array[src2]);
    if(n1->is_float64() && n1->get_float64().is_NaN())
    {
        result = n2;
    }
    else if(n2->is_float64() && n2->get_float64().is_NaN())
    {
        result = n1;
    }
    else
    {
        compare_t const c(Node::compare(n1, n2, Node::compare_mode_t::COMPARE_LOOSE));
        result = c == compare_t::COMPARE_LESS ? n1 : n2;
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(result);
    node_array[dst] = result;
}


/** \brief Apply a MULTIPLY function.
 *
 * This function multiplies source 1 by source 2 and saves the result in
 * the destination. Source 1 and 2 are expected to be literals.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \exception exception_internal_error
 * The function does not check whether the parameters are numbers. They
 * are assumed to be or can be converted to a number. The function uses
 * the to_float64() just before the operation and if the conversion
 * fails (returns false) then this exception is raised.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_MULTIPLY(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    // if both are integers, keep it as an integer
    if(node_array[src1]->is_int64()
    && node_array[src2]->is_int64())
    {
        Int64 i1(node_array[src1]->get_int64());
        Int64 i2(node_array[src2]->get_int64());
        i1.set(i1.get() * i2.get());
        node_array[src1]->set_int64(i1);
    }
    else
    {
        if(!node_array[src1]->to_float64()
        || !node_array[src2]->to_float64())
        {
            throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a float64."); // LCOV_EXCL_LINE
        }
        // make sure we keep NaN numbers as expected
        Float64 f1(node_array[src1]->get_float64());
        Float64 f2(node_array[src2]->get_float64());
        if(f1.is_NaN()
        || f2.is_NaN())
        {
            f1.set_NaN();
        }
        else
        {
            f1.set(f1.get() * f2.get());
        }
        node_array[src1]->set_float64(f1);
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a NEGATE function.
 *
 * This function negate a number and saves the result in the 2nd position.
 *
 * \li 0 -- source
 * \li 1 -- destination
 *
 * \exception exception_internal_error
 * The function may attempt to convert the input to a floating point number.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_NEGATE(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src(optimize->f_indexes[0]),
             dst(optimize->f_indexes[1]);

    Node::node_t type(node_array[src]->get_type());

    // negate the integer or the float
    if(type == Node::node_t::NODE_INT64)
    {
        Int64 i(node_array[src]->get_int64());
        i.set(-i.get());
        node_array[src]->set_int64(i);
    }
    else
    {
        // make sure a and b are floats, then do a + b as floats
        // TODO: check for NaN and other fun things?
        if(!node_array[src]->to_float64())
        {
            throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a float64."); // LCOV_EXCL_LINE
        }
        Float64 f(node_array[src]->get_float64());
        f.set(-f.get());
        node_array[src]->set_float64(f);
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src]);
    node_array[dst] = node_array[src];
}


/** \brief Apply a POWER function.
 *
 * This function computes source 1 power source 2 and saves the result in
 * the destination. Source 1 and 2 are expected to be literals.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \exception exception_internal_error
 * The function does not check whether the parameters are numbers. They
 * are assumed to be or can be converted to a number. The function uses
 * the to_float64() just before the operation and if the conversion
 * fails (returns false) then this exception is raised.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_POWER(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    // for powers, we always return a floating point
    // (think of negative numbers...)
    if(!node_array[src1]->to_float64()
    || !node_array[src2]->to_float64())
    {
        throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a float64."); // LCOV_EXCL_LINE
    }

    // make sure we keep NaN numbers as expected
    Float64 f1(node_array[src1]->get_float64());
    Float64 f2(node_array[src2]->get_float64());
    if(f1.is_NaN()
    || f2.is_NaN())
    {
        f1.set_NaN();
    }
    else
    {
        f1.set(pow(f1.get(), f2.get()));
    }
    node_array[src1]->set_float64(f1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a REMOVE function.
 *
 * This function removes a node from another. In most cases, you remove one
 * of the child of a binary operator or similar
 *
 * \code
 *      a + 0;
 * \endcode
 *
 * You could remove the zero to get:
 *
 * \code
 *      +a;
 * \endcode
 *
 * \li 0 -- source
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_REMOVE(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src(optimize->f_indexes[0]);

    if(src == 0)
    {
        // ah... we cannot remove this one, instead mark it as unknown
        node_array[src]->to_unknown();
    }
    else
    {
        // simply remove from the parent, the smart pointers take care of the rest
        node_array[src]->set_parent(nullptr);
    }
}


/** \brief Apply an ROTATE_LEFT function.
 *
 * This function rotates the first number to the left by the number of bits
 * indicated by the second number and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * Although the rotate left could be computed using 64 bits when handling
 * integer we do 32 bits to make sure that we get a result as
 * JavaScript would.
 *
 * \exception exception_internal_error
 * The function attempts to convert the input to integer numbers.
 * If that fails, this exception is raised. The Optimizer matching
 * mechanism should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_ROTATE_LEFT(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    if(!node_array[src1]->to_int64()
    || !node_array[src2]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an int64."); // LCOV_EXCL_LINE
    }

    // compute the result
    // a >% b
    Int64 i1(node_array[src1]->get_int64());
    Int64 i2(node_array[src2]->get_int64());
    Int64::int64_type v2(i2.get());
    Int64::int64_type v2_and(v2 & 0x1F);
    if(v2 < 0)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static rotate amount is less than zero. " << v2_and << " will be used instead of " << v2 << ".";
    }
    else if(v2 >= 32)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static rotate amount is larger than 31. " << v2_and << " will be used instead of " << v2 << ".";
    }
    // TODO: warn about v1 being larger than 32 bits?
    uint32_t v1(i1.get());
    if(v2_and != 0)
    {
        v1 = (v1 << v2_and) | (v1 >> (32 - v2_and));
    }
    i1.set(v1);
    node_array[src1]->set_int64(i1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply an ROTATE_RIGHT function.
 *
 * This function rotates the first number to the left by the number of bits
 * indicated by the second number and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * Although the rotate left could be computed using 64 bits when handling
 * integer we do 32 bits to make sure that we get a result as
 * JavaScript would.
 *
 * \exception exception_internal_error
 * The function attempts to convert the input to integer numbers.
 * If that fails, this exception is raised. The Optimizer matching
 * mechanism should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_ROTATE_RIGHT(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    if(!node_array[src1]->to_int64()
    || !node_array[src2]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an int64."); // LCOV_EXCL_LINE
    }

    // compute the result
    // a >% b
    Int64 i1(node_array[src1]->get_int64());
    Int64 i2(node_array[src2]->get_int64());
    Int64::int64_type v2(i2.get());
    Int64::int64_type v2_and(v2 & 0x1F);
    if(v2 < 0)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static rotate amount is less than zero. " << v2_and << " will be used instead of " << v2 << ".";
    }
    else if(v2 >= 32)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static rotate amount is larger than 31. " << v2_and << " will be used instead of " << v2 << ".";
    }
    // TODO: warn about v1 being larger than 32 bits?
    uint32_t v1(i1.get());
    if(v2_and != 0)
    {
        v1 = (v1 >> v2_and) | (v1 << (32 - v2_and));
    }
    i1.set(v1);
    node_array[src1]->set_int64(i1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


///** \brief Apply a SET_FLOAT function.
// *
// * This function sets the value of a float node. This is used when an
// * optimization can determine the result of a numeric expression and
// * a simple float can be set as the result.
// *
// * For example, a bitwise operation with NaN returns zero. This
// * function can be used for that purpose.
// *
// * \li 0 -- node to be modified, it must be a NODE_FLOAT64
// * \li 1 -- dividend (taken as a signed 16 bit value)
// * \li 2 -- divisor (unsigned)
// *
// * The value is offset 1 divided by offset 2. Note however that
// * both of those values are only 16 bits... If the divisor is zero
// * then we use NaN as the value.
// *
// * \param[in] node_array  The array of nodes being optimized.
// * \param[in] optimize  The optimization parameters.
// */
//void optimizer_func_SET_FLOAT(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
//{
//    uint32_t dst(optimize->f_indexes[0]),
//             divisor(optimize->f_indexes[2]);
//
//    int32_t  dividend(static_cast<int16_t>(optimize->f_indexes[1]));
//
//    double value(divisor == 0 ? NAN : static_cast<double>(dividend) / static_cast<double>(divisor));
//
//    Float64 v(node_array[dst]->get_float64());
//    v.set(value);
//    node_array[dst]->set_float64(v);
//}


/** \brief Apply a SET_INTEGER function.
 *
 * This function sets the value of an integer node. This is used when
 * an optimization can determine the result of a numeric expression and
 * a simple integer can be set as the result.
 *
 * For example, a bitwise operation with NaN returns zero. This
 * function can be used for that purpose.
 *
 * \li 0 -- node to be modified, it must be a NODE_INT64
 * \li 1 -- new value (taken as a signed 16 bit value)
 *
 * Note that at this point this function limits the value to 16 bits.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_SET_INTEGER(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t dst(optimize->f_indexes[0]);

    int32_t  value(static_cast<int16_t>(optimize->f_indexes[1]));

    Int64 v(node_array[dst]->get_int64());
    v.set(value);
    node_array[dst]->set_int64(v);
}


/** \brief Apply a SET_NODE_TYPE function.
 *
 * This function changes a node with another one. For example, it changes
 * the NODE_ASSIGNMENT_SUBTRACT to a NODE_ASSIGNMENT when the subtract is
 * useless (i.e. the right handside is NaN.)
 *
 * \li 0 -- the new node type (Node::node_t::NODE_...)
 * \li 1 -- the offset of the node to be replaced
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_SET_NODE_TYPE(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    Node::node_t node_type(static_cast<Node::node_t>(optimize->f_indexes[0]));
    uint32_t src(optimize->f_indexes[1]);

    Node::pointer_t node(new Node(node_type));

    Node::pointer_t to_replace(node_array[src]);

    size_t max_children(to_replace->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        node->append_child(to_replace->get_child(0));
    }
    to_replace->replace_with(node);
    node_array[src] = node;
}


/** \brief Apply an SHIFT_LEFT function.
 *
 * This function shifts the first number to the left by the number of bits
 * indicated by the second number and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * Although the shift left could be computed using 64 bits when handling
 * integer we do 32 bits to make sure that we get a result as
 * JavaScript would.
 *
 * \exception exception_internal_error
 * The function attempts to convert the input to itneger numbers.
 * If that fails, this exception is raised. The Optimizer matching
 * mechanism should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_SHIFT_LEFT(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    if(!node_array[src1]->to_int64()
    || !node_array[src2]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an int64."); // LCOV_EXCL_LINE
    }

    // compute the result
    // a << b
    Int64 i1(node_array[src1]->get_int64());
    Int64 i2(node_array[src2]->get_int64());
    // TODO: add a test to warn about 'b' being negative or larger than 31
    Int64::int64_type v2(i2.get());
    Int64::int64_type v2_and(v2 & 0x1F);
    if(v2 < 0)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static shift amount is less than zero. " << v2_and << " will be used instead of " << v2 << ".";
    }
    else if(v2 >= 32)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static shift amount is larger than 31. " << v2_and << " will be used instead of " << v2 << ".";
    }
    // TODO: warn about v1 being larger than 32 bits?
    i1.set((i1.get() << v2_and) & 0xFFFFFFFF);
    node_array[src1]->set_int64(i1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply an SHIFT_RIGHT function.
 *
 * This function shifts the first number to the right by the number of bits
 * indicated by the second number and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * Although the shift right could be computed using 64 bits when handling
 * integer we do 32 bits to make sure that we get a result as
 * JavaScript would.
 *
 * \exception exception_internal_error
 * The function attempts to convert the input to integer point numbers.
 * If that fails, this exception is raised. The Optimizer matching
 * mechanism should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_SHIFT_RIGHT(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    if(!node_array[src1]->to_int64()
    || !node_array[src2]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an int64."); // LCOV_EXCL_LINE
    }

    // compute the result
    // a >> b
    Int64 i1(node_array[src1]->get_int64());
    Int64 i2(node_array[src2]->get_int64());
    Int64::int64_type v2(i2.get());
    Int64::int64_type v2_and(v2 & 0x1F);
    if(v2 < 0)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static shift amount is less than zero. " << v2_and << " will be used instead of " << v2 << ".";
    }
    else if(v2 >= 32)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static shift amount is larger than 31. " << v2_and << " will be used instead of " << v2 << ".";
    }
    // TODO: warn about v1 being larger than 32 bits?
    int32_t v1(i1.get());
    v1 >>= v2_and;
    i1.set(v1);
    node_array[src1]->set_int64(i1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply an SHIFT_RIGHT_UNSIGNED function.
 *
 * This function shifts the first number to the right by the number of bits
 * indicated by the second number and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * Although the shift right could be computed using 64 bits when handling
 * integer we do 32 bits to make sure that we get a result as
 * JavaScript would.
 *
 * \exception exception_internal_error
 * The function attempts to convert the input to integer numbers.
 * If that fails, this exception is raised. The Optimizer matching
 * mechanism should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_SHIFT_RIGHT_UNSIGNED(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    if(!node_array[src1]->to_int64()
    || !node_array[src2]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an int64."); // LCOV_EXCL_LINE
    }

    // compute the result
    // a >>> b
    Int64 i1(node_array[src1]->get_int64());
    Int64 i2(node_array[src2]->get_int64());
    Int64::int64_type v2(i2.get());
    Int64::int64_type v2_and(v2 & 0x1F);
    if(v2 < 0)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static shift amount is less than zero. " << v2_and << " will be used instead of " << v2 << ".";
    }
    else if(v2 >= 32)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_WARNING, err_code_t::AS_ERR_INVALID_NUMBER, node_array[src2]->get_position());
        msg << "this static shift amount is larger than 31. " << v2_and << " will be used instead of " << v2 << ".";
    }
    // TODO: warn about v1 being larger than 32 bits?
    uint32_t v1(i1.get());
    v1 >>= v2_and;
    i1.set(v1);
    node_array[src1]->set_int64(i1);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply an SMART_MATCH function.
 *
 * This function checks source 1 against source 2. String parameters
 * first get simplified, then the function applies the EQUAL operator
 * against both resulting values.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_SMART_MATCH(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    Node::pointer_t s1(node_array[src1]);
    Node::pointer_t s2(node_array[src2]);

    if(s1->get_type() == Node::node_t::NODE_STRING)
    {
        s1.reset(new Node(Node::node_t::NODE_STRING));
        s1->set_string(node_array[src1]->get_string().simplified());
    }

    if(s2->get_type() == Node::node_t::NODE_STRING)
    {
        s2.reset(new Node(Node::node_t::NODE_STRING));
        s2->set_string(node_array[src2]->get_string().simplified());
    }

    compare_t const c(Node::compare(s1, s2, Node::compare_mode_t::COMPARE_SMART));
    Node::pointer_t result(new Node(c == compare_t::COMPARE_EQUAL
                                    ? Node::node_t::NODE_TRUE
                                    : Node::node_t::NODE_FALSE));

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(result);
    node_array[dst] = result;
}


/** \brief Apply an STRICTLY_EQUAL function.
 *
 * This function checks source 1 against source 2. If source 1 is equal to
 * source 2, then the function saves true in the destination, otherwise
 * it saves false.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_STRICTLY_EQUAL(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    compare_t const c(Node::compare(node_array[src1], node_array[src2], Node::compare_mode_t::COMPARE_STRICT));
    Node::pointer_t result(new Node(c == compare_t::COMPARE_EQUAL
                                    ? Node::node_t::NODE_TRUE
                                    : Node::node_t::NODE_FALSE));

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(result);
    node_array[dst] = result;
}


/** \brief Apply an SUBTRACT function.
 *
 * This function adds two numbers and saves the result in the 3rd position.
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 * \li 2 -- destination
 *
 * \exception exception_internal_error
 * The function may attempt to convert the input to floating point numbers.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_SUBTRACT(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             dst(optimize->f_indexes[2]);

    Node::node_t type1(node_array[src1]->get_type());
    Node::node_t type2(node_array[src2]->get_type());

    // add the numbers together
    if(type1 == Node::node_t::NODE_INT64 && type2 == Node::node_t::NODE_INT64)
    {
        // a - b when a and b are integers
        Int64 i1(node_array[src1]->get_int64());
        Int64 i2(node_array[src2]->get_int64());
        // TODO: err on overflows?
        i1.set(i1.get() - i2.get());
        node_array[src1]->set_int64(i1);
    }
    else
    {
        // make sure a and b are floats, then do a - b as floats
        // TODO: check for NaN and other fun things?
        if(!node_array[src1]->to_float64()
        || !node_array[src2]->to_float64())
        {
            throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a float64."); // LCOV_EXCL_LINE
        }
        Float64 f1(node_array[src1]->get_float64());
        Float64 f2(node_array[src2]->get_float64());
        // TODO: err on overflow?
        f1.set(f1.get() - f2.get());
        node_array[src1]->set_float64(f1);
    }

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src1]);
    node_array[dst] = node_array[src1];
}


/** \brief Apply a SWAP function.
 *
 * This function exchanges a node with another. For example, we can inverse
 * a condition in an 'if()' statement and then swap the first list of
 * statements with the second (i.e. the 'then' part with the 'else' part.)
 *
 * \li 0 -- source 1
 * \li 1 -- source 2
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_SWAP(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]);

    // get the existing pointers and offsets
    Node::pointer_t n1(node_array[src1]);
    Node::pointer_t n2(node_array[src2]);

    Node::pointer_t p1(n1->get_parent());
    Node::pointer_t p2(n2->get_parent());

    Node::pointer_t e1(new Node(Node::node_t::NODE_EMPTY));
    Node::pointer_t e2(new Node(Node::node_t::NODE_EMPTY));

    size_t o1(n1->get_offset());
    size_t o2(n2->get_offset());

    p1->set_child(o1, e1);
    p2->set_child(o2, e2);

    p1->set_child(o1, n2);
    p2->set_child(o2, n1);

    node_array[src1] = n2;
    node_array[src2] = n1;

    // WARNING: we do not use the replace_with() function because we would
    //          otherwise lose the parent and offset information
}


/** \brief Apply an TO_CONDITIONAL function.
 *
 * This function replaces a number with a conditional.
 *
 * \li 0 -- source 1 (true/false expression)
 * \li 1 -- source 2 (result if true)
 * \li 2 -- source 3 (result if false)
 * \li 3 -- destination (destination)
 *
 * \exception exception_internal_error
 * The function may attempt to convert the input to floating point numbers.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_TO_CONDITIONAL(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src1(optimize->f_indexes[0]),
             src2(optimize->f_indexes[1]),
             src3(optimize->f_indexes[2]),
             dst(optimize->f_indexes[3]);

    Node::pointer_t conditional(new Node(Node::node_t::NODE_CONDITIONAL));
    conditional->append_child(node_array[src1]);
    conditional->append_child(node_array[src2]);
    conditional->append_child(node_array[src3]);

    // save the result replacing the specified destination
    node_array[dst]->replace_with(conditional);
    node_array[dst] = conditional;
}


///** \brief Apply a TO_FLOAT64 function.
// *
// * This function transforms a node to a floating point number. The
// * to_float64() HAS to work against that node or you will get an
// * exception.
// *
// * \exception exception_internal_error
// * The function attempts to convert the input to a floating point number.
// * If that fails, this exception is raised. The Optimizer matching mechanism
// * should, however, prevent all such problems.
// *
// * \param[in] node_array  The array of nodes being optimized.
// * \param[in] optimize  The optimization parameters.
// */
//void optimizer_func_TO_FLOAT64(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
//{
//    if(!node_array[optimize->f_indexes[0]]->to_float64())
//    {
//        throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a floating point number."); // LCOV_EXCL_LINE
//    }
//}


/** \brief Apply a TO_INT64 function.
 *
 * This function transforms a node to an integer number. The
 * to_int64() HAS to work against that node or you will get an
 * exception.
 *
 * \exception exception_internal_error
 * The function attempts to convert the input to an integer number.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_TO_INT64(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    if(!node_array[optimize->f_indexes[0]]->to_int64())
    {
        throw exception_internal_error("optimizer used function to_int64() against a node that cannot be converted to an integer."); // LCOV_EXCL_LINE
    }
}


/** \brief Apply a TO_NUMBER function.
 *
 * This function transforms a node to a number. The
 * to_number() HAS to work against that node or you will get an
 * exception.
 *
 * \exception exception_internal_error
 * The function attempts to convert the input to an integer number.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_TO_NUMBER(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    if(!node_array[optimize->f_indexes[0]]->to_number())
    {
        throw exception_internal_error("optimizer used function to_number() against a node that cannot be converted to a number."); // LCOV_EXCL_LINE
    }
}


///** \brief Apply a TO_STRING function.
// *
// * This function transforms a node to a string. The to_string() HAS to work
// * against that node or you will get an exception.
// *
// * \exception exception_internal_error
// * The function attempts to convert the input to a string.
// * If that fails, this exception is raised. The Optimizer matching mechanism
// * should, however, prevent all such problems.
// *
// * \param[in] node_array  The array of nodes being optimized.
// * \param[in] optimize  The optimization parameters.
// */
//void optimizer_func_TO_STRING(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
//{
//    if(!node_array[optimize->f_indexes[0]]->to_string())
//    {
//        throw exception_internal_error("optimizer used function to_string() against a node that cannot be converted to a string."); // LCOV_EXCL_LINE
//    }
//}


/** \brief Apply a WHILE_TRUE_TO_FOREVER function.
 *
 * This function transforms a 'while(true)' in a 'for(;;)' which is a bit
 * smaller.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_WHILE_TRUE_TO_FOREVER(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src(optimize->f_indexes[0]),
             dst(optimize->f_indexes[1]);

    Node::pointer_t statements(node_array[src]);
    Node::pointer_t for_statement(new Node(Node::node_t::NODE_FOR));
    Node::pointer_t e1(new Node(Node::node_t::NODE_EMPTY));
    Node::pointer_t e2(new Node(Node::node_t::NODE_EMPTY));
    Node::pointer_t e3(new Node(Node::node_t::NODE_EMPTY));

    node_array[dst]->replace_with(for_statement);
    node_array[dst] = for_statement;
    for_statement->append_child(e1);
    for_statement->append_child(e2);
    for_statement->append_child(e3);
    for_statement->append_child(statements);
}


/** \brief Internal structure used to define a list of optimization functions.
 *
 * This structure is used to define a list of optimization functions which
 * are used to optimize the tree of nodes.
 *
 * In debug mode, we add the function index so we can make sure that the
 * functions are properly sorted and are all defined. (At least we try, we
 * cannot detect whether the last function is defined because we do not
 * have a "max" definition.)
 */
struct optimizer_optimize_function_t
{
#if defined(_DEBUG) || defined(DEBUG)
    /** \brief The function index.
     *
     * This entry is only added in case the software is compiled in debug
     * mode. It allows our functions to verify that all the functions are
     * defined as required.
     *
     * In non-debug, the functions make use of the table as is, expecting
     * it to be proper (which it should be if our tests do their job
     * properly.)
     */
    optimization_function_t     f_func_index;
#endif

    /** \brief The function pointer.
     *
     * When executing the different optimization functions, we call them
     * using this table. This is faster than using a switch, a much less
     * prone to errors since the function index and the function names
     * are tied together.
     *
     * \param[in] node_array  The array of nodes being optimized.
     * \param[in] optimize  The optimization parameters.
     */
    void                        (*f_func)(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize);
};


#if defined(_DEBUG) || defined(DEBUG)
#define OPTIMIZER_FUNC(name) { optimization_function_t::OPTIMIZATION_FUNCTION_##name, optimizer_func_##name }
#else
#define OPTIMIZER_FUNC(name) { optimizer_func_##name }
#endif

/** \brief List of optimization functions.
 *
 * This table is a list of optimization functions called using
 * optimizer_apply_funtion().
 */
optimizer_optimize_function_t g_optimizer_optimize_functions[] =
{
    /* OPTIMIZATION_FUNCTION_ADD            */ OPTIMIZER_FUNC(ADD),
    /* OPTIMIZATION_FUNCTION_BITWISE_AND    */ OPTIMIZER_FUNC(BITWISE_AND),
    /* OPTIMIZATION_FUNCTION_BITWISE_NOT    */ OPTIMIZER_FUNC(BITWISE_NOT),
    /* OPTIMIZATION_FUNCTION_BITWISE_OR     */ OPTIMIZER_FUNC(BITWISE_OR),
    /* OPTIMIZATION_FUNCTION_BITWISE_XOR    */ OPTIMIZER_FUNC(BITWISE_XOR),
    /* OPTIMIZATION_FUNCTION_COMPARE        */ OPTIMIZER_FUNC(COMPARE),
    /* OPTIMIZATION_FUNCTION_CONCATENATE    */ OPTIMIZER_FUNC(CONCATENATE),
    /* OPTIMIZATION_FUNCTION_DIVIDE         */ OPTIMIZER_FUNC(DIVIDE),
    /* OPTIMIZATION_FUNCTION_EQUAL          */ OPTIMIZER_FUNC(EQUAL),
    /* OPTIMIZATION_FUNCTION_LESS           */ OPTIMIZER_FUNC(LESS),
    /* OPTIMIZATION_FUNCTION_LESS_EQUAL     */ OPTIMIZER_FUNC(LESS_EQUAL),
    /* OPTIMIZATION_FUNCTION_LOGICAL_NOT    */ OPTIMIZER_FUNC(LOGICAL_NOT),
    /* OPTIMIZATION_FUNCTION_LOGICAL_XOR    */ OPTIMIZER_FUNC(LOGICAL_XOR),
    /* OPTIMIZATION_FUNCTION_MATCH          */ OPTIMIZER_FUNC(MATCH),
    /* OPTIMIZATION_FUNCTION_MAXIMUM        */ OPTIMIZER_FUNC(MAXIMUM),
    /* OPTIMIZATION_FUNCTION_MINIMUM        */ OPTIMIZER_FUNC(MINIMUM),
    /* OPTIMIZATION_FUNCTION_MODULO         */ OPTIMIZER_FUNC(MODULO),
    /* OPTIMIZATION_FUNCTION_MOVE           */ OPTIMIZER_FUNC(MOVE),
    /* OPTIMIZATION_FUNCTION_MULTIPLY       */ OPTIMIZER_FUNC(MULTIPLY),
    /* OPTIMIZATION_FUNCTION_NEGATE         */ OPTIMIZER_FUNC(NEGATE),
    /* OPTIMIZATION_FUNCTION_POWER          */ OPTIMIZER_FUNC(POWER),
    /* OPTIMIZATION_FUNCTION_REMOVE         */ OPTIMIZER_FUNC(REMOVE),
    /* OPTIMIZATION_FUNCTION_ROTATE_LEFT    */ OPTIMIZER_FUNC(ROTATE_LEFT),
    /* OPTIMIZATION_FUNCTION_ROTATE_RIGHT   */ OPTIMIZER_FUNC(ROTATE_RIGHT),
    /* OPTIMIZATION_FUNCTION_SET_INTEGER    */ OPTIMIZER_FUNC(SET_INTEGER),
    ///* OPTIMIZATION_FUNCTION_SET_FLOAT      */ OPTIMIZER_FUNC(SET_FLOAT),
    /* OPTIMIZATION_FUNCTION_SET_NODE_TYPE  */ OPTIMIZER_FUNC(SET_NODE_TYPE),
    /* OPTIMIZATION_FUNCTION_SHIFT_LEFT     */ OPTIMIZER_FUNC(SHIFT_LEFT),
    /* OPTIMIZATION_FUNCTION_SHIFT_RIGHT    */ OPTIMIZER_FUNC(SHIFT_RIGHT),
    /* OPTIMIZATION_FUNCTION_SHIFT_RIGHT_UNSIGNED */ OPTIMIZER_FUNC(SHIFT_RIGHT_UNSIGNED),
    /* OPTIMIZATION_FUNCTION_SMART_MATCH    */ OPTIMIZER_FUNC(SMART_MATCH),
    /* OPTIMIZATION_FUNCTION_STRICTLY_EQUAL */ OPTIMIZER_FUNC(STRICTLY_EQUAL),
    /* OPTIMIZATION_FUNCTION_SUBTRACT       */ OPTIMIZER_FUNC(SUBTRACT),
    /* OPTIMIZATION_FUNCTION_SWAP           */ OPTIMIZER_FUNC(SWAP),
    /* OPTIMIZATION_FUNCTION_TO_CONDITIONAL */ OPTIMIZER_FUNC(TO_CONDITIONAL),
    ///* OPTIMIZATION_FUNCTION_TO_FLOAT64     */ OPTIMIZER_FUNC(TO_FLOAT64),
    /* OPTIMIZATION_FUNCTION_TO_INT64       */ OPTIMIZER_FUNC(TO_INT64),
    /* OPTIMIZATION_FUNCTION_TO_NUMBER      */ OPTIMIZER_FUNC(TO_NUMBER),
    ///* OPTIMIZATION_FUNCTION_TO_STRING      */ OPTIMIZER_FUNC(TO_STRING),
    /* OPTIMIZATION_FUNCTION_WHILE_TRUE_TO_FOREVER */ OPTIMIZER_FUNC(WHILE_TRUE_TO_FOREVER)
};

/** \brief The size of the optimizer list of optimization functions.
 *
 * This size defines the number of functions available in the optimizer
 * optimize functions table. This size should be equal to the last
 * optimization function number plus one. This is tested in
 * optimizer_apply_function() when in debug mode.
 */
size_t const g_optimizer_optimize_functions_size = sizeof(g_optimizer_optimize_functions) / sizeof(g_optimizer_optimize_functions[0]);


/** \brief Apply optimization functions to a node.
 *
 * This function applies one optimization function to a node. In many
 * cases, the node itself gets replaced by a child.
 *
 * In debug mode the function may raise an exception if a bug is detected
 * in the table data.
 *
 * \param[in] node  The node being optimized.
 * \param[in] optimize  The optimization function to apply.
 */
void apply_one_function(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize)
{
#if defined(_DEBUG) || defined(DEBUG)
    // this loop will not catch the last entries if missing, otherwise,
    // 'internal' functions are detected immediately, hence our other
    // test below
    for(size_t idx(0); idx < g_optimizer_optimize_functions_size; ++idx)
    {
        if(g_optimizer_optimize_functions[idx].f_func_index != static_cast<optimization_function_t>(idx))
        {
            std::cerr << "INTERNAL ERROR: function table index " << idx << " is not valid."            // LCOV_EXCL_LINE
                      << std::endl;                                                                    // LCOV_EXCL_LINE
            throw exception_internal_error("INTERNAL ERROR: function table index is not valid (forgot to add a function to the table?)"); // LCOV_EXCL_LINE
        }
    }
    // make sure the function exists, otherwise we'd just crash (not good!)
    if(static_cast<size_t>(optimize->f_function) >= g_optimizer_optimize_functions_size)
    {
        std::cerr << "INTERNAL ERROR: f_function is too large " << static_cast<int>(optimize->f_function) << " > "      // LCOV_EXCL_LINE
                  << sizeof(g_optimizer_optimize_functions) / sizeof(g_optimizer_optimize_functions[0])                 // LCOV_EXCL_LINE
                  << std::endl;                                                                                         // LCOV_EXCL_LINE
        throw exception_internal_error("INTERNAL ERROR: f_function is out of range (forgot to add a function to the table?)"); // LCOV_EXCL_LINE
    }
#endif
    g_optimizer_optimize_functions[static_cast<size_t>(optimize->f_function)].f_func(node_array, optimize);
}


}
// noname namespace


/** \brief Apply all the optimization functions.
 *
 * This function applies all the optimization functions on the specified
 * array of nodes one after the other.
 *
 * If a parameter (node) is invalid for a function, an exception is raised.
 * Because the optimizer is expected to properly match nodes before
 * an optimization can be applied, the possibility for an error here
 * should be zero.
 *
 * \param[in] node_array  The array of nodes as generated by the matching code.
 * \param[in] optimize  Pointer to the first optimization function.
 * \param[in] optimize_size  The number of optimization functions to apply.
 */
void apply_functions(node_pointer_vector_t& node_array, optimization_optimize_t const *optimize, size_t optimize_size)
{
    for(; optimize_size > 0; --optimize_size, ++optimize)
    {
        apply_one_function(node_array, optimize);
    }
}


}
// namespace optimizer_details
}
// namespace as2js

// vim: ts=4 sw=4 et
