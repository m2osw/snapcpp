/*
 * Text:
 *      QCassandraLock.cpp
 *
 * Description:
 *      Implementation of the Lamport's bakery algorithm.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2013-2016 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "QtCassandra/QCassandraEncoder.h"
#include "QtCassandra/QCassandraLock.h"
#include "QtCassandra/QCassandra.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

using namespace CassWrapper;

namespace QtCassandra
{

// make sure that we use the correct size for pid_t
// (at this time we use a UInt32 value)
static_assert(sizeof(pid_t) <= sizeof(uint32_t), "sizeof(pid_t) has to fit in sizeof(uint32_t) or the code here is incorrect.");

/** \class QCassandraLock
 * \brief Lock mechanism using only Cassandra.
 *
 * So... You're super happy, you just found Cassandra and started using it to
 * rewrite your app. You slowly notice that there are things that you can
 * do with Cassandra at lightning speed, no way in 1,000 years would you want
 * to change from this concept to any other concept. But...
 *
 * All the read and write in Cassandra are individual commands. Cassandra
 * itself does not offer any way to synchronize a read or a write from all
 * the running hosts. What can you do?
 *
 * The need arise, in most cases, when your application has to create a unique
 * row. For example, you have a registration form where a user can enter a
 * username. That name must be unique throughout the entire database. When
 * that happens, the Cassandra system does not help you. You need to lock that
 * table (or at least that row) before you can create that unique row.
 *
 * This is where the QCassandraLock object comes in. It will lock an object
 * so a single process will have access to it for a while (until the lock
 * goes out of scope or is deleted.)
 *
 * The usage is expected to be something like this:
 *
 * \code
 *   //... ready to create the new user in the database ...
 *   {
 *      QCassandraLock lock(context, "user_table");
 *      if(users->exists(username))
 *      {
 *          // problem
 *          throw std::runtime_error("sorry, a user with that name exists");
 *          // Note: the lock destructor takes care of unlocking
 *      }
 *      users->row(username)->cell("email")->setValue(email);
 *      // Note: the lock destructor takes care of unlocking
 *   }
 * \endcode
 *
 * By default the lock is for 60 seconds and it is given 5 seconds to hold.
 * You may change these values by using an RAII class as this safe_timeout
 * implementation:
 *
 * \code
 *   void my_class::lock_database(lock_name)
 *   {
 *      class safe_timeout
 *      {
 *      public:
 *          safe_timeout(QtCassandra::QCassandraContext::pointer_t context)
 *              : f_context(context)
 *              , f_old_timeout(context->lockTimeout())
 *              , f_old_ttl(context->lockTtl())
 *          {
 *              // as short as possible
 *              f_context->setLockTimeout(1);       // decrease wait to 1 second (minimum)
 *              f_context->setLockTtl(4 * 60 * 60); // increase duration to 4 hours
 *          }
 *
 *          ~safe_timeout()
 *          {
 *              f_context->setLockTimeout(f_old_timeout);
 *              f_context->setLockTtl(f_old_ttl);
 *          }
 *
 *      private:
 *          QtCassandra::QCassandraContext::pointer_t   f_context;
 *          int                                         f_old_timeout;
 *          int                                         f_old_ttl;
 *      };
 *      safe_timeout st(f_context);
 *
 *      // we use a special name for the backend to avoid clashes with
 *      // standard plugin locks
 *      //
 *      return f_lock.lock(lock_name);
 *  }
 * \endcode
 *
 * The lock is implemented using the Cassandra database system itself
 * with the help of the Leslie Lamport's bakery algorithm (1974). You can
 * find detailed explanation of the code on Wikipedia:
 *
 *   http://en.wikipedia.org/wiki/Lamport's_bakery_algorithm
 *
 * More details are found in the French version, somehow:
 *
 *   http://fr.wikipedia.org/wiki/Lamport's_bakery_algorithm
 *
 * A Cassandra version is proposed on the following page:
 *
 *   http://wiki.apache.org/cassandra/Locking
 *
 * The bakery algorithm is based on the basic idea that a large number
 * of customers go to the store to buy bread. In order to make sure
 * they all are served in the order they come in, they are given a ticket
 * with a number. The ticket numbers increase by one for each new customer.
 * The person with the smallest ticket number is served next. Once served,
 * the ticket is destroyed. The ticket numbers can restart at one whenever
 * the queue of customers goes empty.
 *
 * On a computer without any synchronization mechanism available (our case)
 * two customers may enter the bakery simultaneously (especially since we're
 * working with processes that may run on different computers.) This means
 * two customers may end up with the exact same ticket number and there are
 * no real means to avoid that problem. However, each customer is also
 * assigned two unique numbers on creation: its host number and its process
 * number. These two numbers are used to further order processes.
 *
 * So, the basic bakery algorithm looks like this in C++. This algorithm
 * expects memory to be guarded (shared or "volatile"; always visible by
 * all threads.)
 *
 * \code
 *     // declaration and initial values of global variables
 *     namespace {
 *         int num_threads = 100;
 *         std::vector<bool> entering;
 *         std::vector<uint32_t> tickets;
 *     }
 *
 *     // initialize the vectors
 *     void init()
 *     {
 *         entering.reserve(num_threads);
 *         tickets.reserve(num_threads);
 *     }
 *
 *     // i is the thread number
 *     void lock(int i)
 *     {
 *         // get the next ticket
 *         entering[i] = true;
 *         int my_ticket(0);
 *         for(int j(0); j < num_threads; ++j)
 *         {
 *             if(ticket[k] > my_ticket)
 *             {
 *                 my_ticket = ticket[k];
 *             }
 *         }
 *         ++my_ticket; // add 1, we want the next ticket
 *         entering[i] = false;
 *
 *         for(int j(0); j < num_threads; ++j)
 *         {
 *             // wait until thread j receives its ticket number
 *             while(entering[j])
 *             {
 *                 sleep();
 *             }
 *
 *             // there are several cases:
 *             //
 *             // (1) tickets that are 0 are not assigned so we can just go
 *             //     through
 *             //
 *             // (2) smaller tickets win over us (have a higher priority,)
 *             //     so if there is another thread with a smaller ticket
 *             //     sleep a little and try again; that ticket must go to
 *             //     zero to let us through that guard
 *             //
 *             // (3) if tickets are equal, compare the thread numbers and
 *             //     like the tickets, the smallest thread wins
 *             //
 *             while(ticket[j] != 0 && (ticket[j] < ticket[i] || (ticket[j] == ticket[i] && j < i))
 *             {
 *                 sleep();
 *             }
 *         }
 *     }
 *     
 *     // i is the thread number
 *     void unlock(int i)
 *     {
 *         // release our ticket
 *         ticket[i] = 0;
 *     }
 *   
 *     void SomeThread(int i)
 *     {
 *         while(true)
 *         {
 *             [...]
 *             // non-critical section...
 *             lock(i);
 *             // The critical section code goes here...
 *             unlock(i);
 *             // non-critical section...
 *             [...]
 *         }
 *     }
 * \endcode
 *
 * The algorithm requires one important set of information: a list of
 * host numbers from 1 to n. Without that list, the algorithm cannot
 * function. Therefore, before you can use the QCassandraLock object
 * you must add each one of your hosts to the Cassandra lock table in
 * a row named "hosts". You need to list at least all the hosts that
 * are to use this lock functionality; other hosts are not required.
 * Hosts that cannot find themselves in that list generate an
 * exception when trying to use a lock.
 *
 * Adding hosts to the database is a one time call per host to the
 * QCassandraContext::add_lock_host() function. \b WARNING: the
 * add_lock_host() function cannot be called by more than one
 * host at a time, in general, you'll use the snap_add_host command
 * line tool to add your hosts and it should be run on a single
 * computer at a time.
 *
 * Now, to apply that algorithm to a Cassandra cluster, you want several
 * small modifications. Our algorithm is 100% based on the bakery
 * algorithm, however, it makes use of fully dynamic vectors for the
 * entering and the tickets variables. These are replaced by columns in
 * a row of the host lock table. We offer a way to lock any kind of
 * object by give the user of the lock a way to indicate what needs to
 * be locked by name. For example, if you're creating a new row and it
 * needs to be unique throughout all your Cassandra host, the object to
 * lock is that new row, so you can use the key of that new row as the
 * name of the lock. In the sample code below we use object_name for
 * that information. This means we can use a different vector for each
 * lock!
 *
 * \code
 *      // lock "object_name"
 *      void lock(QString object_name)
 *      {
 *          // note: somehow object_name gets attached to this object
 *          QString locks = context->lockTableName();
 *          QString hosts_key = context->lockHostsKey();
 *          QString host_name = context->lockHostName();
 *          int host = table[locks][hosts_key][host_name];
 *          pid_t pid = getpid();
 *
 *          // get the next available ticket
 *          table[locks]["entering::" + object_name][host + "/" + pid] = true;
 *          int my_ticket(0);
 *          QCassandraCells tickets(table[locks]["tickets::" + object_name]);
 *          foreach(tickets as t)
 *          {
 *              // we assume that t.name is the column name
 *              // and t.value is its value
 *              if(t.value > my_ticket)
 *              {
 *                  my_ticket = t.value;
 *              }
 *          }
 *          ++my_ticket; // add 1, since we want the next ticket
 *          to_unlock = my_ticket + "/" + host + "/" + pid;
 *          table[locks]["tickets::" + object_name][my_ticket + "/" + host + "/" + pid] = 1;
 *          // not entering anymore, by deleting the cell we also release the row
 *          // once all the processes are done with that object_name
 *          table[locks]["entering::" + object_name].dropCell(host + "/" + pid);
 *   
 *          // here we wait on all the other processes still entering at this
 *          // point; if entering more or less at the same time we cannot
 *          // guarantee that their ticket number will be larger, it may instead
 *          // be equal; however, anyone entering later will always have a larger
 *          // ticket number so we won't have to wait for them they will have to wait
 *          // on us instead; note that we load the list of "entering" once;
 *          // then we just check whether the column still exists; it is enough
 *          QCassandraCells entering(table[locks]["entering::" + object_name]);
 *          foreach(entering as e)
 *          {
 *              while(table[locks]["entering::" + object_name].exists(e))
 *              {
 *                  sleep();
 *              }
 *          }
 *
 *          // now check whether any other process was there before us, if
 *          // so sleep a bit and try again; in our case we only need to check
 *          // for the processes registered for that one lock and not all the
 *          // processes (which could be 1 million on a large system!);
 *          // like with the entering vector we really only need to read the
 *          // list of tickets once and then check when they get deleted
 *          // (unfortunately we can only do a poll on this one too...);
 *          // we exit the foreach() loop once our ticket is proved to be the
 *          // smallest or no more tickets needs to be checked; when ticket
 *          // numbers are equal, then we use our host numbers, the smaller
 *          // is picked; when host numbers are equal (two processes on the
 *          // same host fighting for the lock), then we use the processes
 *          // pid since these are unique on a system, again the smallest wins.
 *          tickets = table[locks]["tickets::" + object_name];
 *          foreach(tickets as t)
 *          {
 *              // do we have the smallest ticket?
 *              // note: the t.ticket,  t.host and t.pid come from the column key
 *              if(t.ticket > my_ticket
 *              || (t.ticket == my_ticket && t.host > host)
 *              || (t.ticket == my_ticket && t.host == host && t.pid >= pid))
 *              {
 *                  // do not wait on larger tickets, just ignore them
 *                  continue;
 *              }
 *              // not smaller, wait for the ticket to go away
 *              while(table[locks]["tickets::" + object_name][t.column_key].exists(t.name))
 *              {
 *                  sleep();
 *              }
 *              // that ticket was released, we may have priority now
 *              // check the next ticket
 *          }
 *      }
 *      
 *      // unlock "object_name" (as saved in this object)
 *      void unlock()
 *      {
 *          // release our ticket
 *          QString locks = context->lockTableName();
 *          table[locks]["tickets::" + object_name].dropCell(to_unlock);
 *      }
 *      
 *      // sample process
 *      void SomeProcess(QString object_name)
 *      {
 *          while(true)
 *          {
 *              [...]
 *              // non-critical section...
 *              lock(object_name);
 *              // The critical section code goes here...
 *              unlock(object_name);
 *              // non-critical section...
 *              [...]
 *          }
 *      }
 * \endcode
 *
 * \b VERY \b IMPORTANT: all database accesses must be done with at
 * least QUORUM if you have multiple centers and want to lock between
 * all centers then the Network Quorum must be used. The ALL is also
 * another option. Only if you want to lock local processes can you
 * use ONE, assuming that all those processes attach themselves to
 * the same Cassandra server.
 *
 * Note that the name of the lock table can be changed in your context
 * if done early enough (i.e. before any lock is ever created by that
 * process.) By default it is set to "lock_table". See the
 * QCassandraContext::set_lock_table_name() function for details.
 *
 * Locks are created to lock any resource that you want to lock. It
 * does not even have to be Cassandra data. Just resources that need to
 * be accessed by at most one process at a time.
 *
 * The name of the object (\p object_name) represents the resource to
 * be locked. This can be anything you want. For example, to lock a
 * row in a table, we suggest the name of the table followed by the
 * name of the row, eventually using a separator such as "::". This
 * lock works even for rows that do not yet exist since the lock itself
 * doesn't need the row to lock it.
 *
 * \code
 *     // Note: Since "table_name" cannot include a ":" this is always
 *     //       a unique object name!
 *     object_name = table_name + "::" + row_name;
 * \endcode
 *
 * For example, you have a user registering on your website and you
 * request that user to enter a username. That username becomes the row
 * key in your users table. Say the user enters "snap" for his username,
 * then following our example the object name would be: "users::snap".
 *
 * The lock is attempted for a limited amount of time as specified in
 * the context with the QCassandraContext::set_lock_timeout().
 *
 * \note
 * It is possible to lock as many resources as you want. However, it is
 * very likely that you will run in lockup problems if you attempt to
 * lock more than one resource at a time from multiple processes. Instead,
 * think about the problem and create one higher level lock that locks
 * everything you need at once. That way you completely avoid lockups in
 * your applications.
 *
 * \warning
 * Althoug this class allows you to lock multiple processes, it is NOT
 * thread safe. If you are using multiple threads in your application,
 * then you should create one thread that locks and unlocks process
 * resources.
 *
 * \bug
 * If something goes wrong, as in a read or a write fails, the system
 * throws and tries to remove the lock. When a lock times out (takes
 * too long to lock the resource,) it may be because a process died
 * without unlocking the given resource. This also means that the lock
 * table now includes information that will actually not be deleted
 * until it times out (because of its TTL.) Obviously that means all the
 * other processes will timeout until that TTL kicks in. Finally, there
 * isn't a really safe way to clean up such a mess. More or less, what
 * this means is that the entire process better never fail (which is why
 * you want "many" nodes and a QUORUM is pretty much always attainable.)
 * This being said, we use a short TTL on all the data because a process
 * should never take that long to handle its resource. That TTL is a
 * guarantee that resources get released even if the resource owner does
 * not properly release it (because of a bad crash, the host gets
 * interrupted, the Internet connections are not cooperative...) Obviously
 * you will always be able to connect to your cluster with cassandra-cli and
 * delete the offensive lock data if required.
 *
 * \warning
 * The main failure mechanism requires your C++ compiler to make use of
 * proper RAII all the way. In Linux this often means that you need to
 * have a try/catch somewhere like in your main() function. Without that,
 * the unlock() function may not be called blocking all the other
 * processes until the TTL kicks in.
 */

