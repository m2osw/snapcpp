/* node_tree.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/node.h"

#include    "as2js/exceptions.h"

#include    <algorithm>


/** \file
 * \brief Handle the node tree.
 *
 * This file includes the implementation of the various
 * functions used to handle the tree of nodes.
 *
 * The main function is the set_parent() function, which is
 * used to manage the tree (parent/children relationships).
 *
 * Most of the other functions call the set_parent() function
 * after some verifications and with the parameters as
 * expected.
 *
 * Note that all nodes are expected to live in a tree. There
 * are some cases when one node has more than one list of
 * children. These are the links and variables as defined
 * by their respective function implementations. Those are
 * not handled in the tree, instead the Node object includes
 * another set of Node::pointer_t arrays to handle those
 * special cases.
 *
 * The parent reference is a weak pointer. This allows a
 * parent to get rid of a child without too much work.
 */


namespace as2js
{



/**********************************************************************/
/**********************************************************************/
/***  NODE TREE  ******************************************************/
/**********************************************************************/
/**********************************************************************/



/** \brief This function sets the parent of a node.
 *
 * This function is the only function that handles the tree of nodes,
 * in other words, the only one that modifies the f_parent and
 * f_children pointers. It is done that way to make 100% sure (assuming
 * it is itself correct) that we do not mess up the tree.
 *
 * This node loses its current parent, and thus is removed from the
 * list of children of that parent. Then is is assigned the new
 * parent as passed to this function.
 *
 * If an \p index is specified, the child is inserted at that specific
 * location. Otherwise the child is appended.
 *
 * The function does nothing if the current parent is the same as the
 * new parent and the default \p index is used (-1).
 *
 * Use an \p index of 0 to insert the item at the start of the list
 * of children. Use an \p index of get_children_size() to force the
 * child at the end of the list even if the parent remains the same.
 *
 * Helper functions are available to make more sense of the usage of
 * this function but they all are based on the set_parent() function:
 *
 * \li delete_child() -- delete a child at that specific index.
 * \li append_child() -- append a child to this parent.
 * \li insert_child() -- insert a child to this parent.
 * \li set_child() -- replace a child with another in this parent.
 * \li replace_with() -- replace a child with another not knowing its offset.
 *
 * \note
 * This Node and the \p parent Node must not be locked.
 * If the parent is being changed, then the other existing parent
 * must also not be locked either.
 *
 * \note
 * The vector of children of the parent node changes, be careful.
 * Whenever possible, to avoid bugs, you may want to consider using
 * the lock() function through the NodeLock object to avoid having
 * such changes happen on nodes you are currently working on.
 *
 * \exception exception_incompatible_node_type
 * The parent must be a node of a type which is compatible with
 * being a parent. We actually limit the type to exactly and just
 * and only the types of nodes that receive children. For example,
 * a NODE_INT64 cannot receive children. Trying to add a child
 * to such a node raises this exception. Similarly, some nodes,
 * such as NODE_CLOSE_PARENTHESIS, cannot be children of another.
 * The same exception is raised if such a node receives a parent.
 *
 * \exception exception_internal_error
 * When the Node already had a parent and it gets replaced, we
 * have to remove that existing parent first. This exception is
 * raised if we cannot find this Node in the list of children of
 * the existing parent.
 *
 * \exception exception_index_out_of_range
 * If the \p index parameter is larger than the number of children
 * currently available in the new parent or is negative, then this
 * exception is raised. Note that if the index is -1, then the
 * correct value is used to append the child.
 *
 * \param[in] parent  The new parent of the node. May be set to nullptr.
 * \param[in] index  The position where the new item is inserted in the parent
 *                   array of children. If -1, append at the end of the list.
 *
 * \sa lock()
 * \sa get_parent()
 * \sa get_children_size()
 * \sa delete_child()
 * \sa append_child()
 * \sa insert_child()
 * \sa set_child()
 * \sa replace_with()
 * \sa get_child()
 * \sa find_first_child()
 * \sa find_next_child()
 * \sa clean_tree()
 * \sa get_offset()
 */
void Node::set_parent(pointer_t parent, int index)
{
    // we are modifying the child and both parents
    modifying();

    if(parent)
    {
        parent->modifying();
    }

    Node::pointer_t p(f_parent.lock());
    if(parent != p && p)
    {
        p->modifying();
    }

    // already a child of that parent?
    // (although in case of an insert, we force the re-parent
    // to the right location)
    if(parent == p && index == -1)
    {
        return;
    }

    // tests to make sure that the parent accepts children
    // (if we got a parent pointer)
    if(parent) switch(parent->get_type())
    {
    case node_t::NODE_UNKNOWN: // this can be anything so we keep it here
    case node_t::NODE_ADD:
    case node_t::NODE_BITWISE_AND:
    case node_t::NODE_BITWISE_NOT:
    case node_t::NODE_ASSIGNMENT:
    case node_t::NODE_BITWISE_OR:
    case node_t::NODE_BITWISE_XOR:
    case node_t::NODE_CONDITIONAL:
    case node_t::NODE_DIVIDE:
    case node_t::NODE_GREATER:
    case node_t::NODE_LESS:
    case node_t::NODE_LOGICAL_NOT:
    case node_t::NODE_MODULO:
    case node_t::NODE_MULTIPLY:
    case node_t::NODE_MEMBER:
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
    case node_t::NODE_CALL:
    case node_t::NODE_CASE:
    case node_t::NODE_CATCH:
    case node_t::NODE_CLASS:
    case node_t::NODE_COMPARE:
    case node_t::NODE_DEBUGGER:
    case node_t::NODE_DECREMENT:
    case node_t::NODE_DELETE:
    case node_t::NODE_DIRECTIVE_LIST:
    case node_t::NODE_DO:
    case node_t::NODE_ENSURE:
    case node_t::NODE_ENUM:
    case node_t::NODE_EQUAL:
    case node_t::NODE_EXCLUDE:
    case node_t::NODE_EXPORT:
    case node_t::NODE_EXTENDS:
    case node_t::NODE_FINALLY:
    case node_t::NODE_FOR:
    case node_t::NODE_FUNCTION:
    case node_t::NODE_GREATER_EQUAL:
    case node_t::NODE_IF:
    case node_t::NODE_IMPLEMENTS:
    case node_t::NODE_IMPORT:
    case node_t::NODE_IN:
    case node_t::NODE_INCLUDE:
    case node_t::NODE_INCREMENT:
    case node_t::NODE_INSTANCEOF:
    case node_t::NODE_INTERFACE:
    case node_t::NODE_INVARIANT:
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
    case node_t::NODE_NOT_MATCH:
    case node_t::NODE_OBJECT_LITERAL:
    case node_t::NODE_PACKAGE:
    case node_t::NODE_PARAM:
    case node_t::NODE_PARAMETERS:
    case node_t::NODE_PARAM_MATCH:
    case node_t::NODE_POST_DECREMENT:
    case node_t::NODE_POST_INCREMENT:
    case node_t::NODE_POWER:
    case node_t::NODE_PROGRAM:
    case node_t::NODE_RANGE:
    case node_t::NODE_REQUIRE:
    case node_t::NODE_RETURN:
    case node_t::NODE_ROOT:
    case node_t::NODE_ROTATE_LEFT:
    case node_t::NODE_ROTATE_RIGHT:
    case node_t::NODE_SCOPE:
    case node_t::NODE_SET:
    case node_t::NODE_SHIFT_LEFT:
    case node_t::NODE_SHIFT_RIGHT:
    case node_t::NODE_SHIFT_RIGHT_UNSIGNED:
    case node_t::NODE_SMART_MATCH:
    case node_t::NODE_STRICTLY_EQUAL:
    case node_t::NODE_STRICTLY_NOT_EQUAL:
    case node_t::NODE_SUPER:
    case node_t::NODE_SWITCH:
    case node_t::NODE_SYNCHRONIZED:
    case node_t::NODE_THROW:
    case node_t::NODE_THROWS:
    case node_t::NODE_TRY:
    case node_t::NODE_TYPE:
    case node_t::NODE_TYPEOF:
    case node_t::NODE_USE:
    case node_t::NODE_VAR:
    case node_t::NODE_VARIABLE:
    case node_t::NODE_VAR_ATTRIBUTES:
    case node_t::NODE_WHILE:
    case node_t::NODE_WITH:
    case node_t::NODE_YIELD:
        break;

    // All those node types are assumed to never support a child
    case node_t::NODE_ABSTRACT:
    case node_t::NODE_AUTO:
    case node_t::NODE_BOOLEAN:
    case node_t::NODE_BREAK:
    case node_t::NODE_BYTE:
    case node_t::NODE_CHAR:
    case node_t::NODE_CLOSE_CURVLY_BRACKET:
    case node_t::NODE_CLOSE_PARENTHESIS:
    case node_t::NODE_CLOSE_SQUARE_BRACKET:
    case node_t::NODE_COLON:
    case node_t::NODE_COMMA:
    case node_t::NODE_CONST:
    case node_t::NODE_CONTINUE:
    case node_t::NODE_DEFAULT:
    case node_t::NODE_DOUBLE:
    case node_t::NODE_ELSE:
    case node_t::NODE_EMPTY:
    case node_t::NODE_EOF:
    case node_t::NODE_FINAL:
    case node_t::NODE_FLOAT:
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_INLINE:
    case node_t::NODE_INT64:
    case node_t::NODE_FALSE:
    case node_t::NODE_FLOAT64:
    case node_t::NODE_GOTO:
    case node_t::NODE_LONG:
    case node_t::NODE_NATIVE:
    case node_t::NODE_NULL:
    case node_t::NODE_OPEN_CURVLY_BRACKET:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_PRIVATE:
    case node_t::NODE_PROTECTED:
    case node_t::NODE_PUBLIC:
    case node_t::NODE_REGULAR_EXPRESSION:
    case node_t::NODE_REST:
    case node_t::NODE_SEMICOLON:
    case node_t::NODE_SHORT:
    case node_t::NODE_STATIC:
    case node_t::NODE_STRING:
    case node_t::NODE_THEN:
    case node_t::NODE_THIS:
    case node_t::NODE_TRANSIENT:
    case node_t::NODE_TRUE:
    case node_t::NODE_UNDEFINED:
    case node_t::NODE_VIDENTIFIER:
    case node_t::NODE_VOID:
    case node_t::NODE_VOLATILE:
    case node_t::NODE_other:        // for completeness
    case node_t::NODE_max:          // for completeness
        // ERROR: some values are not valid as a type
        if(parent->get_parent())
        {
            std::cerr << *parent->get_parent() << std::endl;
        }
        throw exception_incompatible_node_type(std::string("invalid type: \"")
                    + parent->get_type_name() + "\" used as a parent node of child with type: \""
                    + get_type_name() + "\".");

    }

    // verify that 'this' can be a child
    switch(f_type)
    {
    case node_t::NODE_CLOSE_CURVLY_BRACKET:
    case node_t::NODE_CLOSE_PARENTHESIS:
    case node_t::NODE_CLOSE_SQUARE_BRACKET:
    case node_t::NODE_COLON:
    case node_t::NODE_COMMA:
    case node_t::NODE_ELSE:
    case node_t::NODE_THEN:
    case node_t::NODE_EOF:
    case node_t::NODE_OPEN_CURVLY_BRACKET:
    case node_t::NODE_OPEN_PARENTHESIS:
    case node_t::NODE_OPEN_SQUARE_BRACKET:
    case node_t::NODE_ROOT: // correct?
    case node_t::NODE_SEMICOLON:
    case node_t::NODE_other:        // for completeness
    case node_t::NODE_max:          // for completeness
        throw exception_incompatible_node_type("invalid type used as a child node");

    default:
        // all others can be children (i.e. most everything)
        break;

    }

    if(p)
    {
        // very similar to the get_offset() call only we want the iterator
        // in this case, not the index
        pointer_t me(shared_from_this());
        vector_of_pointers_t::iterator it(std::find(p->f_children.begin(), p->f_children.end(), me));
        if(it == p->f_children.end())
        {
            throw exception_internal_error("trying to remove a child from a parent which does not know about that child"); // LCOV_EXCL_LINE
        }
        p->f_children.erase(it);
        f_parent.reset();
    }

    if(parent)
    {
        if(index == -1)
        {
            parent->f_children.push_back(shared_from_this());
        }
        else
        {
            if(static_cast<size_t>(index) > parent->f_children.size())
            {
                throw exception_index_out_of_range("trying to insert a node at the wrong position");
            }
            parent->f_children.insert(parent->f_children.begin() + index, shared_from_this());
        }
        f_parent = parent;
    }
}


/** \brief Get a pointer to the parent of this node.
 *
 * This function returns the pointer to the parent of this node. It may be
 * a null pointer.
 *
 * Note that the parent is kept as a weak pointer internally. However, when
 * returned it gets locked first (as in shared pointer lock) so you do not
 * have to do that yourselves.
 *
 * \return A smart pointer to the parent node, may be null.
 *
 * \sa set_parent()
 */
Node::pointer_t Node::get_parent() const
{
    return f_parent.lock();
}


/** \brief Return the number of children available in this node.
 *
 * This function returns the number of children we have available in this
 * node.
 *
 * \return The number of children, may be zero.
 *
 * \sa set_parent()
 */
size_t Node::get_children_size() const
{
    return f_children.size();
}


/** \brief Delete the specified child from the parent.
 *
 * This function removes a child from its parent (i.e. "unparent" a
 * node.)
 *
 * The following two lines of code are identical:
 *
 * \code
 *     parent->delete_child(index);
 *     // or
 *     parent->set_parent(nullptr, index);
 * \endcode
 *
 * Note that the vector of children of 'this' node changes, be careful.
 * Whenever possible, to avoid bugs, you may want to consider using
 * the lock() function through the NodeLock object.
 *
 * \note
 * The child node being "deleted" is not actively deleted. That is, if
 * anyone still holds a shared pointer of that node, it will not actually
 * get deleted. If that was the last shared pointer holding that node,
 * then it gets deleted automatically by the smart pointer implementation.
 *
 * \param[in] index  The index of the child node to remove from 'this' node.
 *
 * \sa lock()
 * \sa set_parent()
 */
void Node::delete_child(int index)
{
    f_children.at(index)->set_parent();
}


/** \brief Append a child to 'this' node.
 *
 * This function appends (adds at the end of the vector of children) a
 * child to 'this' node, which means the child is given 'this'
 * node as a parent.
 *
 * The following two lines of code are identical:
 *
 * \code
 *     parent->append_child(child);
 *     // or
 *     child->set_parent(parent);
 * \endcode
 *
 * \exception exception_invalid_data
 * This function verifies that the \p child pointer is not null. If null,
 * this exception is raised.
 *
 * \param[in] child  The child to be added at the end of 'this' vector
 *                   of children.
 *
 * \sa set_parent()
 */
void Node::append_child(pointer_t child)
{
    if(!child)
    {
        throw exception_invalid_data("cannot append a child if its pointer is null");
    }
    child->set_parent(shared_from_this());
}


/** \brief Insert the specified child at the specified location.
 *
 * When adding a child to a node, it can be placed before existing
 * children of that node. This function is used for this purpose.
 *
 * By default the index is set to -1 which means that the child is
 * added at the end of the list (see also the append_child() function.)
 *
 * This is a helper function since you could as well call the set_parent()
 * function directly:
 *
 * \code
 *     parent->insert_child(index, child);
 *     // or
 *     child->set_parent(parent, index);
 * \endcode
 *
 * \exception exception_invalid_data
 * This function verifies that the \p child pointer is not null. If null,
 * this exception is raised.
 *
 * \param[in] index  The index where the child will be inserted.
 * \param[in,out] child  The child to insert at that index.
 *
 * \sa set_parent()
 */
void Node::insert_child(int index, pointer_t child)
{
    if(!child)
    {
        throw exception_invalid_data("cannot insert a child if its pointer is null");
    }
    child->set_parent(shared_from_this(), index);
}


/** \brief Replace the current child at position \p index with \p child.
 *
 * This function replaces the child in this node at \p index with
 * the new specified \p child.
 *
 * This is a helper function as this functionality is offered by
 * the set_parent() function as in:
 *
 * \code
 *     parent->set_child(index, child);
 *     // or
 *     parent->set_parent(nullptr, index);
 *     child->set_parent(index, parent);
 * \endcode
 *
 * \exception exception_invalid_data
 * This function verifies that the \p child pointer is not null. If null,
 * this exception is raised.
 *
 * \param[in] index  The position where the new child is to be placed.
 * \param[in,out] child  The new child replacing the existing child at \p index.
 *
 * \sa set_parent()
 */
void Node::set_child(int index, pointer_t child)
{
    // to respect the contract, we have to test child here too
    if(!child)
    {
        throw exception_invalid_data("cannot insert a child if its pointer is null");
    }
    delete_child(index);
    insert_child(index, child);
}


/** \brief Replace this node with the \p node parameter.
 *
 * This function replaces this node with the specified node. This is used
 * in the optimizer and in the compiler.
 *
 * It is useful in a case such as an if() statement that has a resulting
 * Boolean value known at compile time. For example:
 *
 * \code
 *  if(true)
 *      blah;
 *  else
 *      foo;
 * \endcode
 *
 * can be optimized by just this:
 *
 * \code
 *  blah;
 * \endcode
 *
 * In that case what we do is replace the NODE_IF (this) with the content
 * of the 'blah' node. This can be done with this function.
 *
 * This function is very similar to the set_child() when you do not know
 * the index position of 'this' node in its parent or do not have the
 * parent node handy. The following code shows what the function does
 * internally (without all the additional checks):
 *
 * \code
 *     child->replace_with(node);
 *     // or
 *     int index(child->get_offset());
 *     Node::pointer_t parent(child->get_parent());
 *     parent->set_child(index, node);
 * \endcode
 *
 * \warning
 * This function modifies the tree in a way that may break loops over
 * node children.
 *
 * \exception exception_no_parent
 * If 'this' node does not have a parent node, then this exception is
 * raised.
 *
 * \param[in] node  The node to replace 'this' node with.
 *
 * \sa set_parent()
 */
void Node::replace_with(pointer_t node)
{
    // to respect the contract, we have to test child here too
    if(!node)
    {
        throw exception_invalid_data("cannot replace with a node if its pointer is null");
    }
    pointer_t p(f_parent.lock());
    if(!p)
    {
        throw exception_no_parent("trying to replace a node which has no parent");
    }

    // the replace is safe so force an unlock in the parent if necessary
    // it is safe in the sense that the count will remain the same and
    // that specific offset will remain in place
    //
    // specifically, I know this happens when replacing a reference to a
    // constant variable with its value in the parent expression, the parent
    // node is locked in that case

    int32_t const save_lock(p->f_lock);
    p->f_lock = 0;
    try
    {
        p->set_child(get_offset(), node);
    }
    catch(...)
    {
        p->f_lock = save_lock;
        throw;
    }
    p->f_lock = save_lock;
}


/** \brief Retrieve a child.
 *
 * This function retrieves a child from this parent node.
 *
 * The \p index parameter must be between 0 and get_children_size() - 1.
 * If get_children_size() returns zero, then you cannot call this function.
 *
 * \exception std::out_of_range
 * If the index is out of bounds, the function throws this exception.
 *
 * \param[in] index  The index of the child to retrieve.
 *
 * \return A shared pointer to the specified child.
 *
 * \sa set_parent()
 * \sa get_parent()
 * \sa get_children_size()
 */
Node::pointer_t Node::get_child(int index) const
{
    return f_children.at(index);
}


/** \brief Find the first child of a given type.
 *
 * This function searches the vector of children for the first child
 * with the specified \p type. This can be used to quickly scan a
 * list of children for the first node with a specific type.
 *
 * \note
 * This function calls the find_next_child() with a null pointer in
 * the child parameter.
 *
 * \code
 *      find_next_child(nullptr, type);
 * \endcode
 *
 * \param[in] type  The type of nodes you are interested in.
 *
 * \return A pointer to the first node of that type or a null pointer.
 *
 * \sa find_next_child()
 */
Node::pointer_t Node::find_first_child(node_t type) const
{
    Node::pointer_t child;
    return find_next_child(child, type);
}


/** \brief Find the next child with the specified type.
 *
 * This function searches the vector of children for the next child
 * with the specified \p type. This can be used to quickly scan a
 * list of children for a specific type of node.
 *
 * The \p child parameter can be set to nullptr in which case the
 * first child of that type is returned (like find_first_child()
 * would do for you.)
 *
 * \bug
 * If you have to manage all the nodes of a given type in a large
 * list, it is wise to create your own loop because this loop
 * restarts from index zero every single time. This should get
 * fixed in a future release, although I am not so sure there a
 * way to do that without a context.
 *
 * \param[in] child  A pointer to the previously returned child node.
 * \param[in] type  The type of children to look for.
 *
 * \return The next child of that type or a null pointer.
 *
 * \sa find_first_child()
 */
Node::pointer_t Node::find_next_child(pointer_t child, node_t type) const
{
    size_t const max(f_children.size());
    for(size_t idx(0); idx < max; ++idx)
    {
        // if child is defined, skip up to it first
        if(child && child == f_children[idx])
        {
            child.reset();
        }
        else if(f_children[idx]->get_type() == type)
        {
            return f_children[idx];
        }
    }

    // not found...
    return pointer_t();
}


/** \brief Remove all the unknown nodes.
 *
 * This function goes through the entire tree starting at 'this' node
 * and remove all the children that are marked as NODE_UNKNOWN.
 *
 * This allows many functions to clear out many nodes without
 * having to have very special handling of their loops while
 * scanning all the children of a node.
 *
 * \note
 * The nodes themselves do not get deleted by this function. If
 * it was their last reference then it will be deleted by the
 * shared pointer code as expected.
 *
 * \sa set_parent()
 * \sa delete_child()
 */
void Node::clean_tree()
{
    size_t idx(f_children.size());
    while(idx > 0)
    {
        --idx;
        if(f_children[idx]->get_type() == node_t::NODE_UNKNOWN)
        {
            // a delete is automatically recursive
            delete_child(idx);
        }
        else
        {
            f_children[idx]->clean_tree();  // recursive
        }
    }
}


/** \brief Find the offset of this node in its parent array of children.
 *
 * This function searches for a node in its parent list of children and
 * returns the corresponding index so we can apply functions to that
 * child from the parent.
 *
 * \exception exception_no_parent
 * This exception is raised if this Node object does not have a parent.
 *
 * \exception exception_internal_error
 * This exception is raised if the node has a parent, but the function
 * cannot find the child in the f_children vector of the parent.
 * (This should never occur because the set_parent() makes sure
 * to always keep this relationship proper.)
 *
 * \return The offset (index, position) of the child in its parent
 *         f_children vector.
 *
 * \sa set_parent()
 * \sa replace_with()
 */
size_t Node::get_offset() const
{
    Node::pointer_t p(f_parent.lock());
    if(!p)
    {
        // no parent
        throw exception_no_parent("get_offset() only works against nodes that have a parent.");
    }

    pointer_t me(const_cast<Node *>(this)->shared_from_this());
    vector_of_pointers_t::iterator it(std::find(p->f_children.begin(), p->f_children.end(), me));
    if(it == p->f_children.end())
    {
        // if this happen, we have a bug in the set_parent() function
        throw exception_internal_error("get_offset() could not find this node in its parent"); // LCOV_EXCL_LINE
    }

    // found ourselves in our parent
    return it - p->f_children.begin();
}


void Node::set_instance(pointer_t node)
{
    f_instance = node;
}


Node::pointer_t Node::get_instance() const
{
    return f_instance.lock();
}


}
// namespace as2js

// vim: ts=4 sw=4 et
