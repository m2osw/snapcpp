// Snap Websites Server -- check uploaded files for viruses
// Copyright (C) 2014-2015  Made to Order Software Corp.
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

#include "antivirus.h"

#include "../output/output.h"

#include "process.h"
#include "not_reached.h"
#include "not_used.h"

#include "poison.h"


SNAP_PLUGIN_START(antivirus, 1, 0)


/** \brief Get a fixed antivirus name.
 *
 * The antivirus plugin makes use of different names in the database. This
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
    case name_t::SNAP_NAME_ANTIVIRUS_SCAN_ARCHIVE:
        return "antivirus::scan_archive";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_ANTIVIRUS_...");

    }
    NOTREACHED();
}


/** \class antivirus
 * \brief Check uploaded files for virus infections.
 *
 * This plugin runs clamav against uploaded files to verify whether these
 * are viruses or not. If a file is found to be a virus, it is then marked
 * as not secure and download of the file are prevented.
 */


/** \brief Initialize the antivirus plugin.
 *
 * This function is used to initialize the antivirus plugin object.
 */
antivirus::antivirus()
    //: f_snap(NULL) -- auto-init
{
}

/** \brief Clean up the antivirus plugin.
 *
 * Ensure the antivirus object is clean before it is gone.
 */
antivirus::~antivirus()
{
}

/** \brief Initialize the antivirus.
 *
 * This function terminates the initialization of the antivirus plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void antivirus::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(antivirus, "content", content::content, check_attachment_security, _1, _2, _3);
    SNAP_LISTEN(antivirus, "versions", versions::versions, versions_tools, _1);
}

/** \brief Get a pointer to the antivirus plugin.
 *
 * This function returns an instance pointer to the antivirus plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the antivirus plugin.
 */
antivirus *antivirus::instance()
{
    return g_plugin_antivirus_factory.instance();
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
QString antivirus::description() const
{
    return "The anti-virus plugin is used to verify that a file is not a"
        " virus. When a file that a user uploaded is found to be a virus"
        " this plugin marks that file as unsecure and the file cannot be"
        " downloaded by end users.";
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
int64_t antivirus::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our antivirus references.
 *
 * Send our antivirus to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void antivirus::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);
    content::content::instance()->add_xml(get_plugin_name());
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
 * Note that this is NOT the HTML output. It is the \<page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The path to a template page in case cpath is not defined.
 */
void antivirus::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    // our settings pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body, ctemplate);
}


void antivirus::on_check_attachment_security(content::attachment_file const& file, content::permission_flag& secure, bool const fast)
{
    if(fast)
    {
        // TODO: add support to check some extensions / MIME types that we
        //       do not want (for example we could easily forbid .exe files
        //       from being uploaded)
        return;
    }

    // slow test, here we check whether the file is a virus
    QString data_path(f_snap->get_server_parameter("data_path"));
    if(data_path.isEmpty())
    {
        // a default that works, not that /tmp is not considered secure
        // although this backend should be running on a computer that is
        // not shared between users
        data_path = "/tmp";
    }
    QString log_path(f_snap->get_server_parameter("log_path"));
    if(log_path.isEmpty())
    {
        // a default that works, not that /tmp is not considered secure
        // although this backend should be running on a computer that is
        // not shared between users
        log_path = "/var/log/snapwebsites";
    }

    process p("antivirus::clamscan");
    p.set_mode(process::mode_t::PROCESS_MODE_INOUT);
    p.set_command("clamscan");
    p.add_argument("--tempdir=" + data_path);
    p.add_argument("--quiet");
    p.add_argument("--stdout");
    p.add_argument("--no-summary");
    p.add_argument("--infected");
    p.add_argument("--log=" + log_path + "/antivirus.log");
    p.add_argument("-");
    p.set_input(file.get_file().get_data()); // pipe data in
    p.run();
    QString const output(p.get_output(true));

    if(!output.isEmpty())
    {
        secure.not_permitted("anti-virus: " + output);
    }
}


void antivirus::on_versions_tools(filter::filter::token_info_t& token)
{
    process p("antivirus::clamscan-version");
    p.set_mode(process::mode_t::PROCESS_MODE_OUTPUT);
    p.set_command("clamscan");
    p.add_argument("--version");
    p.run();
    QString const output(p.get_output(true));

    token.f_replacement += "<li>";
    token.f_replacement += output;
    token.f_replacement += "</li>";
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
