// Snap Websites Server -- manage double links
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

#include "links.h"
#include "../path/path.h"
#include "../content/content.h"
#include "not_reached.h"
#include <iostream>
#include <QtCore/QDebug>


SNAP_PLUGIN_START(links, 1, 0)

/** \brief Get a fixed links name.
 *
 * The links plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
	switch(name) {
	case SNAP_NAME_LINKS_TABLE: // sorted index of links
		return "links";

	case SNAP_NAME_LINKS_NAMESPACE:
		return "links";

	default:
		// invalid index
		throw snap_exception();

	}
	NOTREACHED();
}

/** \fn link_info::link_info(const QString new_name, bool unique, const QString new_key);
 * \brief Create a link descriptor.
 *
 * Initialize the link information with a name and a key. See the set_name() and
 * set_key() descriptions for more information.
 *
 * Note that a key and a name are ultimately necessary. If not defined on
 * creation then you must call the set_name() and set_key() functions later
 * but before making use of the link_info object.
 *
 * \param[in] new_name  The name of the key (column.)
 * \param[in] unique  Whether the link is unique (one to one, or one to many.)
 * \param[in] new_key  The key is the name of the row.
 *
 * \sa set_name()
 * \sa set_key()
 */

/** \fn void link_info::set_name(QString& new_name, bool unique);
 * \brief Set the name of the column to use for the link.
 *
 * The name is used to distinguish the different links used within a row.
 * The name must include the plugin name (i.e. filter::category).
 *
 * By default a link is expected to be: many to many or many to one. The
 * unique flag can be used to transform it to: one to many or one to one
 * link.
 *
 * The name of the columns is set to include a number when unique is false.
 * The gives us a many to many or many to one link capability:
 *
 *   "links::<name>-<number>"
 *
 * When the unique flag is set to true, the name of the column does not
 * include a number:
 *
 *   "links::<name>"
 *
 * \param[in] new_name  The name of the link used as the column name.
 * \param[in] unique  The unique flag, if true it means 'one', of false, it manes 'many'
 *
 * \sa name()
 * \sa is_unique()
 */

/** \fn void link_info::set_key(QString& new_key)
 * \brief Set the key (row name) where the link is to be saved.
 *
 * This function saves the key where the link is to be saved.
 * The key actually represents the exact name of the row where the link is
 * saved.
 *
 * The destination (i.e the data of the link) is defined from another link_info
 * (i.e. the on_create_link() uses source (src) and destination (dst)
 * parameters which are both link_info.)
 * 
 * \param[in] new_key  The key to the row where the link is to be saved.
 *
 * \sa key()
 */

/** \fn bool link_info::is_unique() const
 * \brief Check whether this link is marked as unique.
 *
 * The function returns the current value of the unique flag as set on
 * construction or with the set_name() function.
 *
 * \return true if the link is unique (one to many or one to one)
 *
 * \sa set_name()
 */

/** \fn const QString& link_info::name() const
 * \brief Retrieve the name of the link.
 *
 * The function returns the name the link as set on construction
 * or with the set_name() function.
 *
 * \return the name of the link that is used to form the name of the column
 *
 * \sa set_name()
 */

/** \fn const QString& link_info::key() const
 * \brief Retrieve the key of the link.
 *
 * The function returns the key for the link as set on construction
 * or with the set_name() function.
 *
 * \return the key of the link that is used as the row key
 *
 * \sa set_key()
 */

/** \var controlled_vars::zbool_t link_info::f_unique;
 * \brief Unique (one) or not (many) links.
 *
 * This flag is used to save whether the link is unique or not.
 *
 * \sa set_name()
 */

/** \var QString link_info::f_name;
 * \brief The name of the column.
 *
 * The name of the column used for the link.
 *
 * \sa set_name()
 */

/** \var QString link_info::f_key;
 * \brief The key of this link.
 *
 * The key of a link is the key of the row where the link is to be
 * saved.
 *
 * \sa set_key()
 */

/** \brief Verify that the name is valid.
 *
 * Because the links system makes use of the name in a certain way, we
 * want to ensure that the name is valid.
 *
 * A valid name includes letters A to Z in upper or lower case,
 * digits 0 to 9, and underscores. Any other character is illegal.
 *
 * Note that the system adds -<server>-<counter> to the name when
 * multiple links can exist (one to many, many to many).
 *
 * \exception links_exception_invalid_name
 * The links_exception_invalid_name is raised if the name is not valid.
 */
