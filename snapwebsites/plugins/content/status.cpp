// Snap Websites Server -- all the user content and much of the system content
// Copyright (C) 2011-2015  Made to Order Software Corp.
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
 * \brief The implementation of the content plugin status class.
 *
 * This file contains the status class implementation.
 */

#include "content.h"

#include "log.h"

#include "poison.h"


SNAP_PLUGIN_EXTENSION_START(content)

/** \typedef uint32_t path_info_t::status_t::status_type
 * \brief Basic status type to save the status in the database.
 *
 * This basic status is used by the content plugin to manage a page
 * availability. It is called "basic" because this feature does not
 * use the taxonomy to mark the page as being in a specific status
 * that the end user has control over.
 *
 * By default a page is in the "normal state"
 * (path_info_t::status_t::NORMAL). A normal page can be viewed as
 * fully available and will be shown to anyone with enough permissions
 * to access that page.
 *
 * A page can also be hidden from view (path_info_t::status_t::HIDDEN),
 * in which case the page is accessible by the administrators with enough
 * permissions to see hidden pages, but no one else who get an error
 * (probably a 404, although if the hidden page is to be shown again
 * later a 503 is probably more appropriate.)
 *
 * Finally, a page can be given a working status:
 *
 * \li path_info_t::status_t::NOT_WORKING -- no processes are working on
 *     the page
 * \li path_info_t::status_t::CREATING -- the page is being created
 * \li path_info_t::status_t::CLONING -- the page is being cloned from
 *     another page
 * \li path_info_t::status_t::REMOVING -- the page is being moved or deleted
 * \li path_info_t::status_t::UPDATING -- the page is being updated
 *
 * These states are used in parallel with the basic state of the page.
 * So a page can be normal and updating at the same time. This is useful
 * in order to allow a page to revert back to a standard state (i.e. not
 * being processed) without having to have many more states making it
 * much harder to handle.
 *
 * The status_t class gives you two sets of functions to handle the state
 * and the working state separatly. There is also a common function,
 * reset_state(), which modifies both values at the same time.
 *
 * Note that a deleted page (path_info_t::status_t::DELETED) is similar
 * to a normal page, only it is found in the trashcan and thus it cannot
 * be edited. It can only be "undeleted" (cloned back to its original
 * location or to a new location in the regular tree.)
 */

/** \var path_info_t::status_t::NO_ERROR
 * \brief No error occured.
 *
 * When creating a new status object, we mark it as a "no error" object.
 *
 * In this state a status can be saved to the database. If not in this
 * state, trying to save the status will fail with an exception.
 */

/** \var path_info_t::status_t::UNSUPPORTED
 * \brief Read a status that this version does not know about.
 *
 * This value is returned by the get_status() function whenever a path
 * to a page returns a number that the current status implementation does
 * not understand. Unfortunately, such status cannot really be dealt with
 * otherwise.
 */

/** \var path_info_t::status_t::UNDEFINED
 * \brief The state is not defined in the database.
 *
 * This value is returned by the get_status() function whenever a path
 * to a non-existant page is read.
 *
 * This is similar to saying this is a 404. There is no redirect or anything
 * else that will help in this circumstance.
 */

/** \var path_info_t::status_t::UNKNOWN_STATE
 * \brief The state was not yet defined.
 *
 * This value is used internally to indicate that the status was not yet
 * read from the database. It should never be saved in the database itself.
 *
 * This is used in the status_t class up until the status gets read from
 * the content table.
 */

/** \var path_info_t::status_t::CREATE
 * \brief We are in the process of creating a page.
 *
 * While creating a page, the page is marked with this state.
 *
 * Once the page is created, it is marked as path_info_t::status_t::NORMAL.
 */

/** \var path_info_t::status_t::NORMAL
 * \brief This page is valid. You can use it as is.
 *
 * This is the only status that makes a page 100% valid for anyone with
 * enough permissions to visit the page.
 */

/** \var path_info_t::status_t::HIDDEN
 * \brief The page is currently hidden.
 *
 * A hidden page is similar to a normal page, only it returns a 404 to
 * normal users.
 *
 * Only administrators with the correct permissions can see the page.
 */

