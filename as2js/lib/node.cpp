/* node.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/node.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"

#include    <controlled_vars/controlled_vars_auto_enum_init.h>

#include    <algorithm>
#include    <sstream>
#include    <iomanip>


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE  ***********************************************************/
/**********************************************************************/
/**********************************************************************/



/** \brief Initialize a node.
 *
 * This function initializes a new node. The specified type is assigned to
 * the new node as expected.
 *
 * If the \p type parameter does not represent a valid type of node, then
 * the function throws. This means only valid type of nodes can be created.
 *
 * \param[in] type  The type of node to create.
 */
Node::Node(node_t type)
    : f_type(type)
    //, f_flags() -- auto-init
    //, f_attributes() -- auto-init
    //, f_switch_operator(NODE_UNKNOWN) -- auto-init
    //, f_lock(0) -- auto-init
    //, f_position() -- auto-init
    //, f_int() -- auto-init
    //, f_float() -- auto-init
    //, f_str() -- auto-init
    //, f_parent() -- auto-init
    //, f_offset(0) -- auto-init
    //, f_children() -- auto-init
    //, f_link() -- auto-init
    //, f_variables() -- auto-init
    //, f_labels() -- auto-init
{
    switch(type)
    {
    case node_t::NODE_EOF:
    case node_t::NODE_UNKNOWN:
    case node_t::NODE_ADD:
    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_ASSIGNMENT:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_CLOSE_CURVLY_BRACKET:
    case node_t::NODE_CLOSE_PARENTHESIS:
    case node_t::NODE_CLOSE_SQUARE_BRACKET:
    case node_t::NODE_COLON:
    case node_t::NODE_COMMA:
    case node_t::NODE_CONDITIONAL:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_GREATER:
    case node_t::NODE_LESS:
    case node_t::NODE_LOGICAL_NOT:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_OPEN_CURVLY_BRACKET:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_MEMBER:
    case node_t::NODE_SEMICOLON:
    case node_t::NODE_SUBTRACT:
    case node_t::NODE_ARRAY:
    case node_t::NODE_ARRAY_LITERAL:
    case node_t::NODE_AS:
    case node_t::NODE_ASSIGNMENT_ADD:
    case node_t::NODE_ASSIGNMENT_BITWISE_AND:
    case node_t::NODE_ASSIGNMENT_BITWISE_OR:
    case node_t::NODE_ASSIGNMENT_BITWISE_XOR:
    case node_t::NODE_ASSIGNMENT_DIVIDE:
    case node_t::NODE_ASSIGNMENT_LOGICAL_AND:
    case node_t::NODE_ASSIGNMENT_LOGICAL_OR:
    case node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
    case node_t::NODE_ASSIGNMENT_MAXIMUM:
    case node_t::NODE_ASSIGNMENT_MINIMUM:
    case node_t::NODE_ASSIGNMENT_MODULO:
    case node_t::NODE_ASSIGNMENT_MULTIPLY:
    case node_t::NODE_ASSIGNMENT_POWER:
    case node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
    case node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
    case node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_ASSIGNMENT_SUBTRACT:
    case node_t::NODE_ATTRIBUTES:
    case node_t::NODE_AUTO:
    case node_t::NODE_BREAK:
    case node_t::NODE_CALL:
    case node_t::NODE_CASE:
    case node_t::NODE_CATCH:
    case node_t::NODE_CLASS:
    case node_t::NODE_CONST:
    case node_t::NODE_CONTINUE:
    case node_t::NODE_DEBUGGER:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DEFAULT:
    case node_t::NODE_DELETE:
    case node_t::NODE_DIRECTIVE_LIST:
    case node_t::NODE_DO:
    case node_t::NODE_ELSE:
    case node_t::NODE_EMPTY:
    case node_t::NODE_ENUM:
    case node_t::NODE_EQUAL:
    case node_t::NODE_EXCLUDE:
    case node_t::NODE_EXTENDS:
    case node_t::NODE_FALSE:
    case node_t::NODE_FINALLY:
    case node_t::NODE_FLOAT64:
    case node_t::NODE_FOR:
    case node_t::NODE_FUNCTION:
    case node_t::NODE_GOTO:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_IF:
    case node_t::NODE_IMPLEMENTS:
    case node_t::NODE_IMPORT:
    case node_t::NODE_IN:
    case node_t::NODE_INCLUDE:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_INSTANCEOF:
    case node_t::NODE_INT64:
    case node_t::NODE_INTERFACE:
    case node_t::NODE_IS:
    case node_t::NODE_LABEL:
    case node_t::NODE_LESS_EQUAL:
    case node_t::NODE_LIST:
    case node_t::NODE_LOGICAL_AND:
    case node_t::NODE_LOGICAL_OR:
    case node_t::NODE_LOGICAL_XOR:
    case node_t::NODE_MATCH:
    case node_t::NODE_MAXIMUM:
    case node_t::NODE_MINIMUM:
    case node_t::NODE_NAME:
    case node_t::NODE_NAMESPACE:
    case node_t::NODE_NEW:
    case node_t::NODE_NOT_EQUAL:
    case node_t::NODE_NULL:
    case node_t::NODE_OBJECT_LITERAL:
    case node_t::NODE_PACKAGE:
    case node_t::NODE_PARAM:
    case node_t::NODE_PARAMETERS:
    case node_t::NODE_PARAM_MATCH:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
    case node_t::NODE_POWER:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_PROGRAM:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_RANGE:
    case node_t::NODE_REGULAR_EXPRESSION:
    case node_t::NODE_REST:
    case node_t::NODE_RETURN:
    case node_t::NODE_ROOT:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SCOPE:
    case node_t::NODE_SET:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
    case node_t::NODE_STRING:
    case node_t::NODE_SUPER:
    case node_t::NODE_SWITCH:
    case node_t::NODE_THIS:
    case node_t::NODE_THROW:
    case node_t::NODE_TRUE:
    case node_t::NODE_TRY:
    case node_t::NODE_TYPE:
    case node_t::NODE_TYPEOF:
    case node_t::NODE_UNDEFINED:
    case node_t::NODE_USE:
    case node_t::NODE_VAR:
    case node_t::NODE_VARIABLE:
    case node_t::NODE_VAR_ATTRIBUTES:
    case node_t::NODE_VIDENTIFIER:
    case node_t::NODE_VOID:
    case node_t::NODE_WHILE:
    case node_t::NODE_WITH:
        break;

    default:
        // ERROR: some values are not valid as a type
        throw exception_incompatible_node_type("invalid type used to create a node");

    }
}