void link_info::verify_name()
{
	for(QString::const_iterator it(f_name.begin()); it != f_name.end(); ++it)
	{
		ushort c = it->unicode();
		if((c < '0' || c > '9')
		&& (c < 'A' || c > 'Z')
		&& (c < 'a' || c > 'z')
		&& c != '_')
		{
			throw links_exception_invalid_name();
		}
	}
}

/** \brief Retrieve the data to be saved in the database.
 *
 * Defines the string to be saved in the database. We could use the serializer
 * but this is just two variables: key and name, so instead we manage that
 * manually here. Plus they key and name cannot include a "\n" character so
 * we don't have to check for that.
 *
 * \return The string representing this link.
 *
 * \sa from_data()
 */
QString link_info::data() const
{
	return "key=" + f_key + "\nname=" + f_name;
}

/** \brief Parse a string of key & name back to a link info.
 *
 * This function is the inverse of the data() function. It takes
 * a string as input and defines the f_key and f_name parameters
 * from the data found in that string.
 *
 * \sa data()
 */
void link_info::from_data(const QString& db_data)
{
	QStringList lines(db_data.split('\n'));
	if(lines.count() != 2) {
		throw links_exception_invalid_db_data();
	}
	QStringList key_data(lines[0].split('='));
	QStringList name_data(lines[1].split('='));
	if(key_data.count() != 2 || name_data.count() != 2
	|| key_data[0] != "key" || name_data[0] != "name") {
		throw links_exception_invalid_db_data();
	}
	set_key(key_data[1]);
	set_name(name_data[1], f_unique);
}





/** \brief Initialize a link context to read links.
 *
 * This object is used to read links from the database.
 * This is particularly useful in this case because you may need
 * to call the function multiple times before you read all the
 * links.
 *
 * \param[in] snap  The snap_child object pointer.
 * \param[in] info  The link information about this link context.
 */
link_context::link_context(::snap::snap_child *snap, const link_info& info)
	: f_snap(snap)
	, f_info(info)
	//, f_row() -- auto-init
	//, f_column_predicate() -- auto-init
	//, f_link() -- auto-init
{
	// if the link is unique, it only appears in the content
	// and we don't need the context per se, so we just read
	// the info and keep it in the context for retrieval;
	// if not unique, then we read the first 1,000 links and
	// make them available in the context to the caller
	if(f_info.is_unique())
	{
		QSharedPointer<QtCassandra::QCassandraTable> table(content::content::instance()->get_content_table());
		if(table.isNull())
		{
			// the table does not exist?!
			throw links_exception_missing_content_table();
		}
		// f_row remains null (isNull() returns true)
		//QSharedPointer<QtCassandra::QCassandraRow> row(table->row(f_info.key()));
		QString links_namespace(get_name(SNAP_NAME_LINKS_NAMESPACE));
//printf("links: content read row key [%s] cell [%s]\n", f_info.key().toUtf8().data(), (links_namespace + "::" + f_info.name()).toUtf8().data());
		QtCassandra::QCassandraValue link(content::content::instance()->get_content_table()->row(f_info.key())->cell(links_namespace + "::" + f_info.name())->value());
		if(!link.nullValue())
		{
			f_link = link.stringValue();
		}
	}
	else
	{
		// since we're loading these links from the links index we do
		// not need to specify the column names in the column predicate
		// it will automatically read all the data from that row
		QSharedPointer<QtCassandra::QCassandraTable> table(links::links::instance()->get_links_table());
		if(table.isNull())
		{
			// the table does not exist?!
			// (since the links is a core plugin, that should not happen)
			throw links_exception_missing_links_table();
		}
		f_row = table->row(f_info.key());
		// TBD: should we give the caller the means to change this 1,000 count?
		f_column_predicate.setCount(1000);
		f_column_predicate.setIndex(); // behave like an index
		// we MUST clear the cache in case we read the same list of links twice
		f_row->clearCache();
		// at this point begin() == end()
		f_cell_iterator = f_row->cells().begin();
	}
}

/** \brief Retrieve the next link.
 *
 * This function reads one link and saves it in the info parameter.
 * If no more link is available, then the function returns false
 * and the info parameter is not modified.
 *
 * \param[out] info  The structure where the result is saved if available.
 *
 * \return true if info is set with the next link, false if no more links are available.
 */