/** \var QCassandraLock::f_context
 * \brief The context used to create this Cassandra lock.
 *
 * The QCassandraLock needs to have access to the context in which it is
 * created. Its shared pointer is saved in this variable.
 *
 * The context pointer cannot be null (the constructors throw if the
 * context pointer is null.)
 */

/** \var QCassandraLock::f_table
 * \brief Pointer to the lock table.
 *
 * On creation of a lock object, it calls its parent context to request
 * the lock table. If it does not exist yet, the context creates it and
 * waits until the Cassandra cluster is synchronized. This means the
 * first lock may actually take a very long time to obtain. To make sure
 * it always works, you should have an initialization process in your
 * application such that a lock table is created before your full
 * application ever runs.
 *
 * The test cassandra_lock can currently be used for that purpose. A
 * tool will be added later to apply those commands by scripts.
 */

/** \var QCassandraLock::f_object_name
 * \brief The name of the object being locked.
 *
 * Whenever you create a lock, you give it a name. This is where the lock
 * object saves that name. The name is expected to be shared between all
 * the locking processes.
 *
 * The name is used by the unlock() to remove the lock, it is then cleared.
 */

/** \var QCassandraLock::f_ticket_id
 * \brief The ticket of the lock.
 *
 * When a call to lock() succeeds, the lock created a ticket and this is
 * its identifier. The ticket is kept internally so the unlock() functions
 * does not have to attempt to determine its value later. It can just
 * delete it.
 *
 * The ticket identifier is cleared by the unlock() function.
 */

