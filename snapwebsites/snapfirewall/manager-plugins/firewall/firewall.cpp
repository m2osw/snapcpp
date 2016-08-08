// Snap Websites Server -- manage the snapfirewall settings
// Copyright (C) 2016  Made to Order Software Corp.
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

// firewall
//
#include "firewall.h"

// our lib
//
#include "snapmanager/form.h"

// snapwebsites lib
//
#include "join_strings.h"
#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "qdomhelpers.h"
#include "qdomxpath.h"
#include "string_pathinfo.h"
#include "tokenize_string.h"

// Qt lib
//
#include <QFile>

// C lib
//
#include <sys/file.h>

// last entry
//
#include "poison.h"


SNAP_PLUGIN_START(firewall, 1, 0)


namespace
{

// TODO: offer the user a way to change this path?
char const * g_service_filename = "/etc/snapwebsites/services.d/service-snapfirewall.xml";



void file_descriptor_deleter(int * fd)
{
    if(close(*fd) != 0)
    {
        int const e(errno);
        SNAP_LOG_WARNING("closing file descriptor failed (errno: ")(e)(", ")(strerror(e))(")");
    }
}


} // no name namespace



/** \brief Get a fixed cpu plugin name.
 *
 * The cpu plugin makes use of different fixed names. This function
 * ensures that you always get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_SNAPMANAGERCGI_FIREWALL_NAME:
        return "name";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_SNAPMANAGERCGI_FIREWALL_...");

    }
    NOTREACHED();
}




/** \brief Initialize the firewall plugin.
 *
 * This function is used to initialize the firewall plugin object.
 */
firewall::firewall()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the firewall plugin.
 *
 * Ensure the firewall object is clean before it is gone.
 */
firewall::~firewall()
{
}


/** \brief Get a pointer to the firewall plugin.
 *
 * This function returns an instance pointer to the firewall plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the firewall plugin.
 */