bool link_context::next_link(link_info& info)
{
	// special case of a unique link
	if(f_info.is_unique())
	{
		// return the f_link entry once, then an empty string
		// if the link did not exist, the caller only gets an empty string
		if(f_link.isEmpty()) {
			return false;
		}
		info.from_data(f_link);
		f_link.clear();
		return true;
	}

	const QtCassandra::QCassandraCells& cells(f_row->cells());
	if(f_cell_iterator == cells.end()) {
		// no more cells available in the cells, try to read more
		f_row->clearCache();
		f_row->readCells(f_column_predicate);
		f_cell_iterator = cells.begin();
		if(f_cell_iterator == cells.end()) {
			// no more cells available
			return false;
		}
	}

	// the result is at the current iterator
	// note that from the links table we only get keys, no names
	// which doesn't matter as the name is f_info.name() anyway
	info.set_key(QString::fromUtf8(f_cell_iterator.key()));
	info.set_name(f_info.name());
	++f_cell_iterator;

	return true;
}

/** \brief Return the key of the link.
 *
 * This is the key of the row, without the site key, where the column
 * of this link is saved (although the link may not exist.)
 *
 * \return The key of the row where the link is saved.
 */
//const QString& link_context::key() const
//{
//	return f_info.key();
//}

/** \brief Return the name of the link.
 *
 * This is the name of the column without the "links::" namespace.
 *
 * \return The name of the column where the link is saved.
 */
//const QString& link_context::name() const
//{
//	return f_info.name();
//}







/** \brief Initialize the links plugin.
 *
 * This function is used to initialize the links plugin object.
 */
links::links()
	//: f_snap(NULL) -- auto-init
	//  f_links_table() -- auto-init
	//  f_content_table() -- auto-init
{
}

/** \brief Clean up the links plugin.
 *
 * Ensure the links object is clean before it is gone.
 */
links::~links()
{
}

/** \brief Initialize the links plug-in.
 *
 * This function terminates the initialization of the links plug-in
 * by registring for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void links::on_bootstrap(::snap::snap_child *snap)
{
	f_snap = snap;

	//std::cerr << " - Bootstrapping links!\n";
	//SNAP_LISTEN(links, "server", server, update, _1); -- replaced with do_update()
}

/** \brief Get a pointer to the links plug-in.
 *
 * This function returns an instance pointer to the links plug-in.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the links plug-in.
 */