/** \var QCassandraLock::f_locked
 * \brief The current state of this lock.
 *
 * Whether the lock is currently in effect (true) or not (false).
 *
 * The lock() function sets this value to true when the lock succeeds.
 *
 * The unlock() function sets this value to false once the lock was
 * released.
 *
 * The lock constructors set this flag to false by default. If you
 * specify the name of the object to lock in the constructor, then
 * the lock() is called so in effect it will look like the constructor
 * sets the variable to true.
 */

/** \var QCassandraLock::f_consistency
 * \brief This variable holds the consistency of the lock.
 *
 * This variable is used to hold the consistency level used to read and
 * write data for this lock.
 *
 * By default is is set to QUORUM so it works accross your entire cluster.
 * However, you may have optimizations which could be used to allow a
 * LOCAL QUORUM instead (much faster if you have many data centers.)
 *
 * At this point all the consistency levels are accepted in this variable.
 */

/** \brief Create a lock for mutual exclusion.
 *
 * This function is an overload of the QCassandraLock. It does the same
 * as the other constructors except that the name of the object can be
 * specified as UTF-8.
 *
 * \param[in] context  The context where the lock is to be created.
 * \param[in] object_name  The name of the object to be locked as a QString.
 * \param[in] consistency_level  The level to use for the lock, defaults to QUORUM.
 */