firewall * firewall::instance()
{
    return g_plugin_firewall_factory.instance();
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
QString firewall::description() const
{
    return "Manage the snapfirewall settings.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString firewall::dependencies() const
{
    return "|server|";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in snapmanager.cgi and snapmanagerdaemon plugins.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t firewall::do_update(int64_t last_updated)
{
    NOTUSED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in snapmanager*
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize firewall.
 *
 * This function terminates the initialization of the firewall plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void firewall::bootstrap(snap_child * snap)
{
    f_snap = dynamic_cast<snap_manager::manager *>(snap);
    if(f_snap == nullptr)
    {
        throw snap_logic_exception("snap pointer does not represent a valid manager object.");
    }

    SNAP_LISTEN(firewall, "server", snap_manager::manager, retrieve_status, _1);
}


/** \brief Determine this plugin status data.
 *
 * This function builds a tree of statuses.
 *
 * \param[in] server_status  The map of statuses.
 */
void firewall::on_retrieve_status(snap_manager::server_status & server_status)
{
    if(f_snap->stop_now_prima())
    {
        return;
    }

    // TODO: make the path a parameter from snapinit somehow?
    //       (also it will change once we have a broken up version of
    //       the file)
    //
    bool valid_xml(false);
    QFile input(g_service_filename);
    if(input.open(QIODevice::ReadOnly))
    {
        QDomDocument doc;
        doc.setContent(&input, false);

        // TBD: do we need the search? We expect only one <service> root tag
        //      with a name, we could just check the name?
        QDomXPath dom_xpath;
        dom_xpath.setXPath("/service[@name=\"snapfirewall\"]");
        QDomXPath::node_vector_t result(dom_xpath.apply(doc));
        if(result.size() > 0)
        {
            if(result[0].isElement())
            {
                QDomElement service(result[0].toElement());
                QString const disabled_attr(service.attribute("disabled"));
                snap_manager::status_t const disabled(snap_manager::status_t::state_t::STATUS_STATE_INFO,
                                                      get_plugin_name(),
                                                      "disabled",
                                                      disabled_attr.isEmpty() ? "enabled" : "disabled");
                server_status.set_field(disabled);

                QDomElement recovery_tag(service.firstChildElement("recovery"));
                snap_manager::status_t const recovery(snap_manager::status_t::state_t::STATUS_STATE_INFO,
                                                      get_plugin_name(),
                                                      "recovery",
                                                      recovery_tag.text());
                server_status.set_field(recovery);

                valid_xml = true;
            }
        }
    }
    if(!valid_xml)
    {
        snap_manager::status_t const snapinit(snap_manager::status_t::state_t::STATUS_STATE_ERROR,
                                              get_plugin_name(),
                                              "snapinit",
                                              QString("Could not read \"%1\" file or it was missing a snapfirewall service.")
                                                    .arg(g_service_filename));
        server_status.set_field(snapinit);
    }
}



/** \brief Transform a value to HTML for display.
 *
 * This function expects the name of a field and its value. It then adds
 * the necessary HTML to the specified element to display that value.
 *
 * If the value is editable, then the function creates a form with the
 * necessary information (hidden fields) to save the data as required
 * by that field (i.e. update a .conf/.xml file, create a new file,
 * remove a file, etc.)
 *
 * \param[in] server_status  The map of statuses.
 * \param[in] s  The field being worked on.
 *
 * \return true if we handled this field.
 */
bool firewall::display_value(QDomElement parent, snap_manager::status_t const & s, snap::snap_uri const & uri)
{
    QDomDocument doc(parent.ownerDocument());

    if(s.get_field_name() == "disabled")
    {
        // the list if frontend snapmanagers that are to receive statuses
        // of the cluster computers; may be just one computer; should not
        // be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Enable/Disable Firewall"
                        , s.get_field_name()
                        , s.get_value()
                        , "Define whether the firewall is \"enabled\" or \"disabled\"."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    if(s.get_field_name() == "recovery")
    {
        // the list of URIs from which we can download software bundles;
        // this should not be empty; shows a text input field
        //
        snap_manager::form f(
                  get_plugin_name()
                , s.get_field_name()
                , snap_manager::form::FORM_BUTTON_RESET | snap_manager::form::FORM_BUTTON_SAVE_EVERYWHERE | snap_manager::form::FORM_BUTTON_SAVE | snap_manager::form::FORM_BUTTON_RESTORE_DEFAULT
                );

        snap_manager::widget_input::pointer_t field(std::make_shared<snap_manager::widget_input>(
                          "Recovery Delay"
                        , s.get_field_name()
                        , s.get_value()
                        , "Delay before restarting snapfirewall if it fails to restart immediately after a crash. This number is in seconds."
                        ));
        f.add_widget(field);

        f.generate(parent, uri);

        return true;
    }

    return false;
}


/** \brief Save 'new_value' in field 'field_name'.
 *
 * This function saves 'new_value' in 'field_name'.
 *
 * \param[in] button_name  The name of the button the user clicked.
 * \param[in] field_name  The name of the field to update.
 * \param[in] new_value  The new value to save in that field.
 * \param[in] old_or_installation_value  The old value, just in case
 *            (usually ignored,) or the installation values (only
 *            for the self plugin that manages bundles.)
 * \param[in] affected_services  The list of services that were affected
 *            by this call.
 *
 * \return true if the new_value was applied successfully.
 */
bool firewall::apply_setting(QString const & button_name, QString const & field_name, QString const & new_value, QString const & old_or_installation_value, std::set<QString> & affected_services)
{
    NOTUSED(old_or_installation_value);

    // restore defaults?
    //
    bool const use_default_value(button_name == "restore_default");

    if(field_name == "disabled")
    {
        QFile file(g_service_filename);
        if(file.open(QIODevice::ReadWrite))
        {
            QDomDocument doc;
            doc.setContent(&file, false);

            QDomXPath dom_xpath;
            dom_xpath.setXPath("/service[@name=\"snapfirewall\"]");
            QDomXPath::node_vector_t result(dom_xpath.apply(doc));
            if(result.size() > 0)
            {
                if(result[0].isElement())
                {
                    // although this is about the snapfirewall, we have to
                    // restart the snapinit process if we want the change to
                    // be taken in account
                    //
                    affected_services.insert("snapinit");

                    QDomElement service(result[0].toElement());
                    if(use_default_value
                    || new_value.mid(0, 1).toUpper() == "D")
                    {
                        service.setAttribute("disabled", "disabled");
                    }
                    else
                    {
                        service.removeAttribute("disabled");
                    }

                    QString output(doc.toString(2));
                    QByteArray output_utf8(output.toUtf8());
                    file.seek(0L);
                    file.write(output_utf8);
                    file.resize(output_utf8.size());
                    return true;
                }
            }
        }
        return false;
    }

    if(field_name == "recovery")
    {
        QFile file(g_service_filename);
        if(file.open(QIODevice::ReadWrite))
        {
            QDomDocument doc;
            doc.setContent(&file, false);

            QDomXPath dom_xpath;
            dom_xpath.setXPath("/service[@name=\"snapfirewall\"]/recovery");
            QDomXPath::node_vector_t result(dom_xpath.apply(doc));
            if(result.size() > 0)
            {
                if(result[0].isElement())
                {
                    // although this is about the snapfirewall, we have to
                    // restart the snapinit process if we want the change to
                    // be taken in account
                    //
                    affected_services.insert("snapinit");

                    QDomElement recovery(result[0].toElement());
                    // remove existing children
                    while(!recovery.firstChild().isNull())
                    {
                        recovery.removeChild(result[0].firstChild());
                    }
                    // now save the new recovery value
                    QDomText recovery_text(doc.createTextNode(use_default_value ? QString("60") : new_value));
                    recovery.appendChild(recovery_text);

                    QString output(doc.toString(2));
                    QByteArray output_utf8(output.toUtf8());
                    file.seek(0L);
                    file.write(output_utf8);
                    file.resize(output_utf8.size());
                    return true;
                }
            }
        }
        return false;
    }

    return false;
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