/**********************************************************************/
/**********************************************************************/
/***  NODE SWITCH  ****************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Retrieve the switch operator.
 *
 * A switch statement can be constrained to use a specific operator
 * using the with() syntax as in:
 *
 * \code
 * switch(foo) with(===)
 * {
 *    ...
 * }
 * \endcode
 *
 * This operator is saved in the switch node and can later be retrieved
 * with this function.
 *
 * \exception exception_internal_error
 * If the function is called on a node of a type other than NODE_SWITCH
 * then this exception is raised.
 *
 * \return The operator of the switch statement, or NODE_UNKNOWN if undefined.
 */
Node::node_t Node::get_switch_operator() const
{
    if(node_t::NODE_SWITCH != f_type)
    {
        throw exception_internal_error("INTERNAL ERROR: set_switch_operator() called on a node which is not a switch node.");
    }

    return f_switch_operator;
}


/** \brief Set the switch statement operator.
 *
 * This function saves the operator defined following the switch statement
 * using the with() instruction as in:
 *
 * \code
 * switch(foo) with(===)
 * {
 *    ...
 * }
 * \endcode
 *
 * \exception exception_internal_error
 * If the function is called on a node of a type other than NODE_SWITCH
 * then this exception is raised.
 *
 * \param[in] op  The new operator to save in this switch statement.
 */