QCassandraLock::QCassandraLock(QCassandraContext::pointer_t context, const QString& object_name, consistency_level_t consistency_level )
    : f_context(context)
    , f_consistency(consistency_level)
      //f_table(NULL) -- auto-init
      //f_object_name() -- auto-init
      //f_ticket_id() -- auto-init
      //f_locked(false) -- auto-init
{
    // make sure user gives us a context
    if(!context) {
        throw std::logic_error("the context pointer cannot be NULL when calling QCassandraLock constructor");
    }

    internal_init(object_name.toUtf8());
}

/** \brief Create a lock for mutual exclusion.
 *
 * This function creates an inter process lock for the safety usage of
 * shared resources by means of mutual exclusion. In other words, a
 * read/write full exclusive access to any Cassandra content.
 *
 * The lock implementation is documented in the class documentation, see
 * QCassandraLock.
 *
 * Note that if object_name is set to an empty string, then the lock is
 * not obtained in the constructor. Instead you have to call the lock()
 * function. Not obtaining the lock in the constructor gives you a chance
 * to avoid the throw on failure.
 *
 * \warning
 * The object name is left available in the lock table. Do not use any
 * secure/secret name/word, etc. as the object name.
 *
 * \exception std::runtime_error
 * This exception is raised if the lock cannot be obtained after
 * the lock times out. This only happens if a named object is
 * specified on the constructor. To avoid this exception, use the
 * lock() function after construction and make sure to test the
 * returned result.
 *
 * \param[in] context  The context where the lock is created.
 * \param[in] object_name  The resource to be locked.
 * \param[in] consistency_level  The level to use for the lock, defaults to QUORUM.
 *
 * \sa QCassandraContext::addLockHost()
 * \sa QCassandraContext::setLockTimeout()
 * \sa QCassandraContext::setLockTableName()
 * \sa lock()
 * \sa unlock()
 */
