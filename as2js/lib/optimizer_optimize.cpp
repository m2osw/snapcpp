/* optimizer_optimize.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "optimizer_tables.h"

#include    "as2js/exceptions.h"


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
void optimizer_func_ADD(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
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
void optimizer_func_CONCATENATE(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
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
void optimizer_func_MOVE(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src(optimize->f_indexes[0]),
             dst(optimize->f_indexes[1]);

    // move the source in place of the destination
    node_array[dst]->replace_with(node_array[src]);
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
void optimizer_func_NEGATE(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
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
}


/** \brief Apply a LOGICAL_NOT function.
 *
 * This function applies a logical not and saves the result in the
 * 2nd position.
 *
 * The logical not is applied whatever the literal after a conversion
 * to a boolean. If the conversion fails, then an exception is raised.
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
void optimizer_func_LOGICAL_NOT(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src(optimize->f_indexes[0]),
             dst(optimize->f_indexes[1]);

    if(!node_array[src]->to_boolean())
    {
        throw exception_internal_error("optimizer used function to_boolean() against a node that cannot be converted to a boolean."); // LCOV_EXCL_LINE
    }
    bool b(node_array[src]->get_boolean());
    node_array[src]->set_boolean(!b);

    // save the result replacing the destination as specified
    node_array[dst]->replace_with(node_array[src]);
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
void optimizer_func_REMOVE(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
{
    uint32_t src(optimize->f_indexes[0]);

    // simply remove from the parent, the smart pointers take care of the rest
    node_array[src]->set_parent(nullptr);
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
void optimizer_func_SUBTRACT(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
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
}


/** \brief Apply a TO_FLOAT64 function.
 *
 * This function transforms a node to a floating point number. The
 * to_float64() HAS to work against that node or you will get an
 * exception.
 *
 * \exception exception_internal_error
 * The function attempts to convert the input to a floating point number.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_TO_FLOAT64(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
{
    if(!node_array[optimize->f_indexes[0]]->to_float64())
    {
        throw exception_internal_error("optimizer used function to_float64() against a node that cannot be converted to a floating point number."); // LCOV_EXCL_LINE
    }
}


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
void optimizer_func_TO_INT64(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
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
void optimizer_func_TO_NUMBER(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
{
    if(!node_array[optimize->f_indexes[0]]->to_number())
    {
        throw exception_internal_error("optimizer used function to_number() against a node that cannot be converted to a number."); // LCOV_EXCL_LINE
    }
}


/** \brief Apply a TO_STRING function.
 *
 * This function transforms a node to a string. The to_string() HAS to work
 * against that node or you will get an exception.
 *
 * \exception exception_internal_error
 * The function attempts to convert the input to a string.
 * If that fails, this exception is raised. The Optimizer matching mechanism
 * should, however, prevent all such problems.
 *
 * \param[in] node_array  The array of nodes being optimized.
 * \param[in] optimize  The optimization parameters.
 */
void optimizer_func_TO_STRING(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
{
    if(!node_array[optimize->f_indexes[0]]->to_string())
    {
        throw exception_internal_error("optimizer used function to_string() against a node that cannot be converted to a string."); // LCOV_EXCL_LINE
    }
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
    void                        (*f_func)(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize);
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
    /* OPTIMIZATION_FUNCTION_CONCATENATE    */ OPTIMIZER_FUNC(CONCATENATE),
    /* OPTIMIZATION_FUNCTION_MOVE           */ OPTIMIZER_FUNC(MOVE),
    /* OPTIMIZATION_FUNCTION_NEGATE         */ OPTIMIZER_FUNC(NEGATE),
    /* OPTIMIZATION_FUNCTION_LOGICAL_NOT    */ OPTIMIZER_FUNC(LOGICAL_NOT),
    /* OPTIMIZATION_FUNCTION_REMOVE         */ OPTIMIZER_FUNC(REMOVE),
    /* OPTIMIZATION_FUNCTION_SUBTRACT       */ OPTIMIZER_FUNC(SUBTRACT),
    /* OPTIMIZATION_FUNCTION_TO_FLOAT64     */ OPTIMIZER_FUNC(TO_FLOAT64),
    /* OPTIMIZATION_FUNCTION_TO_INT64       */ OPTIMIZER_FUNC(TO_INT64),
    /* OPTIMIZATION_FUNCTION_TO_NUMBER      */ OPTIMIZER_FUNC(TO_NUMBER),
    /* OPTIMIZATION_FUNCTION_TO_STRING      */ OPTIMIZER_FUNC(TO_STRING)
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
void apply_one_function(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize)
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
void apply_functions(node_pointer_vector_t const& node_array, optimization_optimize_t const *optimize, size_t optimize_size)
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
