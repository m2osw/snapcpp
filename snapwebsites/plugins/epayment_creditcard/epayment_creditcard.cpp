// Snap Websites Server -- handle credit card data for other plugins
// Copyright (C) 2014-2016  Made to Order Software Corp.
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

#include "epayment_creditcard.h"

#include "../editor/editor.h"

#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "qdomhelpers.h"
#include "qdomxpath.h"

#include "poison.h"


SNAP_PLUGIN_START(epayment_creditcard, 1, 0)



/* \brief Get a fixed path name.
 *
 * The path plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_DEFAULT_COUNTRY:
        return "epayment::default_country";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SETTINGS_PATH:
        return "admin/settings/epayment/creditcard";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_ADDRESS2:
        return "epayment::show_address2";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_COUNTRY:
        return "epayment::show_country";

    case name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_PROVINCE:
        return "epayment::show_province";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_EPAYMENT_CREDITCARD_...");

    }
    NOTREACHED();
}


/** \brief Initialize the epayment_creditcard plugin.
 *
 * This function is used to initialize the epayment_creditcard plugin object.
 */
epayment_creditcard::epayment_creditcard()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the epayment_creditcard plugin.
 *
 * Ensure the epayment_creditcard object is clean before it is gone.
 */
epayment_creditcard::~epayment_creditcard()
{
}


/** \brief Get a pointer to the epayment_creditcard plugin.
 *
 * This function returns an instance pointer to the epayment_creditcard plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the epayment_creditcard plugin.
 */
epayment_creditcard * epayment_creditcard::instance()
{
    return g_plugin_epayment_creditcard_factory.instance();
}


/** \brief Send users to the plugin settings.
 *
 * This path represents this plugin settings.
 */
QString epayment_creditcard::settings_path() const
{
    return "/admin/settings/epayment/creditcard";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString epayment_creditcard::icon() const
{
    return "/images/epayment/epayment-credit-card-logo-64x64.png";
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
QString epayment_creditcard::description() const
{
    return "Generate a credit card form that the end user is expected to"
          " fill in. This plugin is generally not installed by itself,"
          " instead it is marked as a dependency of a plugin that is"
          " capable of processing credit cards.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString epayment_creditcard::dependencies() const
{
    return "|date_widgets|editor|epayment|messages|path|permissions|users|";
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
int64_t epayment_creditcard::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 3, 30, 21, 30, 16, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update (in
 *                                 micro-seconds).
 */
void epayment_creditcard::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the epayment_creditcard.
 *
 * This function terminates the initialization of the epayment_creditcard plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void epayment_creditcard::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(epayment_creditcard, "server", server, process_post, _1);
    SNAP_LISTEN(epayment_creditcard, "editor", editor::editor, dynamic_editor_widget, _1, _2, _3);
}


/** \brief Accept a POST to request information about the server.
 *
 * This function manages the data sent to the server by a client script.
 * In many cases, it is used to know whether something is true or false,
 * although the answer may be any valid text.
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
void epayment_creditcard::on_process_post(QString const & uri_path)
{
    NOTUSED(uri_path);
}




void epayment_creditcard::on_dynamic_editor_widget(
        content::path_info_t & ipath,
        QString const & name,
        QDomDocument & editor_widgets)
{
    NOTUSED(ipath);
    NOTUSED(name);

    // are we dealing with the epayment credit card form?
    //
    QDomElement root(editor_widgets.documentElement());
    if(root.isNull())
    {
        return;
    }
    QString const owner_name(root.attribute("owner"));
    if(owner_name != "epayment_creditcard")
    {
        return;
    }
    QString const form_id(root.attribute("id"));
    if(form_id != "creditcard_form")
    {
        return;
    }

    // read the settings
    //
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    content::path_info_t epayment_creditcard_settings_ipath;
    epayment_creditcard_settings_ipath.set_path(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SETTINGS_PATH));
    if(!content_table->exists(epayment_creditcard_settings_ipath.get_key())
    || !content_table->row(epayment_creditcard_settings_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
    {
        // the form by default is what we want if no settings were defined
        return;
    }
    QtCassandra::QCassandraRow::pointer_t settings_row(revision_table->row(epayment_creditcard_settings_ipath.get_revision_key()));

    // remove the unwanted widgets if the administrator required so...
    //

    // address2
    //
    {
        bool const show_address2(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_ADDRESS2))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_address2)
        {
            // forget that widget
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='address2']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
        }
    }

    // country
    //
    {
        bool const show_country(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_COUNTRY))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_country)
        {
            // forget that widget
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='country']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
        }
        else
        {
            // setup the default if there is one and we did not remove the
            // widget
            QString const default_country(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_DEFAULT_COUNTRY))->value().stringValue());
            if(!default_country.isEmpty())
            {
                QDomXPath dom_xpath;
                dom_xpath.setXPath("/editor-form/widget[@id='country']");
                QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
                if(result.size() > 0
                && result[0].isElement())
                {
                    QDomElement default_value(editor_widgets.createElement("value"));
                    result[0].appendChild(default_value);
                    snap_dom::append_plain_text_to_node(default_value, default_country);
                }
            }
        }
    }

    // province
    //
    {
        bool const show_province(settings_row->cell(get_name(name_t::SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_PROVINCE))->value().safeSignedCharValue(0, 1) != 0);
        if(!show_province)
        {
            // forget that widget
            QDomXPath dom_xpath;
            dom_xpath.setXPath("/editor-form/widget[@id='province']");
            QDomXPath::node_vector_t result(dom_xpath.apply(editor_widgets));
            if(result.size() > 0
            && result[0].isElement())
            {
                result[0].parentNode().removeChild(result[0]);
            }
        }
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
