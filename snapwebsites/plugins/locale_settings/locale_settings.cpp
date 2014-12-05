// Snap Websites Server -- offer a website global locale settings page
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

#include "locale_settings.h"

#include "../locale/snap_locale.h"

#include "not_reached.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(locale_settings, 1, 0)


/* \brief Get a fixed locale_settings name.
 *
 * The locale_settings plugin makes use of different names in the database.
 * This function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_LOCALE_SETTINGS_LOCALE:
        return "locale_settings::locale";

    case SNAP_NAME_LOCALE_SETTINGS_TIMEZONE:
        return "locale_settings::timezone";

    case SNAP_NAME_LOCALE_SETTINGS_PATH:
        return "admin/locale";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_LOCALE_SETTINGS_...");

    }
    NOTREACHED();
}









/** \brief Initialize the locale plugin.
 *
 * This function is used to initialize the locale plugin object.
 */
locale_settings::locale_settings()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the locale plugin.
 *
 * Ensure the locale object is clean before it is gone.
 */
locale_settings::~locale_settings()
{
}


/** \brief Initialize the locale.
 *
 * This function terminates the initialization of the locale plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void locale_settings::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN0(locale_settings, "locale", locale::locale, set_locale);
    SNAP_LISTEN0(locale_settings, "locale", locale::locale, set_timezone);
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
locale_settings *locale_settings::instance()
{
    return g_plugin_locale_settings_factory.instance();
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
QString locale_settings::description() const
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
int64_t locale_settings::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 12, 4, 16, 52, 8, content_update);

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
void locale_settings::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Set locale (language) if not defined yet.
 *
 * This function checks for a locale definition in the locale settings
 * defined for the entire website. If there is such a locale, then it
 * gets used.
 *
 * This is generally the locale used by the website when a non logged in
 * visitor views the website (i.e. website wide locale.)
 */
void locale_settings::on_set_locale()
{
    locale::locale *locale_plugin(locale::locale::instance());
    QString const current_locale(locale_plugin->get_current_locale());

    if(current_locale.isEmpty())
    {
        // check for a locale
        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(SNAP_NAME_LOCALE_SETTINGS_PATH));
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));
        locale_plugin->set_current_locale(revision_row->cell(get_name(SNAP_NAME_LOCALE_SETTINGS_LOCALE))->value().stringValue());
    }
}


/** \brief Set timezone if not defined yet.
 *
 * This function checks for a timezone definition in the locale settings
 * defined for the entire website. If there is such a timezone, then it
 * gets used.
 *
 * This is generally the timezone used by the website when a non logged in
 * visitor views the website (i.e. website wide timezone.)
 */
void locale_settings::on_set_timezone()
{
    locale::locale *locale_plugin(locale::locale::instance());
    QString const current_timezone(locale_plugin->get_current_timezone());

    if(current_timezone.isEmpty())
    {
        // check for a locale
        content::path_info_t settings_ipath;
        settings_ipath.set_path(get_name(SNAP_NAME_LOCALE_SETTINGS_PATH));
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));
        QString const timezone_name(revision_row->cell(get_name(SNAP_NAME_LOCALE_SETTINGS_TIMEZONE))->value().stringValue());
        locale_plugin->set_current_timezone(timezone_name);

std::cerr << "*** Set LOCALE timezone [" << timezone_name << "]\n";
    }
}


//
// A reference of the library ICU library can be found here:
// /usr/include/x86_64-linux-gnu/unicode/timezone.h
// file:///usr/share/doc/icu-doc/html/index.html
//

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