void Node::set_switch_operator(node_t op)
{
    if(node_t::NODE_SWITCH != f_type)
    {
        throw exception_internal_error("INTERNAL ERROR: set_switch_operator() called on a node which is not a switch node.");
    }

    f_switch_operator = op;
}


/**********************************************************************/
/**********************************************************************/
/***  NODE POSITION  **************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Create a new node with the given type.
 *
 * This function creates a new node that is expected to be used as a
 * replacement of this node.
 *
 * Note that this node does not get modified by this call.
 *
 * \param[in] type  The type of the new node.
 *
 * \return A new node pointer.
 */
Node::pointer_t Node::create_replacement(node_t type) const
{
    Node::pointer_t n(new Node(type));

    // this is why we want to have a function instead of doing new Node().
    n->f_position = f_position;

    return n;
}


/** \brief Change the position of the node.
 *
 * As you are reading a file, a position object gets updated. That position
 * object represents the location where different token are found in the
 * source files. It is saved in a node as it is created to represent the
 * position where the data was found. This helps in indicating to the user
 * where an error occurred.
 *
 * The position used as input can later change as the node keeps a copy of
 * the parameter passed to it.
 *
 * The position can later be retrieved with the get_position() function.
 *
 * When the compiler creates new nodes as required, it generally will make
 * use of the create_replacement() function which creates a new node with
 * a new type, but keeps the position information of the old node.
 *
 * \param[in] position  The new position to copy in this node.
 */
void Node::set_position(Position const& position)
{
    f_position = position;
}


/** \brief The position of the node.
 *
 * This function returns a reference to the position of the node.
 * The position represents the filename, line number, character position,
 * function name, etc. where this specific node was read. It can be used
 * to print out the line to the user and to show him exactly where the
 * error occurred.
 *
 * This position can be changed with the set_position() function. By
 * default a node has a default position: no file name, no function name,
 * and positions are all set to 1.
 *
 * \return The position of this node.
 */
Position const& Node::get_position() const
{
    return f_position;
}


/**********************************************************************/
/**********************************************************************/
/***  NODE LINK  ******************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Save a link in this node.
 *
 * This function saves a link pointer in this node. It can later be
 * retrieved using the get_link() function.
 *
 * If a link was already defined at that offset, the function raises
 * an exception and the existing offset is not modified.
 *
 * It is possible to clear a link by passing an empty smart pointer
 * down (i.e. pass nullptr.) If you first clear a link in this way,
 * you can then replace it with another pointer.
 *
 * \code
 *     // do not throw because the link is already defined:
 *     node->set_link(Node::link_t::LINK_TYPE, nullptr);
 *     node->set_link(Node::link_t::LINK_TYPE, link);
 * \endcode
 *
 * Links are used to save information about a node such as its
 * type and attributes.
 *
 * \note
 * Links are saved as full smart pointers, not weak pointers. This means
 * a node that references another in this way may generate loops that
 * will not easily break when trying to release the whole tree.
 *
 * \exception exception_index_out_of_range
 * The index is out of range.
 *
 * \exception exception_internal_error
 * The link at that index is already defined and the function was called
 * anyway. This is an internal error because you should check whether the
 * value was already defined and if so use that value.
 *
 * \param[in] index  The index of the link to save.
 * \param[in] link  A smart pointer to the link.
 *
 * \return A smart pointer to this link node.
 */
void Node::set_link(link_t index, pointer_t link)
{
    modifying();

    if(index >= link_t::LINK_max)
    {
        throw exception_index_out_of_range("set_link() called with an index out of bounds.");
    }

    // make sure the size is reserved on first set
    if(f_link.empty())
    {
        f_link.resize(static_cast<vector_of_pointers_t::size_type>(link_t::LINK_max));
    }

    if(link)
    {
        // link already set?
        if(f_link[static_cast<size_t>(index)])
        {
            throw exception_internal_error("a link was set twice at the same offset");
        }

        f_link[static_cast<size_t>(index)] = link;
    }
    else
    {
        f_link[static_cast<size_t>(index)].reset();
    }
}