/** \var path_info_t::status_t::MOVED
 * \brief This page was moved, users coming here shall be redirected.
 *
 * This page content is still intact from the time it was cloned and it
 * should not be used. Instead, since it is considered moved, it generates
 * a 301 (it could be made a 302?) so that way the users who had links to
 * the old path still get to the page.
 *
 * A moved page may get deleted at a later time.
 */

/** \var path_info_t::status_t::DELETED
 * \brief This page was deleted (moved to the trash).
 *
 * A page that gets moved to the trashcan is marked as deleted since we
 * cannot redirect someone (other than an administrator with enough
 * permissions) to the trashcan.
 *
 * Someone with enough permission can restore a deleted page.
 *
 * A page marked as deleted is eventually removed from the database by
 * the content backend. Pages in the trashcan are also eventually deleted
 * from the database. That depends on the trashcan policy settings.
 */

/** \var path_info_t::status_t::NOT_WORKING
 * \brief Indicate that no processes are working on this page.
 *
 * This value indicates that the page is not being worked on. In most cases
 * backend processes use that signal to know whether to process a page
 * or not for a reason or another. For example, the list plugin will
 * avoid including pages in a list while those pages are being
 * created or updated. It will keep those pages in its list of pages
 * to be processed later on instead.
 */

/** \var path_info_t::status_t::CREATING
 * \brief Working on a page while creating it.
 *
 * This working value is used to mark a page being created. In a way, this
 * working state is a plain state too (we use CREATE/CREATING and then
 * transform that in NORMAL/NOT_WORKING).
 */

/** \var path_info_t::status_t::CLONING
 *
 * This status is similar to the creating status
 * (path_info_t::status_t::CREATING) only the data comes from another page
 * instead of the user.
 *
 * You have similar restriction on a page being cloned as a page being
 * created. While this status is set, someone visiting the page can only
 * get a signal such as "server busy".
 *
 * Once the cloning is done, the page can go to the normal state.
 */

/** \var path_info_t::status_t::REMOVING
 *
 * This status is used to mark the source page in a cloning process as
 * the page is going to be removed (i.e. the page is being moved to the
 * trashcan).
 *
 * If the page is simply being moved, then the status can remain normal
 * (path_info_t::status_t::NORMAL) since the source remains perfectly valid
 * while the page gets cloned. Once the cloning is done then the page is
 * marked as moved (path_info_t::status_t::MOVED).
 *
 * Once the remove process is done, the page gets marked as deleted
 * (path_info_t::status_t::DELETED). Remember that deleted pages return
 * a 404 to the client even though all the data is still available in
 * the database.
 */

/** \var path_info_t::status_t::UPDATING
 *
 * A page that gets heavily updated (more than one or two fields in a row)
 * should be marked as path_info_t::status_t::UPDATING. However, you want
 * to be careful as a page current status should not change once the update
 * is done (i.e. if the page was hidden then reverting it back to hidden
 * after the update is what you should do; so if you change that to normal
 * instead, you are in trouble.)
 */

/** \var path_info_t::status_t::f_error
 * \brief The current error of this status object.
 *
 * The error of this status. By default this parameter is set to
 * path_info_t::status_t::NO_ERROR.
 *
 * When a status is erroneous, the is_error() function returns true and
 * the status cannot be saved in the database.
 *
 * The state and working state of the status are ignored if
 * the status is in error (is_error() returns true.)
 *
 * There is one special case the transition function accepts a
 * path_info_t::status_t::UNDEFINED status as a valid input to
 * transit to a path_info_t::status_t::CREATE and
 * path_info_t::status_t::CREATING status. However, the erroneous
 * status itself is otherwise still considered to be in error.
 */

/** \var path_info_t::status_t::f_state
 * \brief The current state of the status.
 *
 * The state of this status. By default this parameter is set to
 * path_info_t::status_t::UNKNOWN_STATE. You may check whether the
 * state is unknown using the is_unknown() function.
 *
 * \warning
 * The working state is ignored if is_error() is true.
 */

