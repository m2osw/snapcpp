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

#include "snap_locale.h"

#include "../editor/editor.h"
#include "../output/output.h"

#include "not_reached.h"

#include <libtld/tld.h>
#include <QtSerialization/QSerialization.h>

#include <unicode/datefmt.h>
#include <unicode/errorcode.h>
#include <unicode/locid.h>
#include <unicode/timezone.h>

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(locale, 1, 0)


///* \brief Get a fixed locale name.
// *
// * The locale plugin makes use of different names in the database. This
// * function ensures that you get the right spelling for a given name.
// *
// * \param[in] name  The name to retrieve.
// *
// * \return A pointer to the name.
// */
//char const *get_name(name_t name)
//{
//    switch(name)
//    {
//    case SNAP_NAME_LOCALE_NAME:
//        return "locale::name";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid SNAP_NAME_LOCALE_...");
//
//    }
//    NOTREACHED();
//}









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
    return "Define base locale functions to be used throughout all the"
        " plugins. It handles time and date, timezone, numbers, currency,"
        " etc.";
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
    static_cast<void>(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Return the list of available locales.
 *
 * This function gets all the available locales from the Locale class
 * and returns them in an array of locale information.
 *
 * \return A vector of locale_info_t structures.
 */
locale::locale_list_t const& locale::get_locale_list()
{
    if(f_locale_list.isEmpty())
    {
        // we use the DateFormat to retrieve the list of locales;
        // it is most likely the same or very similar list in all
        // the various objects offering a getAvailableLocales()
        // function... (TBD)
        int32_t count;
        const Locale *l(DateFormat::getAvailableLocales(count));
        for(int32_t i(0); i < count; ++i)
        {
            locale_info_t info;

            // Language
            {
                info.f_abbreviations.f_language = QString::fromAscii(l[i].getLanguage());
                UnicodeString lang;
                l[i].getDisplayLanguage(lang);
                info.f_display_names.f_language = QString::fromUtf16(lang.getTerminatedBuffer());
            }

            // Variant
            {
                info.f_abbreviations.f_variant = QString::fromAscii(l[i].getVariant());
                UnicodeString variant;
                l[i].getDisplayVariant(variant);
                info.f_display_names.f_variant = QString::fromUtf16(variant.getTerminatedBuffer());
            }

            // Country
            {
                info.f_abbreviations.f_country = QString::fromAscii(l[i].getCountry());
                UnicodeString country;
                l[i].getDisplayVariant(country);
                info.f_display_names.f_country = QString::fromUtf16(country.getTerminatedBuffer());
            }

            // Script
            {
                info.f_abbreviations.f_script = QString::fromAscii(l[i].getScript());
                UnicodeString script;
                l[i].getDisplayVariant(script);
                info.f_display_names.f_script = QString::fromUtf16(script.getTerminatedBuffer());
            }

            info.f_locale = QString::fromAscii(l[i].getName());

            f_locale_list.push_back(info);
        }
    }

    return f_locale_list;
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


/** \brief Retrieve the currently setup locale.
 *
 * This function retrieves the current locale of this run.
 *
 * You may change the locale with a call to the set_locale() signal.
 *
 * If you are dealing with a date and/or time, you probably also want
 * to call the set_timezone() function.
 *
 * \return The current locale, may be empty.
 */
QString locale::get_current_locale() const
{
    return f_current_locale;
}


/** \brief Define the current locale.
 *
 * This function saves a new locale as the current locale.
 *
 * This function is semi-internal as it should only be called from
 * plugins that implement the set_locale() signal.
 *
 * \warning
 * This function does NOT setup the locale. Instead you MUST call the
 * set_locale() signal and plugins that respond to that signal call
 * the set_current_locale(). Once the signal is done, then and only
 * then is the system locale actually set.
 *
 * \param[in] new_locale  Change the locale.
 *
 * \sa set_locale()
 *
 */
void locale::set_current_locale(QString const& new_locale)
{
    f_current_locale = new_locale;
}


/** \brief Retrieve the currently setup timezone.
 *
 * This function retrieves the current timezone of this run.
 *
 * You may change the timezone with a call to the set_timezone() signal.
 *
 * If you are dealing with formatting a date and/or time, you probably
 * also want to call the set_locale() function.
 *
 * \return The current timezone, may be empty.
 *
 * \sa set_timezone()
 * \sa set_current_timezone()
 */
QString locale::get_current_timezone() const
{
    return f_current_timezone;
}


/** \brief Define the current timezone.
 *
 * This function saves a new timezone as the current timezone.
 *
 * This function is semi-internal as it should only be called from
 * plugins that implement the set_timezone() signal.
 *
 * \warning
 * This function does NOT setup the timezone. Instead you MUST call the
 * set_timezone() signal and plugins that respond to that signal call
 * the set_current_timezone(). Once the signal is done, then and only
 * then is the system timezone actually set.
 *
 * \param[in] new_timezone  Change the timezone.
 *
 * \sa set_timezone()
 */
void locale::set_current_timezone(QString const& new_timezone)
{
    f_current_timezone = new_timezone;
}


/** \brief Reset the locale current setup.
 *
 * This function should be called if the calls to the set_timezone()
 * or set_locale() may result in something different after a change
 * you made.
 *
 * For example, once a user is viewed as logged in, the
 * result changes since now we can make use of the user's data to
 * determine a timezone and locale.
 */
void locale::reset_locale()
{
    f_current_locale.clear();
    f_current_timezone.clear();
}


/** \brief Set the locale for this session.
 *
 * This function checks whether the current locale is already set. If so,
 * then the function returns false which means that we do not need to
 * run any additional signal.
 *
 * Otherwise the signal is sent to all the plugins and various plugins
 * may set the locale with the set_current_locale() function.
 *
 * \return true if the current locale is not yet set, false otherwise.
 */
bool locale::set_locale_impl()
{
    return f_current_locale.isEmpty();
}


/** \brief Set the default locale for this session.
 *
 * This function checks whether a current locale was set by the set_locale()
 * signal. If so, then it does nothing. Otherwise, it checks for the default
 * locale parameters and sets those up.
 *
 * The default locale is defined as:
 *
 * \li The user locale if the user defined such.
 * \li The website locale if the website defined such.
 * \li The internal Snap default locale (i.e. left as is).
 */
void locale::set_locale_done()
{
    // if the timezone was not defined, it is an empty string which is
    // exactly what we want to pass to the child set_timezone() function
    f_snap->set_locale(f_current_locale);
}


/** \brief Setup the timzone as required.
 *
 * This function checks whether the timezone is already set for this
 * session. If it, then it returns false and no signal is sent to the
 * other plugins.
 *
 * It is possible to reset the timezone with a call to the
 * logged_in_user_ready() function. This automatically happens
 * when a user is found to be logged in.
 *
 * \return true if the current timezone is not yet defined.
 */
bool locale::set_timezone_impl()
{
    return f_current_timezone.isEmpty();
}


/** \brief Finish up with the timezone setup.
 *
 * This function is called last, after all the other plugin set_timezone()
 * signals were called. If the current timezone is still undefined when
 * this function is called, then the locale plugin defines the default
 * timezone.
 *
 * The default timezone is:
 *
 * \li The timezone of the currently logged in user if one is defined;
 * The functions checks for a SNAP_NAME_LOCALE_TIMEZONE field in the user's
 * current revision content;
 * \li The timezone of the website if one is defined under
 * admin/locale/timezone as a field named SNAP_NAME_LOCALE_TIMEZONE
 * found in the current revision of that page.
 */
void locale::set_timezone_done()
{
    // if the timezone was not defined, it is an empty string which is
    // exactly what we want to pass to the child set_timezone() function
    f_snap->set_timezone(f_current_timezone);
}


/** \brief Convert the specified date and time to a string date.
 *
 * This function makes use of the locale to define the resulting
 * formatted date.
 *
 * The time is in seconds. The time itself is ignored except if
 * it has an effect on the date (i.e. leap year.)
 *
 * \todo
 * Save the DateFormat so if the function is called multiple times,
 * we do not have to re-create it.
 *
 * \param[in] d  The time to be presented to the end user.
 *
 * \return A string formatted as per the locale.
 */
QString locale::format_date(time_t d)
{
    std::string tz_utf8(f_current_timezone.toUtf8().data());
    UnicodeString timezone_id(tz_utf8.c_str(), tz_utf8.size(), "utf-8");
    // TODO: use ICU shared pointer
    TimeZone *tz(TimeZone::createTimeZone(timezone_id)); // TODO: verify that it took properly
    Locale l(f_current_locale.toUtf8().data());
    // TODO: use ICU shared pointer -- I get a double pointer error at this
    //       point!?
    //LocalUDateFormatPointer dt(DateFormat::createDateInstance(DateFormat::kDefault, l));
    DateFormat *dt(DateFormat::createDateInstance(DateFormat::kDefault, l));
    dt->setTimeZone(*tz);
    UDate udate(d * 1000LL);
    UnicodeString u;
    dt->format(udate, u);
    delete dt;
    delete tz;

    std::string final_date;
    u.toUTF8String(final_date);
    return QString::fromUtf8(final_date.c_str());
}


/** \brief Convert the specified date and time to a string time.
 *
 * This function makes use of the locale to define the resulting
 * formatted time.
 *
 * The time is in seconds. The date itself is ignored (i.e. only
 * the value modulo 86400 is used). The ICU library does not support
 * leap seconds here (TBD).
 *
 * \todo
 * Save the DateFormat so if the function is called multiple times,
 * we do not have to re-create it.
 *
 * \param[in] d  The time to be presented to the end user.
 *
 * \return A string formatted as per the locale.
 */
QString locale::format_time(time_t d)
{
    std::string tz_utf8(f_current_timezone.toUtf8().data());
    UnicodeString timezone_id(tz_utf8.c_str(), tz_utf8.size(), "utf-8");
    // TODO: use ICU shared pointer
    TimeZone *tz(TimeZone::createTimeZone(timezone_id)); // TODO: verify that it took properly
    Locale l(f_current_locale.toUtf8().data()); // TODO: verify that it took properly
    // TODO: use ICU shared pointer
    //LocalUDateFormatPointer dt(DateFormat::createDateInstance(DateFormat::kDefault, l));
    DateFormat *dt(DateFormat::createTimeInstance(DateFormat::kDefault, l));
    dt->setTimeZone(*tz);
    UDate udate(d * 1000LL);
    UnicodeString u;
    dt->format(udate, u);
    delete dt;
    delete tz;

    std::string final_time;
    u.toUTF8String(final_time);
    return QString::fromUtf8(final_time.c_str());
}


//
// A reference of the library ICU library can be found here:
// /usr/include/x86_64-linux-gnu/unicode/timezone.h
// file:///usr/share/doc/icu-doc/html/index.html
//
// Many territory details by Unicode.org
// http://unicode.org/repos/cldr/trunk/common/supplemental/supplementalData.xml
//

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
