// Snap Websites Server -- APT watchdog: record apt-check results
// Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved.
//
// https://snapwebsites.org/project/sitter
// contact@m2osw.com
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


// self
//
#include    "apt.h"


// snapwebsites
//
#include    <snapwebsites/qdomhelpers.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>
#include    <snapdev/not_reached.h>


// last include
//
#include    <snapdev/poison.h>



namespace sitter
{
namespace apt
{


CPPTHREAD_PLUGIN_START(apt, 1, 0)
    , ::cppthread::plugin_description(
            "Check the apt-check results. If an update is available, it"
            " will show up as a low priority \"error\" unless it is marked"
            " as a security upgrade.")
    , ::cppthread::plugin_dependency("server")
    , ::cppthread::plugin_help_uri("https://snapwebsites.org/help")
    , ::cppthread::plugin_categorization_tag("packages")
CPPTHREAD_PLUGIN_END()



/** \brief Get a fixed apt plugin name.
 *
 * The apt plugin makes use of different fixed names. This function
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
    case name_t::SNAP_NAME_WATCHDOG_APT_NAME:
        return "name";

    default:
        // invalid index
        throw apt_logic_error("Invalid SNAP_NAME_SITTER_APT_...");

    }
    snapdev::NOT_REACHED();
}



/** \brief Check whether updates are necessary.
 *
 * This function is ignored in the watchdog.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t apt::do_update(int64_t last_updated)
{
    snapdev::NOT_USED(last_updated);

    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Initialize apt.
 *
 * This function terminates the initialization of the apt plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void apt::bootstrap(void * snap)
{
    f_snap = static_cast<void *>(snap);

    SNAP_LISTEN(apt, "server", watchdog_server, process_watch, boost::placeholders::_1);
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void apt::on_process_watch(QDomDocument doc)
{
    SNAP_LOG_DEBUG
        << "apt::on_process_watch(): processing"
        << SNAP_LOG_SEND;

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "apt"));

    // get path to apt-check file
    //
    // first use the default path
    //
    QString manager_cache_path("/var/cache/snapwebsites");

    // then check whether the user changed that default in the configuration
    // files
    //
    advgetopt::conf_file_setup setup_config("snapmanager");
    advgetopt::conf_file::pointer_t manager_config(advgetopt::conf_file::get_conf_file(setup_config));
    if(manager_config->has_parameter("cache_path"))
    {
        manager_cache_path = QString::fromUtf8(manager_config->get_parameter("cache_path").c_str());
    }

    // now define the name we use to save the apt-check output
    //
    QString const apt_check_output(QString("%1/apt-check.output").arg(manager_cache_path));

    // load the apt-check file
    //
    QFile apt_check(apt_check_output);
    if(apt_check.open(QIODevice::ReadOnly))
    {
        QByteArray const output(apt_check.readAll());
        apt_check.close();

        QString const content(QString::fromUtf8(output).trimmed());
        if(content == "-1")
        {
            QString const err_msg(QString("we are unable to check whether updates are available (`apt-check` was not found)").arg(apt_check_output));
            e.setAttribute("error", err_msg);
            f_snap->append_error(doc, "apt", err_msg, 98);
            return;
        }

        snap::snap_string_list const counts(content.split(";"));
        if(counts.size() == 3)
        {
            time_t const now(time(nullptr));
            time_t const cached_on(counts[0].toLongLong());

            // save the date when it was last updated
            //
            e.setAttribute("last-updated", static_cast<qlonglong>(cached_on));

            // out of date tested with a +1h because it could take a little
            // while to check for new update and the date here is not updated
            // while that happens
            //
            if(cached_on + 86400 + 60 * 60 >= now)
            {
                // cache is still considered valid
                //
                if(counts[1] == "0")
                {
                    // nothing needs to be upgraded
                    //
                    return;
                }
                // counts[1] packages can be upgraded
                // counts[2] are security upgrades

                e.setAttribute("total-updates", counts[1]);
                e.setAttribute("security-updates", counts[2]);

                // the following generates an "error" with a low priority
                // (under 50) in case a regular set of files can be upgraded
                // and 52  when there are security updates
                //
                int priority(0);
                QString err_msg;
                if(counts[2] != "0")
                {
                    priority = 52;
                    err_msg = "there are packages including security updates that need to be upgraded on this system.";
                }
                else
                {
                    priority = 45;
                    err_msg = "there are standard package updates that can be upgraded now on this system.";
                }
                e.setAttribute("error", err_msg);
                f_snap->append_error(doc, "apt", err_msg, priority);
                return;
            }
            else
            {
                QString const err_msg(QString("\"%1\" file is out of date, the snapmanagerdaemon did not update it for more than a day").arg(apt_check_output));
                e.setAttribute("error", err_msg);
                f_snap->append_error(doc, "apt", err_msg, 50);
                return;
            }
        }
        else
        {
            // low priority (15): the problem is here but we don't tell the
            //                    admin unless another high level error occurs
            //
            QString const err_msg(QString("could not figure out the contents of \"%1\", snapmanagerdaemon may have changed the format since we wrote the snapwatchdog apt plugin.").arg(apt_check_output));
            e.setAttribute("error", err_msg);
            f_snap->append_error(doc, "apt", err_msg, 15);
            return;
        }
    }
    else
    {
        // when not present, we want to generate an error because that
        // could mean something is wrong on that system, but we make it
        // a low priority for a while (i.e. hitting the Reset button
        // in the snapmanager.cgi interface deletes that file!)
        //
        QString const err_msg(QString("\"%1\" file is missing, snapwatchdog is not getting APT status updates from snapmanagerdaemon").arg(apt_check_output));
        e.setAttribute("error", err_msg);
        f_snap->append_error(doc, "apt", err_msg, 20);
        return;
    }
}



} // namespace apt
} // namespace sitter
// vim: ts=4 sw=4 et
