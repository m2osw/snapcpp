// Snap Websites Server -- JavaScript WYSIWYG editor
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

#include "editor.h"

#include "../output/output.h"
#include "../messages/messages.h"
#include "../permissions/permissions.h"
#include "../sessions/sessions.h"

#include "not_reached.h"
#include "log.h"

#include <QtCassandra/QCassandraLock.h>

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(editor, 1, 0)


/** \brief Get a fixed editor plugin name.
 *
 * The editor plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name) {
    case SNAP_NAME_EDITOR_DRAFTS_PATH:
        return "admin/drafts";

    case SNAP_NAME_EDITOR_PAGE_TYPE:
        return "editor::page_type";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_EDITOR_...");

    }
    NOTREACHED();
}




/** \brief Initialize the editor plugin.
 *
 * This function is used to initialize the editor plugin object.
 */
editor::editor()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Clean up the editor plugin.
 *
 * Ensure the editor object is clean before it is gone.
 */
editor::~editor()
{
}


/** \brief Initialize editor.
 *
 * This function terminates the initialization of the editor plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void editor::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(editor, "server", server, process_post, _1);
    SNAP_LISTEN(editor, "layout", layout::layout, generate_header_content, _1, _2, _3, _4);
    //SNAP_LISTEN(editor, "layout", layout::layout, generate_page_content, _1, _2, _3, _4);
    SNAP_LISTEN(editor, "form", form::form, validate_post_for_widget, _1, _2, _3, _4, _5, _6);
}


/** \brief Get a pointer to the editor plugin.
 *
 * This function returns an instance pointer to the editor plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the editor plugin.
 */
editor *editor::instance()
{
    return g_plugin_editor_factory.instance();
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
QString editor::description() const
{
    return "Offer a WYSIWYG* editor to people using the website."
        " The editor appears wherever a plugin creates a div tag with"
        " the contenteditable attribute set to true."
        "\n(*) WYSIWYG: What You See Is What You Get.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t editor::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 2, 27, 20, 10, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void editor::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Add editor specific tags to the layout DOM.
 *
 * This function adds different editor specific tags to the layout page
 * and body XML documents.
 *
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The template in case path does not exist.
 */
void editor::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    // a regular page
    output::output::instance()->on_generate_main_content(ipath, page, body, ctemplate);
}


/** \brief Setup page for the editor.
 *
 * The editor has a set of dynamic parameters that the users are offered
 * to setup. These parameters need to be sent to the user and we use this
 * function for that purpose.
 *
 * \todo
 * Look for a way to generate the editor data only if necessary (too
 * complex for now.)
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 * \param[in] ctemplate  The template in case path does not exist.
 */
void editor::on_generate_header_content(content::path_info_t& ipath, QDomElement& header, QDomElement& metadata, QString const& ctemplate)
{
    static_cast<void>(metadata);
    static_cast<void>(ctemplate);

    QDomDocument doc(header.ownerDocument());

    {
        QDomElement editor_tag(doc.createElement("editor"));
        metadata.appendChild(editor_tag);

        // define a set of dynamic parameters as defined by the user
        // /snap/head/metadata/session/<session-id>
        {
            sessions::sessions::session_info info;
            info.set_session_type(sessions::sessions::session_info::SESSION_INFO_FORM);
            info.set_session_id(EDITOR_SESSION_ID_EDIT);
            info.set_plugin_owner(get_plugin_name()); // ourselves
            info.set_page_path(ipath.get_key());
            //info.set_object_path();
            info.set_user_agent(f_snap->snapenv(snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT)));
            info.set_time_to_live(86400);  // 24 hours
            QString const session(sessions::sessions::instance()->create_session(info));
            int32_t random(info.get_session_random());

            QDomElement session_tag(doc.createElement("session"));
            editor_tag.appendChild(session_tag);
            QDomText session_text(doc.createTextNode(QString("%1/%2").arg(session).arg(random)));
            session_tag.appendChild(session_text);
        }
    }

    // TODO find a way to include the editor only if required
    content::content::instance()->add_javascript(ipath, header.ownerDocument(), "editor");
}


