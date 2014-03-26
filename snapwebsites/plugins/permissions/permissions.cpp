// Snap Websites Server -- manage permissions for users, forms, etc.
// Copyright (C) 2013-2014  Made to Order Software Corp.
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

//#define SHOW_RIGHTS

#include "permissions.h"

#include "../output/output.h"
#include "../users/users.h"
#include "../messages/messages.h"

#include "plugins.h"
#include "not_reached.h"
#include "qstring_stream.h"
#include "log.h"

#include <QtCassandra/QCassandraValue.h>

#include <openssl/rand.h>

#include "poison.h"

SNAP_PLUGIN_START(permissions, 1, 0)


/** \enum name_t
 * \brief Names used by the permissions plugin.
 *
 * This enumeration is used to avoid entering the same names over and
 * over and the likelihood of misspelling that name once in a while.
 */


/** \brief Get a fixed permissions plugin name.
 *
 * The permissions plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_PERMISSIONS_ACTION_PATH:
        return "types/permissions/actions";

    case SNAP_NAME_PERMISSIONS_ADMINISTER:
        return "permissions::administer";

    case SNAP_NAME_PERMISSIONS_DYNAMIC:
        return "permissions::dynamic";

    case SNAP_NAME_PERMISSIONS_EDIT:
        return "permissions::edit";

    case SNAP_NAME_PERMISSIONS_GROUP:
        return "permissions::group";

    case SNAP_NAME_PERMISSIONS_GROUP_RETURNING_REGISTERED_USER:
        return "permissions::group::returning_registered_user";

    case SNAP_NAME_PERMISSIONS_GROUPS_PATH:
        return "types/permissions/groups";

    case SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER:
        return "permissions::login_status::spammer";

    case SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR:
        return "permissions::login_status::visitor";

    case SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_VISITOR:
        return "permissions::login_status::returning_visitor";

    case SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED:
        return "permissions::login_status::returning_registered";

    case SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED:
        return "permissions::login_status::registered";

    case SNAP_NAME_PERMISSIONS_MAKE_ROOT:
        return "makeroot";

    case SNAP_NAME_PERMISSIONS_NAMESPACE:
        return "permissions";

    case SNAP_NAME_PERMISSIONS_PATH:
        return "types/permissions";

    case SNAP_NAME_PERMISSIONS_RIGHTS_PATH:
        return "types/permissions/rights";

    case SNAP_NAME_PERMISSIONS_USERS_PATH:
        return "types/permissions/users";

    case SNAP_NAME_PERMISSIONS_VIEW:
        return "permissions::view";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_PERMISSIONS_...");

    }
    NOTREACHED();
}


/** \class snap::permissions::permissions::sets_t
 * \brief Handle sets of permissions.
 *
 * The permissions are represented by sets. A permission set includes rights,
 * which are paths to different permission types. Each plugin can offer its
 * own specific rights or make use of rights offered by other plugins.
 *
 * The user is given rights depending on his status on the website. A simple
 * visitor will only get a very few rights. A full administrator will have
 * many rights.
 *
 * Rights are represented by paths to types. For example, you could be given
 * the right to tweak basic information on your website with this type:
 *
 * \code
 * /types/permissions/rights/administer/website/info
 * \endcode
 *
 * The interesting aspect of having a path is that by itself it already
 * represents a set. So the filter module offers a filter right as follow:
 *
 * \code
 * /types/permissions/rights/administer/website/filter
 * \endcode
 *
 * A user who has the .../website/info right does not have the
 * .../website/fitler right. However, a user who has the .../website
 * right is allowed to access both: .../website/info and .../website/filter.
 * This is because the parent of a right gives the user all the rights
 * below that parent.
 *
 * In the following, the user has access:
 *
 * \code
 * [User]
 * /type/permissions/rights/administer/website
 *
 * [Plugins]
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website/filter
 * \endcode
 *
 * However, that works the other way around in the permissions set. This
 * means the parent is required by the user if the parent is required
 * by the plugin.
 *
 * In the following, the user does not have access:
 *
 * \code
 * [User]
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website/filter
 *
 * [Plugins]
 * /type/permissions/rights/administer/website
 * \endcode
 *
 * A top administrator has the full administer right:
 *
 * \code
 * /type/permissions/rights/administer
 * \endcode
 *
 * Because of those special features, we do not use the QSet class
 * because QSet doesn't know anything about paths, parent/children, etc.
 * Thus we use maps and vectors and compute the intersections ourselves.
 * Actually, we very much avoid inserting duplicates within one set.
 * For example, if a user has the following three rights:
 *
 * \code
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website/filter
 * /type/permissions/rights/administer/website
 * \endcode
 *
 * Only the last one is required in the user's set since it covers the
 * other two in the event the plugin require them too.
 */


/** \brief Initialize a permission sets_t object.
 *
 * A sets_t object includes all the sets linked to path and action.
 * The constructor saves the path and action in the object. These two
 * parameters are read-only parameters.
 *
 * \param[in] user_path  The path to the user.
 * \param[in,out] ipath  The path being queried.
 * \param[in] action  The action being used in this query.
 * \param[in] login_status  The state of the log in of this user.
 */
permissions::sets_t::sets_t(QString const& user_path, content::path_info_t& ipath, QString const& action, QString const& login_status)
    : f_user_path(user_path)
    , f_ipath(ipath)
    , f_action(action)
    , f_login_status(login_status)
    //, f_user_rights() -- auto-init
    //, f_plugin_permissions() -- auto-init
{
}


/** \brief Set the log in status of the user.
 *
 * This function is used to define the status login of the user. This
 * is used by the get_user_rights() signal to know which set of rights
 * should be added for the user.
 *
 * So defining the user is not enough, you also need to define his
 * currently considered status. You could check whether a user can
 * access a page as a visitor, as a returning registered user, or
 * as a logged in user.
 *
 * \li SETS_LOGIN_STATUS_SPAMMER -- the user is considered a spammer
 * \li SETS_LOGIN_STATUS_VISITOR -- the user is anonymous
 * \li SETS_LOGIN_STATUS_RETURNING_VISITOR -- the user is anonymous, but
 *                                            is a returning visitor
 * \li SETS_LOGIN_STATUS_RETURNING_REGISTERED -- user logged in more than
 *                                               3h ago (time can be changed)
 * \li SETS_LOGIN_STATUS_REGISTERED -- the user logged in recently
 *
 * \note
 * You are not supposed to modify the status while generating user or
 * plugin rights. However, a plugin testing permissions for a user
 * may want to test with multiple login status.
 *
 * \param[in] login_status  The log in status to consider while adding user rights.
 */
void permissions::sets_t::set_login_status(QString const& login_status)
{
    f_login_status = login_status;
}


