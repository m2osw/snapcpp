// Snap Websites Server -- manage permissions for users, forms, etc.
// Copyright (C) 2013  Made to Order Software Corp.
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

#include "permissions.h"
#include "plugins.h"
#include "not_reached.h"
#include "../content/content.h"
#include "../users/users.h"
#include "../messages/messages.h"
#include <QtCassandra/QCassandraValue.h>
#include <openssl/rand.h>
#include <iostream>
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

    case SNAP_NAME_PERMISSIONS_VIEW:
        return "permissions::view";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_PERMISSIONS_...");

    }
    NOTREACHED();
}


/** \class permissions::sets_t
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
 * \param[in] path  The path being queried.
 * \param[in] action  The action being used in this query.
 */
permissions::sets_t::sets_t(const QString& user_path, const QString& path, const QString& action, const QString& login_status)
    : f_user_path(user_path)
    , f_path(path)
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
const QString& permissions::sets_t::get_user_path() const
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
const QString& permissions::sets_t::get_path() const
{
    return f_path;
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
printf("  USER RIGHT -> [%s] (ignore, better already there)\n", right.toUtf8().data());
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
printf("  USER RIGHT -> [%s] (shrunk)\n", right.toUtf8().data());
                return;
            }
        }
        else /*if(l == len)*/
        {
            if(f_user_rights[i] == right)
            {
                // that's exactly the same, no need to have it twice
printf("  USER RIGHT -> [%s] (already present)\n", right.toUtf8().data());
                return;
            }
        }
    }

    // this one was not there yet, just append
    f_user_rights.push_back(right);
printf("  USER RIGHT -> [%s] (add)\n", right.toUtf8().data());
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
printf("  PLUGIN [%s] PERMISSION -> [%s] (add, new plugin)\n", plugin.toUtf8().data(), right.toUtf8().data());
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
printf("  PLUGIN [%s] PERMISSION -> [%s] (REMOVING)\n", plugin.toUtf8().data(), set[i].toUtf8().data());
                set.remove(i);
                continue;
            }
        }
        else if(l > len)
        {
            if(set[i].startsWith(right))
            {
                // this new right is harder to get, ignore it
printf("  PLUGIN [%s] PERMISSION -> [%s] (skipped)\n", plugin.toUtf8().data(), right.toUtf8().data());
                return;
            }
        }
        else /*if(l == len)*/
        {
            if(set[i] == right)
            {
                // that's exactly the same, no need to have it twice
printf("  PLUGIN [%s] PERMISSION -> [%s] (already present)\n", plugin.toUtf8().data(), right.toUtf8().data());
                return;
            }
        }
        ++i;
    }

    // this one was not there yet, just append
    set.push_back(right);
printf("  PLUGIN [%s] PERMISSION -> [%s] (add right)\n", plugin.toUtf8().data(), right.toUtf8().data());
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
        // if the plugins added nothing, there are no rights to compare
        // or worst, the user have no rights at all (Should not happen,
        // although someone could add a plugin testing something such as
        // your IP address to strip you off all your rights unless you
        // have an IP address considered "valid")
        return false;
    }

printf("final USER RIGHTS:\n");
for(int i(0); i < f_user_rights.size(); ++i)
{
    printf("  [%s]\n", f_user_rights[i].toUtf8().data());
}
printf("final PLUGINS:\n");
    for(req_sets_t::const_iterator pp(f_plugin_permissions.begin());
            pp != f_plugin_permissions.end();
            ++pp)
{
    printf("  [%s]:\n", pp.key().toUtf8().data());
    for(int j(0); j < pp->size(); ++j)
    {
        printf("    [%s]\n", (*pp)[j].toUtf8().data());
    }
}

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
                if(i->startsWith(f_user_rights[j]))
                {
                    //break 2;
                    goto next_plugin;
                }
            }
        }
        // XXX add a log to determine the name of the plugin that
        //     failed the user?
printf("  failed, no match for [%s]\n", pp.key().toUtf8().data());
        return false;
next_plugin:;
    }

printf("  allowed!!!\n");
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

    SNAP_LISTEN(permissions, "server", server, validate_action, _1, _2);
    SNAP_LISTEN(permissions, "server", server, access_allowed, _1, _2, _3, _4, _5);
    SNAP_LISTEN(permissions, "server", server, register_backend_action, _1);
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

    SNAP_PLUGIN_UPDATE(2013, 12, 25, 11, 19, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the content with our references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void permissions::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml(get_plugin_name());
}
#pragma GCC diagnostic pop


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
    (void)perms;

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
            QString user_key(sets.get_user_path());