/** \var path_info_t::status_t::f_working
 * \brief The current working state of the status.
 *
 * The status of a page may include a working state which represents what
 * the process working on the page is doing. By default this parameter is
 * set to path_info_t::status_t::NOT_WORKING.
 *
 * When a process is working on a page, its status is_working() function
 * returns true.
 *
 * \warning
 * The working state is ignored if is_error() is true.
 */

/** \typedef path_info_t::status_t::safe_error_t
 * \brief Safe as in auto-initialized error_t variable type.
 *
 * This typedef is used to define error_t variable members in classes
 * so they automatically get initialized and they test the range of
 * the error data saved into them when modifying them.
 */

/** \typedef path_info_t::status_t::safe_state_t
 * \brief Safe as in auto-initialized state_t variable type.
 *
 * This typedef is used to define state_t variable members in classes
 * so they automatically get initialized and they test the range of
 * the state data saved into them when modifying them.
 */

/** \typedef path_info_t::status_t::safe_working_t
 * \brief Safe as in auto-initialized working_t variable type.
 *
 * This typedef is used to define working_t variable members in classes
 * so they automatically get initialized and they test the range of
 * the working data saved into them when modifying them.
 */


/** \brief Initialize the status with the default status values.
 *
 * The default constructor of the status class defines a status object
 * with default values.
 *
 * The default values are:
 *
 * \li path_info_t::status_t::NO_ERROR for error
 * \li path_info_t::status_t::UNKNOWN_STATE for state
 * \li path_info_t::status_t::NOT_WORKING for working
 *
 * The default values can then be changed using the set_...() functions
 * of the class.
 *
 * You may also set the status using the set_status() function in case
 * you get a \p current_status after you created a status object.
 */
path_info_t::status_t::status_t()
    //: f_error(error_t::NO_ERROR)
    //, f_state(state_t::UNKNOWN_STATE)
    //, f_working(working_t::NOT_WORKING)
{
}


/** \brief Initialize the status with the specified current_status value.
 *
 * The constructor and get_status() make use of an integer to
 * save in the database but they do not declare the exact format
 * of that integer (i.e. the format is internal, hermetic.)
 *
 * The input parameter can only be defined from the get_status() of
 * another status. If you are not reading a new status, you must make
 * use of the constructor without a status specified.
 *
 * \param[in] current_status  The current status to save in this instance.
 */
path_info_t::status_t::status_t(status_type current_status)
    //: f_error(error_t::NO_ERROR)
    //, f_state(state_t::UNKNOWN_STATE)
    //, f_working(working_t::NOT_WORKING)
{
    set_status(current_status);
}


/** \brief Set the current status from the specified \p current_status value.
 *
 * This function accepts a \p current_status value which gets saved in the
 * corresponding f_state and f_working variable members.
 *
 * How the status is encoded in the \p current_status value is none of your
 * business. It is encoded by the get_status() and decoded using the
 * set_status(). That value can be saved in the database.
 *
 * \note
 * The constructor accepting a \p current_status parameter calls this
 * set_status() function to save its input value.
 *
 * \note
 * The error value is set to error_t::NO in this case.
 *
 * \param[in] current_status  The current status to save in this instance.
 */
void path_info_t::status_t::set_status(status_type current_status)
{
    // set some defaults so that way we have "proper" defaults on errors
    f_state = state_t::UNKNOWN_STATE;
    f_working = working_t::NOT_WORKING;

    state_t state(static_cast<state_t>(static_cast<int>(current_status) & 255));
    switch(state)
    {
    case state_t::UNKNOWN_STATE:
    case state_t::CREATE:
    case state_t::NORMAL:
    case state_t::HIDDEN:
    case state_t::MOVED:
    case state_t::DELETED:
        break;

    default:
        // any other status is not understood by this version of snap
//std::cerr << "status is " << static_cast<int>(static_cast<status_t>(f_status)) << "\n";
        f_error = error_t::UNSUPPORTED;
        return;

    }

    working_t working(static_cast<working_t>((static_cast<int>(current_status) / 256) & 255));
    switch(working)
    {
    case working_t::UNKNOWN_WORKING:
    case working_t::NOT_WORKING:
    case working_t::CREATING:
    case working_t::CLONING:
    case working_t::REMOVING:
    case working_t::UPDATING:
        break;

    default:
        // any other status is not understood by this version of snap
        f_error = error_t::UNSUPPORTED;
        return;

    }

    f_error = error_t::NO_ERROR;
    f_state = state;
    f_working = working;
}