links *links::instance()
{
	return g_plugin_links_factory.instance();
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
QString links::description() const
{
	return "This plugin offers functions to link rows of data together."
		" For example, it allows you to attach a tag to the page of content."
		" This plugin is part of core since it links everything that core"
		" needs to make the system function as expected.";
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
int64_t links::do_update(int64_t last_updated)
{
	SNAP_PLUGIN_UPDATE_INIT();

//std::cerr << "Got the do_update() in links! "
//		<< static_cast<int64_t>(last_updated) << ", "
//		<< static_cast<int64_t>(SNAP_UNIX_TIMESTAMP(2012, 1, 1, 0, 0, 0) * 1000000LL) << "\n";

	SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);

	SNAP_PLUGIN_UPDATE_EXIT();
}

/** \brief First update to run for the links plugin.
 *
 * This function is the first update for the links plugin. It installs
 * the initial data required by the links plugin.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void links::initial_update(int64_t variables_timestamp)
{
}

/** \brief Initialize the content table.
 *
 * This function creates the content table if it doesn't exist yet. Otherwise
 * it simple initializes the f_content_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The pointer to the content table.
 */
QSharedPointer<QtCassandra::QCassandraTable> links::get_links_table()
{
	// create an index so we can search by content
	return f_snap->create_table(get_name(SNAP_NAME_LINKS_TABLE), "Links index table.");
}

/** \brief Create a link between two rows.
 *
 * Links are always going both ways: the source links to the destination
 * and the destination to the source.
 *
 * If the source or destination links have a name that already exists in
 * the corresponding row and the unique flag is true, then that link will
 * be overwritten with the new information. If the unique flag is false,
 * then a new column is created unless that exact same link already exists
 * in that row.
 *
 * In order to test whether a link already exists we need to make use of
 * an index. This is done with the content of the link used as the key
 * of a column defined in the links table (ColumnFamily). This is very
 * important for very large data sets (i.e. think of a website with
 * one million pages, all of which would be of type "page". This means
 * one million links from the type "page" to the one million pages.)
 * We can forfeit the creation of that index for links marked as being
 * unique.
 *
 * A good example of unique link is a parent link (assuming a content
 * type can have only one parent.)
 *
 * References about indexes in Cassandra:
 * http://maxgrinev.com/2010/07/12/do-you-really-need-sql-to-do-it-all-in-cassandra/
 * http://stackoverflow.com/questions/3779239/how-do-i-filter-through-data-in-cassandra
 * http://www.datastax.com/docs/1.1/dml/using_cli#indexing-a-column
 *
 * Example:
 *
 * Say that:
 *
 * \li The source key is "example.com/test1"
 * \li The source name is "tag"
 * \li The destination key is "example.com/root/tags"
 * \li The destination name is "children"
 *
 * We create 2 to 4 entries as follow:
 *
 * \code
 * link table[source key][destination key] = source column number;
 * link table[destination key][source key] = destination column number;
 * content table[source key][source name + source column number] = destination key;
 * content table[destination key][destination name + destination column number] = source key;
 * \endcode
 *
 * If the source name is unique, then no link table entry for the source is
 * created and the source column number is empty ("").
 *
 * Similarly, if the destination name is unique, then no link table entry
 * for the destination is created and the destination column number is
 * empty ("").
 *
 * The link table is used as an index and for unique entries it is not required
 * since we already know where that data is
 * (i.e. the data saved in content table[source key][source name .*] for the
 * source is the destination and we know exactly where it is.)
 *
 * \note
 * A link cannot be marked as unique once and non-unique another.
 * This is concidered an internal error. If you change your mind and already
 * released a plugin with a link defined one way, then you must change
 * the name in the next version.
 *
 * \todo
 * Find a way to test whether the caller changed the unicity and is about
 * to break something...
 *
 * \param[in] src  The source link
 * \param[in] dst  The destination link
 *
 * \sa snap_child::get_unique_number()
 */
void links::create_link(const link_info& src, const link_info& dst)
{
	// retrieve links index table if not there yet
	if(f_links_table.isNull()) {
		QSharedPointer<QtCassandra::QCassandraTable> table(get_links_table());
		if(table.isNull()) {
			// the table does not exist?!
			throw links_exception_missing_links_table();
		}
		f_links_table = table;
	}
	// retrieve content table if not there yet
	if(f_content_table.isNull()) {
		QSharedPointer<QtCassandra::QCassandraTable> table(content::content::instance()->get_content_table());
		if(table.isNull()) {
			// links cannot work if the content table doesn't already exist
			throw links_exception_missing_content_table();
		}
		f_content_table = table;
	}

	// define the column names
	QString src_col, dst_col;

	QString links_namespace(get_name(SNAP_NAME_LINKS_NAMESPACE));
	src_col = links_namespace + "::" + src.name();
	if(!src.is_unique()) {
		src_col += "-";
		// not unique, first check whether it was already created
		QtCassandra::QCassandraValue value(f_links_table->row(src.key())->cell(dst.key())->value());
		if(value.nullValue()) {
			// it does not exist, create a unique number
			QString no(f_snap->get_unique_number());
			src_col += no;
			// save in the index table
			(*f_links_table)[src.key()][dst.key()] = QtCassandra::QCassandraValue(src_col);
		}
		else {
			// it exists, make use of the existing key
			src_col = value.stringValue();
		}
	}

	dst_col = links_namespace + "::" + dst.name();
	if(!dst.is_unique()) {
		dst_col += "-";
		// not unique, first check whether it was already created
		QtCassandra::QCassandraValue value(f_links_table->row(dst.key())->cell(src.key())->value());
		if(value.nullValue()) {
			// it does not exist, create a unique number
			QString no(f_snap->get_unique_number());
			dst_col += no;
			// save in the index table
			(*f_links_table)[dst.key()][src.key()] = QtCassandra::QCassandraValue(dst_col);
		}
		else {
			// it exists, make use of the existing key
			dst_col = value.stringValue();
		}
	}

	// define the columns data
	//QString src_data, dst_data;
	//src_data = "key=" + dst.key() + "\nname=" + dst.name();
	//dst_data = "key=" + src.key() + "\nname=" + src.name();

	// save the links in the rows
	(*f_content_table)[src.key()][src_col] = dst.data(); // save dst in src
	(*f_content_table)[dst.key()][dst_col] = src.data(); // save src in dst
}

/** \brief Create a new link context to read links from.
 *
 * This function creates a new link context instance using your
 * link_info information. The resulting context can be used to
 * read all the links using the next_link() function to read
 * the following link.
 *
 * Note that if no such link exists then the function returns a
 * link context which immediately returns false when next_link()
 * is called. On creation we do not count the number of links
 * because we do not know that number without reading all the
 * links.
 *
 * \param[in] info  The link key and name.
 *
 * \return A shared pointer to a link context, it will always exist.
 */
QSharedPointer<link_context> links::new_link_context(const link_info& info)
{
	QSharedPointer<link_context> context(new link_context(f_snap, info));
	return context;
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4