/* \brief Check whether \p cpath matches our introducers.
 *
 * This function checks that cpath matches our introducer and if
 * so we tell the path plugin that we're taking control to
 * manage this path.
 *
 * We understand "user" as in list of users.
 *
 * We understand "user/<name>" as in display that user information
 * (this may be turned off on a per user or for the entire website.)
 * Websites that only use an email address for the user identification
 * do not present these pages publicly.
 *
 * We understand "profile" which displays the current user profile
 * information in detail and allow for editing of what can be changed.
 *
 * We understand "login" which displays a form for the user to log in.
 *
 * We understand "verify-credentials" which is very similar to "login"
 * albeit simpler and only appears if the user is currently logged in
 * but not recently logged in (i.e. administration rights.)
 *
 * We understand "logout" to allow users to log out of Snap! C++.
 *
 * We understand "register" to display a registration form to users.
 *
 * We understand "verify" to check a session that is being returned
 * as the user clicks on the link we sent on registration.
 *
 * We understand "forgot-password" to let users request a password reset
 * via a simple form.
 *
 * \todo
 * If we cannot find a global way to check the Origin HTTP header
 * sent by the user agent, we probably want to check it here in
 * pages where the referrer should not be a "weird" 3rd party
 * website.
 *
 * \param[in,out] ipath  The path being handled dynamically.
 * \param[in,out] plugin_info  If you understand that cpath, set yourself here.
 */
//void editor::on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info)
//{
//    if(ipath.get_cpath() == "admin/drafts/new")
//    {
//        // tell the path plugin that this is ours
//        plugin_info.set_plugin(this);
//    }
//}


/** \brief Execute the specified path.
 *
 * This is a dynamic page which the users plugin knows how to handle.
 *
 * This function never returns if the "page" is just a verification
 * process which redirects the user (i.e. "verify/<id>", and
 * "new-password/<id>" at this time.)
 *
 * Other paths may also redirect the user in case the path is not
 * currently supported (mainly because the user does not have
 * permission.)
 *
 * \param[in,out] ipath  The canonicalized path.
 *
 * \return true if the processing worked as expected, false if the page
 *         cannot be created ("Page Not Present" results on false)
 */