/** \brief Retrieve the log in status of the user.
 *
 * By default the login status of the user is set to "visitor". Other
 * statuses can be used to check whether a user has rights if logged in
 * or as a spammer, etc.
 *
 * This function is used by the get_user_rights() signal to know which
 * set of rights to add to the user.
 *
 * \return The login status of the user being checked.
 */
QString const& permissions::sets_t::get_login_status() const
{
    return f_login_status;
}


/** \brief The user being checked.
 *
 * By default the permissions are checked for the current user. As you can
 * see in the on_validate_action() signal handler, the user parameter is
 * set to the currently logged in user.
 *
 * This function is used by the users plugin to assign the correct rights
 * to this sets_t object.
 *
 * \return The user concerned by this right validation.
 *
 * \sa add_user_right()
 */
QString const& permissions::sets_t::get_user_path() const
{
    return f_user_path;
}


/** \brief The path this permissions are checked against.
 *
 * The user rights are being checked against this path. This path represents
 * the page being viewed and any plugin that "recognizes" that path shall
 * define rights as offered by that plugin.
 *
 * \return The path being checked against user's rights.
 */
content::path_info_t& permissions::sets_t::get_ipath() const
{
    return f_ipath;
}


/** \brief Get the sets action.
 *
 * Whenever add rights to the sets, we pre-intersect those with the action.
 * This is a powerful optimization since that way we avoid adding or testing
 * many things which would be totally useless (i.e. imagine adding 100 rights
 * when that action only offers 3 rights!)
 *
 * \return The action of these sets.
 */
const QString& permissions::sets_t::get_action() const
{
    return f_action;
}


/** \brief Rights the user has are added with this function.
 *
 * This function is to be used to add rights that the user has. A
 * right is a link path (i.e. /types/permissions/rights/\<name>).
 *
 * If the same right is added more than once, then only one instance is
 * kept. Actually, if a better right is added, the old not as good right
 * gets removed.
 *
 * \code
 * # When adding this right:
 * /type/permissions/rights/administer/website
 *
 * # and that right was in the set, then only the new right is kept
 * /type/permissions/rights/administer/website/info
 * \endcode
 *
 * Actually it is possible that adding one new right removes many existing
 * rights because it is a much higher level right:
 *
 * \code
 * # When adding this right:
 * /type/permissions/rights/administer/website
 *
 * # then all those rights get removed:
 * /type/permissions/rights/administer/website/layout
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website/feed
 * ...
 * \endcode
 *
 * \param[in] right  The right being added.
 */
void permissions::sets_t::add_user_right(QString right)
{
    // so the startsWith() works as is:
    if(right.right(1) != "/")
    {
        right = right + "/";
    }
    int const len(right.length());
    int max(f_user_rights.size());
    for(int i(0); i < max; ++i)
    {
        int const l(f_user_rights[i].length());
        if(len > l)
        {
            if(right.startsWith(f_user_rights[i]))
            {
                // we're done, that right is already here
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                std::cout << "  USER RIGHT -> [" << right << "] (ignore, better already there)" << std::endl;
#endif
#endif
                return;
            }
        }
        else if(l > len)
        {
            if(f_user_rights[i].startsWith(right))
            {
                // this new right is smaller, keep that smaller one instead
                f_user_rights[i] = right;
                ++i;
                while(i < max)
                {
                    if(f_user_rights[i].startsWith(right))
                    {
                        f_user_rights.remove(i);
                        --max; // here max decreases!
                    }
                    else
                    {
                        ++i; // in this case i increases
                    }
                }
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                std::cout << "  USER RIGHT -> [" << right << "] (shrunk)" << std::endl;
#endif
#endif
                return;
            }
        }
        else /*if(l == len)*/
        {
            if(f_user_rights[i] == right)
            {
                // that's exactly the same, no need to have it twice
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                std::cout << "  USER RIGHT -> [" << right << "] (already present)" << std::endl;
#endif
#endif
                return;
            }
        }
    }

    // this one was not there yet, just append
    f_user_rights.push_back(right);
#ifdef DEBUG
#ifdef SHOW_RIGHTS
    std::cout << "  USER RIGHT -> [" << right << "] (add)" << std::endl;
#endif
#endif
}


/** \brief Return the number of user rights.
 *
 * This function returns the number of rights the user has. Note that
 * user rights are added only if those rights match the specified
 * action. So for example we do not add "view" rights for a user if
 * the action is "delete". This means the number of a user rights
 * represents the intersection between all the user rights and the
 * action specified in this sets_t object. If empty, then the user
 * does not even have that very permission at all.
 *
 * \return The number of rights the user has for this action.
 */
int permissions::sets_t::get_user_rights_count() const
{
    return f_user_rights.size();
}


/** \brief Add a permission from the specified plugin.
 *
 * This function adds a right for the specified plugin. In most cases this
 * works on a per plugin basis. So a plugin adds its own rights only.
 * However, some plugins can add rights for other plugins (right
 * complements or right sharing.)
 *
 * The plugin name is used to create a separate set of rights, one per
 * plugin. The user must have enough rights for each separate group of
 * plugin to be allowed the action sought.
 *
 * \note
 * This function optimizes the set by removing more constraning rights.
 * This means when adding a permissions such as .../administer/website/info
 * and the set already has a permission .../administer/website then that
 * permission gets replaced with the new one because the user may have
 * right .../administer/website and match both those rights anyway.
 *
 * \code
 * # In plugin permissions, the following
 * /type/permissions/rights/administer/website/info
 * /type/permissions/rights/administer/website
 *
 * # Is equivalent to the following:
 * /type/permissions/rights/administer/website/info
 * \endcode
 *
 * \note
 * Similarly, we do not add a plugin permission if an easier to get right
 * is already present in the set of that plugin.
 *
 * \param[in] plugin  The plugin adding this permission.
 * \param[in] right  The right the plugin offers.
 */
