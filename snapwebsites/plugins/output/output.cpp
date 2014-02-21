// Snap Websites Server -- handle the basic display of the website content
// Copyright (C) 2011-2014  Made to Order Software Corp.
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

#include "output.h"

#include "../messages/messages.h"

#include "not_reached.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(output, 1, 0)

/** \brief Get a fixed output name.
 *
 * The output plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
//char const *get_name(name_t name)
//{
//    switch(name)
//    {
//    case SNAP_NAME_OUTPUT_ACCEPTED:
//        return "output::accepted";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid SNAP_NAME_OUTPUT_...");
//
//    }
//    NOTREACHED();
//}









/** \brief Initialize the output plugin.
 *
 * This function is used to initialize the output plugin object.
 */
output::output()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the output plugin.
 *
 * Ensure the output object is clean before it is gone.
 */
output::~output()
{
}


/** \brief Initialize the output.
 *
 * This function terminates the initialization of the output plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void output::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(output, "layout", layout::layout, generate_page_content, _1, _2, _3, _4);
}


/** \brief Get a pointer to the output plugin.
 *
 * This function returns an instance pointer to the output plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the output plugin.
 */
output *output::instance()
{
    return g_plugin_output_factory.instance();
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
QString output::description() const
{
    return "Output nearly all the content of your website. This plugin handles"
        " the transformation of you pages to HTML, PDF, text, etc.";
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
int64_t output::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 2, 20, 12, 58, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void output::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Execute a page: generate the complete output of that page.
 *
 * This function displays the page that the user is trying to view. It is
 * supposed that the page permissions were already checked and thus that
 * its contents can be displayed to the current user.
 *
 * Note that the path was canonicalized by the path plugin and thus it does
 * not require any further corrections.
 *
 * \param[in,out] ipath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool output::on_path_execute(content::path_info_t& ipath)
{
    // TODO: we probably do not want to check for attachments to send if the
    //       action is not "view"...

    // on entry ipath is defined with the "output" as the owner
    // we want to test the same with "attachment" as the owner
    content::path_info_t attachment_ipath;
    attachment_ipath.set_path(ipath.get_cpath());
    //attachment_ipath.set_owner(content::get_name(content::SNAP_NAME_CONTENT_ATTACHMENT_OWNER));
    QtCassandra::QCassandraTable::pointer_t data_table(content::content::instance()->get_data_table());
    if(data_table->exists(ipath.get_revision_key())
    && data_table->row(ipath.get_revision_key())->exists(content::get_name(content::SNAP_NAME_CONTENT_ATTACHMENT)))
    {
        QtCassandra::QCassandraValue attachment_key(data_table->row(ipath.get_revision_key())->cell(content::get_name(content::SNAP_NAME_CONTENT_ATTACHMENT))->value());
        if(!attachment_key.nullValue())
        {
            QtCassandra::QCassandraTable::pointer_t files_table(content::content::instance()->get_files_table());
            if(!files_table->exists(attachment_key.binaryValue())
            || !files_table->row(attachment_key.binaryValue())->exists(content::get_name(content::SNAP_NAME_CONTENT_FILES_DATA)))
            {
                // somehow the file data is not available
                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "Attachment Not Found",
                        "The attachment \"" + ipath.get_key() + "\" was not found.",
                        QString("Could not find field \"%1\" of file \"%2\".")
                                .arg(content::get_name(content::SNAP_NAME_CONTENT_FILES_DATA))
                                .arg(QString::fromAscii(attachment_key.binaryValue().toHex())));
                NOTREACHED();
            }

            QtCassandra::QCassandraRow::pointer_t file_row(files_table->row(attachment_key.binaryValue()));

            //int pos(cpath.lastIndexOf('/'));
            //QString basename(cpath.mid(pos + 1));
            //f_snap->set_header("Content-Disposition", "attachment; filename=" + basename);

            //f_snap->set_header("Content-Transfer-Encoding", "binary");

            // this is an attachment, output it as such
            QtCassandra::QCassandraValue attachment_mime_type(file_row->cell(content::get_name(content::SNAP_NAME_CONTENT_FILES_MIME_TYPE))->value());
            f_snap->set_header("Content-Type", attachment_mime_type.stringValue());

            QtCassandra::QCassandraValue data(file_row->cell(content::get_name(content::SNAP_NAME_CONTENT_FILES_DATA))->value());
            f_snap->output(data.binaryValue());
            return true;
        }
    }

    f_snap->output(layout::layout::instance()->apply_layout(ipath, this));

    return true;
}


/** \brief Generate the page main content.
 *
 * This function generates the main output of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the <page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  A fallback path in case ipath is not satisfactory.
 */
void output::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    static_cast<void>(page);

    // if the content is the main page then define the titles and body here
    FIELD_SEARCH
        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
        (content::field_search::COMMAND_ELEMENT, body)
        (content::field_search::COMMAND_PATH_INFO_REVISION, ipath)

        // switch to the current data
        // TODO: we need to know which locale/branch.revision to use
        // content::revision_control::<owner>::current_revision_key::<branch>::<locale>
        //(content::field_search::COMMAND_REVISION_OWNER, get_plugin_name())
        //(content::field_search::COMMAND_REVISION_PATH, static_cast<int64_t>(true))
        //(content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY))
        //(content::field_search::COMMAND_SELF)
        //(content::field_search::COMMAND_TABLE, "data")

        // /snap/page/body/titles
        (content::field_search::COMMAND_CHILD_ELEMENT, "titles")
        // /snap/page/body/titles/title
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_TITLE))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_IF_FOUND, 1)
            (content::field_search::COMMAND_PATH, ctemplate)
            (content::field_search::COMMAND_SELF)
            (content::field_search::COMMAND_PATH_INFO_REVISION, ipath)
        (content::field_search::COMMAND_LABEL, 1)
        (content::field_search::COMMAND_SAVE, "title")
        // /snap/page/body/titles/short-title
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_SHORT_TITLE))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_IF_FOUND, 2)
            (content::field_search::COMMAND_PATH, ctemplate)
            (content::field_search::COMMAND_SELF)
            (content::field_search::COMMAND_PATH_INFO_REVISION, ipath)
        (content::field_search::COMMAND_LABEL, 2)
        (content::field_search::COMMAND_SAVE, "short-title")
        // /snap/page/body/titles/long-title
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_LONG_TITLE))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_IF_FOUND, 3)
            (content::field_search::COMMAND_PATH, ctemplate)
            (content::field_search::COMMAND_SELF)
            (content::field_search::COMMAND_PATH_INFO_REVISION, ipath)
        (content::field_search::COMMAND_LABEL, 3)
        (content::field_search::COMMAND_SAVE, "long-title")
        (content::field_search::COMMAND_PARENT_ELEMENT)

        // /snap/page/body/content
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_BODY))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_IF_FOUND, 10)
            (content::field_search::COMMAND_PATH, ctemplate)
            (content::field_search::COMMAND_SELF)
            //(content::field_search::COMMAND_PATH_INFO_REVISION, ipath) -- uncomment if we go on
        (content::field_search::COMMAND_LABEL, 10)
        (content::field_search::COMMAND_SAVE_XML, "content")

        // generate!
        ;
}


