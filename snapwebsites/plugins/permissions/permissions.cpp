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


SNAP_PLUGIN_START(permissions, 1, 0)


/** \brief Get a fixed permissions plugin name.
 *
 * The permissions plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
    switch(name) {
    case SNAP_NAME_PERMISSIONS_PATH:
        return "/types/permissions";

    case SNAP_NAME_PERMISSIONS_ACTION_PATH:
        return "/types/permissions/actions";

    case SNAP_NAME_PERMISSIONS_GROUPS_PATH:
        return "/types/permissions/groups";

    case SNAP_NAME_PERMISSIONS_RIGHTS_PATH:
        return "/types/permissions/rights";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_PERMISSIONS_...");

    }
    NOTREACHED();
}


/** \brief Initialize a permission sets_t object.
 *
 * A sets_t object includes all the sets linked to path and action.
 * The constructor saves the path and action in the object. These two
 * parameters are read-only parameters.
 *
 * \param[in] path  The path being queried.
 * \param[in] action  The action being used in this query.
 */
permissions::sets_t::sets_t(const QString& user_path, const QString& path, const QString& action)
    : f_user_path(user_path)
    , f_path(path)
    , f_action(action)
    //, f_user_rights() -- auto-init
    //, f_plugin_permissions() -- auto-init
{
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
 * kept.
 *
 * \param[in] right  The right being added.
 */
void permissions::sets_t::add_user_right(const QString& right)
{
    f_user_rights.insert(right);
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
 * This function adds a right for the specific plugin. In most cases this
 * works on a per plugin basis. So a plugin adds its own rights only.
 * However, some plugins can add rights for other plugins (right
 * complements.)
 *
 * The plugin name is used to create a separate set of rights per plugin.
 * The user must have that right for each separate group of plugin
 * permissions to be allowed the action sought.
 *
 * \param[in] plugin  The plugin adding this permission.
 * \param[in] right  The right the plugin offers.
 */
void permissions::sets_t::add_plugin_permission(const QString& plugin, const QString& right)
{
    f_plugin_permissions[plugin].insert(right);
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
    for(req_sets_t::iterator pp(f_plugin_permissions.begin());
            pp != f_plugin_permissions.end();
            ++pp)
    {
        // enough rights with this one?
        if(pp->intersect(f_user_rights).isEmpty())
        {
            // XXX add a log to determine the name of the plugin that
            //     failed the user?
            return false;
        }
    }

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

    SNAP_PLUGIN_UPDATE(2013, 12, 10, 2, 53, 30, content_update);

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
    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Implementation of the get_user_rights signal.
 *
 * This function readies the user rights in the specified \p sets.
 *
 * The plugins that capture this function are expected to add user
 * rights to the sets (with the add_user_right() function.) No other
 * permissions should be modified.
 *
 * Note that only the right that correspond to the specified action are
 * to be added here.
 *
 * \code
 *   sets.add_user_right("/types/permissions/rights/edit");
 * \endcode
 *
 * \param[in] permissions  A pointer to the permissions plugin.
 * \param[in,out] sets  The sets_t object where rights get added.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool permissions::get_user_rights_impl(permissions * /*p*/, sets_t& sets)
{
    return true;
}


/** \brief Implementation of the get_plugin_permissions signal.
 *
 * This function readies the plugin rights in the specified \p sets.
 *
 * The plugins that capture this function are expected to add plugin
 * permissions to the sets (with the add_plugin_permission() function.)
 * No user rights should be modified in this process.
 *
 * Note that for plugins we use the term permissions because the plugin
 * allows that capability, whereas a user has rights. However, in the end,
 * the two terms point to the exact same thing: a string to a right defined
 * in the /types/permissions/actions/\<name>.
 *
 * \code
 *   sets.add_plugin_permission(get_plugin_name(), "/types/permissions/rights/edit");
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
 * \param[in] permissions  A pointer to the permissions plugin.
 * \param[in,out] sets  The sets_t object where rights get added.
 *
 * \return true if the signal has to be sent to other plugins.
 */
bool permissions::get_plugin_permissions_impl(permissions * /*p*/, sets_t& sets)
{
    return true;
}


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
void permissions::on_validate_action(const QString& path, QString& action)
{
    // use the default (i.e. "view") if action is still empty
    if(action.isEmpty())
    {
        action = "view";
    }

    QString user_path(users::users::instance()->get_user_path());
    const bool allowed(access_allowed(user_path, path, action));
    if(!allowed)
    {
        if(users::users::instance()->get_user_key().isEmpty())
        {
            // user is anonymous, there is hope, he may have access once
            // logged in
            messages::messages::instance()->set_error(
                "Access Denied",
                "The page you were trying to access (" + path
                        + ") requires more privileges. If you think you have such, try to log in first.",
                "user trying to \"" + action + "\" on page \"" + path + "\" when not logged in.",
                false
            );
            f_snap->page_redirect("user/password", snap_child::HTTP_CODE_ACCESS_DENIED);
        }
        else
        {
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
 */
bool permissions::access_allowed(const QString& user_path, const QString& path, QString& action)
{
    // check that the action is defined in the database (i.e. valid)
    QSharedPointer<QtCassandra::QCassandraTable> content_table(content::content::instance()->get_content_table());
    QString key(get_name(SNAP_NAME_PERMISSIONS_ACTION_PATH) + ("/" + action));
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
    sets_t sets(user_path, path, action);

    // first we get the user rights for that action because that's a lot
    // smaller and if empty we do not have to get anything else
    // (intersection of an empty set with anything else is the empty set)
    get_user_rights(this, sets);
    if(sets.get_user_rights_count() != 0)
    {
        get_plugin_permissions(this, sets);
        return sets.allowed();
    }

    return false;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