void permissions::sets_t::add_plugin_permission(const QString& plugin, QString right)
{
    // so the startsWith() works as is:
    if(right.right(1) != "/")
    {
        right = right + "/";
    }

    if(!f_plugin_permissions.contains(plugin))
    {
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        std::cout << "  PLUGIN [" << plugin << "] PERMISSION -> [" << right << "] (add, new plugin)" << std::endl;
#endif
#endif
        f_plugin_permissions[plugin].push_back(right);
        return;
    }

    set_t& set(f_plugin_permissions[plugin]);
    int const len(right.length());
    int const max(set.size());
    int i(0);
    while(i < max)
    {
        int const l(set[i].length());
        if(len > l)
        {
            if(right.startsWith(set[i]))
            {
                // the new right is generally considered easier to get
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                std::cout << "  PLUGIN [" << plugin << "] PERMISSION -> [" << set[i] << "] (REMOVING)" << std::endl;
#endif
#endif
                set.remove(i);
                continue;
            }
        }
        else if(l > len)
        {
            if(set[i].startsWith(right))
            {
                // this new right is harder to get, ignore it
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                std::cout << "  PLUGIN [" << plugin << "] PERMISSION -> [" << right << "] (skipped)" << std::endl;
#endif
#endif
                return;
            }
        }
        else /*if(l == len)*/
        {
            if(set[i] == right)
            {
                // that's exactly the same, no need to have it twice
#ifdef DEBUG
#ifdef SHOW_RIGHTS
                std::cout << "  PLUGIN [" << right << "] PERMISSION -> [" << right << "] (already present)" << std::endl;
#endif
#endif
                return;
            }
        }
        ++i;
    }

    // this one was not there yet, just append
    set.push_back(right);
#ifdef DEBUG
#ifdef SHOW_RIGHTS
    std::cout << "  PLUGIN [" << plugin << "] PERMISSION -> [" << right << "] (add right)" << std::endl;
#endif
#endif
}


/** \brief Check whether the user has root permissions.
 *
 * This function quickly searches for the one permission that marks a
 * user as the root user. This is done by testing whether the user has
 * the main rights permission (types/permissions/rights).
 *
 * This function is called before checking all the rights on a page
 * because whatever those rights, if the user has root permissions,
 * then he will have the right. Period.
 *
 * This being said, the data that's locked is still checked and the
 * access on those is still prevented if a system lock exists.
 *
 * \return true if the user is a root user.
 */
bool permissions::sets_t::is_root() const
{
    // the top rights type represents the full root user (i.e. all rights)
    return f_user_rights.contains(get_name(SNAP_NAME_PERMISSIONS_RIGHTS_PATH));
}


/** \brief Check whether the user is allowed to perform the action.
 *
 * This function executes the intersection between the user rights
 * and the different plugin rights found while running the
 * get_plugin_permissions() signal. If the intersection of the user
 * rights with any one list is the empty set, then the function returns
 * false. Otherwise it returns true.
 *
 * \return true if the user is allowed to perform this action.
 */
bool permissions::sets_t::allowed() const
{
    if(f_user_rights.isEmpty()
    || f_plugin_permissions.isEmpty())
    {
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        std::cout << "f_user_rights.size()=" << f_user_rights.size() << ", f_plugin_permissions.size() = " << f_plugin_permissions.size() << std::endl;
        std::cout << "sets are not allowed!" << std::endl;
#endif
#endif
        // if the plugins added nothing, there are no rights to compare
        // or worst, the user have no rights at all (Should not happen,
        // although someone could add a plugin testing something such as
        // your IP address to strip you off all your rights unless you
        // have an IP address considered "valid")
        return false;
    }

#ifdef DEBUG
#ifdef SHOW_RIGHTS
std::cout << "final USER RIGHTS:" << std::endl;
for(int i(0); i < f_user_rights.size(); ++i)
{
    std::cout << "  [" << f_user_rights[i] << "]" << std::endl;
}
std::cout << "final PLUGIN PERMISSIONS:" << std::endl;
for(req_sets_t::const_iterator pp(f_plugin_permissions.begin());
        pp != f_plugin_permissions.end();
        ++pp)
{
    std::cout << "  [" << pp.key() << "]:" << std::endl;
    for(int j(0); j < pp->size(); ++j)
    {
        std::cout << "    [" << (*pp)[j] << "]" << std::endl;
    }
}
#endif
#endif

    int const max(f_user_rights.size());
    for(req_sets_t::const_iterator pp(f_plugin_permissions.begin());
            pp != f_plugin_permissions.end();
            ++pp)
    {
        // enough rights with this one?
        set_t p(*pp);
        set_t::const_iterator i;
        for(i = p.begin(); i != p.end(); ++i)
        {
            // the list of plugin permissions is generally smaller so
            // we loop through that list although this contains can be
            // slow... we may want to change the set_t type to a QMap
            // which uses a faster binary search; although on very small
            // maps it may not be any faster
            for(int j(0); j < max; ++j)
            {
                const QString& plugin_permission ( *i               );
                const QString& user_right        ( f_user_rights[j] );
                if( plugin_permission.startsWith(user_right) )
                {
                    //break 2;
                    goto next_plugin;
                }
            }
        }
        // XXX add a log to determine the name of the plugin that
        //     failed the user?
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        std::cout << "  failed, no match for [" << pp.key() << "]" << std::endl;
#endif
#endif
        return false;
next_plugin:;
    }

#ifdef DEBUG
#ifdef SHOW_RIGHTS
    std::cout << "  allowed!!!" << std::endl;
#endif
#endif
    return true;
}


/** \brief Initialize the permissions plugin.
 *
 * This function is used to initialize the permissions plugin object.
 */
permissions::permissions()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Clean up the permissions plugin.
 *
 * Ensure the permissions object is clean before it is gone.
 */
permissions::~permissions()
{
}


/** \brief Initialize the permissions.
 *
 * This function terminates the initialization of the permissions plugin
 * by registering for different events it supports.
 *
 * \param[in] snap  The child handling this request.
 */
void permissions::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(permissions, "path", path::path, validate_action, _1, _2, _3);
    SNAP_LISTEN(permissions, "path", path::path, access_allowed, _1, _2, _3, _4, _5);
    SNAP_LISTEN(permissions, "server", server, register_backend_action, _1);
    SNAP_LISTEN(permissions, "users", users::users, user_verified, _1, _2);
}


/** \brief Get a pointer to the permissions plugin.
 *
 * This function returns an instance pointer to the permissions plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the permissions plugin.
 */
