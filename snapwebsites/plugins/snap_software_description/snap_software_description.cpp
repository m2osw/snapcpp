// Snap Websites Server -- Snap Software Description handling
// Copyright (C) 2012-2015  Made to Order Software Corp.
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

/** \file
 * \brief Snap Software Description plugin.
 *
 * This plugin manages Snap Software Descriptions. This means it lets you
 * enter software descriptions, including links, logos, licenses, fees,
 * etc. and then transforms that data to XML and makes those files available
 * to the world to see.
 *
 * This is a complete redesign from the PAD File XML format which is really
 * weak and exclusively designed for Microsoft windows executables (even if
 * you can say Linux in there, the format is a one to one match with the
 * Microsoft environment and as such has many limitations.)
 *
 * The format is described on snapwebsites.org:
 * http://snapwebsites.org/implementation/feature-requirements/pad-and-snsd-files-feature/snap-software-description
 */

#include "snap_software_description.h"

#include "../filter/filter.h"
#include "../list/list.h"
#include "../shorturl/shorturl.h"

#include "http_strings.h"
#include "log.h"
#include "not_used.h"
#include "qdomhelpers.h"

#include <QFile>

#include "poison.h"


SNAP_PLUGIN_START(snap_software_description, 1, 0)



/** \brief Get a fixed snap_software_description plugin name.
 *
 * The snap_software_description plugin makes use of different names
 * in the database. This function ensures that you get the right
 * spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char * get_name(name_t name)
{
    switch(name)
    {
    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_ENABLE:
        return "snap_software_description::enable";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_MAX_FILES:
        return "snap_software_description::max_files";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_PATH:
        return "admin/settings/snap-software-description";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_END_MARKER:
        return "snap_software_description::teaser_end_marker";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_TAGS:
        return "snap_software_description::teaser_tags";

    case name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_WORDS:
        return "snap_software_description::teaser_words";

    default:
        // invalid index
        throw snap_logic_exception("invalid name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_...");

    }
    NOTREACHED();
}


/** \class snap_software_description
 * \brief The snap_software_description plugin handles application authentication.
 *
 * Any Snap! website can be setup to accept application authentication.
 *
 * The website generates a token that can be used to log you in.
 */


/** \brief Initialize the snap_software_description plugin.
 *
 * This function initializes the snap_software_description plugin.
 */
snap_software_description::snap_software_description()
    //: f_snap(nullptr) -- auto-init
{
}

/** \brief Destroy the snap_software_description plugin.
 *
 * This function cleans up the snap_software_description plugin.
 */
snap_software_description::~snap_software_description()
{
}


/** \brief Get a pointer to the snap_software_description plugin.
 *
 * This function returns an instance pointer to the snap_software_descriptiosnap_software_descriptionin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the snap_software_description plugin.
 */