/** \brief Retrieve a link previously saved with set_link().
 *
 * This function returns a pointer to a link that was previously
 * saved in this node using the set_link() function.
 *
 * Links are used to save information about a node such as its
 * type and attributes.
 *
 * \param[in] index  The index of the link to retrieve.
 *
 * \return A smart pointer to this link node.
 */
Node::pointer_t Node::get_link(link_t index)
{
    if(index >= link_t::LINK_max)
    {
        throw exception_index_out_of_range("get_link() called with an index out of bounds.");
    }

    if(f_link.empty())
    {
        return nullptr;
    }

    return f_link[static_cast<size_t>(index)];
}


/**********************************************************************/
/**********************************************************************/
/***  NODE VARIABLE  **************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Add avariable to this node.
 *
 * A node can hold pointers to variable nodes. This is used to handle variable
 * scopes properly. Note that the \p variable parameter must be a node of
 * type NODE_VARIABLE.
 *
 * \note
 * This is not an execution environment and as such the variables are simply
 * added one after another (not sorted, no attempt to later retrieve
 * variables by name.) This may change in the future.
 *
 * \todo
 * Add a test of the node type so we can make sure we do not call this
 * function on nodes that cannot have variables.
 *
 * \exception exception_incompatible_node_type
 * This exception is raised if the \p variable parameter is not of type
 * NODE_VARIABLE.
 *
 * \param[in] variable  The variable to be added.
 */
void Node::add_variable(pointer_t variable)
{
    if(node_t::NODE_VARIABLE != variable->f_type)
    {
        throw exception_incompatible_node_type("the variable parameter of the add_variable() function must be a NODE_VARIABLE");
    }
    // TODO: test the destination (i.e. this) to make sure only valid nodes
    //       accept variables

    f_variables.push_back(variable);
}


/** \brief Retrieve the number of variables defined in this node.
 *
 * A node can hold variable pointers. This is used to handle variable
 * scopes properly.
 *
 * \todo
 * Add a test of the node type so we can make sure we do not call this
 * function on nodes that cannot have variables.
 *
 * \return The number of variables currently held in this node.
 */
size_t Node::get_variable_size() const
{
    return f_variables.size();
}


/** \brief Retrieve the variable at the specified index.
 *
 * This function retrieves the variable at the specified index. If the
 * index is out of the variable array bounds, then the function raises
 * an error.
 *
 * \todo
 * Add a test of the node type so we can make sure we do not call this
 * function on nodes that cannot have variables.
 *
 * \param[in] index  The index of the variable to retrieve.
 */
Node::pointer_t Node::get_variable(int index) const
{
    return f_variables.at(index);
}


/**********************************************************************/
/**********************************************************************/
/***  NODE LABEL  *****************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Add a label to a function.
 *
 * This function adds a label to this function node. Labels are saved
 * using a map so we can quickly find them.
 *
 * \note
 * After a label was added to a function, its name should never get
 * modified or it will be out of synchronization with the function.
 *
 * \exception exception_incompatible_node_type
 * If this function is called with objects other than a NODE_LABEL
 * as the label parameter and a NODE_FUNCTION as 'this' parameter,
 * then this exception is raised.
 *
 * \param[in] label  A smart pointer to the label node to add.
 */
void Node::add_label(pointer_t label)
{
    if(node_t::NODE_LABEL != label->f_type
    || node_t::NODE_FUNCTION != f_type)
    {
        throw exception_incompatible_node_type("invalid type of node to call add_label() with");
    }

    f_labels[label->f_str] = label;
}


/** \brief Find a label previously added with the add_label() function.
 *
 * This function checks whether a label was defined in this function. If
 * so, then its smart pointer gets returned.
 *
 * The \p name parameter represents the name of the label exactly. The
 * returned label will have the same name.
 *
 * \param[in] name  The name of the label to retrieve.
 */
Node::pointer_t Node::find_label(String const& name) const
{
    map_of_pointers_t::const_iterator it(f_labels.find(name));
    return it == f_labels.end() ? pointer_t() : it->second;
}




}
// namespace as2js

// vim: ts=4 sw=4 et