permissions *permissions::instance()
{
    return g_plugin_permissions_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString permissions::description() const
{
    return "The permissions plugin is one of the most important plugins of the"
          " Snap! system. It allows us to determine whether the current user"
          " has enough rights to act on a specific page.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run yet.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when this plugin was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t permissions::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 3, 14, 0, 45, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the content with our references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void permissions::content_update(int64_t variables_timestamp)
{
    (void) variables_timestamp;
    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Implementation of the get_user_rights signal.
 *
 * This function readies the user rights in the specified \p sets.
 *
 * The plugins that capture this function are expected to add user
 * rights to the sets (with the add_user_right() function.) No plugin
 * permissions should be modified at this point. The
 * get_plugin_permissions() is used to that effect.
 *
 * Note that only the rights that correspond to the specified action are
 * to be added here.
 *
 * \code
 *   QString const site_key(f_snap->get_site_key_with_slash());
 *   sets.add_user_right(site_key + "types/permissions/rights/edit/page");
 * \endcode
 *
 * \note
 * Although it is a permission thing, the user plugin makes most of the
 * work of determining the user rights (although the user plugin calls
 * functions of the permission plugin such as the add_user_rights()
 * function.) This is because the user plugin can include the permissions
 * plugin, but not the other way around.
 *
 * \todo
 * Fix the dependencies so one of permissions or user uses the other,
 * at this time they depend on each other!
 *
 * \param[in] perms  A pointer to the permissions plugin.
 * \param[in,out] sets  The sets_t object where rights get added.
 *
 * \return true if the signal has to be sent to other plugins.
 *
 * \sa add_user_rights()
 */
bool permissions::get_user_rights_impl(permissions *perms, sets_t& sets)
{
    QString const& login_status(sets.get_login_status());

    // if spammers are logged in they don't get access to anything anyway
    // (i.e. they are UNDER visitors!)
    QString const site_key(f_snap->get_site_key_with_slash());
    if(login_status == get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER))
    {
        perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user/returning-visitor/visitor/spammer", sets);
    }
    else
    {
        if(login_status == get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_VISITOR))
        {
            perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user/returning-visitor", sets);
        }
        else
        {
            // unfortunately, whatever the login status, if we were not given
            // a valid user path, we just cannot test anything else than
            // some kind of visitor
            QString const user_key(sets.get_user_path());
//#ifdef DEBUG
//std::cout << "  +-> user key = [" << user_key << "]" << std::endl;
//#endif
            if(user_key.isEmpty()
            || login_status == get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR))
            {
                // in this case the user is an anonymous user and thus we want to
                // add the anonymous user rights
                perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user/returning-visitor/visitor", sets);
            }
            else
            {
                content::path_info_t user_ipath;
                user_ipath.set_path(user_key);

                // add all the groups the user is a member of
                QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
                if(!content_table->exists(user_ipath.get_key()))
                {
                    // that user is gone, this will generate a 500 by Apache
                    throw permissions_exception_invalid_path("could not access user \"" + user_ipath.get_key() + "\"");
                }

                // should this one NOT be offered to returning users?
                sets.add_user_right(user_ipath.get_key());

                if(login_status == get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED))
                {
                    // users who are logged in always have registered-user
                    // rights if nothing else
                    perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user", sets);

                    // add assigned groups
                    {
                        QString const link_start_name(get_name(SNAP_NAME_PERMISSIONS_GROUP));
                        links::link_info info(link_start_name, false, user_ipath.get_key(), user_ipath.get_branch());
                        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                        links::link_info right_info;
                        while(link_ctxt->next_link(right_info))
                        {
                            QString const right_key(right_info.key());
                            perms->add_user_rights(right_key, sets);
                        }
                    }

                    // we can also assign permissions directly to a user so get those too
                    {
                        QString const link_start_name(QString("%1::%2").arg(get_name(SNAP_NAME_PERMISSIONS_NAMESPACE)).arg(sets.get_action()));
                        links::link_info info(link_start_name, false, user_ipath.get_key(), user_ipath.get_branch());
                        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                        links::link_info right_info;
                        while(link_ctxt->next_link(right_info))
                        {
                            QString const right_key(right_info.key());
                            perms->add_user_rights(right_key, sets);
                        }
                    }
                }
                else
                {
                    // this is a registered user who comes back and is
                    // semi-logged in so we do not give this user full rights
                    // to avoid potential security problems; we have to look
                    // into a way to offer different group/rights for such a
                    // user...
                    perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user", sets);

                    // add assigned groups
                    // by groups limited to returning registered users, not the logged in registered user
                    {
                        QString const link_start_name(get_name(SNAP_NAME_PERMISSIONS_GROUP_RETURNING_REGISTERED_USER));
                        links::link_info info(link_start_name, false, user_ipath.get_key(), user_ipath.get_branch());
                        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                        links::link_info right_info;
                        while(link_ctxt->next_link(right_info))
                        {
                            QString const right_key(right_info.key());
                            perms->add_user_rights(right_key, sets);
                        }
                    }
                }
            }
        }
    }
    return true;
}