snap_software_description * snap_software_description::instance()
{
    return g_plugin_snap_software_description_factory.instance();
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
QString snap_software_description::description() const
{
    return "The Snap Software Description plugin offers you a way to"
          " define a set of descriptions for software that you are offering"
          " for download on your website. The software may be free or for"
          " a fee. It may also be a shareware.";
}


/** \brief Return our dependencies.
 *
 * This function builds the list of plugins (by name) that are considered
 * dependencies (required by this plugin.)
 *
 * \return Our list of dependencies.
 */
QString snap_software_description::dependencies() const
{
    return "|editor|layout|output|path|";
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
 *                          updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t snap_software_description::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2015, 1, 23, 13, 39, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the snap_software_description plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void snap_software_description::content_update(int64_t variables_timestamp)
{
    NOTUSED(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Bootstrap the snap_software_description.
 *
 * This function adds the events the snap_software_description plugin is listening for.
 *
 * \param[in] snap  The child handling this request.
 */
void snap_software_description::bootstrap(::snap::snap_child * snap)
{
    f_snap = snap;

    SNAP_LISTEN0(snap_software_description, "server", server, backend_process);
    SNAP_LISTEN(snap_software_description, "robotstxt", robotstxt::robotstxt, generate_robotstxt, _1);
    SNAP_LISTEN(snap_software_description, "shorturl", shorturl::shorturl, allow_shorturl, _1, _2, _3, _4);
}


/** \brief Implementation of the robotstxt signal.
 *
 * This function adds the Snap Software Description field to the
 * robotstxt file as a global field. (i.e. you are expected to
 * have only one Snap Software Description root file per website.)
 *
 * \param[in] r  The robotstxt object.
 */
void snap_software_description::on_generate_robotstxt(robotstxt::robotstxt * r)
{
    r->add_robots_txt_field(f_snap->get_site_key_with_slash() + "types/snap-websites-description.xml", "Snap-Websites-Description", "", true);
}


/** \brief Prevent short URL on snap-software-description.xml files.
 *
 * snap-software-description.xml and any other file generated by this
 * plugin really do not need a short URL so we prevent those on such paths.
 *
 * \param[in,out] ipath  The path being checked.
 * \param[in] owner  The plugin that owns that page.
 * \param[in] type  The type of this page.
 * \param[in,out] allow  Whether the short URL is allowed.
 */
void snap_software_description::on_allow_shorturl(content::path_info_t & ipath, QString const & owner, QString const & type, bool & allow)
{
    NOTUSED(owner);
    NOTUSED(type);

    if(!allow)
    {
        // already forbidden, cut short
        return;
    }

    //
    // all our files do not need a short URL definition
    //
    QString const cpath(ipath.get_cpath());
    if(cpath.startsWith("types/snap-software-description") && cpath.endsWith(".xml"))
    {
        allow = false;
    }
}


/** \brief Implementation of the backend process signal.
 *
 * This function captures the backend processing signal which is sent
 * by the server whenever the backend tool is run against a site.
 *
 * The backend processing of the Snap Software Description plugin
 * generates all the XML files somehow linked to the Snap Software
 * Description plugin.
 *
 * The files include tags as described in the documentation:
 * http://snapwebsites.org/implementation/feature-requirements/pad-and-snsd-files-feature/snap-software-description
 *
 * The backend processing is done with multiple levels as in:
 *
 * \li start with the root, which is defined as files directly
 *     linked to ".../types/snap-software-description", and
 *     categories: types defined under
 *     ".../types/snap-software-description/...".
 * \li as we find files, create their respective XML files.
 * \li repeat the process with each category; defining sub-categories.
 * \li repeat the process with sub-categories; defining sub-sub-categories.
 *
 * We start at sub-sub-categories (3 levels) because there is generally
 * no reason to go further. The category tree is probably not that well
 * defined for everyone where sub-sub-sub-categories would become useful.
 */
void snap_software_description::on_backend_process()
{
    SNAP_LOG_TRACE("snap_software_description::on_backend_process(): process snap-software-description.xml content.");

    content::content * content_plugin(content::content::instance());
    //QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(content_plugin->get_revision_table());

    content::path_info_t snap_software_description_settings_ipath;
    snap_software_description_settings_ipath.set_path(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_PATH));
    f_snap_software_description_settings_row = revision_table->row(snap_software_description_settings_ipath.get_revision_key());

    content::path_info_t ipath;
    ipath.set_path("/types/snap-software-description");

    int depth(0);
    create_catalog(ipath, depth);

    // reset the main URI
    f_snap->set_uri_path("/");
}


/** \brief Create a catalog.
 *
 * This function is called recursively to create all catalog files
 * for all categories. Note that if a category is considered empty,
 * then it does not get created.
 *
 * The root catalog is saved in /types/snap-software-description
 * with the .xml extension. The other catalogs are saved under
 * each category found under /types/snap-software-description.
 *
 * The software specific XML files are created on various pages
 * throughout the website, but never under /types/snap-software
 * description.
 *
 * The function calls itself as it finds children representing
 * categories, which have to have a catalog. The function takes
 * a depth parameter, which allows it to avoid going too deep
 * in that matter. We actually only allow three levels of
 * categorization. After the third level, we ignore further
 * children.
 *
 * The interface is aware of the maximum number of categorization
 * levels and thus prevents end users from creating more than
 * the allowed number of levels.
 *
 * Note that the maximum number of level is purely for our own
 * sake since there are no real limits to the categorization
 * of a software.
 *
 * The software makes use of the list plugin to create its own
 * lists since the list plugin can do all the work to determine
 * what page is linked with what type, whether the page is
 * publicly available, verify that the page was not deleted,
 * etc. However, a page can only support one list, so it
 * supports the list of files and nothing about the categories.
 * In other words, we are still responsible for the categories.
 *
 * The list saves an item count. We use that parameter to know
 * whether to include a category in our XML files or not. However,
 * the top snap-software-description.xml file is always created.
 *
 * \param[in] ipath  The path of the category to work on.
 * \param[in] depth  The depth at which we currently are working.
 */
void snap_software_description::create_catalog(content::path_info_t & ipath, int const depth)
{
    list::list * list_plugin(list::list::instance());
    path::path * path_plugin(path::path::instance());
    layout::layout * layout_plugin(layout::layout::instance());

    // The PAD file format offered several descriptions, I'm not so
    // sure we want to have 4 like them... for now, we'd have two:
    // the teaser and the main description
    //
    filter::filter::filter_teaser_info_t teaser_info;
    teaser_info.set_max_words (f_snap_software_description_settings_row->cell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_WORDS     ))->value().safeInt64Value(0, 200));
    teaser_info.set_max_tags  (f_snap_software_description_settings_row->cell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_TAGS      ))->value().safeInt64Value(0, 100));
    teaser_info.set_end_marker(f_snap_software_description_settings_row->cell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_TEASER_END_MARKER))->value().stringValue());

    // already loaded?
    if(f_snap_software_description_parser_xsl.isEmpty())
    {
        QFile file(":/xsl/layout/snap-software-description-parser.xsl");
        if(!file.open(QIODevice::ReadOnly))
        {
            SNAP_LOG_FATAL("snap_software_description::create_catalog() could not open the snap-software-description-parser.xsl resource file.");
            return;
        }
        QByteArray data(file.readAll());
        f_snap_software_description_parser_xsl = QString::fromUtf8(data.data(), data.size());
        if(f_snap_software_description_parser_xsl.isEmpty())
        {
            SNAP_LOG_FATAL("snap_software_description::create_catalog() could not read the snap-software-description-parser.xsl resource file.");
            return;
        }

        // replace <xsl:include ...> with other XSLT files (should be done
        // by the parser, but Qt's parser does not support it yet)
        layout_plugin->replace_includes(f_snap_software_description_parser_xsl);
    }

    QDomDocument result;

    // TODO: this is not correct for this implementation
    bool first(false);

    int const max_files(f_snap_software_description_settings_row->cell(get_name(name_t::SNAP_NAME_SNAP_SOFTWARE_DESCRIPTION_SETTINGS_MAX_FILES))->value().safeInt64Value(0, 1000));
    list::list_item_vector_t list(list_plugin->read_list(ipath, 0, max_files));
    int const max_items(list.size());
    for(int idx(0); idx < max_items; ++idx)
    {
        content::path_info_t page_ipath;
        page_ipath.set_path(list[idx].get_uri());

        // only pages that can be handled by layouts are added
        // others are silently ignored (note that only broken
        // pages should fail the following test)
        //
        quiet_error_callback snap_software_description_error_callback(f_snap, true);
        plugins::plugin * layout_ready(path_plugin->get_plugin(page_ipath, snap_software_description_error_callback));
        layout::layout_content * layout_ptr(dynamic_cast<layout::layout_content *>(layout_ready));
        if(!layout_ptr)
        {
            // log the error?
            // this is probably not the role of the snap-software-description
            // implementation...
            //
            continue;
        }

        // since we are a backend, the main ipath remains equal
        // to the home page and that is what gets used to generate
        // the path to each page in the feed data so we have to
        // change it before we apply the layout
        f_snap->set_uri_path(QString("/%1").arg(page_ipath.get_cpath()));

        QDomDocument doc(layout_plugin->create_document(page_ipath, layout_ready));
        layout_plugin->create_body(doc, page_ipath, f_snap_software_description_parser_xsl, layout_ptr, false, "feed-parser");

        // generate the teaser
        if(teaser_info.get_max_words() > 0)
        {
            QDomElement output_description(snap_dom::get_child_element(doc, "snap/page/body/output/description"));
            // do not create a link, often those are removed in some
            // weird way; readers will make the title a link anyway
            //teaser_info.set_end_marker_uri(page_ipath.get_key(), "Click to read the full article.");
            filter::filter::body_to_teaser(output_description, teaser_info);
        }

        if(first)
        {
            first = false;
            result = doc;
        }
        else
        {
            // only keep the output of further pages
            // (the header should be the same, except for a few things
            // such as the path and data extracted from the main page,
            // which should not be used in the feed...)
            QDomElement output(snap_dom::get_child_element(doc, "snap/page/body/output"));
            QDomElement body(snap_dom::get_child_element(result, "snap/page/body"));
            body.appendChild(output);
        }
    }

    // save the resulting XML document

    // if we already are pretty deep, stop here
    if(depth >= 5)
    {
        return;
    }
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