bool editor::on_path_execute(content::path_info_t& ipath)
{
    // the editor forms are generated using token replacements
    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


void editor::on_validate_post_for_widget(content::path_info_t& ipath, sessions::sessions::session_info& info,
                                         QDomElement const& widget, QString const& widget_name,
                                         QString const& widget_type, bool is_secret)
{
    static_cast<void>(widget);
    static_cast<void>(widget_type);
    static_cast<void>(is_secret);

    messages::messages *messages(messages::messages::instance());

    // we're only interested by our widgets
    QString const cpath(ipath.get_cpath());
    if(cpath == "admin/drafts/new")
    {
        // verify the type of the new page
        if(widget_name == "type")
        {
            // get the value
            QString const type(f_snap->postenv(widget_name));

            QtCassandra::QCassandraTable::pointer_t content_table(content::content::instance()->get_content_table());
            QString const site_key(f_snap->get_site_key_with_slash());
            QString const type_key(site_key + "types/taxonomy/system/content-types/" + type);
            if(!content_table->exists(type_key))
            {
                // TODO: test whether the user could create a new type, if so
                //       then do not err at all here
                messages->set_error(
                    "Unknown Type",
                    QString("Type \"%1\" is not yet defined and you do not have permission to create a new type of pages at this point.").arg(type),
                    "type doesn't exist and we do not yet offer a way to auto-create a content type",
                    false
                );
                info.set_session_type(sessions::sessions::session_info::SESSION_INFO_INCOMPATIBLE);
            }
        }
    }
}


/** \brief Process a post from one of the editor forms.
 *
 * This function processes the post of an editor form. The function uses the
 * \p ipath parameter in order to determine which form is being processed.
 *
 * \param[in,out] ipath  The path the user is accessing now.
 * \param[in] session_info  The user session being processed.
 */
void editor::on_process_form_post(content::path_info_t& ipath, sessions::sessions::session_info const& session_info)
{
    static_cast<void>(session_info);

    QString const cpath(ipath.get_cpath());
    if(cpath == "admin/drafts/new")
    {
        process_new_draft();
    }
    else
    {
        // this should not happen because invalid paths will not pass the
        // session validation process
        throw editor_exception_invalid_path("users::on_process_form_post() was called with an unsupported path: \"" + ipath.get_key() + "\"");
    }
}


/** \brief Finish the processing of a new draft.
 *
 * This function ends the processing of a new draft by saving the information
 * the user entered in the new draft form. This function creates a draft
 * under the admin/draft path under the user publishes the page. This allows
 * for the path of the new page to be better defined than if we were creating
 * the page at once.
 *
 * The path used under admin/draft simply makes use of the Unix time value.
 * If two or more users create a new draft simultaneously (within the same
 * second) then an additional .1 to .99 is added to the path. If more than
 * 100 users create a page simultaneously, the 101 and further fail saving
 * the new draft and will have to test again later.
 */
void editor::process_new_draft()
{
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());

    // get the 3 parameters entered by the user to get the new page started
    QString const type(f_snap->postenv("type"));
    QString const sibling(f_snap->postenv("sibling"));
    QString const title(f_snap->postenv("title"));
    QString const page_description(f_snap->postenv("description"));

    // TODO: test that "type" exists and if not creating it (if the user
    //       has engouh rights); we already checked whether the type
    //       existed and the user had engouh rights, but we want to test
    //       again; that being said, until we support creating new types
    //       we don't have to do anything here

    // now create the new page as a pure draft (opposed to an unpublised set
    // of changes on a page which is also called a draft, but is directly
    // linked to that one page.)
    time_t const start_time(f_snap->get_start_time());
    int64_t const start_date(f_snap->get_start_date());
    char const *drafts_path(get_name(SNAP_NAME_EDITOR_DRAFTS_PATH));
    QString const site_key(f_snap->get_site_key_with_slash());
    QString new_draft_key(QString("%1%2/%3").arg(site_key).arg(drafts_path).arg(start_time));

    // we got as much as we could ready before locking
    {
        // make sure this draft key is unique
        // lock the parent briefly
        QtCassandra::QCassandraLock lock(f_snap->get_context(), QByteArray(drafts_path));
        for(int extra(1); content_table->exists(new_draft_key); ++extra)
        {
            // TBD: Could it really ever happen that a website would have over
            //      100 people (i.e. not robots) create a page all at once?
            //      Should we offer to make this number a variable that
            //      administrators could bump up to be "safe"?
            if(extra >= 100) // 100 excluded since we start with zero (.0 is not included in the very first name)
            {
                // TODO: this error needs to be reported to the administrator(s)
                //       (especially if it happens often because that means
                //       robots are working on the website!)
                f_snap->die(snap_child::HTTP_CODE_CONFLICT,
                    "Conflict Error", "We could not create a new draft entry for you. Too many other drafts existed already. Please try again later.",
                    "Somehow the server was not able to generated another draft entry.");
                NOTREACHED();
            }
            new_draft_key = QString("%1%2/%3.%4").arg(site_key).arg(drafts_path).arg(start_time).arg(extra);
        }
        // create that row so the next user will detect it as existing
        // and we can then unlock the parent row
        content_table->row(new_draft_key)->cell(content::get_name(content::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    }

    // before we go further officially create the content
    // TODO: fix the locale; it should come from the favorite locale of that
    //       user and we should offer the user to select another locale if
    //       he/she has more than one in his account
    QString const locale("xx");
    QString const owner(output::output::instance()->get_plugin_name());
    content::path_info_t draft_ipath;
    draft_ipath.set_path(new_draft_key);
    draft_ipath.force_branch(content_plugin->get_current_user_branch(new_draft_key, owner, locale, true));
    draft_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
    draft_ipath.force_locale(locale);
    content_plugin->create_content(draft_ipath, owner, "page/draft");

    // save the title, description, and link to the type as a "draft type"
    QtCassandra::QCassandraTable::pointer_t data_table(content_plugin->get_data_table());
    QtCassandra::QCassandraRow::pointer_t revision_row(data_table->row(draft_ipath.get_revision_key()));
    revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_TITLE))->setValue(title);
    revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_DESCRIPTION))->setValue(page_description);
    revision_row->cell(content::get_name(content::SNAP_NAME_CONTENT_BODY))->setValue(QString("enter page content here ([year])"));

    // link to the type, but not as the official type yet since this page
    // has to have a "draft page" type for a while
    {
        QString const link_name(get_name(SNAP_NAME_EDITOR_PAGE_TYPE));
        bool const source_unique(true);
        QString const link_to(get_name(SNAP_NAME_EDITOR_PAGE_TYPE));
        bool const destination_unique(false);
        content::path_info_t type_ipath;
        QString const type_key(site_key + "types/taxonomy/system/content-types/" + type);
        type_ipath.set_path(type_key);
        links::link_info source(link_name, source_unique, draft_ipath.get_key(), draft_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, type_ipath.get_key(), type_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // give edit permission of the draft
    // <link name="permissions::view" to="permissions::view" mode="*:*">/types/permissions/rights/view/page/for-spammers</link>
    {
        QString const link_name(permissions::get_name(permissions::SNAP_NAME_PERMISSIONS_EDIT));
        bool const source_unique(false);
        QString const link_to(permissions::get_name(permissions::SNAP_NAME_PERMISSIONS_EDIT));
        bool const destination_unique(false);
        content::path_info_t type_ipath;
        // TBD -- should this includes the type of page?
        QString const type_key(site_key + "types/permissions/rights/edit/page");
        type_ipath.set_path(type_key);
        links::link_info source(link_name, source_unique, draft_ipath.get_key(), draft_ipath.get_branch());
        links::link_info destination(link_to, destination_unique, type_ipath.get_key(), type_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // redirect the user to the new page so he can edit it
    QString const qs_action(f_snap->get_server_parameter("qs_action"));
    f_snap->page_redirect(QString("%1?%2=edit").arg(draft_ipath.get_key()).arg(qs_action), snap_child::HTTP_CODE_FOUND,
            "Page was created successfully",
            "Sending you to your new page so that way you can edit it and ultimately publish it.");
    NOTREACHED();
}


/** \brief Check the URL and process the POST data accordingly.
 *
 * This function manages the data sent back by the editor.js script
 * and save the new values as reuired.
 *
 * The function verifies that the "editor_session" variable is set, if
 * not it ignores the POST sine another plugin may be the owner.
 *
 * \note
 * This function is a server signal generated by the snap_child
 * execute() function.
 *
 * \param[in] uri_path  The path received from the HTTP server.
 */
void editor::on_process_post(QString const& uri_path)
{
    QString const editor_full_session(f_snap->postenv("editor_session"));
    if(editor_full_session.isEmpty())
    {
        // if the editor_session variable does not exist, do not consider this
        // POST as an Editor POST
        return;
    }
    save_mode_t editor_save_mode(string_to_save_mode(f_snap->postenv("editor_save_mode")));
    if(editor_save_mode == EDITOR_SAVE_MODE_UNKNOWN)
    {
        // this could happen between versions (i.e. newer version wants to
        // use a new mode which we did not yet implement in the
        // string_to_save_mode() function.) -- it could be a problem between
        // a server that has a newer version and a server that does not...
        f_snap->die(snap_child::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                "Somehow the editor does not understand the Save command sent to the server.",
                QString("User gave us an unknown save mode (%1).").arg(f_snap->postenv("editor_save_mode")));
        NOTREACHED();
    }

    // [0] -- session Id, [1] -- random number
    QStringList const session_data(editor_full_session.split("/"));

    messages::messages *messages(messages::messages::instance());

    content::path_info_t ipath;
    ipath.set_path(uri_path);
    ipath.set_main_page(true);
    ipath.force_locale("xx");

    // First we verify the session information from the meta tag:
    // <meta name="editor_session" content="session_id/random_number"/>
    sessions::sessions::session_info info;
    sessions::sessions::instance()->load_session(session_data[0], info, false);
    switch(info.get_session_type())
    {
    case sessions::sessions::session_info::SESSION_INFO_VALID:
        // unless we get this value we've got a problem with the session itself
        break;

    case sessions::sessions::session_info::SESSION_INFO_MISSING:
        f_snap->die(snap_child::HTTP_CODE_GONE, "Editor Session Gone", "It looks like you attempted to submit editor content without first loading it.", "User sent editor content with a session identifier that is not available.");
        NOTREACHED();
        return;

    case sessions::sessions::session_info::SESSION_INFO_OUT_OF_DATE:
        messages->set_http_error(snap_child::HTTP_CODE_GONE, "Editor Timeout", "Sorry! You sent this request back to Snap! way too late. It timed out. Please re-enter your information and re-submit.", "User did not click the submit button soon enough, the server session timed out.", true);
        return;

    case sessions::sessions::session_info::SESSION_INFO_USED_UP:
        // this should not happen because we do not mark editor sessions
        // for one time use
        messages->set_http_error(snap_child::HTTP_CODE_CONFLICT, "Editor Already Submitted", "This editor session was already processed.", "The user submitted the same session more than once.", true);
        return;

    default:
        throw snap_logic_exception("load_session() returned an unexpected SESSION_INFO_... value in editor::on_process_post()");

    }

    // verify that the session random number is compatible
    if(info.get_session_random() != session_data[1].toInt())
    {
        f_snap->die(snap_child::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                "The POST request does not correspond to the session that the editor generated.",
                QString("User POSTed a request with random number %1, but we expected %2.")
                        .arg(info.get_session_random())
                        .arg(session_data[1]));
        NOTREACHED();
    }

    // verify that the path is correct
    if(info.get_page_path() != ipath.get_key()
    || info.get_user_agent() != f_snap->snapenv(snap::get_name(SNAP_NAME_CORE_HTTP_USER_AGENT))
    || info.get_plugin_owner() != get_plugin_name())
    {
        // the path was tempered with? the agent changes between hits?
        f_snap->die(snap_child::HTTP_CODE_NOT_ACCEPTABLE, "Not Acceptable",
                "The POST request does not correspond to the editor it was defined for.",
                QString("User POSTed a request against \"%1\" with an incompatible page path (%2) or a different plugin (%3).")
                        .arg(ipath.get_key())
                        .arg(info.get_page_path())
                        .arg(info.get_plugin_owner()));
        NOTREACHED();
    }

    // editing a draft?
    if(ipath.get_cpath().startsWith("admin/drafts/"))
    {
        // adjust the mode for drafts are "special" content
        switch(editor_save_mode)
        {
        case EDITOR_SAVE_MODE_DRAFT:
            editor_save_mode = EDITOR_SAVE_MODE_SAVE;
        case EDITOR_SAVE_MODE_SAVE:
            break;

        case EDITOR_SAVE_MODE_PUBLISH:
            editor_save_mode = EDITOR_SAVE_MODE_NEW_BRANCH;
        case EDITOR_SAVE_MODE_NEW_BRANCH: // should not be accessible
            break;

        case EDITOR_SAVE_MODE_AUTO_DRAFT: // TBD
            break;

        case EDITOR_SAVE_MODE_UNKNOWN:
            // this should never happen
            throw snap_logic_exception("The UNKNOWN save mode was ignore, yet we have an edit_save_mode set to UNKNOWN.");

        }
    }

    // act on the data as per the user's specified mode
    switch(editor_save_mode)
    {
    case EDITOR_SAVE_MODE_DRAFT:
        break;

    case EDITOR_SAVE_MODE_NEW_BRANCH:
        break;

    case EDITOR_SAVE_MODE_SAVE:
        editor_save(ipath);
        break;

    case EDITOR_SAVE_MODE_PUBLISH:
        break;

    case EDITOR_SAVE_MODE_AUTO_DRAFT:
        break;

    case EDITOR_SAVE_MODE_UNKNOWN:
        // this should never happen
        throw snap_logic_exception("The UNKNOWN save mode was ignore, yet we have an edit_save_mode set to UNKNOWN.");

    }
}


/** \brief Transform the editor save mode to a number.
 *
 * This function transforms \p mode into a number representing the
 * save mode used with a POST.
 *
 * If the mode is not known, then EDITOR_SAVE_MODE_UNKNOWN is returned.
 * If your function cannot manage any mode, it should die() with a
 * corresponding error.
 *
 * \param[in] mode  The mode to be transformed.
 *
 * \return One of the EDITOR_SAVE_MODE_...
 */
editor::save_mode_t editor::string_to_save_mode(QString const& mode)
{
    if(mode == "draft")
    {
        return EDITOR_SAVE_MODE_DRAFT;
    }
    if(mode == "publish")
    {
        return EDITOR_SAVE_MODE_PUBLISH;
    }
    if(mode == "save")
    {
        return EDITOR_SAVE_MODE_SAVE;
    }
    if(mode == "new_branch")
    {
        return EDITOR_SAVE_MODE_NEW_BRANCH;
    }
    if(mode == "auto_draft")
    {
        return EDITOR_SAVE_MODE_AUTO_DRAFT;
    }

    return EDITOR_SAVE_MODE_UNKNOWN;
}


/** \brief Save the fields in a new revision.
 *
 * This function ensures that the current revision is copied in a new
 * revision and overwritten with the new data that the editor just
 * received (i.e. the user may just have changed his page title.)
 *
 * \param[in,out] ipath  The path to the page being updated.
 */
void editor::editor_save(content::path_info_t& ipath)
{
    bool switch_branch(false);
    snap_version::version_number_t branch_number(ipath.get_branch());
    if(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH == branch_number)
    {
        // force a user branch if that page still uses a system branch!
        branch_number = static_cast<int>(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH);  // FIXME cast
        switch_branch = true;
    }
    QString const key(ipath.get_key());
    QString const locale(ipath.get_locale());
    content::content *content_plugin(content::content::instance());
    QString const owner(output::output::instance()->get_plugin_name());

    // create the new revision and make it current
    //
    // TODO: if multiple users approval is required, we cannot make this
    //       new revision the current revision except if that's the very
    //       first (although the very first is not created here)
    //
    snap_version::version_number_t revision_number(content_plugin->get_new_revision(key, owner, branch_number, locale, true));

    // make this newer revision the current one
    if(switch_branch)
    {
        // working branch cannot really stay as the system branch
        // so force both branches in this case
        content_plugin->set_branch(key, owner, branch_number, false);
        content_plugin->set_branch(key, owner, branch_number, true);
        content_plugin->set_branch_key(key, owner, branch_number, true);
        content_plugin->set_branch_key(key, owner, branch_number, false);

        // in that case we also need to save the new revision accordingly
        content_plugin->set_current_revision(key, owner, branch_number, revision_number, locale, false);
        content_plugin->set_revision_key(key, owner, branch_number, revision_number, locale, false);
    }
    content_plugin->set_current_revision(key, owner, branch_number, revision_number, locale, true);
    content_plugin->set_revision_key(key, owner, branch_number, revision_number, locale, true);

    // now save the new data
    ipath.force_revision(revision_number);
    QString const revision_key(ipath.get_revision_key());
    QtCassandra::QCassandraTable::pointer_t data_table(content_plugin->get_data_table());
    QtCassandra::QCassandraRow::pointer_t row(data_table->row(revision_key));
    save_editor_fields(ipath, row);

    // save the modification date in the branch
    content_plugin->modified_content(ipath);
}


/** \brief Save fields that the editor and other plugins manage.
 *
 * This signal can be overridden by other plugins to save the fields that
 * they add to the editor manager.
 *
 * The row parameter passed down to this function is the revision row in
 * the data table. If you need to save data in another location (i.e. the
 * branch or even in the content table) then you want to look into generating
 * a key for that content and get the corresponding row. In most cases, though
 * saving your data in the revision row is the way to go.
 *
 * Note that the ipath parameter has its revision number set to the new
 * revision number that was allocated to save this data.
 *
 * \param[in,out] ipath  The ipath to the page being modified.
 * \param[in,out] row  The row where all the fields are to be saved.
 */
bool editor::save_editor_fields_impl(content::path_info_t& ipath, QtCassandra::QCassandraRow::pointer_t row)
{
    static_cast<void>(ipath);

    if(f_snap->postenv_exists("title"))
    {
        QString const title(f_snap->postenv("title"));
        // TODO: XSS filter title
        row->cell(content::get_name(content::SNAP_NAME_CONTENT_TITLE))->setValue(title);
    }
    if(f_snap->postenv_exists("body"))
    {
        QString const body(f_snap->postenv("body"));
        // TODO: XSS filter body
        row->cell(content::get_name(content::SNAP_NAME_CONTENT_BODY))->setValue(body);
    }

    return true;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