/** \brief Implementation of the get_plugin_permissions signal.
 *
 * This function readies the plugin rights in the specified \p sets.
 *
 * The plugins that capture this function are expected to add plugin
 * permissions to the sets (with the add_plugin_permission() function.)
 * No user rights should be modified in this process. Those are taken
 * cared of by the get_user_rights().
 *
 * Note that for plugins we use the term permissions because the plugin
 * allows that capability, whereas a user has rights. However, in the end,
 * the two terms point to the exact same thing: a string to a right defined
 * in the /types/permissions/actions/\<name>.
 *
 * \code
 *   QString const site_key(f_snap->get_site_key_with_slash());
 *   sets.add_plugin_permission(get_plugin_name(), site_key + "types/permissions/rights/edit");
 * \endcode
 *
 * Note that using the get_plugin_name() is a good idea to avoid typing the
 * wrong name. It is legal for a plugin to add a permission for another
 * plugin in which case the name can be retrieved using the the fully
 * qualified name:
 *
 * \code
 *   users::users::get_plugin_name();
 * \endcode
 *
 * \note
 * Although the permissions are rather tangled with the content, this
 * plugin checks the basic content setup because the content plugin cannot
 * have references to the permissions (because permissions include content
 * we cannot then include permissions from the content.)
 *
 * \param[in] perms  A pointer to the permissions plugin.
 * \param[in,out] sets  The sets_t object where rights get added.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool permissions::get_plugin_permissions_impl(permissions *perms, sets_t& sets)
{
    static_cast<void>(perms);

    // the user plugin cannot include the permissions (since the
    // permissions includes the user plugin) so we implement this
    // user plugin feature in the permissions
    content::path_info_t& ipath(sets.get_ipath());
    if(ipath.get_cpath().left(5) == "user/")
    {
        QString const user_id(ipath.get_cpath().mid(5));
        QByteArray id_str(user_id.toUtf8());
        char const *s;
        for(s = id_str.data(); *s != '\0'; ++s)
        {
            if(*s < '0' || *s > '9')
            {
                // only digits allowed (i.e. user/123)
                break;
            }
        }
        if(*s != '\0')
        {
            sets.add_plugin_permission(content::content::instance()->get_plugin_name(), ipath.get_key());
            //"types/permissions/rights/view/page/private"
        }
    }

    // the content plugin cannot include the permissions (since the
    // permissions includes the content plugin) so we implement this
    // content plugin feature in the permissions
    //
    // this very page may be assigned direct permissions
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString key(ipath.get_key());
    if(!content_table->exists(key)
    || !content_table->row(ipath.get_key())->exists(content::get_name(content::SNAP_NAME_CONTENT_PRIMARY_OWNER)))
    {
        // if that page does not exist, it may be dynamic, try to go up
        // until we have one name in the path then check that the page
        // allows such, if so, we have a chance, otherwise no rights
        // from here... (as an example see /verify in plugins/users/content.xml)
        QStringList parts(ipath.get_cpath().split('/'));
        int depth(0);
        for(;;)
        {
            parts.pop_back();
            if(parts.isEmpty())
            {
                // let other modules take over, we're done here
                return true;
            }
            ++depth;
            QString const parent_path(parts.join("/"));
            key = site_key + parent_path;
            if(content_table->exists(key))
            {
                break;
            }
        }
        QtCassandra::QCassandraRow::pointer_t row(content_table->row(key));
        char const *dynamic(get_name(SNAP_NAME_PERMISSIONS_DYNAMIC));
        if(!row->exists(dynamic))
        {
            // well, there is a page, but it does not authorize sub-pages
            return true;
        }
        QtCassandra::QCassandraValue value(row->cell(dynamic)->value());
        if(depth > value.signedCharValue())
        {
            // there is a page, it gives permissions, but this very
            // page is too deep to be allowed
            return true;
        }
        ipath.set_real_path(key);
    }

    content::path_info_t page_ipath;
    page_ipath.set_path(key);
    QString const link_start_name("permissions::" + sets.get_action());
    {
        // check local links for this action
        links::link_info info(link_start_name, false, key, page_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            QString const right_key(right_info.key());
            sets.add_plugin_permission(content::content::instance()->get_plugin_name(), right_key);
        }
    }

    {
        // get the content type (content::page_type) and then retrieve
        // the rights directly from that type
        QString const link_name(get_name(content::SNAP_NAME_CONTENT_PAGE_TYPE));
        links::link_info info(link_name, true, key, page_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info content_type_info;
        if(link_ctxt->next_link(content_type_info)) // use if() since it is unique on this end
        {
            content::path_info_t tpath;
            tpath.set_path(content_type_info.key());

            {
                // read from the content type now
                links::link_info perm_info(link_start_name, false, tpath.get_key(), tpath.get_branch());
                link_ctxt = links::links::instance()->new_link_context(perm_info);
                links::link_info right_info;
                while(link_ctxt->next_link(right_info))
                {
                    QString const right_key(right_info.key());
                    sets.add_plugin_permission(content::content::instance()->get_plugin_name(), right_key);
                }
            }

            {
                // finally, check if there are groups defined for this content type
                // groups here function the same way as user groups
                links::link_info perm_info("permissions::groups", false, tpath.get_key(), tpath.get_branch());
                link_ctxt = links::links::instance()->new_link_context(perm_info);
                links::link_info right_info;
                while(link_ctxt->next_link(right_info))
                {
                    QString const right_key(right_info.key());
                    add_plugin_permissions(content::content::instance()->get_plugin_name(), right_key, sets);
                }
            }
        }
    }

    return true;
}


/** \brief Generate the actual content of the statistics page.
 *
 * This function generates the contents of the statistics page of the
 * permissions plugin.
 *
 * \todo
 * TBD -- Determine whether this function is really necessary in such a
 * low level plugin (plus this means a dependency to the layout plugin
 * which is probably incorrect here.)
 *
 * \param[in,out] ipath  The path to this page.
 * \param[in,out] page  The page element being generated.
 * \param[in,out] body  The body element being generated.
 * \param[in] ctemplate  Template used for parameters that are otherwise missing.
 */
void permissions::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    // show the permission pages as information (many of these are read-only)
    output::output::instance()->on_generate_main_content(ipath, page, body, ctemplate);
}


/** \brief Validate an action.
 *
 * Whenever a user accesses the website, his action needs to first be
 * verified and then permitted by checking whether the user has enough
 * rights to access the page and apply the action. For example, a user
 * who wants to edit a page needs to have enough rights to edit that
 * page.
 *
 * The name of the action is defined as "view" (the default) or the
 * name of the action defined in the action variable of the URL. By
 * default that variable is "a". So a user who wants to edit a page
 * makes use of "a=edit" as one of the query variables.
 *
 * \param[in,out] ipath  The path the user wants to access.
 * \param[in] action  The action to be taken, the function may redefine it.
 * \param[in,out] err_callback  Call functions on errors.
 */