//printf("  +-> user key = [%s]\n", user_key.toUtf8().data());
            if(user_key.isEmpty()
            || login_status == get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR))
            {
                // in this case the user is an anonymous user and thus we want to
                // add the anonymous user rights
                perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user/returning-visitor/visitor", sets);
            }
            else
            {
                user_key = f_snap->get_site_key_with_slash() + user_key;

                // add all the groups the user is a member of
                QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
                if(!content_table->exists(user_key))
                {
                    // that user is gone, this will generate a 500 by Apache
                    throw permissions_exception_invalid_path("could not access user \"" + user_key + "\"");
                }

                //QSharedPointer<QtCassandra::QCassandraRow> row(content_table->row(user_key));

                // should this one NOT be offered to returning users?
                sets.add_user_right(user_key);

                if(login_status == get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED))
                {
                    // users who are logged in always have registered-user rights
                    // if nothing else
                    perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user", sets);

                    // add assigned groups
                    {
                        QString const link_start_name(get_name(SNAP_NAME_PERMISSIONS_GROUP));
                        links::link_info info(link_start_name, false, user_key);
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
                        QString const link_start_name(get_name(SNAP_NAME_PERMISSIONS_NAMESPACE) + ("::" + sets.get_action()));
                        links::link_info info(link_start_name, false, user_key);
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
                    // this is a registered user who comes back and is semi-logged
                    // in so we do go give this user full rights to avoid potential
                    // problems; we have to look into a way to offer different
                    // group/rights for such a user...
                    perms->add_user_rights(site_key + "types/permissions/groups/root/administrator/editor/moderator/author/commenter/registered-user/returning-registered-user", sets);

                    // add assigned groups
                    // by groups limited to returning registered users, not the logged in registered user
                    {
                        QString const link_start_name(get_name(SNAP_NAME_PERMISSIONS_GROUP_RETURNING_REGISTERED_USER));
                        links::link_info info(link_start_name, false, user_key);
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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
bool permissions::get_plugin_permissions_impl(permissions *perms, sets_t& sets)
{
    // the user plugin cannot include the permissions (since the
    // permissions includes the user plugin) so we implement this
    // user plugin feature in the permissions
    QString const path(sets.get_path());
    if(path.left(5) == "user/")
    {
        QString const user_id(path.mid(5));
        char const *s;
        for(s = user_id.toUtf8().data(); *s != '\0'; ++s)
        {
            if(*s < '0' || *s > '9')
            {
                // only digits allowed (i.e. user/123)
                break;
            }
        }
        if(*s != '\0')
        {
            QString const site_key(f_snap->get_site_key_with_slash());
            sets.add_plugin_permission(content::content::instance()->get_plugin_name(), site_key + path);
            //"types/permissions/rights/view/page/private"
        }
    }

    // the content plugin cannot include the permissions (since the
    // permissions includes the content plugin) so we implement this
    // content plugin feature in the permissions
    //
    // this very page may be assigned direct permissions
    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString key(site_key + path);
    if(!content_table->exists(key))
    {
        // if that page does not exist, it may be dynamic, try to go up
        // until we have one name in the path then check that the page
        // allows such, if so, we have a chance, otherwise no rights
        // from here... (as an example see /verify in plugins/users/content.xml)
        QStringList parts(path.split('/'));
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
        QSharedPointer<QtCassandra::QCassandraRow> row(content_table->row(key));
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
    }

    QString const link_start_name("permissions::" + sets.get_action());
    {
        // check local links for this action
        links::link_info info(link_start_name, false, key);
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
        links::link_info info(link_name, true, key);
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info content_type_info;
        if(link_ctxt->next_link(content_type_info)) // use if() since it is unique on this end
        {
            QString const content_type_key(content_type_info.key());

            {
                // read from the content type now
                links::link_info perm_info(link_start_name, false, content_type_key);
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
                links::link_info perm_info("permissions::groups", false, content_type_key);
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
#pragma GCC diagnostic pop


/** \brief Generate the actual content of the statistics page.
 *
 * This function generates the contents of the statistics page of the
 * permissions plugin.
 *
 * \param[in] l  The layout used to generate this page.
 * \param[in] path  The path to this page.
 * \param[in] page  The page element being generated.
 * \param[in] body  The body element being generated.
 */
void permissions::on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
    // show the permission pages as information (many of these are read-only)
    content::content::instance()->on_generate_main_content(l, path, page, body, ctemplate);
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
 * \param[in] path  The path the user wants to access.
 * \param[in,out] action  The action to be taken, the function may redefine it.
 */
void permissions::on_validate_action(QString const& path, QString const& action)
{
    if(action.isEmpty())
    {
        f_snap->die(snap_child::HTTP_CODE_ACCESS_DENIED,
                "Access Denied",
                "You are not authorized to access our website.",
                "programmer checking permission access with an empty action on page \"" + path + "\".");
        NOTREACHED();
    }

    users::users *users_plugin(users::users::instance());
    QString user_path(users_plugin->get_user_path());
    if(user_path == "user")
    {
        user_path.clear();
    }
    QString key(path);
    f_snap->canonicalize_path(key); // remove the starting slash
    QString login_status(get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_SPAMMER));
    if(!users_plugin->user_is_a_spammer())
    {
        if(user_path.isEmpty())
        {
            login_status = get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_VISITOR);
            // TODO determine, once possible, whether the user came on the
            //      website before (i.e. returning visitor)
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
    bool const allowed(f_snap->access_allowed(user_path, key, action, login_status));
    if(!allowed)
    {
        if(users_plugin->get_user_key().isEmpty())
        {
            // special case of spammers
            if(users_plugin->user_is_a_spammer())
            {
                // force a redirect on error not from the home page
                if(path != "/")
                {
                    // spammers are expected to have enough rights to access
                    // the home page so we try to redirect them there
                    messages::messages::instance()->set_error(
                        "Access Denied",
                        "The page you were trying to access (" + path + ") requires more privileges.",
                        "spammer trying to \"" + action + "\" on page \"" + path + "\".",
                        false
                    );
                    f_snap->page_redirect("/", snap_child::HTTP_CODE_ACCESS_DENIED);
                }
                else
                {
                    // if user does not even have access to the home page...
                    f_snap->die(snap_child::HTTP_CODE_ACCESS_DENIED,
                            "Access Denied",
                            "You are not authorized to access our website.",
                            "spammer trying to \"" + action + "\" on page \"" + path + "\" with unsufficient rights.");
                }
                NOTREACHED();
            }

            if(path == "/login")
            {
                // An IP, Agent, etc. based test could get us here...
                f_snap->die(snap_child::HTTP_CODE_ACCESS_DENIED,
                        "Access Denied",
                        action != "view"
                            ? "You are not authorized to access the login page with action " + action
                            : "Somehow you are not authorized to access the login page.",
                        "user trying to \"" + action + "\" on page \"" + path + "\" with unsufficient rights.");
                NOTREACHED();
            }

            // user is anonymous, there is hope, he may have access once
            // logged in
            // TODO all redirects need to also include a valid action!
            users_plugin->attach_to_session(get_name(users::SNAP_NAME_USERS_LOGIN_REFERRER), path);
            messages::messages::instance()->set_error(
                "Unauthorized",
                "The page you were trying to access (" + path
                        + ") requires more privileges. If you think you have such, try to log in first.",
                "user trying to \"" + action + "\" on page \"" + path + "\" when not logged in.",
                false
            );
            f_snap->page_redirect("login", snap_child::HTTP_CODE_UNAUTHORIZED);
        }
        else
        {
            if(login_status == get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_RETURNING_REGISTERED))
            {
                bool const allowed_if_logged_in(f_snap->access_allowed(user_path, key, action, get_name(SNAP_NAME_PERMISSIONS_LOGIN_STATUS_REGISTERED)));
                if(allowed_if_logged_in)
                {
                    // ah! the user is not allowed here but he would be if
                    // only he were recently logged in (with the last 3h or
                    // whatever the administrator set that to.)
                    users_plugin->attach_to_session(get_name(users::SNAP_NAME_USERS_LOGIN_REFERRER), path);
                    messages::messages::instance()->set_error(
                        "Unauthorized",
                        "The page you were trying to access (" + path
                                + ") requires you to verify your credentials. Please log in again and the system will send you back there.",
                        "user trying to \"" + action + "\" on page \"" + path + "\" when not recently logged in.",
                        false
                    );
                    f_snap->page_redirect("verify-credentials", snap_child::HTTP_CODE_UNAUTHORIZED);
                    NOTREACHED();
                }
            }
            // user is already logged in; no redirect even once we support
            // the double password feature
            f_snap->die(snap_child::HTTP_CODE_ACCESS_DENIED,
                    "Access Denied",
                    "You are not authorized to apply this action (" + action + ") to this page (" + path + ").",
                    "user trying to \"" + action + "\" on page \"" + path + "\" with unsufficient rights.");
        }
        NOTREACHED();
    }
}


/** \brief Check whether the user has permission to access path.
 *
 * This function checks whether the specified \p user has enough rights,
 * type of which is defined by \p action, to access the specified \p path.
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
 * \param[in] path  The path that the user is trying to access.
 * \param[in] action  The action that the user is trying to perform.
 * \param[in] login_status  The supposed status for that user.
 * \param[in] result  The result of the test.
 */
void permissions::on_access_allowed(QString const& user_path, QString const& path, QString const& action, QString const& login_status, server::permission_flag& result)
{
    // check that the action is defined in the database (i.e. valid)
    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(site_key + get_name(SNAP_NAME_PERMISSIONS_ACTION_PATH) + ("/" + action));
    if(!content_table->exists(key))
    {
        // TODO it is rather easy to arrive here so we need to test whether
        //      the same IP does it over and over again and block them if so
        f_snap->die(snap_child::HTTP_CODE_ACCESS_DENIED,
                "Unknown Action",
                "The action you are trying to performed is not known.",
                "permissions::on_validate_action() was used with action \"" + action + "\".");
        NOTREACHED();
    }

    // setup a sets object which will hold all the user's sets
    sets_t sets(user_path, path, action, login_status);

    // first we get the user rights for that action because that's a lot
    // smaller and if empty we do not have to get anything else
    // (intersection of an empty set with anything else is the empty set)
printf("retrieving USER rights... [%s] [%s] [%s]\n", sets.get_action().toUtf8().data(), sets.get_login_status().toUtf8().data(), path.toUtf8().data());
    get_user_rights(this, sets);
    if(sets.get_user_rights_count() != 0)
    {
        if(sets.is_root())
        {
            return;
        }
printf("retrieving PLUGIN permissions... [%s]\n", sets.get_action().toUtf8().data());
        get_plugin_permissions(this, sets);
printf("now compute the intersection!\n");
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
    if(group.contains("types/permissions/rights"))
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
    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
    if(!content_table->exists(group))
    {
        throw permissions_exception_invalid_group_name("caller is trying to access group \"" + group + "\"");
    }

    QSharedPointer<QtCassandra::QCassandraRow> row(content_table->row(group));

    // get the rights at this level
    {
        QString const link_start_name("permissions::" + sets.get_action());
        links::link_info info(link_start_name, false, group);
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // an author is attached to this page
            QString const right_key(right_info.key());
            sets.add_user_right(right_key);
        }
    }

    // get all the children and do a recursive call with them all
    {
        QString const children_name("content::children");
        links::link_info info(children_name, false, group);
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // an author is attached to this page
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
    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
    if(!content_table->exists(group))
    {
        throw permissions_exception_invalid_group_name("caller is trying to access group \"" + group + "\"");
    }

    QSharedPointer<QtCassandra::QCassandraRow> row(content_table->row(group));

    // get the rights at this level
    {
        QString const link_start_name("permissions::" + sets.get_action());
        links::link_info info(link_start_name, false, group);
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
        links::link_info info(children_name, false, group);
        QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
        links::link_info right_info;
        while(link_ctxt->next_link(right_info))
        {
            // an author is attached to this page
            const QString child_key(right_info.key());
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
        QSharedPointer<QtCassandra::QCassandraTable> user_table(users::users::instance()->get_users_table());
        if(user_table.isNull())
        {
            std::cerr << "error: table \"users\" not found." << std::endl;
            exit(1);
        }
        QString const email(f_snap->get_server_parameter("ROOT_USER_EMAIL"));
        if(!user_table->exists(email))
        {
            std::cerr << "error: user \"" << email.toStdString() << "\" not found." << std::endl;
            exit(1);
        }
        QSharedPointer<QtCassandra::QCassandraRow> user_row(user_table->row(email));
        if(!user_row->exists(users::get_name(users::SNAP_NAME_USERS_IDENTIFIER)))
        {
            std::cerr << "error: user \"" << email.toStdString() << "\" was not given an identifier." << std::endl;
            exit(1);
        }
        QtCassandra::QCassandraValue identifier_value(user_row->cell(users::get_name(users::SNAP_NAME_USERS_IDENTIFIER))->value());
        if(identifier_value.nullValue() || identifier_value.size() != 8)
        {
            std::cerr << "error: user \"" << email.toStdString() << "\" identifier could not be read." << std::endl;
            exit(1);
        }
        int64_t const identifier(identifier_value.int64Value());

        QString const site_key(f_snap->get_site_key_with_slash());
        QString const user_key(site_key + users::get_name(users::SNAP_NAME_USERS_PATH) + QString("/%1").arg(identifier));
        QString const key(site_key + get_name(SNAP_NAME_PERMISSIONS_GROUPS_PATH) + "/root");

        QString const link_name(get_name(SNAP_NAME_PERMISSIONS_GROUP));
        bool const source_multi(false);
        links::link_info source(link_name, source_multi, user_key);
        QString const link_to(get_name(SNAP_NAME_PERMISSIONS_GROUP));
        bool const destination_multi(false);
        links::link_info destination(link_to, destination_multi, key);
        links::links::instance()->create_link(source, destination);
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
