// Snap Websites Server -- manage double links
// Copyright (C) 2012-2013  Made to Order Software Corp.
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
#include "log.h"
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
    switch(name)
    {
    case SNAP_NAME_LINKS_TABLE: // sorted index of links
        return "links";

    case SNAP_NAME_LINKS_NAMESPACE:
        return "links";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_LINKS_...");

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
 * A number is appended to the column names when \p unique is false.
 * This gives us a many to many or many to one link capability:
 *
 *   "links::<plugin name>::<link name>-<server name>-<unique number>"
 *
 * When the unique flag is set to true, the name of the column does not
 * include the unique number:
 *
 *   "links::<plugin name>::<link name>"
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
 * The destination (i.e the data of the link) is defined using another
 * link_info (i.e. the on_create_link() uses source (src) and
 * destination (dst) parameters which are both link_info.)
 *
 * \note
 * What changes depending on the link category (unique or not) is the
 * column name.
 * 
 * \param[in] new_key  The key to the row where the link is to be saved.
 *
 * \sa key()
 */

/** \fn bool link_info::is_unique() const
 * \brief Check whether this link is marked as unique.
 *
 * The function returns the current value of the unique flag as set on
 * construction. It can be changed with the set_name() function as
 * the second parameter. By default the set_name() function assumes that
 * the link is not unique (many).
 *
 * \return true if the link is unique (one to many, many to one, or one to one)
 *
 * \sa set_name()
 */

/** \fn const QString& link_info::name() const
 * \brief Retrieve the name of the link.
 *
 * The function returns the name of the link as set on construction
 * or with the set_name() function.
 *
 * \return The name of the link that is used to form the full name of
 *         the column.
 *
 * \sa set_name()
 */

/** \fn const QString& link_info::key() const
 * \brief Retrieve the key of the link.
 *
 * The function returns the key for the link as set on construction
 * or with the set_key() function.
 *
 * \return the key of the link that is used as the row key
 *
 * \sa set_key()
 */

/** \var controlled_vars::zbool_t link_info::f_unique;
 * \brief Unique (one) or not (many) links.
 *
 * This flag is used to tell the link system whether the link is
 * unique or not.
 *
 * \sa is_unique() const
 * \sa set_name() const
 */

/** \var QString link_info::f_name;
 * \brief The name of the column.
 *
 * The name of the column used for the link.
 *
 * \sa name() const
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
 * Because of the way the link plugin makes use of the link name, we
 * want to make sure that the name is valid according to the rules
 * defined below. The main reason is so we can avoid problems. A
 * link name is expected to include a plugin name and a link name.
 * There may be more than once plugin name when useful. For example,
 * the "permissions::users::edit" link name is considered valid.
 *
 * For links that are not unique, the system appends the server name
 * and a unique number separated by dashes. This is why the link plugin
 * forbids the provided link names from including a dash.
 *
 * So, a link name in the database looks like this:
 *
 * \code
 *    links::(<plugin-name>::)+<link-name>
 *    links::(<plugin-name>::)+<link-name>-<server-name>-<unique-number>
 * \endcode
 *
 * Valid link and plugin names are defined with the following BNF:
 *
 * \code
 *   plugin_name ::= link_name
 *   link_name ::= word
 *               | word '::' link_name
 *   word ::= letters | digits | '_'
 *   letters ::= ['A'-'Z']
 *             | ['a'-'z']
 *   digits ::= ['0'-'9']
 * \endcode
 *
 * As we can see, this BNF does not allow for any '-' in the link name.
 *
 * \note
 * It is to be noted that the syntax allows for a name to start with a
 * digit. This may change in the future and only letters may be allowed.
 *
 * \exception links_exception_invalid_name
 * The links_exception_invalid_name is raised if the name is not valid.
 *
 * \param[in] vname  The name to be verified
 */
void link_info::verify_name(const QString& vname)
{
    // the namespace is really only for debug purposes
    // but at this time we'll keep it for security
    const char *links_namespace(get_name(SNAP_NAME_LINKS_NAMESPACE));
    QString ns;
    ns.reserve(64);
    bool has_namespace(false);
    for(QString::const_iterator it(vname.begin()); it != vname.end(); ++it)
    {
        ushort c = it->unicode();
        if(c == ':' && it != vname.begin())
        {
            // although "links" is a valid name, it is in conflict because
            // out column name already starts with "links::" and it is not
            // unlikely that a programmer is trying to make sure that the
            // start of the name is "links::"...
            if(ns == links_namespace)
            {
                throw links_exception_invalid_name("name \"" + vname + "\" is not acceptable, a name cannot make use of the \"links\" namespace");
            }
            ns.clear(); // TBD does that free the reserved buffer?

            // we found a ':' which was not the very first character
            ++it;
            if(it == vname.end())
            {
                throw links_exception_invalid_name("name \"" + vname + "\" is not acceptable, a name cannot end with a ':'");
            }
            if(it->unicode() != ':')
            {
                throw links_exception_invalid_name("name \"" + vname + "\" is not acceptable, the namespace operator must be '::'");
            }
            ++it;
            if(it == vname.end())
            {
                throw links_exception_invalid_name("name \"" + vname + "\" is not acceptable, a name cannot end with a namespace operator '::'");
            }
            // we must have a character that's not a ':' after a '::'
            c = it->unicode();
            has_namespace = true;
        }
        // colons are not acceptable here, we must have a valid character not
        if((c < '0' || c > '9')
        && (c < 'A' || c > 'Z')
        && (c < 'a' || c > 'z')
        && c == '_')
        {
            throw links_exception_invalid_name("name \"" + vname + "\" is not acceptable, character '" + QChar(c) + "' is not valid");
        }
        ns += c;
    }
    if(!has_namespace)
    {
        // at least one namespace is mandatory
        throw links_exception_invalid_name("name \"" + vname + "\" is not acceptable, at least one namespace is expected");
    }

    if(ns == links_namespace)
    {
        throw links_exception_invalid_name("name \"" + vname + "\" is not acceptable, a name cannot end with \"links\"");
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
    if(lines.count() != 2)
    {
        throw links_exception_invalid_db_data("db_data is not exactly 2 lines");
    }
    QStringList key_data(lines[0].split('='));
    QStringList name_data(lines[1].split('='));
    if(key_data.count() != 2 || name_data.count() != 2
    || key_data[0] != "key" || name_data[0] != "name")
    {
        throw links_exception_invalid_db_data("db_data variables are not key and name");
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
            throw links_exception_missing_content_table("could not get the content table");
        }
        // f_row remains null (isNull() returns true)
        //QSharedPointer<QtCassandra::QCassandraRow> row(table->row(f_info.key()));
        const QString links_namespace(get_name(SNAP_NAME_LINKS_NAMESPACE));
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
            throw links_exception_missing_links_table("could not find the links table");
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
 * If no more links are available, then the function returns false
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
        if(f_link.isEmpty())
        {
            return false;
        }
        info.from_data(f_link);
        f_link.clear();
        return true;
    }

    const QtCassandra::QCassandraCells& cells(f_row->cells());
    if(f_cell_iterator == cells.end())
    {
        // no more cells available in the cells, try to read more
        f_row->clearCache();
        f_row->readCells(f_column_predicate);
        f_cell_iterator = cells.begin();
        if(f_cell_iterator == cells.end())
        {
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
//    return f_info.key();
//}

/** \brief Return the name of the link.
 *
 * This is the name of the column without the "links::" namespace.
 *
 * \return The name of the column where the link is saved.
 */
//const QString& link_context::name() const
//{
//    return f_info.name();
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

/** \brief Initialize the links plugin.
 *
 * This function terminates the initialization of the links plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void links::on_bootstrap(::snap::snap_child *snap)
{
    f_snap = snap;

    //std::cerr << " - Bootstrapping links!\n";
    //SNAP_LISTEN(links, "server", server, update, _1); -- replaced with do_update()
}

/** \brief Get a pointer to the links plugin.
 *
 * This function returns an instance pointer to the links plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the links plugin.
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
//        << static_cast<int64_t>(last_updated) << ", "
//        << static_cast<int64_t>(SNAP_UNIX_TIMESTAMP(2012, 1, 1, 0, 0, 0) * 1000000LL) << "\n";

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


/** \brief Initialize the content and links table.
 *
 * The first time one of the functions that require the links and contents
 * table runs, it calls this function to get the QCassandraTable.
 */
void links::init_tables()
{
    // retrieve links index table if not there yet
    if(f_links_table.isNull())
    {
        QSharedPointer<QtCassandra::QCassandraTable> table(get_links_table());
        if(table.isNull())
        {
            // the table does not exist?!
            throw links_exception_missing_links_table("could not find the links table");
        }
        f_links_table = table;
    }

    // retrieve content table if not there yet
    if(f_content_table.isNull())
    {
        QSharedPointer<QtCassandra::QCassandraTable> table(content::content::instance()->get_content_table());
        if(table.isNull())
        {
            // links cannot work if the content table doesn't already exist
            throw links_exception_missing_content_table("could not get the content table");
        }
        f_content_table = table;
    }
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
    // define the column names
    QString src_col, dst_col;

    init_tables();

    const QString links_namespace(get_name(SNAP_NAME_LINKS_NAMESPACE));
    src_col = links_namespace + "::" + src.name();
    if(!src.is_unique())
    {
        src_col += "-";
        // not unique, first check whether it was already created
        QtCassandra::QCassandraValue value(f_links_table->row(src.key())->cell(dst.key())->value());
        if(value.nullValue())
        {
            // it does not exist, create a unique number
            QString no(f_snap->get_unique_number());
            src_col += no;
            // save in the index table
            (*f_links_table)[src.key()][dst.key()] = QtCassandra::QCassandraValue(src_col);
        }
        else
        {
            // it exists, make use of the existing key
            src_col = value.stringValue();
        }
    }

    dst_col = links_namespace + "::" + dst.name();
    if(!dst.is_unique())
    {
        dst_col += "-";
        // not unique, first check whether it was already created
        QtCassandra::QCassandraValue value(f_links_table->row(dst.key())->cell(src.key())->value());
        if(value.nullValue())
        {
            // it does not exist, create a unique number
            QString no(f_snap->get_unique_number());
            dst_col += no;
            // save in the index table
            (*f_links_table)[dst.key()][src.key()] = QtCassandra::QCassandraValue(dst_col);
        }
        else
        {
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


/** \brief Make sure that the specified link is deleted.
 *
 * Once two nodes are linked together, it is possible to remove that
 * link by calling this function.
 *
 * When nodes are linked with mode (1:1), then either node can be picked
 * to delete that link. Links created with (1:*) or (*:1) should pick the
 * node that had the (1) to remove just that one link. In all other cases,
 * all the links get deleted (which is useful when you delete something
 * such as a tag because all the pages that were linked to that tag must
 * not be linked to it anymore.)
 *
 * In order to find the data in the database, the info must be properly
 * initialized with the link name  and the full URI &amp; path to the link.
 * The unicity flag is ignored to better ensure that the link will be
 * deleted whether it is unique or not.
 *
 * If the link does not exist, nothing happens. Actually, when a multi-link
 * gets deleted, all problems are reported, but as many links that can be
 * deleted get deleted.
 *
 * \warning
 * If more than one computer tries to delete the same link at the same
 * time errors will ensue. This should be relatively rare though and most
 * certainly still be safe. However, if someone adds a link at the same
 * time as it gets deleted, the result can be that the new link gets
 * partially created and deleted.
 *
 * \param[in] info  The key and name of the link to be deleted.
 */
void links::delete_link(const link_info& info)
{
    // here I assume that the is_unique() could be misleading
    // this way we can avoid all sorts of pitfalls where someone
    // creates a link with "*:1" and tries to delete it with "1:*"

    init_tables();

    if(!f_content_table->exists(info.key()))
    {
        // probably not an error if a link does not exist at all...
        return;
    }

    // note: we consider the content row defined in the info structure
    //       to be the source; obviously, as a result, the other one will
    //       be the destination
    QSharedPointer<QtCassandra::QCassandraRow> src_row(f_content_table->row(info.key()));

    // check if the link is defined as is (i.e. this info represents
    // a unique link, a "1")

    const QString links_namespace(get_name(SNAP_NAME_LINKS_NAMESPACE));
    const QString unique_link_name(links_namespace + "::" + info.name());
    if(src_row->exists(unique_link_name))
    {
        // we're here, this means it was a "1,1" or "1,*" link
        QtCassandra::QCassandraValue link(src_row->cell(unique_link_name)->value());

        // delete the source link right now
        src_row->dropCell(unique_link_name, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());

        // we read the link so that way we have information about the
        // destination and can delete it too
        link_info destination;
        destination.from_data(link.stringValue());
        if(!f_content_table->exists(destination.key()))
        {
            SNAP_LOG_WARNING("links::delete_link() could not find the destination link for \"")
                        (destination.key())("\" (destination row missing in content).");
            return;
        }
        QSharedPointer<QtCassandra::QCassandraRow> dst_row(f_content_table->row(destination.key()));

        // to delete the link on the other side, we have to test whether
        // it is unique (1:1) or multiple (1:*)
        QString dest_cell_unique_name(links_namespace + "::" + destination.name());
        if(dst_row->exists(dest_cell_unique_name))
        {
            // unique links are easy to handle!
            dst_row->dropCell(dest_cell_unique_name, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
        }
        else
        {
            // with a multiple link we have to use the links table to find the
            // exact destination
            if(!f_links_table->exists(destination.key()))
            {
                // if the unique name does not exist,
                // then the multi-name must exist...
                SNAP_LOG_WARNING("links::delete_link() could not find the destination link for \"")
                            (destination.key())("\" (destination row missing in links).");
                return;
            }
            QSharedPointer<QtCassandra::QCassandraRow> dst_multi_row(f_links_table->row(destination.key()));
            if(!dst_multi_row->exists(info.key()))
            {
                // the destination does not exist anywhere!?
                // (this could happen in case the server crashes or something
                // of the sort...)
                SNAP_LOG_WARNING("links::delete_link() could not find the destination link for \"")
                            (destination.key())(" / ")
                            (info.key())("\" (cell missing in links).");
                return;
            }
            // note that this is a multi-link, but in a (1:*) there is only
            // one destination that correspond to the (1:...) and thus only
            // one link that we need to load here
            QtCassandra::QCassandraValue destination_link(dst_multi_row->cell(info.key())->value());

            // we can drop that link immediately, since we got the information we needed
            dst_multi_row->dropCell(info.key(), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());

            // TODO: should we drop the row if empty?
            //       I think it automatically happens when a row is empty
            //       (no more cells) then it gets removed by Cassandra anyway

            // this value represents the multi-name (i.e. <link namespace>::<link name>-<server name>-<number>)
            QString dest_cell_multi_name(destination_link.stringValue());
            if(dst_row->exists(dest_cell_multi_name))
            {
                dst_row->dropCell(dest_cell_multi_name, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
            }
            else
            {
                // again, this could happen if the server crashed or was
                // killed at the wrong time or another computer was deleting
                // under our feet
                SNAP_LOG_WARNING("links::delete_link() could not find the destination link for \"")
                            (destination.key())(" / ")
                            (dest_cell_multi_name)("\" (destination cell missing in content).");
                return;
            }
        }
    }
    else
    {
        // in this case we have a "*,1" or a "*,*" link
        // the links need to be loaded from the links table and there can
        // be many so we have to loop over the rows we read

        // here we get the row, we do not delete it yet because we need
        // to go through the whole list first
        QSharedPointer<QtCassandra::QCassandraRow> row(f_links_table->row(info.key()));
        QtCassandra::QCassandraColumnRangePredicate column_predicate;
        column_predicate.setCount(1000);
        column_predicate.setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->clearCache();
            row->readCells(column_predicate);
            const QtCassandra::QCassandraCells& cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            //for(auto cell_iterator : cells)
            {
                QString key(QString::fromUtf8(cell_iterator.key()));
                if(!f_content_table->exists(key))
                {
                    // probably not an error if a link does not exist at all...
                    SNAP_LOG_WARNING("links::delete_link() could not find the destination link for \"")
                                (key)(" / ")
                                (unique_link_name)("\" (destination row missing in content.");
                }
                else
                {
                    QSharedPointer<QtCassandra::QCassandraRow> dst_row(f_content_table->row(key));
                    //const QString unique_link_name(links_namespace + "::" + info.name());
                    if(dst_row->exists(unique_link_name))
                    {
                        // here we have a "*:1"
                        dst_row->dropCell(unique_link_name, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
                    }
                    else
                    {
                        if(!f_links_table->exists(key))
                        {
                            SNAP_LOG_WARNING("links::delete_link() could not find the destination link for \"")
                                        (key)("\" (destination row missing in links).");
                        }
                        else
                        {
                            QSharedPointer<QtCassandra::QCassandraRow> link_row(f_links_table->row(key));
                            // here we have a "*:*" although note that we want to
                            // only delete one link in this destination
                            const QString dest_cell_unique_name(links_namespace + "::" + cell_iterator.value()->value().stringValue());
                            if(!link_row->exists(dest_cell_unique_name))
                            {
                                // the destination does not exist anywhere!?
                                // (this could happen in case the server crashes or something
                                // of the sort...)
                                SNAP_LOG_WARNING("links::delete_link() could not find the destination link for \"")
                                            (key)(" / ")
                                            (dest_cell_unique_name)("\" (cell missing in links).");
                            }
                            else
                            {
                                // we can drop that link now
                                link_row->dropCell(dest_cell_unique_name, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
                            }
                        }
                    }
                }
            }
        }

        // finally we can delete this row
        f_links_table->dropRow(info.key());
    }
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