void permissions::on_validate_action(content::path_info_t& ipath, QString const& action, permission_error_callback& err_callback)
{
    if(action.isEmpty())
    {
        // always emit this error, that's a programmer bug, not a standard
        // user problem that can happen so do not use the err_callback
        f_snap->die(snap_child::HTTP_CODE_ACCESS_DENIED,
                "Access Denied",
                "You are not authorized to access our website in this way.",
                QString("programmer checking permission access with an empty action on page \"%1\".").arg(ipath.get_key()));
        NOTREACHED();
    }

    users::users *users_plugin(users::users::instance());
    QString user_path(users_plugin->get_user_path());
    if(user_path == "user")
    {
        user_path.clear();
    }
    QString login_status(get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER));
    if(!users_plugin->user_is_a_spammer())
    {
        if(user_path.isEmpty())
        {
            login_status = get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR);
            // TODO determine, once possible, whether the user came on the
            //      website before (i.e. returning visitor)
            //      (it is already possible since we have a cookie, just
            //      need to take the time to do it!)
        }
        else if(users_plugin->user_is_logged_in())
        {
            login_status = get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED);
        }
        else
        {
            login_status = get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED);
        }
    }
    content::permission_flag allowed;
    path::path::instance()->access_allowed(user_path, ipath, action, login_status, allowed);
    if(!allowed.allowed())
    {
        if(users_plugin->get_user_key().isEmpty())
        {
            // special case of spammers
            if(users_plugin->user_is_a_spammer())
            {
                // force a redirect on error not from the home page
                if(ipath.get_cpath() != "")
                {
                    // spammers are expected to have enough rights to access
                    // the home page so we try to redirect them there
                    err_callback.on_redirect(
                        // message
                        "Access Denied",
                        QString("The page you were trying to access (%1) requires more privileges.").arg(ipath.get_cpath()),
                        QString("spammer trying to \"%1\" on page \"%2\".").arg(action).arg(ipath.get_cpath()),
                        false,
                        // redirect
                        "/",
                        snap_child::HTTP_CODE_FOUND);
                }
                else
                {
                    // if user does not even have access to the home page...
                    err_callback.on_error(snap_child::HTTP_CODE_ACCESS_DENIED,
                            "Access Denied",
                            "You are not authorized to access our website.",
                            QString("spammer trying to \"%1\" on page \"%2\" with unsufficient rights.").arg(action).arg(ipath.get_cpath()));
                }
                return;
            }

            if(ipath.get_cpath() == "login")
            {
                // An IP, Agent, etc. based test could get us here...
                err_callback.on_error(snap_child::HTTP_CODE_ACCESS_DENIED,
                        "Access Denied",
                        action != "view"
                            ? "You are not authorized to access the login page with action " + action
                            : "Somehow you are not authorized to access the login page.",
                        QString("user trying to \"%1\" on page \"%2\" with unsufficient rights.").arg(action).arg(ipath.get_cpath()));
                return;
            }

            // user is anonymous, there is hope, he may have access once
            // logged in
            // TODO all redirects need to also include a valid action!
            users_plugin->attach_to_session(get_name(users::SNAP_NAME_USERS_LOGIN_REFERRER), ipath.get_cpath());
            err_callback.on_redirect(
                // message
                "Unauthorized",
                QString("The page you were trying to access (%1) requires more privileges. If you think you have such, try to log in first.").arg(ipath.get_cpath()),
                QString("user trying to \"%1\" on page \"%2\" when not logged in.").arg(action).arg(ipath.get_cpath()),
                false,
                // redirect
                "login",
                snap_child::HTTP_CODE_FOUND);
        }
        else
        {
            if(login_status == get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED))
            {
                // allowed if logged in
                content::permission_flag allowed_if_logged_in;
                path::path::instance()->access_allowed(user_path, ipath, action, get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED), allowed_if_logged_in);
                if(allowed_if_logged_in.allowed())
                {
                    // ah! the user is not allowed here but he would be if
                    // only he were recently logged in (with the last 3h or
                    // whatever the administrator set that to.)
                    users_plugin->attach_to_session(get_name(users::SNAP_NAME_USERS_LOGIN_REFERRER), ipath.get_cpath());
                    err_callback.on_redirect(
                        // message
                        "Unauthorized",
                        QString("The page you were trying to access (%1) requires you to verify your credentials. Please log in again and the system will send you back there.").arg(ipath.get_cpath()),
                        QString("user trying to \"%1\" on page \"%2\" when not recently logged in.").arg(action).arg(ipath.get_cpath()),
                        false,
                        // redirect
                        "verify-credentials",
                        snap_child::HTTP_CODE_FOUND);
                    return;
                }
            }
            // user is already logged in; no redirect even once we support
            // the double password feature
            err_callback.on_error(snap_child::HTTP_CODE_ACCESS_DENIED,
                    "Access Denied",
                    QString("You are not authorized to apply this action (%1) to this page (%2).").arg(action).arg(ipath.get_key()),
                    QString("user trying to \"%1\" on page \"%2\" with unsufficient rights.").arg(action).arg(ipath.get_key()));
        }
        return;
    }
}


/** \brief Check whether the user has permission to access path.
 *
 * This function checks whether the specified \p user_path has enough
 * rights, type of which is defined by \p action, to access the
 * specified \p ipath.
 *
 * So for example the anonymous user can "view" a page only if that page
 * is publicly visible. The anonymous user has pretty much only the "view"
 * rights (he can fill some forms too, search, registration, log in,
 * comments, etc. but here we'll limit ourselves to "view".) So, this
 * function asks the users plugin: "Can the anonymous user view things?".
 * The answer is yes, so the system proceeds with the question to all
 * the plugins controlling the specified page: "Is there something that
 * the specified user can view?" If so, then those rights are added for
 * the plugins. If all the plugins with control over that page said that
 * they gave the "view" right, then the user is permitted to see the
 * page and the function returns true.
 *
 * Whenever you need to test whether a user can perform a certain action
 * against some content, this is the function you have to use. For example,
 * the xmlsitemap plugin checks whether pages are publicly accessible before
 * adding them to the sitemap.xml file because pages that are not accessible
 * in this way cannot appear in a search engine since the search engine
 * won't even be able to read the page.
 *
 * \param[in] user_path  The user trying to acccess the specified path.
 * \param[in,out] ipath  The path that the user is trying to access.
 * \param[in] action  The action that the user is trying to perform.
 * \param[in] login_status  The supposed status for that user.
 * \param[in] result  The result of the test.
 */
void permissions::on_access_allowed(QString const& user_path, content::path_info_t& ipath, QString const& action, QString const& login_status, content::permission_flag& result)
{
    // check that the action is defined in the database (i.e. valid)
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(site_key + get_name(SNAP_NAME_PERMISSIONS_ACTION_PATH) + ("/" + action));
    if(!content_table->exists(key))
    {
        // TODO it is rather easy to get here so we need to test whether
        //      the same IP does it over and over again and block them if so
        f_snap->die(snap_child::HTTP_CODE_ACCESS_DENIED,
                "Unknown Action",
                "The action you are trying to performed is not known by Snap!",
                QString("permissions::on_access_allowed() was used with action \"%1\".").arg(action));
        NOTREACHED();
    }

    // setup a sets object which will hold all the user's sets
    sets_t sets(user_path, ipath, action, login_status);

    // first we get the user rights for that action because in most cases
    // that's a lot smaller and if empty we do not have to get anything else
    // (intersection of an empty set with anything else is the empty set)
#ifdef DEBUG
#ifdef SHOW_RIGHTS
    std::cout << "retrieving USER rights... [" << sets.get_action() << "] [" << sets.get_login_status() << "] [" << ipath.get_cpath() << "]" << std::endl;
#endif
#endif
    get_user_rights(this, sets);
    if(sets.get_user_rights_count() != 0)
    {
        if(sets.is_root())
        {
            return;
        }
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        std::cout << "retrieving PLUGIN permissions... [" << sets.get_action() << "]" << std::endl;
#endif
#endif
        get_plugin_permissions(this, sets);
#ifdef DEBUG
#ifdef SHOW_RIGHTS
        std::cout << "now compute the intersection!" << std::endl;
#endif
#endif
        if(sets.allowed())
        {
            return;
        }
    }

    result.not_permitted();
}


/** \brief Add user rights.
 *
 * This function is called to add user rights from the specified group
 * and all of its children.
 *
 * Groups are expected to include links to different permission rights.
 *
 * \param[in] group  The rights of this group are added.
 * \param[in,out] sets  The sets receiving the rights.
 */
void permissions::add_user_rights(QString const& group, sets_t& sets)
{
    // a quick check to make sure that the programmer is not directly
    // adding a right (which he should do to the sets instead of this
    // function although we instead generate an error.)
    if(group.contains(get_name(SNAP_NAME_PERMISSIONS_RIGHTS_PATH)))
    {
        throw snap_logic_exception("you cannot add rights using add_user_rights(), for those just use sets.add_user_right() directly");
    }

    recursive_add_user_rights(group, sets);
}