/** \brief Retrieve the current value of the status of this object.
 *
 * This function returns the encoded status so one can save it in a
 * database, or some other place. The returned value is an integer.
 *
 * Internally, the value is handled as an error, a state, and a
 * working status. The encoder does not know how to handle errors
 * in this function, so if an error is detected, it actually
 * throws an exception. It is expected that your code will first
 * check whether is_error() returns true. If so, then you cannot
 * call this function.
 *
 * Note that if the state is still set to state_t::UNKNOWN_STATE, then
 * the function also raises an exception. This is because we
 * cannot allow saving that kind of a status in the database.
 * Some other combinations are forbidden. For example the
 * working_t::CREATING can only be used with the state_t::CREATING
 * status. All such mixes generate an error here.
 *
 * \exception snap_logic_exception
 * This exception is raised if this function gets called when the
 * status is currently representing an error. This is done that
 * way because there is really no reasons to allow for saving
 * an error in the database.
 *
 * \return The current status encrypted for storage.
 */
path_info_t::status_t::status_type path_info_t::status_t::get_status() const
{
    struct subfunc
    {
        static constexpr int status_combo(state_t s, working_t w)
        {
            return static_cast<unsigned char>(static_cast<int>(s))
                 | static_cast<unsigned char>(static_cast<int>(w)) * 256;
        }
    };

    // errors have priority and you cannot convert an error to a status_type
    if(f_error != error_t::NO_ERROR)
    {
        throw snap_logic_exception(QString("attempting to convert a status to status_type when it represents an error (%1).").arg(static_cast<int>(static_cast<error_t>(f_error))));
    }

    // of the 4 x 5 = 20 possibilities, we only allow 14 of them
    switch(subfunc::status_combo(f_state, f_working))
    {
    // creating
    case subfunc::status_combo(state_t::CREATE, working_t::CREATING):
    // normal
    case subfunc::status_combo(state_t::NORMAL, working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL, working_t::CLONING):
    case subfunc::status_combo(state_t::NORMAL, working_t::REMOVING):
    case subfunc::status_combo(state_t::NORMAL, working_t::UPDATING):
    // hidden
    case subfunc::status_combo(state_t::HIDDEN, working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN, working_t::CLONING):
    case subfunc::status_combo(state_t::HIDDEN, working_t::REMOVING):
    case subfunc::status_combo(state_t::HIDDEN, working_t::UPDATING):
    // moved
    case subfunc::status_combo(state_t::MOVED, working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::MOVED, working_t::REMOVING):
    case subfunc::status_combo(state_t::MOVED, working_t::UPDATING):
    // deleted
    case subfunc::status_combo(state_t::DELETED, working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::DELETED, working_t::UPDATING):
        break;

    default:
        throw snap_logic_exception(QString("attempting to convert status with state %1 and working %2 which is not allowed").arg(static_cast<int>(static_cast<state_t>(f_state))).arg(static_cast<int>(static_cast<working_t>(f_working))));

    }

    // if no error, then the value is (state | (working << 8))
    return static_cast<status_type>(static_cast<state_t>(f_state))
         | static_cast<status_type>(static_cast<working_t>(f_working)) * 256;
}


/** \brief Get 
 * 
 * Verify that going from the current status (this) to the \p destination
 * status is acceptable.
 *
 * \param[in] destination  The new state.
 *
 * \return true if the transition is acceptable, false otherwise.
 */