QCassandraLock::QCassandraLock(QCassandraContext::pointer_t context, const QByteArray& object_name, consistency_level_t consistency_level )
    : f_context(context)
    , f_consistency(consistency_level)
      //f_table(NULL) -- auto-init
      //f_object_name() -- auto-init
      //f_locked(false) -- auto-init
{
    // make sure user gives us a context
    if(!context) {
        throw std::logic_error("the context pointer cannot be NULL when calling QCassandraLock constructor");
    }

    internal_init(object_name);
}

/** \brief Initialize the QCassandraLock object further.
 *
 * This function initialize the QCassandraLock object.
 *
 * \param[in] object_name  The resource to be locked.
 */
void QCassandraLock::internal_init(const QByteArray& object_name)
{
    // get the table
    QCassandraTable::pointer_t table(f_context->lockTable());
    if(!table) {
        // table does not exist yet, it cannot even remotely work
        throw std::runtime_error("the lock table does not exist yet; you must create a lock table and add your computer hosts to the table before you can use a lock; see QCassandraContext::addLockHost()");
    }
    f_table = table;

    // now if the user wanted an auto-lock, do that
    if(!object_name.isEmpty()) {
        if(!lock(object_name)) {
            throw std::runtime_error(QString("QCassandraLock failed, lock \"%1\" could not be obtained within specified timeout (pid:%2)")
                    .arg(QString::fromUtf8(object_name))
                    .arg(getpid()).toUtf8().data());
        }
    }
}

/** \brief Unlock the resource associated with this lock.
 *
 * The QCassandraLock destructor ensures that the associated resource,
 * if any, gets unlocked before it completely goes away.
 *
 * Although the function is considered safe in regard to the C++
 * language semantic, the unlock() may fail for many reasons, one
 * reason being that the Cassandra cluster is somehow not available
 * anymore.
 */
QCassandraLock::~QCassandraLock()
{
    unlock();
}


/** \brief Lock the named resource.
 *
 * This function transforms the object name in a usable key (i.e. the
 * UTF-8 of the object name.) It then calls the other lock() function.
 *
 * \param[in] object_name  The name of the object to lock.
 *
 * \return true if the object lock was obtained; false otherwise
 */
bool QCassandraLock::lock(const QString& object_name)
{
    return lock(object_name.toUtf8());
}