/** \brief Recursively retrieve all the user rights.
 *
 * User rights are defined in groups and this function reads all the
 * rights defined in a group and all of its children.
 *
 * The recursivity works over the group children, and children of those
 * children and so on. So the number of iteration will remain relatively
 * limited.
 *
 * \param[in] group  The group being added (a row).
 * \param[in] sets  The sets where the different paths are being added.
 */
void permissions::recursive_add_user_rights(QString const& group, sets_t& sets)
{
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    if(!content_table->exists(group))
    {
        throw permissions_exception_invalid_group_name("caller is trying to access group \"" + group + "\"");
    }

    QtCassandra::QCassandraRow::pointer_t row(content_table->row(group));

    content::path_info_t group_ipath;
    group_ipath.set_path(group);

    // get the rights at this level
    {
        QString const link_start_name("permissions::" + sets.get_action());
        links::link_info info(link_start_name, false, group_ipath.get_key(), group_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // a user right is attached to this page
            QString const right_key(right_info.key());
            sets.add_user_right(right_key);
        }
    }

    // get all the children and do a recursive call with them all
    {
        QString const children_name("content::children");
        links::link_info info(children_name, false, group_ipath.get_key(), group_ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // a user right is attached to this page
            const QString child_key(right_info.key());
            recursive_add_user_rights(child_key, sets);
        }
    }
}


/** \brief Add plugin rights.
 *
 * This function is called to add plugin rights from the specified group
 * and all of its children.
 *
 * Groups are expected to include links to different permission rights.
 *
 * \todo
 * It seems to me that the name of the plugin should always be the
 * same when groups are concerned, i.e. "content", but I'm not really
 * 100% convinced of that so at this point it is a parameter. Also it
 * could be that the name should be defined in the group and not by
 * the caller (that may be the real deal).
 *
 * \param[in] plugin_name  The name of the plugin adding these rights.
 * \param[in] group  The rights of this group are added.
 * \param[in,out] sets  The sets receiving the rights.
 */
void permissions::add_plugin_permissions(QString const& plugin_name, QString const& group, sets_t& sets)
{
    // a quick check to make sure that the programmer is not directly
    // adding a right (which he should do to the sets instead of this
    // function although we instead generate an error.)
    if(group.contains("types/permissions/rights"))
    {
        throw snap_logic_exception("you cannot add rights using add_plugin_permissions(), for those just use sets.add_plugin_permission() directly");
    }

    recursive_add_plugin_permissions(plugin_name, group, sets);
}


/** \brief Recursively retrieve all the user rights.
 *
 * User rights are defined in groups and this function reads all the
 * rights defined in a group and all of its children.
 *
 * The recursivity works over the group children, and children of those
 * children and so on. So the number of iteration will remain relatively
 * limited.
 *
 * \param[in] plugin_name  The name of the plugin adding these rights.
 * \param[in] group  The key of the group being added (a row).
 * \param[in] sets  The sets where the different paths are being added.
 */
void permissions::recursive_add_plugin_permissions(QString const& plugin_name, QString const& group, sets_t& sets)
{
    QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
    if(!content_table->exists(group))
    {
        throw permissions_exception_invalid_group_name("caller is trying to access group \"" + group + "\"");
    }

    content::path_info_t ipath;
    ipath.set_path(group);

    // get the rights at this level
    {
        QString const link_start_name("permissions::" + sets.get_action());
        links::link_info info(link_start_name, false, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // an author is attached to this page
            QString const right_key(right_info.key());
            sets.add_plugin_permission(plugin_name, right_key);
        }
    }

    // get all the children and do a recursive call with them all
    {
        QString const children_name("content::children");
        links::link_info info(children_name, false, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // an author is attached to this page
            QString const child_key(right_info.key());
            recursive_add_plugin_permissions(plugin_name, child_key, sets);
        }
    }
}


/** \brief Register the permissions action.
 *
 * This function registers this plugin as supporting the "makeroot" action.
 * After an installation and a user was created on the website, the server
 * is ready to create a root user. This action is used for that purpose.
 *
 * The backend command line looks something like this:
 *
 * \code
 * snapbackend [--config snapserver.conf] --param ROOT_USER_EMAIL=joe@example.com --action makeroot
 * \endcode
 *
 * If you have problems with it (it doesn't seem to work,) try with --debug
 * and make sure to look in the syslog output.
 *
 * \note
 * This should be a user action, unfortunately that would add a permissions
 * dependency in the users plugin which we cannot have (i.e. permissions
 * need to know about users...)
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void permissions::on_register_backend_action(server::backend_action_map_t& actions)
{
    actions[get_name(SNAP_NAME_PERMISSIONS_MAKE_ROOT)] = this;
}


/** \brief Create a root user.
 *
 * This function marks a user as a root user. The user email address has
 * to be specified on the command line.
 *
 * \note
 * This should be a users plugin callback, but it requires access to the
 * permissions plugin so it has to be here instead.
 *
 * \param[in] action  The action the user wants to execute.
 */
