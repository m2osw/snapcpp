/* node_lock.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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


/** \file
 * \brief Manage a node lock.
 *
 * This file defines the implementation of the node lock. At some point
 * in the compiler, a set of node cannot be modified or it could crash
 * or invalidate the current work being done. (i.e. if you assume you
 * have a node of type NODE_INT64 and someone changes it to NODE_FLOAT64
 * under your feet, then calling get_int64() will fail with an exception.
 * However, the real problem would not be the call to the get_int64(),
 * but the earlier call to the to_float64() function.)
 *
 * The lock allows you to mark a node as being read-only for a while.
 *
 * The NodeLock class allows you to use a scoped lock (the destructor
 * automatically unlocks the node.)
 */


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
 * locked cannot be modified. It can be very difficult to determine what
 * is happening on the tree when working with a very large tree.
 * This parameter ensures that nodes we are looping over while doing work
 * do not get modify at the wrong time.
 *
 * To avoid the exception that this function generates, you may instead
 * call the is_locked() function.
 *
 * \note
 * This function is expected to be called BEFORE your function attemps
 * any modification of the node.
 *
 * \exception exception_locked_node
 * If the function detects a lock on this node (i.e. the node should not
 * get modified,) then it raises this exception.
 *
 * \sa lock()
 * \sa unlock()
 * \sa is_locked()
 */
void Node::modifying() const
{
    if(is_locked())
    {
        // print the node in stderr so one can see what node generated a problem
        std::cerr << "The following node is locked:" << std::endl
                  << *this << std::endl;
        throw exception_locked_node("trying to modify a locked node.");
    }
}


/** \brief Check whether a node is locked.
 *
 * This function returns true if the specified node is currently locked.
 * False otherwise.
 *
 * \return true if the node is locked.
 *
 * \sa lock()
 * \sa unlock()
 * \sa modifying()
 */
bool Node::is_locked() const
{
    return f_lock != 0;
}


/** \brief Lock this node.
 *
 * This function locks this node. A node can be locked multiple times. The
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
 *
 * \note
 * This library is NOT multi-thread safe. This lock has nothing to do
 * with protecting a node from multiple accesses via multiple threads.
 *
 * \warning
 * The f_parent makes use of a weak pointer, and thus you will see
 * a call to a lock() function. This is the lock of the smart pointer
 * and not the lock of the node:
 *
 * \code
 *      p = f_parent.lock();   // return a shared_ptr
 *
 *      f_parent->lock();      // lock the parent node (call this function)
 * \endcode
 *
 * \bug
 * This function does not verify that the lock counter does not go
 * over the limit. However, the limit is 2 billion and if you reach
 * such, you probably have an enormous stack... which is rather
 * unlikely. More or less, technically, it just should not ever
 * overflow.
 *
 * \sa is_locked()
 * \sa unlock()
 * \sa modifying()
 */
void Node::lock()
{
    ++f_lock;
}


/** \brief Unlock a node that was previously locked.
 *
 * This function unlocks a node that was previously called with a call
 * to the lock() function.
 *
 * It cannot be called on a node that was not previously locked.
 *
 * To make it safe, you should look into using the NodeLock object to
 * lock your nodes, especially because the NodeLock is exception safe.
 *
 * \code
 *  {
 *      NodeLock lock(my_node);
 *
 *      ...do work...
 *  } // auto-unlock here
 * \endcode
 *
 * \note
 * This library is NOT multi-thread safe. This lock has nothing to do
 * with protecting a node from multiple accesses via multiple threads.
 *
 * \exception exception_internal_error
 * This exception is raised if the unlock() function is called more times
 * than the lock() function was called. It is considered an internal error
 * since it should never happen, especially if you make sure to use the
 * NodeLock object.
 *
 * \sa lock()
 * \sa is_locked()
 * \sa modifying()
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
 * when it gets out of scope (although that bug will be caught).
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
 * The function accepts a null pointer as parameter. This is useful
 * in many situation where we do not know whether the node is null
 * and it would make it complicated to have to check.
 *
 * \param[in] node  The node to be locked.
 *
 * \sa Node::lock()
 * \sa unlock()
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
 * If the pointer was null or the unlock() function was called early,
 * nothing happens.
 *
 * \sa unlock()
 */
NodeLock::~NodeLock()
{
    try
    {
        unlock();
    }
    catch(exception_internal_error const &)
    {
        // not much we can do here, we are in a destructor...
    }
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
 *
 * \sa Node::unlock()
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