bool path_info_t::status_t::valid_transition(status_t destination) const
{
    struct subfunc
    {
        static constexpr int status_combo(state_t s1, working_t w1, state_t s2, working_t w2)
        {
            return static_cast<unsigned char>(static_cast<int>(s1))
                 | static_cast<unsigned char>(static_cast<int>(w1)) * 0x100
                 | static_cast<unsigned char>(static_cast<int>(s2)) * 0x10000
                 | static_cast<unsigned char>(static_cast<int>(w2)) * 0x1000000;
        }
    };

    if(is_error())
    {
        return f_error == error_t::UNDEFINED
            && destination.f_state == state_t::CREATE
            && destination.f_working == working_t::CREATING;
    }

    // shift by 8 is safe since the status is expected to be one byte
    // however, the special statuses are negative so we clear a few bits
    switch(subfunc::status_combo(f_state, f_working, destination.f_state, destination.f_working))
    {
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::MOVED,     working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::CLONING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::REMOVING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::UPDATING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::CLONING,     state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::REMOVING,    state_t::NORMAL,    working_t::NOT_WORKING): // in case of a reset
    case subfunc::status_combo(state_t::NORMAL,     working_t::REMOVING,    state_t::DELETED,   working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::UPDATING,    state_t::NORMAL,    working_t::NOT_WORKING):

    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::CLONING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::REMOVING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::UPDATING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::CLONING,     state_t::HIDDEN,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::REMOVING,    state_t::HIDDEN,    working_t::NOT_WORKING): // in case of a reset
    case subfunc::status_combo(state_t::HIDDEN,     working_t::REMOVING,    state_t::DELETED,   working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::UPDATING,    state_t::HIDDEN,    working_t::NOT_WORKING):

    case subfunc::status_combo(state_t::MOVED,      working_t::NOT_WORKING, state_t::MOVED,     working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::MOVED,      working_t::NOT_WORKING, state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::MOVED,      working_t::NOT_WORKING, state_t::HIDDEN,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::MOVED,      working_t::NOT_WORKING, state_t::MOVED,     working_t::CLONING):
    case subfunc::status_combo(state_t::MOVED,      working_t::CLONING,     state_t::MOVED,     working_t::NOT_WORKING):

    case subfunc::status_combo(state_t::DELETED,    working_t::NOT_WORKING, state_t::DELETED,   working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::DELETED,    working_t::NOT_WORKING, state_t::DELETED,   working_t::CLONING):
    case subfunc::status_combo(state_t::DELETED,    working_t::CLONING,     state_t::DELETED,   working_t::NOT_WORKING):

    // see error handle prior to this switch
    //case subfunc::status_combo(state_t::UNDEFINED,  working_t::NOT_WORKING, state_t::CREATE,    working_t::CREATING):

    case subfunc::status_combo(state_t::CREATE,     working_t::CREATING,    state_t::CREATE,    working_t::CREATING):
    case subfunc::status_combo(state_t::CREATE,     working_t::CREATING,    state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::CREATE,     working_t::CREATING,    state_t::HIDDEN,    working_t::NOT_WORKING):

        // this is a valid combo
        return true;

    default:
        // we do not like this combo at this time
        return false;

    }
}


/** \brief Set the error number in this status.
 *
 * Change the current status in an erroneous status. By default an object
 * is considered to not have any errors.
 *
 * The current state and working statuses do not get modified.
 *
 * \param[in] error  The new error to save in this status.
 */
void path_info_t::status_t::set_error(error_t error)
{
    f_error = error;
}


/** \brief Retrieve the current error.
 *
 * This function returns the current error of an ipath. If this status
 * represents an error, you may also call the is_error() function which
 * will return true for any errors except error_t::NO.
 *
 * \return The current error.
 */
path_info_t::status_t::error_t path_info_t::status_t::get_error() const
{
    return f_error;
}


/** \brief Check whether the path represents an error.
 *
 * If a path represents an error (which means the set_error() was called
 * with a value other than error_t::NO) then this function returns true.
 * Otherwise it returns true.
 *
 * \return true if the error in this status is not error_t::NO.
 */
bool path_info_t::status_t::is_error() const
{
    return f_error != error_t::NO_ERROR;
}


