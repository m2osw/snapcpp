/* node_lock.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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
//#include    "as2js/message.h"
//
//#include    <controlled_vars/controlled_vars_auto_enum_init.h>
//
//#include    <algorithm>
//#include    <sstream>
//#include    <iomanip>


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  NODE LOCK  ******************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Test whether the node can be modified.
 *
 * This function verifies whether the node can be modified. Nodes that were
 * locked cannot be modified. It can be very difficult to detect loops as
 * we handle the large tree of nodes. This parameter ensures that such
 * loops do not modify data that we are working with.
 *
 * \exception exception_locked_node
 * If the function detects a lock on this node (i.e. the node should not
 * get modified,) then it raises this exception.
 */
void Node::modifying() const
{
    if(is_locked())
    {
        throw exception_locked_node("trying to modify a locked node.");
    }
}


/** \brief Check whether a node is locked.
 *
 * This function returns true if the node is currently locked. False
 * otherwise.
 *
 * \return true if the node is locked.
 */
bool Node::is_locked() const
{
    return f_lock != 0;
}


/** \brief Lock the node.
 *
 * This function locks the node. A node can be locked multiple times. The
 * unlock() function needs to be called the same number of times the
 * lock() function was called.
 *
 * It is strongly recommended that you use the NodeLock object in order
 * to lock your nodes. That way they automatically get unlocked when you
 * exit your scope, even if an exception occurs.
 *
 * \code
 *  {
 *      NodeLock lock(my_node);
 *
 *      ...do work...
 *  } // auto-unlock here
 * \endcode
 */
void Node::lock()
{
    ++f_lock;
}


/** \brief Unlock a node that was previously locked.
 *
 * This function unlock a node that was previously called with a call to
 * the lock() function.
 *
 * It cannot be called on a node that was not previously locked.
 *
 * To make it safe, you should look into using the NodeLock object to
 * lock your nodes.
 *
 * \exception exception_internal_error
 * This exception is raised if the unlock() function is called more times
 * thant the lock() function was called. It is considered an internal error
 * since it should never happen, especially if you make sure to use the
 * NodeLock object.
 */
void Node::unlock()
{
    if(f_lock <= 0)
    {
        throw exception_internal_error("somehow the Node::unlock() function was called when the lock counter is zero");
    }

    --f_lock;
}


/** \brief Safely lock a node.
 *
 * This constructor is used to lock a node within a scope.
 *
 * \code
 *     {
 *         NodeLock lock(my_node);
 *         ...code...
 *     } // auto-unlock here
 * \endcode
 *
 * Note that the unlock() function can be used to prematuraly unlock
 * a node. It is very important to use the unlock() function of the
 * NodeLock() otherwise it will attempt to unlock the node again
 * when it gets out of scope.
 *
 * \code
 *     {
 *         NodeLock lock(my_node);
 *         ...code...
 *         lock.unlock();
 *         ...code...
 *     } // already unlocked...
 * \endcode
 *
 * \param[in] node  The node to be locked.
 */
NodeLock::NodeLock(Node::pointer_t node)
    : f_node(node)
{
    if(f_node)
    {
        f_node->lock();
    }
}


/** \brief Destroy the NodeLock object.
 *
 * The destructor of the NodeLock object ensures that the node passed
 * as a parameter to the constructor gets unlocked.
 *
 * If the pointer was null or the unlock() function was called, nothing
 * happens.
 */
NodeLock::~NodeLock()
{
    unlock();
}


/** \brief Prematurely unlock the node.
 *
 * This function can be used to unlock a node before the end of a
 * scope is reached. There are cases where that may be necessary.
 *
 * Note that this function is also called by the destructor. To
 * avoid a double unlock on a node, the function sets the node
 * pointer to null before returning. This means this function
 * can safely be called any number of times and the lock counter
 * of the node will remain valid.
 */
void NodeLock::unlock()
{
    if(f_node)
    {
        f_node->unlock();
        f_node.reset();
    }
}

}
// namespace as2js

// vim: ts=4 sw=4 et