void permissions::on_backend_action(QString const& action)
{
    if(action == get_name(SNAP_NAME_PERMISSIONS_MAKE_ROOT))
    {
        // make specified user root
        QtCassandra::QCassandraTable::pointer_t user_table(users::users::instance()->get_users_table());
        if(!user_table)
        {
            SNAP_LOG_FATAL() << "error: table \"users\" not found.";
            exit(1);
        }
        QString const email(f_snap->get_server_parameter("ROOT_USER_EMAIL"));
        if(!user_table->exists(email))
        {
            SNAP_LOG_FATAL() << "error: user \"" << email.toStdString() << "\" not found.";
            exit(1);
        }
        QtCassandra::QCassandraRow::pointer_t user_row(user_table->row(email));
        if(!user_row->exists(users::get_name(users::SNAP_NAME_USERS_IDENTIFIER)))
        {
            SNAP_LOG_FATAL() << "error: user \"" << email.toStdString() << "\" was not given an identifier.";
            exit(1);
        }
        QtCassandra::QCassandraValue identifier_value(user_row->cell(users::get_name(users::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(identifier_value.nullValue() || identifier_value.size() != 8)
        {
            SNAP_LOG_FATAL() << "error: user \"" << email.toStdString() << "\" identifier could not be read.";
            exit(1);
        }
        int64_t const identifier(identifier_value.int64Value());

        QString const site_key(f_snap->get_site_key_with_slash());
        content::path_info_t user_ipath;
        user_ipath.set_path(QString("%1/%2").arg(users::get_name(users::SNAP_NAME_USERS_PATH)).arg(identifier));
        content::path_info_t dpath;
        dpath.set_path(QString("%1/root").arg(get_name(SNAP_NAME_PERMISSIONS_GROUPS_PATH)));

        QString const link_name(get_name(SNAP_NAME_PERMISSIONS_GROUP));
        bool const source_multi(false);
        links::link_info source(link_name, source_multi, user_ipath.get_key(), user_ipath.get_branch());
        QString const link_to(get_name(SNAP_NAME_PERMISSIONS_GROUP));
        bool const destination_multi(false);
        links::link_info destination(link_to, destination_multi, dpath.get_key(), dpath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
}


/** \brief Signal received when a new user was verified.
 *
 * After a user registers, he receives an email with a magic number that
 * needs to be used for the user to be authorized to access the system as
 * a registered user. This signal is sent once the user email got verified.
 *
 * \param[in,out] ipath  The user path.
 * \param[in] identifier  The user identifier.
 */
void permissions::on_user_verified(content::path_info_t& ipath, int64_t identifier)
{
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t data_table(content_plugin->get_data_table());
    uint64_t const created_date(f_snap->get_start_date());

    // All users are also given a permission group that can be used to
    // give each individual user very specific rights.

    // first we create the user specific right
    content::path_info_t permission_ipath;
    {
        // create a specific permission for that new company
        permission_ipath.set_path(QString("%1/%2").arg(get_name(SNAP_NAME_PERMISSIONS_USERS_PATH)).arg(identifier));
        permission_ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
        permission_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
        permission_ipath.force_locale("xx"); // create the neutral one by default?
        content_plugin->create_content(permission_ipath, get_plugin_name(), "system-page");

        // create a default the title and body
        QtCassandra::QCassandraRow::pointer_t permission_revision_row(data_table->row(permission_ipath.get_revision_key()));
        permission_revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_CREATED))->setValue(created_date);
        // TODO: translate (not too important on this page since it is really not public)
        permission_revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_TITLE))->setValue(QString("User #%1 Private Permission Right").arg(identifier));
        permission_revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_BODY))->setValue(QString("This type represents permissions that are 100% specific to this user."));
    }

    // second we create the user specific group
    content::path_info_t group_ipath;
    {
        // create a specific permission group for that new user
        group_ipath.set_path(QString("%1/users/%2").arg(get_name(SNAP_NAME_PERMISSIONS_GROUPS_PATH)).arg(identifier));
        group_ipath.force_branch(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);
        group_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
        group_ipath.force_locale("xx");
        content_plugin->create_content(group_ipath, get_plugin_name(), "system-page");

        // save the title, description, and link to the type
        QtCassandra::QCassandraRow::pointer_t group_revision_row(data_table->row(group_ipath.get_revision_key()));
        group_revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_CREATED))->setValue(created_date);
        // TODO: translate (not too important on this page since it is really not public)
        group_revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_TITLE))->setValue(QString("User #%1 Private Permission Group").arg(identifier));
        group_revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_BODY))->setValue(QString("This group represents a user private group."));
    }

    // link the permission to the company and the user
    // this user has view and edit rights
    //
    // WARNING: Note that we link the User Page to this new permission, we
    //          are NOT linking the user to the new permission... it can be
    //          very confusing on this one since user ipath looks very much
    //          like the user, doesn't it?
    {
        QString const link_name(get_name(SNAP_NAME_PERMISSIONS_VIEW));
        QString const link_to(get_name(SNAP_NAME_PERMISSIONS_VIEW));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
        links::link_info destination(link_to, destination_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
    {
        QString const link_name(get_name(SNAP_NAME_PERMISSIONS_EDIT));
        QString const link_to(get_name(SNAP_NAME_PERMISSIONS_EDIT));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
        links::link_info destination(link_to, destination_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
    {
        QString const link_name(get_name(SNAP_NAME_PERMISSIONS_ADMINISTER));
        QString const link_to(get_name(SNAP_NAME_PERMISSIONS_ADMINISTER));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
        links::link_info destination(link_to, destination_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // link the user to his private group right
    {
        QString const link_name(get_name(SNAP_NAME_PERMISSIONS_GROUP));
        QString const link_to(get_name(SNAP_NAME_PERMISSIONS_GROUP));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, ipath.get_key(), ipath.get_branch());
        links::link_info destination(link_to, destination_unique, group_ipath.get_key(), group_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // then add permissions for the user to be able to edit his own
    // account information
    //
    // TBD: should we have a way to prevent the user from editing his
    //      information?
    {
        QString const link_name(get_name(SNAP_NAME_PERMISSIONS_VIEW));
        QString const link_to(get_name(SNAP_NAME_PERMISSIONS_VIEW));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, group_ipath.get_key(), group_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
    {
        QString const link_name(get_name(SNAP_NAME_PERMISSIONS_EDIT));
        QString const link_to(get_name(SNAP_NAME_PERMISSIONS_EDIT));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, group_ipath.get_key(), group_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
    {
        QString const link_name(get_name(SNAP_NAME_PERMISSIONS_ADMINISTER));
        QString const link_to(get_name(SNAP_NAME_PERMISSIONS_ADMINISTER));
        bool const source_unique(false);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, permission_ipath.get_key(), permission_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, group_ipath.get_key(), group_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }
}



namespace details
{

void call_perms(snap_expr::variable_t& result, snap_expr::variable_t::variable_vector_t const& sub_results)
{
    if(sub_results.size() != 3)
    {
        throw snap_expr::snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call perms() expected exactly 3");
    }
    QString path(sub_results[0].get_string("perms(1)"));
    QString user_path(sub_results[1].get_string("perms(2)"));
    QString action(sub_results[2].get_string("perms(3)"));

    // setup the parameters to the access_allowed() signal
    content::path_info_t ipath;
    ipath.set_path(path);
    ipath.set_parameter("action", action);
    quiet_error_callback err_callback(content::content::instance()->get_snap(), false);
    path::path::instance()->validate_action(ipath, action, err_callback);
    char const *login_status(get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED));

    // check whether that user is allowed that action with that path
    content::permission_flag allowed;
    path::path::instance()->access_allowed(user_path, ipath, action, login_status, allowed);

    // save the result
    QtCassandra::QCassandraValue value;
    value.setBoolValue(allowed.allowed());
    result.set_value(snap_expr::variable_t::EXPR_VARIABLE_TYPE_BOOL, value);
}


snap_expr::functions_t::function_call_table_t const permissions_functions[] =
{
    { // check whether a user has permissions to access a page
        "perms",
        call_perms
    },
    {
        nullptr,
        nullptr
    }
};


} // namespace details


void permissions::on_add_snap_expr_functions(snap_expr::functions_t& functions)
{
    functions.add_functions(details::permissions_functions);
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
