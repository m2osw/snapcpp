// Snap Websites Server -- JavaScript WYSIWYG form widgets
// Copyright (C) 2013-2016  Made to Order Software Corp.
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

#include "date_widgets.h"

#include "log.h"
#include "mkgmtime.h"
#include "not_reached.h"
#include "not_used.h"

#include "poison.h"


SNAP_PLUGIN_START(date_widgets, 1, 0)





///** \brief Get a fixed date_widgets plugin name.
// *
// * The date_widgets plugin makes use of different names in the database. This
// * function ensures that you get the right spelling for a given name.
// *
// * \param[in] name  The name to retrieve.
// *
// * \return A pointer to the name.
// */
//char const * get_name(name_t name)
//{
//    switch(name)
//    {
//    case name_t::SNAP_NAME_DATE_WIDGETS_...:
//        return "date_widgets::...";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("Invalid name_t::SNAP_NAME_DATE_WIDGETS_...");
//
//    }
//    NOTREACHED();
//}








/** \brief Initialize the date_widgets plugin.
 *
 * This function is used to initialize the date_widgets plugin object.
 */
date_widgets::date_widgets()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the date_widgets plugin.
 *
 * Ensure the date_widgets object is clean before it is gone.
 */
date_widgets::~date_widgets()
{
}


/** \brief Get a pointer to the date_widgets plugin.
 *
 * This function returns an instance pointer to the date_widgets plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the date_widgets plugin.
 */
date_widgets * date_widgets::instance()
{
    return g_plugin_date_widgets_factory.instance();
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString date_widgets::icon() const
{
    return "/images/editor/date-widgets-logo-64x64.png";
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
QString date_widgets::description() const
{
    return "This plugin offers several \"Date\" widgets for the Snap! editor."
        " By default, one can use a Line Edit widgets to let users type in a"
        " date. Only, it is often a lot faster to just click on the date in"
        " small calendar popup. The Date widget also offers a date range"
        " selection and a partial date selection (only one of the day, month"
        " or year; i.e. credit card expiration dates is only the year and the"
        " month.)";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString date_widgets::dependencies() const
{
    return "|editor|";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last
 *            updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t date_widgets::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 2, 27, 23, 8, 37, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables
 *            added to the database by this update (in micro-seconds).
 */
void date_widgets::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize date_widgets.
 *
 * This function terminates the initialization of the date_widgets plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void date_widgets::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(date_widgets, "editor", editor::editor, prepare_editor_form, _1);
    SNAP_LISTEN(date_widgets, "editor", editor::editor, value_to_string, _1);
    SNAP_LISTEN(date_widgets, "editor", editor::editor, string_to_value, _1);
}


/** \brief Add the date widgets to the editor XSLT.
 *
 * The editor is extended by the locale plugin by adding a time zone
 * and other various widgets.
 *
 * \param[in] e  A pointer to the editor plugin.
 */
void date_widgets::on_prepare_editor_form(editor::editor * e)
{
    e->add_editor_widget_templates_from_file(":/xsl/date_widgets/date-form.xsl");
}


/** \brief Transform the dropdown-date value as required.
 *
 * This function transforms dropdown-date value to something more useable
 * than what the function returns by default.
 *
 * \param[in] value_info  A value_to_string_info_t object.
 *
 * \return true if the data_type is not known internally, false when the type
 *         was managed by this very function
 */
void date_widgets::on_value_to_string(editor::editor::value_to_string_info_t & value_info)
{
    if(value_info.is_done()
    || value_info.get_data_type() != "dropdown-date")
    {
        return;
    }

    value_info.set_type_name("date");

    // the value is an int64_t in microsecond; it will include a day,
    // a month and a year. The dropdown will know which do use and which
    // to ignore.
    //

    struct tm time_info;
    time_t seconds(value_info.get_value().safeInt64Value() / 1000000);
    gmtime_r(&seconds, &time_info);
    char buf[256];
    buf[0] = '\0';
    strftime(buf, sizeof(buf), "%Y/%m/%d", &time_info);
    value_info.result() = buf;

    value_info.set_status(editor::editor::value_to_string_info_t::status_t::DONE);
}


/** \brief Transform data to a QCassandraValue.
 *
 * This function transforms a value received from a POST into a
 * QCassandraValue to be saved in the database.
 *
 * \param[in] value_info  Information about the widget to be checked.
 *
 * \return false if the data_type is not known internally, true when the type
 *         was managed by this very function
 */
void date_widgets::on_string_to_value(editor::editor::string_to_value_info_t & value_info)
{
    if(value_info.is_done()
    || value_info.get_data_type() != "dropdown-date")
    {
        return;
    }

    value_info.set_type_name("date");

    // convert a YYYY/MM/DD date to 64 bit value in micro seconds
    //
    snap_string_list ymd(value_info.get_data().split('/'));

    // make sure we have exactly 3 entries
    if(ymd.size() != 3)
    {
        // that's invalid
        value_info.set_status(editor::editor::string_to_value_info_t::status_t::ERROR);
        return;
    }

    struct tm time_info;
    memset(&time_info, 0, sizeof(time_info));
    bool ok(false);

    // verify the year
    if(ymd[0] != "-")
    {
        // limit year between 1800 and 3000
        time_info.tm_year = ymd[0].toInt(&ok) - 1900;
        if(!ok || time_info.tm_year < (1800 - 1900) || time_info.tm_year > (3000 - 1900))
        {
            // that's invalid
            value_info.set_status(editor::editor::string_to_value_info_t::status_t::ERROR);
            return;
        }
    }
    else
    {
        // get current year as the default year
        time_t const now(time(nullptr));
        struct tm now_tm;
        localtime_r(&now, &now_tm);
        time_info.tm_year = now_tm.tm_year;
    }

    // verify the month
    if(ymd[1] != "-")
    {
        time_info.tm_mon = ymd[1].toInt(&ok) - 1;
        if(!ok || time_info.tm_mon < 0 || time_info.tm_mon > 11)
        {
            // that's invalid
            value_info.set_status(editor::editor::string_to_value_info_t::status_t::ERROR);
            return;
        }
    }
    // else time_info.tm_mon = 0; -- this is the default

    // verify the day
    if(ymd[2] != "-")
    {
        time_info.tm_mday = ymd[2].toInt(&ok);
        if(!ok || time_info.tm_mday < 1 || time_info.tm_mday > f_snap->last_day_of_month(time_info.tm_mon + 1, time_info.tm_year + 1900))
        {
            // that's invalid
            value_info.set_status(editor::editor::string_to_value_info_t::status_t::ERROR);
            return;
        }
    }
    else
    {
        time_info.tm_mday = 1;
    }

    time_t const t(mkgmtime(&time_info));
    value_info.result().setInt64Value(t * 1000000); // seconds to microseconds
    value_info.set_status(editor::editor::string_to_value_info_t::status_t::DONE);
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