/** \brief Reset this status with the specified values.
 *
 * This function can be used to reset the status to the specified
 * state and working values. It also resets the current error
 * status.
 *
 * This is particularly useful to go from an undefined status to
 * a creating status.
 *
 * This function is a shortcut from doing:
 *
 * \code
 *      status.set_error(error_t::NO);
 *      status.set_state(state);
 *      status.set_working(working);
 * \endcode
 *
 * \param[in] state  The new state.
 * \param[in] working  The new working value.
 */
void path_info_t::status_t::reset_state(state_t state, working_t working)
{
    f_error = error_t::NO_ERROR;
    f_state = state;
    f_working = working;
}


/** \brief Change the current state of this status.
 *
 * This function can be used to save a new state in this status object.
 *
 * \note
 * This function does NOT affect the error state. This means that if the
 * status object has an error state other than error_t::NO, it is still
 * considered to be erroneous.
 *
 * \param[in] state  The new state of this status.
 */
void path_info_t::status_t::set_state(state_t state)
{
    f_state = state;
}


/** \brief Retrieve the current state.
 *
 * This function returns the current state of this status. The state is
 * set to unknown (path_info_t::status_t::UNKNOWN_STATE) by default if
 * no current_status is passed to the constructor.
 *
 * \return The current state.
 */
path_info_t::status_t::state_t path_info_t::status_t::get_state() const
{
    return f_state;
}


/** \brief Check whether the current state is unknown.
 *
 * When creating a new state object, the state is set to unknown by
 * default. It remains that way until you change it with set_state()
 * or reset_state().
 *
 * This function can be used to know whether the state is still set
 * to unknown.
 *
 * Note that is important because you cannot save an unknown state in
 * the database. The get_status() function will raise an exception
 * if that is attempted.
 */
bool path_info_t::status_t::is_unknown() const
{
    return f_state == state_t::UNKNOWN_STATE;
}


/** \brief Change the working state.
 *
 * This function is used to change the working state of the status object.
 *
 * The state can be set to any valid working state value, however, note
 * that the get_status() prevents a certain number of combinations such
 * as the working_t::CREATING working state with a state other than
 * state_t::CREATING.
 *
 * The default value of the working state is working_t::NO meaning that
 * the page is not being worked on.
 *
 * \note
 * So, this function allows any combinations to be generated, because
 * that way we do not enforce the use of the reset_state() function
 * or a specific order (i.e. change state first then working or
 * vice versa.)
 *
 * \param[in] working  The new working state.
 */
void path_info_t::status_t::set_working(working_t working)
{
    f_working = working;
}


/** \brief Retrieve the current working state.
 *
 * This function returns the current working state of this status.
 *
 * Note that if is_error() is returning true, then this working state
 * is not considered when calling the get_status() function.
 *
 * By default the working state is set to working_t::NO which means
 * that the page is not being worked on.
 *
 * \return The current status working state.
 */
path_info_t::status_t::working_t path_info_t::status_t::get_working() const
{
    return f_working;
}


/** \brief Indicate whether a process is currently working on that page.
 *
 * This function returns true if the current working status was
 * something else than path_info_t::status_t::NOT_WORKING.
 *
 * \return true if a process is working on this page.
 */
bool path_info_t::status_t::is_working() const
{
    return f_working != working_t::NOT_WORKING;
}


/** \brief Convert \p state to a string.
 *
 * This function converts the specified \p state number to a string.
 *
 * The state is expected to be a value returned by the get_state()
 * function.
 *
 * \exception content_exception_content_invalid_state
 * The state must be one of the defined state_t enumerations. Anything
 * else and this function raises this exception.
 *
 * \param[in] state  The state to convert to a string.
 *
 * \return The string representing the \p state.
 */
std::string path_info_t::status_t::status_name_to_string(status_t::state_t const state)
{
    switch(state)
    {
    case status_t::state_t::UNKNOWN_STATE:
        return "unknown";

    case status_t::state_t::CREATE:
        return "create";

    case status_t::state_t::NORMAL:
        return "normal";

    case status_t::state_t::HIDDEN:
        return "hidden";

    case status_t::state_t::MOVED:
        return "moved";

    case status_t::state_t::DELETED:
        return "deleted";

    }

    throw content_exception_content_invalid_state("Unknown status.");
}


