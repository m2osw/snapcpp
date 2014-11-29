// Snap Websites Server -- handle various locale information such as timezone and date output, number formatting for display, etc.
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

#include "./snap_locale.h"

#include "../editor/editor.h"
#include "../output/output.h"
//#include "../permissions/permissions.h"

#include "not_reached.h"

#include <libtld/tld.h>
#include <QtSerialization/QSerialization.h>

#include <unicode/errorcode.h>
#include <unicode/timezone.h>

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(locale, 1, 0)


/* \brief Get a fixed locale name.
 *
 * The locale plugin makes use of different names in the database. This
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
    case SNAP_NAME_LOCALE_TIMEZONE:
        return "locale::timezone";

    case SNAP_NAME_LOCALE_TIMEZONE_CITY:
        return "locale::timezone_city";

    case SNAP_NAME_LOCALE_TIMEZONE_CONTINENT:
        return "locale::timezone_continent";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_LOCALE_...");

    }
    NOTREACHED();
}









/** \brief Initialize the locale plugin.
 *
 * This function is used to initialize the locale plugin object.
 */
locale::locale()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the locale plugin.
 *
 * Ensure the locale object is clean before it is gone.
 */
locale::~locale()
{
}


/** \brief Initialize the locale.
 *
 * This function terminates the initialization of the locale plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void locale::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(locale, "editor", editor::editor, init_editor_widget, _1, _2, _3, _4, _5);
    SNAP_LISTEN(locale, "editor", editor::editor, prepare_editor_form, _1);
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
locale *locale::instance()
{
    return g_plugin_locale_factory.instance();
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
QString locale::description() const
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
int64_t locale::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 11, 20, 1, 10, 8, content_update);

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
void locale::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Return the list of available timezones.
 *
 * We use the ICU which seems to be the best C/C++ library that
 * offers timezone and many other "Unicode" functionality.
 *
 * Also, there is a zone.tab table, and on newer systems, a
 * zone1970.tab table, with the list of all the known timezones.
 *
 * \note
 * Possible graphical JavaScript library for a graphical timezone picker
 * https://github.com/dosx/timezone-picker
 *
 * \note
 * A reference of the library ICU library can be found here:
 * /usr/include/x86_64-linux-gnu/unicode/timezone.h
 *
 * \note
 * The zone[1970].tab file is generally under /usr/share/zoneinfo
 * directory.
 *
 * \note
 * This function caches all the available timezones. So calling it multiple
 * times does not waste time.
 *
 * \return The list of timezone defined on this operating system.
 */
locale::locale::timezone_list_t const& locale::get_timezone_list()
{
    // read the file only if empty
    if(f_timezone_list.empty())
    {
        StringEnumeration *zone_list(TimeZone::createEnumeration());
        if(zone_list != nullptr)
        {
            for(;;)
            {
                // WARNING: you MUST initialize err otherwise
                //          unext() fails immediately
                UErrorCode err(U_ZERO_ERROR);
                UChar const *id(zone_list->unext(nullptr, err));
                if(id == nullptr)
                {
                    if(err != U_ZERO_ERROR)
                    {
                        ErrorCode err_code;
                        err_code.set(err);
                    }
                    break;
                }
                // TODO: The following "works great", only it does not
                //       really remove the equivalent we would want to
                //       remove; for example, it would keep Chile/EasterIsland
                //       instead of the more proper Pacific/Easter entry
                //       We may want to make use of the zone.tab file (see
                //       below) and then check against the ICU entries...
                //
                // skip equivalents, it will make the list shorter and
                // generally less confusing (i.e. both: Faroe / Faeroe)
                //UnicodeString const id_string(id);
                //UnicodeString const eq_string(TimeZone::getEquivalentID(id_string, 0));
                //if(id_string != eq_string)
                //{
                //    continue;
                //}

                QString const qid(QString::fromUtf16(id));
                QStringList const id_segments(qid.split('/'));
                if(id_segments.size() == 2)
                {
                    timezone_info_t info;
                    info.f_timezone_name = qid;
                    info.f_continent = id_segments[0];
                    info.f_continent.replace('_', ' ');
                    info.f_city = id_segments[1];
                    info.f_city.replace('_', ' ');
                    f_timezone_list.push_back(info);
                }
            }
        }

#if 0
        // still empty?
        if(f_timezone_list.empty())
        {

// TODO: use the following as a fallback? (never tested!)
//       we could also have our own copy of the zone.tab file so if we
//       cannot find a zone file, we use our own (possibly outdated)
//       version...

            // new systems should have a zone1970.tab file instead of the
            // zone.tab; this new file is expected to have a slightly different
            // format; only it is not yet available under Ubuntu (including 14.10)
            // so we skip on that for now.
            //
            // IMPORTANT NOTE: this far, I have not see zone1970, it is new
            //                 as of June 2014 so very recent...
            QFile zone("/usr/share/zoneinfo/zone.tab");
            if(!zone.open(QIODevice::ReadOnly))
            {
                // should we just generate an error and return an empty list?
                throw snap_io_exception("server cannot find zone.tab file with timezone information.");
            }

            // load the file, each line is one entry
            // a line that starts with a '#' is ignored (i.e. comment)
            for(;;)
            {
                QByteArray raw_line(zone.readLine());
                if(zone.atEnd() && raw_line.isEmpty())
                {
                    break;
                }

                // skip comments
                if(raw_line.at(0) == '#')
                {
                    continue;
                }

                // get that in a string so we can split it
                QString const line(QString::fromUtf8(raw_line.data()));

                QStringList const line_segments(line.split('\t'));
                if(line_segments.size() < 3)
                {
                    continue;
                }
                timezone_info_t info;

                // 2 letter country name
                info.f_2country = line_segments[0];

                // position (lon/lat)
                QString const lon(line_segments[1].mid(0, 5));
                bool ok;
                info.f_longitude = lon.toInt(&ok, 10);
                if(!ok)
                {
                    info.f_longitude = -1;
                }
                QString const lat(line_segments[1].mid(5, 5));
                info.f_latitude = lat.toInt(&ok, 10);
                if(!ok)
                {
                    info.f_latitude = -1;
                }

                // the continent, country/state, city are separated by a slash
                info.f_timezone_name = line_segments[2];
                QStringList const names(info.f_timezone_name.split('/'));
                if(names.size() < 2)
                {
                    // invalid continent/state/city (TZ) entry
                    continue;
                }
                info.f_continent = names[0];
                info.f_continent.replace('_', ' ');
                if(names.size() == 3)
                {
                    info.f_country_or_state = names[1];
                    info.f_country_or_state.replace('_', ' ');
                    info.f_city = names[2];
                }
                else
                {
                    // no extra country/state name
                    info.f_city = names[1];
                }
                info.f_city.replace('_', ' ');

                // comment is optional, make sure it exists
                if(line_segments.size() >= 3)
                {
                    info.f_comment = line_segments[3];
                }

                f_timezone_list.push_back(info);
            }
        }
#endif
    }

    return f_timezone_list;
}


/** \brief Add the locale widget to the editor XSLT.
 *
 * The editor is extended by the locale plugin by adding a time zone
 * and other various widgets.
 *
 * \param[in] e  A pointer to the editor plugin.
 */
void locale::on_prepare_editor_form(editor::editor *e)
{
    e->add_editor_widget_templates_from_file(":/xsl/locale/locale-form.xsl");
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
void locale::on_init_editor_widget(content::path_info_t& ipath, QString const& field_id, QString const& field_type, QDomElement& widget, QtCassandra::QCassandraRow::pointer_t row)
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
        timezone_list_t const& list(get_timezone_list());

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



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