/** \brief Lock the resource.
 *
 * This function locks the specified resource \p object_name. It returns
 * when the resource is locked or when the lock timeout is reached.
 *
 * See the QCassandraLock constructor for more details about the locking
 * mechanisms.
 *
 * Note that if lock() is called with an empty string then the function
 * unlocks the lock and returns immediately with false. This is equivalent
 * to calling unlock().
 *
 * \note
 * The function reloads all the parameters (outside of the table) because
 * we need to support a certain amount of dynamism. For example, an
 * administrator may want to add a new host on the system. In that case,
 * the list of host changes and it has to be detected here.
 *
 * \warning
 * The object name is left available in the lock table. Do not use any
 * secure/secret name/word, etc. as the object name.
 *
 * \bug
 * At this point there is no proper protection to recover from errors
 * that would happen while working on locking this entry. This means
 * failures may result in a lock that never ends.
 *
 * \param[in] object_name  The resource to be locked.
 *
 * \return true if the lock was successful, false otherwise.
 *
 * \sa unlock()
 */
bool QCassandraLock::lock(const QByteArray& object_name)
{
    class autoDrop
    {
    public:
        autoDrop(const QCassandraRow::pointer_t row, const QByteArray& cell, const consistency_level_t consistency_level)
            : f_row(row)
            , f_cell(cell)
            , f_consistency(consistency_level)
        {
        }

        ~autoDrop()
        {
            dropNow();
        }

        void cancelDrop()
        {
            f_row.reset();
        }

        void dropNow()
        {
            if(f_row)
            {
                QCassandraCell::pointer_t c(f_row->cell(f_cell));
                c->setConsistencyLevel(f_consistency);
                f_row->dropCell(f_cell);
                f_row.reset();
            }
        }

    private:
        QCassandraRow::pointer_t   f_row;
        QByteArray                 f_cell;
        consistency_level_t        f_consistency;
    };

    class timeoutCheck
    {
    public:
        timeoutCheck(int timeout)
        {
            f_limit = QCassandra::timeofday()
                    + static_cast<uint64_t>(timeout) * 1000000ULL;
        }

        bool wait()
        {
            struct timespec pause;
            pause.tv_sec = 0;
            pause.tv_nsec = 100000000; // 100ms
            nanosleep(&pause, NULL);

            return static_cast<uint64_t>(QCassandra::timeofday()) < f_limit;
        }

    private:
        uint64_t        f_limit;
    };

    unlock();

    f_object_name = object_name;
    if(f_object_name.isEmpty()) {
        // no name, just like an unlock
        return false;
    }

    // get the name of the row holding our hosts information
    const QString hosts_key(f_context->lockHostsKey());
    if(!f_table->exists(hosts_key)) {
        throw std::runtime_error(("The hosts row in the lock table does not exist, you must add your computer hosts to the table before you can use a lock. See the tests/cassandra_lock tools. This computer name is \"" + hosts_key + "\"").toUtf8().data());
    }

    // although the row of host names should not change very often at
    // all we still have to re-read it from Cassandra each time, to
    // make 100% sure we're in order
    QCassandraRow::pointer_t hosts_row(f_table->row(hosts_key));
    hosts_row->clearCache();

    // get our identifier
    const QString host_name(f_context->hostName());
    QCassandraCell::pointer_t cell_host_id(hosts_row->cell(host_name));
    cell_host_id->setConsistencyLevel(f_consistency);
    QCassandraValue my_host_id(cell_host_id->value());
    if(my_host_id.nullValue()) {
        throw std::runtime_error("this host does not seem to be defined");
    }
    const uint32_t host_id(my_host_id.uint32Value());
    const pid_t pid(getpid());

    QByteArray my_id;
    appendUInt32Value(my_id, host_id);
    appendUInt32Value(my_id, pid);

    // mark us as entering (entering[i] = true)
    //
    // the TTL uses the time we use in the attempt to obtain a lock plus
    // five seconds to make sure it does not get deleted too soon
    //
    // we use a TTL on top of the autoDrop because the software could
    // crash (abort() is not C++ safe and Qt uses it... or your process
    // gets a KILL signal) and never remove the "entering::..."
    // information which would prevent any further locks from being
    // obtained for that specific object name
    //
    QCassandraRow::pointer_t entering_row(f_table->row("entering::" + f_object_name));
    entering_row->clearCache();
    autoDrop auto_drop_entering(entering_row, my_id, f_consistency);
    QCassandraValue boolean;
    boolean.setConsistencyLevel(f_consistency);
    boolean.setTtl(f_context->lockTimeout() + 5);
    boolean.setCharValue(1);
    entering_row->cell(my_id)->setValue(boolean);

    // get the row specific to that object (that way we don't have to lock
    // everyone each time we want to have a lock; although you can obtain
    // such a feat by using an object name such as "global")
    QCassandraRow::pointer_t tickets_row(f_table->row("tickets::" + f_object_name));
    tickets_row->clearCache(); // make sure we have a clean slate

    // for all the QCassandraRow::cellCounts() calls
    auto column_count(std::make_shared<QCassandraCellPredicate>());
    column_count->setConsistencyLevel(f_consistency);

    // retrieve the largest ticket (ticket[i] = 1 + max(ticket[1], ..., ticket[NUM_THREADS]))
    //
    // IMPORTANT NOTE: Yes. Between here and the time we read the cells,
    //                 and the time we went through all the cells 1,000
    //                 other processes may have gone through and added
    //                 themselves; this is fine, they all will be blocked
    //                 because of our entering flag; then we'll get a
    //                 ticket number equal to one or more those 1,000 other
    //                 processes; again that is fine since we can sort the
    //                 processes using their host identifier and pid.
    //
    auto tickets_predicate(std::make_shared<QCassandraCellRangePredicate>());
    tickets_predicate->setConsistencyLevel(f_consistency);
    tickets_predicate->setCount(tickets_row->cellCount(column_count) + 100);
    tickets_row->readCells(tickets_predicate);
    const QCassandraCells& tickets(tickets_row->cells());

//fprintf(stderr, "%6d -- %d tickets already exist\n", getpid(), tickets.count());
    uint32_t our_ticket(0);
    for(QCassandraCells::const_iterator j(tickets.begin()); j != tickets.end(); ++j)
    {
        QByteArray jticket_key((*j)->columnKey());
        uint32_t jticket(uint32Value(jticket_key, 0));
        if(our_ticket < jticket) {
            our_ticket = jticket; // we become the last ticket, largest + 1 (we do the +1 later)
        }
    }

    // in a system where processes try to acquire new locks without any pauses
    // this could happen; (i.e. imagine a system where new processes are
    // started before all the locks get released, say you get 1 million
    // connections a second and need to lock a row for all of them and it
    // takes too long to do all of that... you pile up and soon enough you
    // get over 4 billion tickets!)
    if(our_ticket == static_cast<uint32_t>(-1))
    {
        throw std::logic_error("somehow the ticket numbers have reached the maximum allowed of 4 billion?");
    }
    ++our_ticket;

    // create the ticket identifier to include the host identifier
    // and the process identifier that way it gets sorted and we can
    // read just what we need for the next loop
    appendUInt32Value(f_ticket_id, our_ticket);
    appendUInt32Value(f_ticket_id, host_id);
    appendUInt32Value(f_ticket_id, pid);

    // save our waiting ticket
    autoDrop auto_drop_ticket(tickets_row, f_ticket_id, f_consistency);
    QCassandraValue ticket_value;
    ticket_value.setConsistencyLevel(f_consistency);
    ticket_value.setTtl(f_context->lockTtl());
    ticket_value.setCharValue(1); // we put some "random" value so it does not match nullValue()
    tickets_row->cell(f_ticket_id)->setValue(ticket_value);

    // mark us as done entering (entering[i] = false)
    // no need to clear the cache since we're writing to Cassandra
    auto_drop_entering.dropNow();

    // prepare our timed context
    timeoutCheck tc(f_context->lockTimeout());

    // loop until all the processes that were entering while we were
    // are all entered; until then we cannot be sure that the list of
    // tickets is complete

    // wait for all the other processes that entered at the same time
    // as us and are still asking for their ticket
    //
    // IMPORTANT NOTE: Yes. Between here and the time we read the cells,
    //                 and the time we went through all the cells 1,000
    //                 other processes may have gone through and added
    //                 themselves; this is fine, they all will be blocked
    //                 because of our entering flag; then we'll get a
    //                 ticket number equal to one or more those 1,000 other
    //                 processes; again that is fine since we can sort the
    //                 processes using their host identifier and pid.
    //
    entering_row->clearCache(); // <- very important or we'd miss those who entered just after us
    auto entering_predicate(std::make_shared<QCassandraCellRangePredicate>());
    entering_predicate->setConsistencyLevel(f_consistency);
    entering_predicate->setCount(entering_row->cellCount(column_count) + 100);
    entering_row->readCells(entering_predicate);
    // get those cells by copy because we expect to reset that map again and again
    const QCassandraCells entering_processes(entering_row->cells());

    for(QCassandraCells::const_iterator j(entering_processes.begin()); j != entering_processes.end(); ++j) {
        // sleep for as long as the cell still exists
        QByteArray jentering_key((*j)->columnKey());
        for(;;) {
            // WARNING: by clearing the cache we prevent ourselves from
            //          reading the value from the cells in entering_processes
            //          however, the column names are still fully available
            entering_row->clearCache();
            // WARNING: at this point the row::exists() has a bug!
            QCassandraCell::pointer_t entering_cell(entering_row->cell(jentering_key));
            entering_cell->setConsistencyLevel(f_consistency);
            QCassandraValue e(entering_cell->value());
            if(e.nullValue()) {
                // once dropped the value of 1 becomes a NULL value
                break;
            }
            if(!tc.wait()) {
                // we timed out!
                return false;
            }
//fprintf(stderr, "waiting on entering %d\n", getpid());
        }
    }

    // finally, we're ready to really wait for our very own turn
    //
    // at this point we know for sure that the list of tickets is
    // complete for our use (not for those processes that arrive
    // after us, but us we can ignore any further additions)
    //
    // there is a very interesting optimization for us here because
    // Cassandra sorts by column keys and therefore our tickets are
    // actually sorted! this means we can do one single query to
    // retrieve all the ticket information we need to wait for our
    // turn, very fast! (okay, the poll afterward is not that fast
    // but it would be required either way...)
    tickets_row->clearCache(); // <- very important or we'd miss those who entered just after us
    tickets_predicate->setCount(tickets_row->cellCount(column_count) + 100);
    tickets_predicate->setEndCellKey(f_ticket_id);
    tickets_row->readCells(tickets_predicate);
    // make a copy of those cells because we're about to reset and
    // re-establish that array over and over again
    const QCassandraCells all_tickets(tickets_row->cells());

    for(QCassandraCells::const_iterator j(all_tickets.begin()); j != all_tickets.end(); ++j)
    {
        // read the that ticket information
        QByteArray jticket_key((*j)->columnKey());
        uint32_t jticket(uint32Value(jticket_key, 0));
        uint32_t jhost_id(uint32Value(jticket_key, sizeof(uint32_t)));
        uint32_t jpid(uint32Value(jticket_key, sizeof(uint32_t) + sizeof(uint32_t)));

        if(jticket > our_ticket
        || (jticket == our_ticket && jhost_id > host_id)
        || (jticket == our_ticket && jhost_id == host_id && jpid >= static_cast<uint32_t>(pid)))
        {
            // do not wait on ourself
            //
            // also do not wait on larger tickets, they are after us so they
            // are waiting on us, not the other way around (although we
            // do not expect the predicate to allow the reading of larger
            // tickets... but that's a good safeguard!)
            (*j)->clearCache();
            continue; // TBD: break since the tickets are ordered by Cassandra?
        }

        // wait on tickets that have priority over us
        for(;;) {
            // WARNING: by clearing the cache we prevent ourselves from
            //          reading the value from the cells in entering_processes
            //          however, the column names are still fully available
            tickets_row->clearCache();
            // WARNING: at this point the row::exists() has a bug!
            QCassandraCell::pointer_t ticket_cell(tickets_row->cell(jticket_key));
            ticket_cell->setConsistencyLevel(f_consistency);
            QCassandraValue t(ticket_cell->value());
            if(t.nullValue()) {
                // once dropped the value of 1 becomes a NULL value
                break;
            }
            if(!tc.wait()) {
                // we timed out!
                return false;
            }
//fprintf(stderr, "waiting on ticket %d - blocked by %d\n", getpid(), jpid);
        }

        (*j)->clearCache();
    }

    // the lock worked
    f_locked = true;
    auto_drop_ticket.cancelDrop();

//fprintf(stderr, "locked!!! = %d\n", getpid());
    return true;
}

/** \brief Unlock the resource.
 *
 * This function unlocks the resource specified in the call to lock().
 */
void QCassandraLock::unlock()
{
    if(!f_locked) {
        return;
    }

    // delete the lock
    QCassandraRow::pointer_t r(f_table->row("tickets::" + f_object_name));
    QCassandraCell::pointer_t c(r->cell(f_ticket_id));
    c->setConsistencyLevel(f_consistency);
    r->dropCell(f_ticket_id);

//QString host_name(f_context->lockHostName());
//fprintf(stderr, "unlock() host_name [%s] pid: %d\n", host_name.toUtf8().data(), getpid());

    f_locked = false;
    f_ticket_id.clear();
    f_object_name.clear();
}

} // namespace QtCassandra
// vim: ts=4 sw=4 et
