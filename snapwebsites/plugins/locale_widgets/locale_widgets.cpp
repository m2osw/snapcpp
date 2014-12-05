// Snap Websites Server -- offer a plethora of localized editor widgets
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

#include "locale_widgets.h"

#include "../editor/editor.h"
#include "../output/output.h"
#include "../locale/snap_locale.h"

#include "not_reached.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(locale_widgets, 1, 0)


///* \brief Get a fixed locale_widgets name.
// *
// * The locale_widgets plugin makes use of different names in the database.
// * This function ensures that you get the right spelling for a given name.
// *
// * \param[in] name  The name to retrieve.
// *
// * \return A pointer to the name.
// */
//char const *get_name(name_t name)
//{
//    switch(name)
//    {
//    case SNAP_NAME_LOCALE_WIDGETS_NAME:
//        return "locale_widgets::name";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid SNAP_NAME_LOCALE_WIDGETS_...");
//
//    }
//    NOTREACHED();
//}









/** \brief Initialize the locale_widgets plugin.
 *
 * This function is used to initialize the locale_widgets plugin object.
 */
locale_widgets::locale_widgets()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the locale_widgets plugin.
 *
 * Ensure the locale_widgets object is clean before it is gone.
 */
locale_widgets::~locale_widgets()
{
}


/** \brief Initialize the locale_widgets.
 *
 * This function terminates the initialization of the locale_widgets plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void locale_widgets::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(locale_widgets, "editor", editor::editor, init_editor_widget, _1, _2, _3, _4, _5);
    SNAP_LISTEN(locale_widgets, "editor", editor::editor, prepare_editor_form, _1);
}


/** \brief Get a pointer to the locale plugin.
 *
 * This function returns an instance pointer to the locale plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the locale plugin.
 */
locale_widgets *locale_widgets::instance()
{
    return g_plugin_locale_widgets_factory.instance();
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
QString locale_widgets::description() const
{
    return "Define locale functions to be used throughout all the plugins."
        " It handles time and date, timezone, numbers, currency, etc.";
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
int64_t locale_widgets::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 12, 4, 16, 44, 8, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void locale_widgets::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Add the locale widget to the editor XSLT.
 *
 * The editor is extended by the locale plugin by adding a time zone
 * and other various widgets.
 *
 * \param[in] e  A pointer to the editor plugin.
 */
void locale_widgets::on_prepare_editor_form(editor::editor *e)
{
    e->add_editor_widget_templates_from_file(":/xsl/locale_widgets/locale-form.xsl");
}


/** \brief Initialize the continent and city widgets.
 *
 * This function initializes continent and city widgets with timezone
 * information.
 *
 * \param[in] ipath  The path of the page being generated.
 * \param[in] field_id  The name of the field being initialized.
 * \param[in] field_type  The type of field being initialized.
 * \param[in] widget  The XML DOM widget.
 * \param[in] row  The row with the saved data.
 */
void locale_widgets::on_init_editor_widget(content::path_info_t& ipath, QString const& field_id, QString const& field_type, QDomElement& widget, QtCassandra::QCassandraRow::pointer_t row)
{
    static_cast<void>(field_id);
    static_cast<void>(row);

    QString const cpath(ipath.get_cpath());
    if(field_type == "locale_timezone")
    {
        QDomDocument doc(widget.ownerDocument());

        // we need script and CSS complements for timezones
        // but we do not have the right document (i.e. we need the -parser.xsl
        // and not the -page.xml file...) but we can put them in the form
        // defining the widget too
        //content::content::instance()->add_javascript(doc, "locale-timezone");
        //content::content::instance()->add_css(doc, "locale-timezone");

        // setup the default values
        QDomElement value(doc.createElement("value"));
        widget.appendChild(value);

        // The default cannot be dealt with like this, it comes from the
        // <file>-page.xml data and not the code!
        // Although we may want to have a "dynamic" default so when a user
        // edits his timezone he sees the website default timezone by
        // default... but I think it is better to try to determine the user
        // timezone instead and if you'd like to have a website specific
        // timezone, define it as a <default> tag in the XML page file
        //QString const current_timezone(row->cell(get_name(SNAP_NAME_LOCALE_TIMEZONE))->value().stringValue());
        //QStringList current_timezone_segments(current_timezone.split('/'));

        //// this is the widget result
        //doc.getElementByTagName("default");
        //QDomElement default_value(doc.createElement("default"));
        //widget.appendChild(default_value);
        //QDomText current_value(doc.createTextNode(current_timezone_segments[0]));
        //default_value.appendChild(current_value);

        //// this is the default for the continent
        //current_timezone_segments[0].replace('_', ' ');
        //QDomElement default_continent(doc.createElement("default_continent"));
        //widget.appendChild(default_continent);
        //QDomText current_continent_value(doc.createTextNode(current_timezone_segments[0]));
        //default_continent.appendChild(current_continent_value);

        //// this is the default for the timezone
        //current_timezone_segments[1].replace('_', ' ');
        //QDomText current_city_value(doc.createTextNode(current_timezone_segments[1]));
        //value.appendChild(current_city_value);

        // setup a dropdown preset list for continents and one for cities
        QDomElement preset_continent(doc.createElement("preset_continent"));
        widget.appendChild(preset_continent);

        QDomElement preset_city(doc.createElement("preset_city"));
        widget.appendChild(preset_city);

        // get the complete list
        locale::locale::timezone_list_t const& list(locale::locale::instance()->get_timezone_list());

        // extract the continents as we setup the cities
        QMap<QString, bool> continents;
        int const max(list.size());
        for(int idx(0); idx < max; ++idx)
        {
            // skip on a few "continent" which we really do not need
            QString const continent(list[idx].f_continent);
            if(continent == "Etc"
            || continent == "SystemV"
            || continent == "US")
            {
                continue;
            }

            continents[continent] = true;

            // create one item per city
            QDomElement item(doc.createElement("item"));
            preset_city.appendChild(item);
            QString const value_city(list[idx].f_city);
            item.setAttribute("class", continent);
            QDomText text(doc.createTextNode(value_city));
            item.appendChild(text);
        }

        // now use the map of continents to add them to the list
        for(auto it(continents.begin());
                 it != continents.end();
                 ++it)
        {
            // create one item per continent
            QDomElement item(doc.createElement("item"));
            preset_continent.appendChild(item);
            QString const value_continent(it.key());
            QDomText text(doc.createTextNode(value_continent));
            item.appendChild(text);
        }
    }
}


//
// A reference of the library ICU library can be found here:
// /usr/include/x86_64-linux-gnu/unicode/timezone.h
// file:///usr/share/doc/icu-doc/html/index.html
//

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
