// Snap Websites Server -- timetracker plugin to track time and generate invoices
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

#include "timetracker.h"

#include "../output/output.h"
#include "../permissions/permissions.h"
#include "../layout/layout.h"
#include "../list/list.h"
#include "../locale/snap_locale.h"
#include "../users/users.h"

#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "qdomhelpers.h"

#include "poison.h"

SNAP_PLUGIN_START(timetracker, 1, 0)


/** \class timetracker
 * \brief Offer a way to track time spent on a project and generate invoices.
 *
 * This is a simple way to track hours of work so you can invoice them later.
 */


/** \brief Get a fixed timetracker name.
 *
 * The timetracker plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * Note that since this plugin is used to edit core and content data
 * more of the names come from those places.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_TIMETRACKER_DATE_QUERY_STRING:
        return "date";

    case name_t::SNAP_NAME_TIMETRACKER_MAIN_PAGE:
        return "timetracker::main_page";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_TIMETRACKER_...");

    }
    NOTREACHED();
}


/** \brief Initialize the timetracker plugin.
 *
 * This function is used to initialize the timetracker plugin object.
 */
timetracker::timetracker()
    //: f_snap(nullptr) -- auto-init
{
}

/** \brief Clean up the timetracker plugin.
 *
 * Ensure the timetracker object is clean before it is gone.
 */
timetracker::~timetracker()
{
}

/** \brief Get a pointer to the timetracker plugin.
 *
 * This function returns an instance pointer to the timetracker plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the timetracker plugin.
 */
timetracker * timetracker::instance()
{
    return g_plugin_timetracker_factory.instance();
}


/** \brief Send users to the timetracker settings.
 *
 * This path represents the timetracker settings.
 */
QString timetracker::settings_path() const
{
    return "/admin/settings/timetracker";
}


/** \brief A path or URI to a logo for this plugin.
 *
 * This function returns a 64x64 icons representing this plugin.
 *
 * \return A path to the logo.
 */
