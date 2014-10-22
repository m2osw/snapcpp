// Snap Websites Server -- watchdog processes
// Copyright (C) 2013-2014  Made to Order Software Corp.
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

#include "processes.h"

#include "snapwatchdog.h"

#include <snapwebsites/process.h>
#include <snapwebsites/qdomhelpers.h>

#include "poison.h"


SNAP_PLUGIN_START(processes, 1, 0)




/** \brief Get a fixed processes plugin name.
 *
 * The processes plugin makes use of different names. This function ensures
 * that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_WATCHDOG_PROCESSES:
        return "watchdog_processes";

    default:
        // invalid index
        throw snap_logic_exception("Invalid SNAP_NAME_WATCHDOG_PROCESSES_...");

    }
    NOTREACHED();
}




/** \brief Initialize the processes plugin.
 *
 * This function is used to initialize the processes plugin object.
 */
processes::processes()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Clean up the processes plugin.
 *
 * Ensure the processes object is clean before it is gone.
 */
processes::~processes()
{
}


/** \brief Initialize processes.
 *
 * This function terminates the initialization of the processes plugin
 * by registering for various events.
 *
 * \param[in] snap  The child handling this request.
 */
void processes::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(processes, "server", watchdog_server, process_watch, _1);
}


/** \brief Get a pointer to the processes plugin.
 *
 * This function returns an instance pointer to the processes plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the processes plugin.
 */
processes *processes::instance()
{
    return g_plugin_processes_factory.instance();
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
QString processes::description() const
{
    return "Check whether a set of processes are running.";
}


/** \brief Check whether updates are necessary.
 *
 * This function is ignored in the watchdog.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t processes::do_update(int64_t last_updated)
{
    static_cast<void>(last_updated);
    SNAP_PLUGIN_UPDATE_INIT();
    // no updating in watchdog
    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Process this watchdog data.
 *
 * This function runs this watchdog.
 *
 * \param[in] doc  The document.
 */
void processes::on_process_watch(QDomDocument doc)
{
std::cerr << "starting on processes...\n";
    QString process_names(f_snap->get_server_parameter(get_name(SNAP_NAME_WATCHDOG_PROCESSES)));
    if(process_names.isEmpty())
    {
        return;
    }

    QDomElement parent(snap_dom::create_element(doc, "watchdog"));
    QDomElement e(snap_dom::create_element(parent, "processes"));

    QStringList name_list(process_names.split(','));
    QVector<QSharedPointer<QRegExp> > re_names;
    {
        int const max_names(name_list.count());
        for(int idx(0); idx < max_names; ++idx)
        {
std::cerr << "add regex [" << name_list[idx] << "]\n";
            re_names.push_back(QSharedPointer<QRegExp>(new QRegExp(name_list[idx])));
        }
    }

    process_list list;

    list.set_field(process_list::field_t::COMMAND_LINE);
    list.set_field(process_list::field_t::STATISTICS);
    while(!re_names.isEmpty())
    {
        process_list::proc_info_pointer_t info(list.next());
        if(!info)
        {
            // some process(es) missing?
            int const max_re(re_names.count());
            for(int j(0); j < max_re; ++j)
            {
                QDomElement proc(doc.createElement("process"));
                e.appendChild(proc);

                proc.setAttribute("name", re_names[j]->pattern());
                proc.setAttribute("error", "missing");
            }
            break;
        }
        std::string name(info->get_process_name());
        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            name = name.substr(p + 1);
        }
        QString utf8_name;
        utf8_name = QString::fromUtf8(name.c_str());

        QString cmdline(utf8_name);
        int count_max(info->get_args_size());
        for(int c(0); c < count_max; ++c)
        {
            // is it Cassandra?
            if(info->get_arg(c) != "")
            {
                cmdline += " ";
                QString arg;
                arg = QString::fromUtf8(info->get_arg(c).c_str());
                cmdline += arg;
            }
        }
        int const max_re(re_names.count());
std::cerr << "check process [" << name << "] -> [" << cmdline << "]\n";
        for(int j(0); j < max_re; ++j)
        {
            if(re_names[j]->indexIn(cmdline) != -1)
            {
                // remove from the list, if the list is empty, we are
                // done; if the list is not empty by the time we return
                // some processes are missing
                re_names.remove(j);

                QDomElement proc(doc.createElement("process"));
                e.appendChild(proc);

                proc.setAttribute("name", utf8_name);

                proc.setAttribute("pcpu", QString("%1").arg(info->get_pcpu()));
                proc.setAttribute("total_size", QString("%1").arg(info->get_total_size()));
                proc.setAttribute("resident", QString("%1").arg(info->get_resident_size()));
                proc.setAttribute("tty", QString("%1").arg(info->get_tty()));

                unsigned long long utime;
                unsigned long long stime;
                unsigned long long cutime;
                unsigned long long cstime;
                info->get_times(utime, stime, cutime, cstime);

                proc.setAttribute("utime", QString("%1").arg(utime));
                proc.setAttribute("stime", QString("%1").arg(stime));
                proc.setAttribute("cutime", QString("%1").arg(cutime));
                proc.setAttribute("cstime", QString("%1").arg(cstime));
                break;
            }
        }
    }
}





SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
