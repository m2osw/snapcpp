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

#include "log.h"
#include "not_reached.h"
#include "not_used.h"
#include "process.h"

#include <QFile>
#include <QDateTime>

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
    case name_t::SNAP_NAME_ANTIVIRUS_ENABLE:
        return "antivirus::enable";

    case name_t::SNAP_NAME_ANTIVIRUS_SETTINGS_PATH:
        return "admin/settings/antivirus";

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
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the antivirus plugin.
 *
 * Ensure the antivirus object is clean before it is gone.
 */
antivirus::~antivirus()
{
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
antivirus * antivirus::instance()
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


/** \brief Return our dependencies
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString antivirus::dependencies() const
{
    return "|content|editor|output|versions|";
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

    SNAP_PLUGIN_UPDATE(2015, 11, 27, 3, 43, 45, content_update);

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


/** \brief Initialize the antivirus.
 *
 * This function terminates the initialization of the antivirus plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void antivirus::bootstrap(snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN(antivirus, "content", content::content, check_attachment_security, _1, _2, _3);
    SNAP_LISTEN(antivirus, "versions", versions::versions, versions_tools, _1);
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
 */
void antivirus::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body)
{
    // our settings pages are like any standard pages
    output::output::instance()->on_generate_main_content(ipath, page, body);
}


/** \brief Check whether the specified file is safe.
 *
 * The content plugin generates this signals twice:
 *
 * 1) once when the attachment is first uploaded and we should test quickly
 *    (fast is set to true)
 *
 * 2) a second time when the backend runs, in this case we can check the
 *    security taking as much time as required (fast is set to false)
 *
 * \param[in] file  The file to check.
 * \param[in] secure  Tells the content plugin whether the file is
 *                    considered safe or not.
 * \param[in] fast  Whether we can take our time (false) or not (true) to
 *                  verify the file.
 */
void antivirus::on_check_attachment_security(content::attachment_file const & file, content::permission_flag & secure, bool const fast)
{
    if(fast)
    {
        // TODO: add support to check some extensions / MIME types that we
        //       do not want (for example we could easily forbid .exe files
        //       from being uploaded)
        return;
    }

    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());
    content::path_info_t settings_ipath;
    settings_ipath.set_path(get_name(name_t::SNAP_NAME_ANTIVIRUS_SETTINGS_PATH));
    QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(settings_ipath.get_revision_key()));
    QtCassandra::QCassandraValue const enable_value(revision_row->cell(get_name(name_t::SNAP_NAME_ANTIVIRUS_ENABLE))->value());
    int8_t const enable(enable_value.nullValue() || enable_value.safeSignedCharValue());
    if(!enable)
    {
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

    SNAP_LOG_INFO("check filename \"")(file.get_file().get_filename())("\" for viruses.");

    // make sure the reset the temporary log file
    //
    QString const temporary_log(QString("%1/antivirus.log").arg(data_path));
    QFile in(temporary_log);
    in.remove();

    process p("antivirus::clamscan");
    p.set_mode(process::mode_t::PROCESS_MODE_INOUT);
    p.set_command("clamscan");
    p.add_argument("--tempdir=" + data_path);
    p.add_argument("--quiet");
    p.add_argument("--stdout");
    p.add_argument("--no-summary");
    p.add_argument("--infected");
    p.add_argument("--log=" + temporary_log);
    p.add_argument("-");
    p.set_input(file.get_file().get_data()); // pipe data in
    p.run();
    QString const output(p.get_output(true));

    if(!output.isEmpty())
    {
        secure.not_permitted("anti-virus: " + output);

        // if an error occurred, also convert the logs
        //
        if(in.open(QIODevice::ReadOnly))
        {
            QFile out(QString("%1/antivirus.log").arg(log_path));
            if(out.open(QIODevice::Append))
            {
                // TODO: convert to use our logger?
                QString const timestamp(QDateTime::currentDateTimeUtc().toString("MM/dd/yyyy hh:mm:ss antivirus: "));
                QByteArray const timestamp_buf(timestamp.toUtf8());
                char buf[1024];
                for(;;)
                {
                    int const r(in.readLine(buf, sizeof(buf)));
                    if(r <= 0)
                    {
                        break;
                    }
                    if(strcmp(buf, "\n") == 0)
                    {
                        continue;
                    }
                    for(char const * s(buf); *s == '-'; ++s)
                    {
                        if(*s != '\n' && *s != '-')
                        {
                            // write lines that are not just '-'
                            out.write(timestamp_buf.data(), timestamp_buf.size());
                            out.write(buf, r);
                            break;
                        }
                    }
                }
            }
        }
    }
}


/** \brief Show the version of clamscan.
 *
 * The antivirus currently makes use of clamscan. This signal
 * adds the version of that tool to the specified token.
 *
 * \param[in] token  The token where the version is added.
 */
void antivirus::on_versions_tools(filter::filter::token_info_t & token)
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