QString timetracker::icon() const
{
    return "/images/timetracker/timetracker-logo-64x64.png";
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
QString timetracker::description() const
{
    return "The time tracker plugin lets you or your employees enter their"
          " hours in order to generate invoices to your clients."
          " The tracker includes notes to describe the work done.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString timetracker::dependencies() const
{
    return "|editor|messages|output|path|permissions|users|";
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
int64_t timetracker::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2016, 1, 4, 2, 15, 41, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our timetracker references.
 *
 * Send our timetracker to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables
 *            added to the database by this update (in micro-seconds).
 */
void timetracker::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the timetracker.
 *
 * This function terminates the initialization of the timetracker plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void timetracker::bootstrap(snap_child * snap)
{
    f_snap = snap;

    //SNAP_LISTEN(timetracker, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(timetracker, "layout", layout::layout, generate_header_content, _1, _2, _3);
    SNAP_LISTEN(timetracker, "filter", filter::filter, replace_token, _1, _2, _3);
    //SNAP_LISTEN(timetracker, "editor", editor::editor, init_editor_widget, _1, _2, _3, _4, _5);
}


void timetracker::on_generate_header_content(content::path_info_t & ipath, QDomElement & header, QDomElement & metadata)
{
    NOTUSED(header);
    NOTUSED(metadata);

    QString const cpath(ipath.get_cpath());
    if(cpath == "timetracker")
    {
        content::content * content_plugin(content::content::instance());

        content_plugin->add_javascript(header.ownerDocument(), "timetracker");
        content_plugin->add_css(header.ownerDocument(), "timetracker");
    }
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
 * \param[in] ipath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool timetracker::on_path_execute(content::path_info_t & ipath)
{
    // TODO: add support to quickly interact with our form(s)

    // let the output plugin take care of this otherwise
    //
    return output::output::instance()->on_path_execute(ipath);
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the \<page\> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void timetracker::on_generate_main_content(content::path_info_t & ipath, QDomElement & page, QDomElement & body)
{
    // our settings pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


//void timetracker::on_can_handle_dynamic_path(content::path_info_t & ipath, path::dynamic_plugin_t & plugin_info)
//{
//    QString const cpath(ipath.get_cpath());
//    if(cpath.startsWith(QString("%1/").arg(sendmail::get_name(sendmail::name_t::SNAP_NAME_SENDMAIL_UNSUBSCRIBE_PATH)))
//    || cpath.startsWith(QString("%1/install/").arg(get_name(name_t::SNAP_NAME_INFO_PLUGIN_SELECTION)))
//    || cpath.startsWith(QString("%1/remove/").arg(get_name(name_t::SNAP_NAME_INFO_PLUGIN_SELECTION))))
//    {
//        // tell the path plugin that this is ours
//        plugin_info.set_plugin(this);
//        return;
//    }
//}


void timetracker::on_replace_token(content::path_info_t & ipath, QDomDocument & xml, filter::filter::token_info_t & token)
{
    NOTUSED(xml);

    // we only support timetracker tokens
    //
    if(token.f_name.length() <= 13
    || !token.is_namespace("timetracker::"))
    {
        return;
    }

    switch(token.f_name[13].unicode())
    {
    case 'c':
        // [timetracker::calendar]
        if(token.is_token("timetracker::calendar"))
        {
            token.f_replacement = token_calendar(ipath);
        }
        break;

    case 'm':
        // [timetracker::main_page]
        if(token.is_token(get_name(name_t::SNAP_NAME_TIMETRACKER_MAIN_PAGE)))
        {
            token.f_replacement = token_main_page(ipath);
        }
        break;

    }
}


/** \brief Define the dynamic content of /timetracker.
 *
 * This function computes the content of the /timetracker page. There
 * are several possibilities:
 *
 * \li User is a Time Tracker Administrator
 *
 * In this case, the page is a list of all the existing Time Tracker
 * users plus a button to add another user. The /timetracker page is a list
 * and what is displayed is that list.
 */
QString timetracker::token_main_page(content::path_info_t & ipath)
{
    content::content * content_plugin(content::content::instance());
    users::users * users_plugin(users::users::instance());
    permissions::permissions * permissions_plugin(permissions::permissions::instance());
    list::list * list_plugin(list::list::instance());

    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());

    QString result;

    // if we are an administrator, show the administrator view of this
    // page:
    //
    //  . our calendar or an Add Self button
    //  . an Add User button
    //  . list of users below
    //
    QString const & login_status(permissions_plugin->get_login_status());
    content::permission_flag allowed;
    path::path::instance()->access_allowed(permissions_plugin->get_user_path(), ipath, "administer", login_status, allowed);
    if(allowed.allowed())
    {
        // check whether the administrator has a calendar, if so, show it
        // otherwise show an "Add Self" button so the administrator can
        // create his own timetracker page but that is not mandatory
        //
        content::path_info_t calendar_ipath;
        ipath.get_child(calendar_ipath, QString("%1").arg(users_plugin->get_user_identifier()));
        calendar_ipath.set_parameter("date", ipath.get_parameter("date"));
        if(content_table->exists(calendar_ipath.get_key())
        && content_table->row(calendar_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
        {
            // calendar exists
            result += token_calendar(calendar_ipath);
        }
        else
        {
            // no calendar
            result +=
                    "<p>"
                        "You do no yet have a Time Tracker calendar. Click"
                        " <a class=\"button time-tracker add-self\""
                        " href=\"#add-self\">Add Self</a> button to add your"
                        " own calendar."
                    "</p>";
        }

        // as an administrator you can always add other users to the
        // Time Tracker system; users can be added as "User" only;
        // bookkeepers and other administrators cannot be added here
        // (at least not at this time.)
        //
        result +=
                "<div class=\"time-tracker-buttons\">"
                    "<a class=\"button time-tracker add-user\""
                    " href=\"#add-user\">Add User</a>"
                "</div>";

        // now show a list of users, we do not show their calendar because
        // that could be too much to generate here; the administrator can
        // click on a link to go see the calendar, though
        //
        result += QString("<div class=\"time-tracker-users\">%1</div>").arg(list_plugin->generate_list(ipath, ipath, 0, 30));
        return result;
    }

    {
        // regular users may have a timetracker page, defined as
        //
        //      /timetracker/<user-identifier>
        //
        // if that page exists, display that only (that is all what regular
        // users can do.)
        //
        content::path_info_t calendar_ipath;
        ipath.get_child(calendar_ipath, QString("%1").arg(users_plugin->get_user_identifier()));
        calendar_ipath.set_parameter("date", ipath.get_parameter("date"));
        if(content_table->exists(calendar_ipath.get_key())
        && content_table->row(calendar_ipath.get_key())->exists(content::get_name(content::name_t::SNAP_NAME_CONTENT_CREATED)))
        {
            // calendar exists
            return token_calendar(calendar_ipath);
        }

        return
            "<p>"
                "You do not yet have a Time Tracker page."
                " Please ask your administrator to create a page for you if you are"
                " allowed to use the Time Tracker system."
            "</p>";
    }

    return result;
}


QString timetracker::token_calendar(content::path_info_t & ipath)
{
    NOTUSED(ipath);

    locale::locale * locale_plugin(locale::locale::instance());

    bool ok(false);

    // by default we want to create the calendar for the current month,
    // if the main URI includes a query string, we may switch to a different
    // month or even year
    //
    time_t const now(f_snap->get_start_time());
    QString const date(locale_plugin->format_date(now, "%Y%m%d", true));
    int year (date.mid(0, 4).toInt(&ok));
    int month(date.mid(4, 2).toInt(&ok));
    int day  (date.mid(6, 2).toInt(&ok));
    int const today_year (year);
    int const today_month(month);
    int const today_day  (day);

    time_t selected_day(now);

    // optionally we expect a full date with format: %Y%m%d
    //
    snap_uri const & uri(f_snap->get_uri());
    QString const when(uri.query_option(get_name(name_t::SNAP_NAME_TIMETRACKER_DATE_QUERY_STRING)));
    if(!when.isEmpty())
    {
        int const when_year(when.mid(0, 4).toInt(&ok, 10));
        if(ok)
        {
            int const when_month(when.mid(4, 2).toInt(&ok, 10));
            if(ok)
            {
                int const when_day(when.mid(6, 2).toInt(&ok, 10));
                if(ok)
                {
                    int const max_when_day(f_snap->last_day_of_month(when_month, when_year));
                    if(when_year  >= 2000 && when_year  <= 3000
                    && when_month >= 1    && when_month <= 12
                    && when_day   >= 1    && when_day   <= max_when_day)
                    {
                        // an acceptable date, use it instead of 'now'
                        //
                        year  = when_year;
                        month = when_month;
                        day   = when_day;

                        // adjust the selected day
                        //
                        selected_day = SNAP_UNIX_TIMESTAMP(year, month, day, 0, 0, 0);
                    }
                }
            }
        }
    }

    QDomDocument doc;
    QDomElement root(doc.createElement("snap"));
    doc.appendChild(root);

    QDomElement month_tag(doc.createElement("month"));
    snap_dom::append_plain_text_to_node(month_tag, locale_plugin->format_date(selected_day, "%B", true));
    month_tag.setAttribute("mm", month);
    root.appendChild(month_tag);

    QDomElement year_tag(doc.createElement("year"));
    snap_dom::append_integer_to_node(year_tag, year);
    root.appendChild(year_tag);

    QDomElement days_tag(doc.createElement("days"));
    root.appendChild(days_tag);

    int const max_day(f_snap->last_day_of_month(month, year));

    for(int line(1); line <= max_day; )
    {
        QDomElement line_tag(doc.createElement("line"));
        days_tag.appendChild(line_tag);

        time_t const day_one(SNAP_UNIX_TIMESTAMP(year, month, line, 0, 0, 0));

        int const week_number(locale_plugin->format_date(day_one, "%U", true).toInt(&ok)); // user should be in control of which number to use, valid formats are: %U, %V, %W
        line_tag.setAttribute("week", week_number);

        int const week_day(locale_plugin->format_date(day_one, "%B", true).toInt(&ok));
#ifdef DEBUG
        if(line != 1 && week_day != 0)
        {
            throw snap_logic_exception(QString("line = %1 and week_day = %2 when it should be zero.").arg(line).arg(week_day));
        }
#endif

        for(int w(0); w <= 6; ++w)
        {
            if(w < week_day
            || line > max_day)
            {
                // this is a day in the previous or next month
                // (a.k.a. out of range)
                //
                QDomElement no_day_tag(doc.createElement("no_day"));
                days_tag.appendChild(no_day_tag);
            }
            else
            {
                QDomElement day_tag(doc.createElement("day"));
                days_tag.appendChild(day_tag);

                // does this day represent today?
                //
                if(line  == today_day
                && month == today_month
                && year  == today_year)
                {
                    day_tag.setAttribute("today", "today");
                }

snap_dom::append_plain_text_to_node(day_tag, QString("%1").arg(line));

                ++line;
            }
        }
    }


    QString result;

    return result;
}



//void timetracker::on_init_editor_widget(content::path_info_t & ipath, QString const & field_id, QString const & field_type, QDomElement & widget, QtCassandra::QCassandraRow::pointer_t row)
//{
//    NOTUSED(field_type);
//    NOTUSED(row);
//
//    QString const cpath(ipath.get_cpath());
//    if(cpath == "unsubscribe")
//    {
//        init_unsubscribe_editor_widgets(ipath, field_id, widget);
//    }
//    else if(cpath == "admin/plugins")
//    {
//        init_plugin_selection_editor_widgets(ipath, field_id, widget);
//    }
//}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