/** \brief Convert a string to a state.
 *
 * This function converts a string to a page state. If the string does
 * not represent a valid state, then the function returns UNKNOWN_STATE.
 *
 * The string must be all lowercase.
 *
 * \param[in] state  The \p state to convert to a number.
 *
 * \return The number representing the specified \p state string or UNKNOWN_STATE.
 */
path_info_t::status_t::state_t path_info_t::status_t::string_to_status_name(std::string const & state)
{
    // this is the default if nothing else matches
    //if(state == "unknown")
    //{
    //    return status_t::state_t::UNKNOWN_STATE;
    //}
    if(state == "create")
    {
        return status_t::state_t::CREATE;
    }
    if(state == "normal")
    {
        return status_t::state_t::NORMAL;
    }
    if(state == "hidden")
    {
        return status_t::state_t::HIDDEN;
    }
    if(state == "moved")
    {
        return status_t::state_t::MOVED;
    }
    if(state == "deleted")
    {
        return status_t::state_t::DELETED;
    }
    // TBD: should we understand "unknown" and throw here instead?
    return status_t::state_t::UNKNOWN_STATE;
}





/** \brief Handle the status of a page safely.
 *
 * This class saves the current status of a page and restores it when
 * the class gets destroyed with the hope that the page status will
 * always stay valid. We still have a "resetstate" action and call
 * that function from our backend whenever the backend runs.
 *
 * The object is actually used to change the status to the status
 * specified in \p now. You may set \p now to the current status
 * if you do not want to change it until you are done.
 *
 * The \p end parameter is what the status will be once the function
 * ends and this RAII object gets destroyed. This could be the
 * current status to restore the status after you are done with your
 * work.
 *
 * \param[in] ipath  The path of the page of which the path is to be safe.
 * \param[in] now  The status of the page when the constructor returns.
 * \param[in] end  The status of the page when the destructor returns.
 */
path_info_t::raii_status_t::raii_status_t(path_info_t & ipath, status_t now, status_t end)
    : f_ipath(ipath)
    , f_end(end)
{
    status_t current(f_ipath.get_status());

    // reset the error in case we are loading from a non-existant page
    if(current.is_error())
    {
        if(current.get_error() != status_t::error_t::UNDEFINED)
        {
            // the page probably exists, but we still got an error
            throw content_exception_content_invalid_state(QString("get error %1 when trying to change \"%2\" status.")
                    .arg(static_cast<int>(current.get_error()))
                    .arg(f_ipath.get_key()));
        }
        current.set_error(status_t::error_t::NO_ERROR);
    }

    // setup state if requested
    if(now.get_state() != status_t::state_t::UNKNOWN_STATE)
    {
        current.set_state(now.get_state());
    }

    // setup working state if requested
    if(now.get_working() != status_t::working_t::UNKNOWN_WORKING)
    {
        current.set_working(now.get_working());
    }

    f_ipath.set_status(current);
}


/** \brief This destructor attempts to restore the page status.
 *
 * This function is the counter part of the constructor. It ensures that
 * the state changes to what you want it to be when you release the
 * RAII object.
 */
path_info_t::raii_status_t::~raii_status_t()
{
    status_t current(f_ipath.get_status());
    if(f_end.get_state() != status_t::state_t::UNKNOWN_STATE)
    {
        // avoid exceptions in destructors
        try
        {
            current.set_state(f_end.get_state());
        }
        catch(...)
        {
            SNAP_LOG_ERROR("caught exception in raii_status_t::~raii_status_t() -- set_state() failed");
        }
    }
    if(f_end.get_working() != status_t::working_t::UNKNOWN_WORKING)
    {
        // avoid exceptions in destructors
        try
        {
            current.set_working(f_end.get_working());
        }
        catch(...)
        {
            SNAP_LOG_ERROR("caught exception in raii_status_t::~raii_status_t() -- set_working() failed");
        }
    }
    try
    {
        f_ipath.set_status(current);
    }
    catch(...)
    {
        SNAP_LOG_ERROR("caught exception in raii_status_t::~raii_status_t() -- set_status() failed");
    }
}






SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