/** \brief Generate the page common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The body being generated.
 */
void output::on_generate_page_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    static_cast<void>(page);
    static_cast<void>(ctemplate);

    // create information mainly used in the HTML <head> tag
    QString up;
    int const p(ipath.get_cpath().lastIndexOf('/'));
    if(p == -1)
    {
        // in this case it is an equivalent to top
        up = f_snap->get_site_key();
    }
    else
    {
        up = f_snap->get_site_key_with_slash() + ipath.get_cpath().mid(0, p);
    }

    FIELD_SEARCH
        (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
        (content::field_search::COMMAND_ELEMENT, body)
        (content::field_search::COMMAND_PATH_INFO_GLOBAL, ipath)

        // /snap/page/body/page-created
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_CREATED))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "page-created")
        (content::field_search::COMMAND_WARNING, "field missing")

        // /snap/page/body/branch-created
        (content::field_search::COMMAND_PATH_INFO_BRANCH, ipath)
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_CREATED))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "created")
        (content::field_search::COMMAND_WARNING, "field missing")

        // /snap/page/body/updated
        // XXX should it be mandatory or just use "created" as the default?
        // modified in the branch "converted" to updated
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_MODIFIED))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "updated")
        (content::field_search::COMMAND_WARNING, "field missing")

        // /snap/page/body/accepted
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_ACCEPTED))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "accepted")

        // /snap/page/body/submitted
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_SUBMITTED))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "submitted")

        // /snap/page/body/since
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_SINCE))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "since")

        // /snap/page/body/until
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_UNTIL))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "until")

        // /snap/page/body/copyrighted
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_COPYRIGHTED))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "copyrighted")

        // /snap/page/body/issued
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_ISSUED))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "issued")

        // /snap/page/body/modified
        // XXX should it be mandatory or just use "created" as the default?
        (content::field_search::COMMAND_PATH_INFO_REVISION, ipath)
        (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_MODIFIED))
        (content::field_search::COMMAND_SELF)
        (content::field_search::COMMAND_SAVE_INT64_DATE, "modified")
        (content::field_search::COMMAND_WARNING, "field missing")

        // test whether we're dealing with the home page, if not add these links:
        // /snap/page/body/navigation/link[@rel="top"][@title="Index"][@href="<site key>"]
        // /snap/page/body/navigation/link[@rel="up"][@title="Up"][@href="<path/..>"]
        (content::field_search::COMMAND_DEFAULT_VALUE_OR_NULL, ipath.get_cpath())
        (content::field_search::COMMAND_IF_NOT_FOUND, 1)
            //(content::field_search::COMMAND_RESET) -- uncomment if we go on with other things
            (content::field_search::COMMAND_CHILD_ELEMENT, "navigation")

            // Index
            (content::field_search::COMMAND_CHILD_ELEMENT, "link")
            (content::field_search::COMMAND_ELEMENT_ATTR, "rel=top")
            (content::field_search::COMMAND_ELEMENT_ATTR, "title=Index") // TODO: translate
            (content::field_search::COMMAND_ELEMENT_ATTR, "href=" + f_snap->get_site_key())
            (content::field_search::COMMAND_PARENT_ELEMENT)

            // Up
            (content::field_search::COMMAND_CHILD_ELEMENT, "link")
            (content::field_search::COMMAND_ELEMENT_ATTR, "rel=up")
            (content::field_search::COMMAND_ELEMENT_ATTR, "title=Up") // TODO: translate
            (content::field_search::COMMAND_ELEMENT_ATTR, "href=" + up)
            //(content::field_search::COMMAND_PARENT_ELEMENT) -- uncomment if we go on with other things

            //(content::field_search::COMMAND_PARENT_ELEMENT) -- uncomment if we go on with other things
        (content::field_search::COMMAND_LABEL, 1)

        // generate!
        ;

