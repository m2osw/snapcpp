// Snap Websites Server -- manage types
// Copyright (C) 2012  Made to Order Software Corp.
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

#include "taxonomy.h"
#include "../content/content.h"
#include "not_reached.h"
#include <iostream>
#include <QtCore/QDebug>


SNAP_PLUGIN_START(taxonomy, 1, 0)



/** \brief Get a fixed taxnomy name.
 *
 * The taxnomy plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
	switch(name)
	{
	case SNAP_NAME_TAXONOMY_NAME:
		return "taxonomy::name";

	default:
		// invalid index
		throw snap_exception();

	}
	NOTREACHED();
}


/** \brief Initialize the taxonomy plugin.
 *
 * This function is used to initialize the taxonomy plugin object.
 */
taxonomy::taxonomy()
	//: f_snap(NULL) -- auto-init
{
}

/** \brief Clean up the taxonomy plugin.
 *
 * Ensure the taxonomy object is clean before it is gone.
 */
taxonomy::~taxonomy()
{
}

/** \brief Initialize the taxonomy plugin.
 *
 * This function terminates the initialization of the taxonomy plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void taxonomy::on_bootstrap(::snap::snap_child *snap)
{
	f_snap = snap;
}

/** \brief Get a pointer to the taxonomy plugin.
 *
 * This function returns an instance pointer to the taxonomy plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the taxonomy plugin.
 */
taxonomy *taxonomy::instance()
{
	return g_plugin_taxonomy_factory.instance();
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
QString taxonomy::description() const
{
	return "This plugin manages the different types on your website."
		" Types include categories, tags, permissions, etc."
		" Some of these types are locked so the system continues to"
		" work, however, all can be edited by the user in some way.";
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
int64_t taxonomy::do_update(int64_t last_updated)
{
	SNAP_PLUGIN_UPDATE_INIT();

	SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
	SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, content_update);

	SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the taxonomy plugin.
 *
 * This function is the first update for the taxonomy plugin. It installs
 * the initial data required by the taxonomy plugin.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                                 to the database by this update
 *                                 (in micro-seconds).
 */
void taxonomy::initial_update(int64_t variables_timestamp)
{
}

/** \brief Update the taxonomy plugin content.
 *
 * This function updates the contents in the database using the
 * system update settings found in the resources.
 *
 * This file, contrary to most content files, makes heavy use
 * of the overwrite flag to make sure that the basic system
 * types are and stay defined as expected.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void taxonomy::content_update(int64_t variables_timestamp)
{
	content::content::instance()->add_xml("taxonomy");
}

/** \brief Search for a field in a type tree.
 *
 * This function checks for the \p col_name field in the specified type
 * and up checking each parent up to and including the parent as specified
 * by the \p limit_name column name.
 *
 * If the limit is not found, then an error is generated because it should
 * always exist (i.e. be a system type that the user cannot edit.)
 *
 * \param[in] cpath  The content where we start.
 * \param[in] taxonomy_name  The name of the link to the taxonomy to use for the search.
 * \param[in] col_name  The name of the column to search.
 * \param[in] limit_name  The title at which was stop the search.
 *
 * \return The value found in the Cassandra database.
 */
QtCassandra::QCassandraValue taxonomy::find_type_with(const QString& cpath, const QString& taxonomy_name, const QString& col_name, const QString& limit_name)
{
	QtCassandra::QCassandraValue not_found;
	QString site_key(f_snap->get_site_key_with_slash());
	QString content_key(site_key + cpath);
	// get link taxonomy_name from cpath
	links::link_info type_info(taxonomy_name, true, content_key);
	QSharedPointer<links::link_context> type_ctxt(links::links::instance()->new_link_context(type_info));
	links::link_info link_type;
	if(!type_ctxt->next_link(link_type))
	{
		// this should never happen because we should always have a parent
		// up until limit_name is found
		return not_found;
	}
	QString type_key(link_type.key());

	//QtCassandra::QCassandraValue type_path(content::content::instance()->get_content_table()->row(content_key)->cell(taxonomy_name)->value());
	//if(type_path.nullValue())
	if(type_key.isEmpty())
	{
		return not_found;
	}
	//QString type_key(site_key + type_path.stringValue());
	for(;;)
	{
		if(!content::content::instance()->get_content_table()->exists(type_key))
		{
			// TODO: should this be an error instead? all the types should exist!
			return not_found;
		}
		// check for the key, if it exists we found what the user is
		// looking for!
		QtCassandra::QCassandraValue result(content::content::instance()->get_content_table()->row(type_key)->cell(col_name)->value());
		if(!result.nullValue())
		{
			return result;
		}
		// have we reached the limit
		QtCassandra::QCassandraValue limit(content::content::instance()->get_content_table()->row(type_key)->cell(QString(get_name(SNAP_NAME_TAXONOMY_NAME)))->value());
		if(!limit.nullValue() && limit.stringValue() == limit_name)
		{
			// we reached the limit and have not found a result
			return not_found;
		}
		// get the parent
		links::link_info info("parent", true, type_key);
		QSharedPointer<links::link_context> ctxt(links::links::instance()->new_link_context(info));
		links::link_info link_info;
		if(!ctxt->next_link(link_info))
		{
			// this should never happen because we should always have a parent
			// up until limit_name is found
			return not_found;
		}
		type_key = link_info.key();
	}
	NOTREACHED();
}


bool taxonomy::on_path_execute(const QString& path)
{
	f_snap->output(layout::layout::instance()->apply_layout(path, this));

	return true;
}


void taxonomy::on_generate_main_content(layout::layout *l, const QString& cpath, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
	// a type is just like a regular page
	content::content::instance()->on_generate_main_content(l, cpath, page, body, ctemplate);
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4