//QDomDocument doc(page.ownerDocument());
//printf("content XML [%s]\n", doc.toString().toUtf8().data());

    // go through the list of messages and append them to the body
    //
    // IMPORTANT NOTE: we handle the output of the messages in the output
    //                 plugin because the messages cannot depend on the
    //                 layout plugin (circular dependencies)
    messages::messages *messages_plugin(messages::messages::instance());
    int const max_messages(messages_plugin->get_message_count());
    if(max_messages > 0)
    {
        QDomDocument doc(page.ownerDocument());

        QDomElement messages_tag(doc.createElement("messages"));
        int const errcnt(messages_plugin->get_error_count());
        messages_tag.setAttribute("error-count", QString("%1").arg(errcnt));
        messages_tag.setAttribute("warning-count", QString("%1").arg(messages_plugin->get_warning_count()));
        body.appendChild(messages_tag);

        for(int i(0); i < max_messages; ++i)
        {
            QString type;
            messages::messages::message const& msg(messages_plugin->get_message(i));
            switch(msg.get_type())
            {
            case messages::messages::message::MESSAGE_TYPE_ERROR:
                type = "error";
                break;

            case messages::messages::message::MESSAGE_TYPE_WARNING:
                type = "warning";
                break;

            case messages::messages::message::MESSAGE_TYPE_INFO:
                type = "info";
                break;

            case messages::messages::message::MESSAGE_TYPE_DEBUG:
                type = "debug";
                break;

            // no default, compiler knows if one missing
            }
            {
                // create the message tag with its type
                QDomElement msg_tag(doc.createElement("message"));
                msg_tag.setAttribute("id", QString("messages_message_%1").arg(msg.get_id()));
                msg_tag.setAttribute("type", type);
                messages_tag.appendChild(msg_tag);

                // there is always a title
                {
                    QDomDocument message_doc("snap");
                    message_doc.setContent("<title><span class=\"message-title\">" + msg.get_title() + "</span></title>");
                    QDomNode message_title(doc.importNode(message_doc.documentElement(), true));
                    msg_tag.appendChild(message_title);
                }

                // don't create the body if empty
                if(!msg.get_body().isEmpty())
                {
                    QDomDocument message_doc("snap");
                    message_doc.setContent("<body><span class=\"message-body\">" + msg.get_body() + "</span></body>");
                    QDomNode message_body(doc.importNode(message_doc.documentElement(), true));
                    msg_tag.appendChild(message_body);
                }
            }
        }
        messages_plugin->clear_messages();

        if(errcnt != 0)
        {
            // on errors generate a warning in the header
            f_snap->set_header(messages::get_name(messages::SNAP_NAME_MESSAGES_WARNING_HEADER),
                    QString("This page generated %1 error%2")
                            .arg(errcnt).arg(errcnt == 1 ? "" : "s"));
        }

        content::content::instance()->add_javascript(ipath, page.ownerDocument(), "output");
    }
}


// javascript can depend on content, not the other way around
// so this plugin has to define the default content support


int output::js_property_count() const
{
    return 1;
}


QVariant output::js_property_get(const QString& name) const
{
    if(name == "modified")
    {
        return "content::modified";
    }
    return QVariant();
}


QString output::js_property_name(int index) const
{
    if(index == 0)
    {
        return "modified";
    }
    return "";
}


QVariant output::js_property_get(int index) const
{
    if(index == 0)
    {
        return "content::modified";
    }
    return QVariant();
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
