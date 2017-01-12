// Snap Websites Server -- links backends
// Copyright (C) 2011-2017  Made to Order Software Corp.
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
 * \brief The implementation of the links plugin class backend parts.
 *
 * This file contains the implementation of the various links backend
 * functions of the links plugin.
 */

#include "links.h"

#include "../content/content.h"

#include <snapwebsites/log.h>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(links)


/** \brief Register the links action.
 *
 * This function registers this plugin actions as listed below. These
 * actions allows administrators to manage website links from the
 * command line with the snapbackend tool.
 *
 * To create a link use the following syntax. In this example, we are
 * creating a link from the front page to user 1 making user 1 the
 * author of the front page.
 *
 * \li cleanuplinks -- check that all links are valid on a given website
 * (i.e. links use 2 to 4 columns in 2 to 4 different rows and if any
 * one of these entries is not valid, the link is broken and needs to be
 * deleted.) This may become a problem that we automatically run once in
 * a while so the database does not decay over time.
 *
 * \li createlink -- create a link between two pages
 *
 * \code
 * snapbackend [--config snapserver.conf] [website-url] \
 *      --action links::createlink \
 *      --param SOURCE_LINK_NAME=users::author \
 *              SOURCE_LINK=http://csnap.example.com/ \
 *              DESTINATION_LINK_NAME=users::authored_pages \
 *              DESTINATION_LINK=http://csnap.example.com/user/1 \
 *              'LINK_MODE=1,*'
 * \endcode
 *
 * \li deletelink -- delete the specified link, either specific link between
 * two pages or all the links with a given name from the specified page
 *
 * In order to delete a link, use the deletelink action, specify the name
 * of the field, and one or two URLs as in:
 *
 * \code
 * # delete one specific link between two pages
 * snapbackend your-snap.website.ext \
 *      [--config snapserver.conf]
 *      --action links::deletelink \
 *      --param SOURCE_LINK_NAME=users::author \
 *              SOURCE_LINK=/ \
 *              DESTINATION_LINK_NAME=users::authored_pages \
 *              DESTINATION_LINK=/user/1 \
 *              'LINK_MODE=1,*'
 *
 * # delete all links named users::author in this page
 * snapbackend your-snap.website.ext \
 *      [--config snapserver.conf]
 *      --action links::deletelink \
 *      --param SOURCE_LINK_NAME=users::author \
 *              SOURCE_LINK=/ \
 *              LINK_MODE=1
 * \endcode
 *
 * WARNING: If you do not specify the URI of the website you want to work
 * on, snapbackend runs the process against all the existing websites.
 *
 * If you have problems with this action (it does not seem to work,)
 * try with --debug and make sure to look in the syslog and snapserver.log
 * files.
 *
 * \note
 * This should be a user action, unfortunately that would add a permissions
 * dependency in the users plugin which we cannot have (i.e. permissions
 * need to know about users...)
 *
 * \todo
 * The links::deletelink needs to allow for the branch to be specified.
 * Right now it deletes the links in the current branch only.
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void links::on_register_backend_action(server::backend_action_set & actions)
{
    actions.add_action(get_name(name_t::SNAP_NAME_LINKS_CLEANUPLINKS), this);
    actions.add_action(get_name(name_t::SNAP_NAME_LINKS_CREATELINK),   this);
    actions.add_action(get_name(name_t::SNAP_NAME_LINKS_DELETELINK),   this);
}


/** \brief Create or delete a link.
 *
 * This function creates or deletes a link.
 *
 * \param[in] action  The action the user wants to execute.
 */
void links::on_backend_action(QString const & action)
{
//std::cerr << "   a: " << action << "\n";
//std::cerr << "mode: " << f_snap->get_server_parameter("LINK_MODE") << "\n";
//std::cerr << " src: " << f_snap->get_server_parameter("SOURCE_LINK") << "\n";
//std::cerr << "  sn: " << f_snap->get_server_parameter("SOURCE_LINK_NAME") << "\n";
//std::cerr << " dst: " << f_snap->get_server_parameter("DESTINATION_LINK") << "\n";
//std::cerr << "  dn: " << f_snap->get_server_parameter("DESTINATION_LINK_NAME") << "\n";

    if(action == get_name(name_t::SNAP_NAME_LINKS_CREATELINK))
    {
        on_backend_action_create_link();
    }
    else if(action == get_name(name_t::SNAP_NAME_LINKS_DELETELINK))
    {
        on_backend_action_delete_link();
    }
    else if(action == get_name(name_t::SNAP_NAME_LINKS_CLEANUPLINKS))
    {
        cleanup_links();
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        throw snap_logic_exception(QString("links.cpp:on_backend_action(): links::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


void links::on_backend_action_create_link()
{
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());

    // create a link
    QString const mode(f_snap->get_server_parameter("LINK_MODE"));
    snap_string_list unique(mode.split(","));
    if(unique.size() != 2)
    {
        SNAP_LOG_FATAL("invalid mode \"")(mode)("\", missing comma or more than one comma.");
        exit(1);
    }
    if((unique[0] != "*" && unique[0] != "1")
    || (unique[1] != "*" && unique[1] != "1"))
    {
        SNAP_LOG_FATAL("invalid mode \"")(mode)("\", one of the repeat is not \"*\" or \"1\".");
        exit(1);
    }

    content::path_info_t source_ipath;
    source_ipath.set_path(f_snap->get_server_parameter("SOURCE_LINK"));
    if(!content_table->exists(source_ipath.get_key()))
    {
        SNAP_LOG_FATAL("invalid source URI \"")(source_ipath.get_key())("\", page does not exist.");
        exit(1);
    }

    QString const link_name(f_snap->get_server_parameter("SOURCE_LINK_NAME"));
    bool const source_unique(unique[0] == "1");
    link_info source(link_name, source_unique, source_ipath.get_key(), source_ipath.get_branch());

    content::path_info_t destination_ipath;
    destination_ipath.set_path(f_snap->get_server_parameter("DESTINATION_LINK"));
    if(!content_table->exists(destination_ipath.get_key()))
    {
        SNAP_LOG_FATAL("invalid destination URI \"")(destination_ipath.get_key())("\", page does not exist.");
        exit(1);
    }

    QString const link_to(f_snap->get_server_parameter("DESTINATION_LINK_NAME"));
    bool const destination_unique(unique[1] == "1");
    link_info destination(link_to, destination_unique, destination_ipath.get_key(), destination_ipath.get_branch());

    // everything looked good, attempt the feat
    create_link(source, destination);
}


void links::on_backend_action_delete_link()
{
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());

    // delete a link
    QString const mode(f_snap->get_server_parameter("LINK_MODE"));
    snap_string_list unique(mode.split(","));
    if(unique.size() == 1)
    {
        if(unique[0] != "*" && unique[0] != "1")
        {
            SNAP_LOG_FATAL("invalid mode \"")(mode)("\", the repeat is not \"*\" or \"1\".");
            exit(1);
        }

        content::path_info_t source_ipath;
        source_ipath.set_path(f_snap->get_server_parameter("SOURCE_LINK"));
        if(!content_table->exists(source_ipath.get_key()))
        {
            SNAP_LOG_FATAL("invalid source URI \"")(source_ipath.get_key())("\", page does not exist.");
            exit(1);
        }

        QString const link_name(f_snap->get_server_parameter("SOURCE_LINK_NAME"));
        bool const source_unique(unique[0] == "1");
        link_info source(link_name, source_unique, source_ipath.get_key(), source_ipath.get_branch());

        // everything looked good, attempt the feat
        delete_link(source);
    }
    else if(unique.size() == 2)
    {
        if((unique[0] != "*" && unique[0] != "1")
        || (unique[1] != "*" && unique[1] != "1"))
        {
            SNAP_LOG_FATAL("invalid mode \"")(mode)("\", one of the repeat is not \"*\" or \"1\".");
            exit(1);
        }

        content::path_info_t source_ipath;
        source_ipath.set_path(f_snap->get_server_parameter("SOURCE_LINK"));
        if(!content_table->exists(source_ipath.get_key()))
        {
            SNAP_LOG_FATAL("invalid source URI \"")(source_ipath.get_key())("\", page does not exist.");
            exit(1);
        }

        QString const link_name(f_snap->get_server_parameter("SOURCE_LINK_NAME"));
        bool const source_unique(unique[0] == "1");
        link_info source(link_name, source_unique, source_ipath.get_key(), source_ipath.get_branch());

        content::path_info_t destination_ipath;
        destination_ipath.set_path(f_snap->get_server_parameter("DESTINATION_LINK"));
        if(!content_table->exists(destination_ipath.get_key()))
        {
            SNAP_LOG_FATAL("invalid destination URI \"")(destination_ipath.get_key())("\", page does not exist.");
            exit(1);
        }

        QString const link_to(f_snap->get_server_parameter("DESTINATION_LINK_NAME"));
        bool const destination_unique(unique[1] == "1");
        link_info destination(link_to, destination_unique, destination_ipath.get_key(), destination_ipath.get_branch());

        // everything looked good, attempt the feat
        delete_this_link(source, destination);
    }
    else
    {
        SNAP_LOG_FATAL("invalid mode \"")(mode)("\", two or more commas.");
        exit(1);
    }
}


/** \brief Clean up the links of a given website.
 *
 * This function goes through all the pages to clean up their links.
 *
 * It searches all the links (i.e. fields that start with "links::")
 * and checks whether the name includes a dash, if so, it is a
 * multi-link and this means it may need to be removed.
 *
 * Whether to remove the link is determined by searching for the link
 * in the "links" table; if not there then that column simply gets
 * removed from the branch table.
 */
void links::cleanup_links()
{
    content::content * content_plugin(content::content::instance());

    QtCassandra::QCassandraTable::pointer_t links_table(get_links_table());

    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    branch_table->clearCache();

    QString const site_key(f_snap->get_site_key_with_slash());

    // to check all the branches, we actually read from the branch table
    // directly instead of the page + branch; here we prepare the
    // predicate start and end strings once
    //
    QString const links_namespace_start(QString("%1::").arg(get_name(name_t::SNAP_NAME_LINKS_NAMESPACE)));
    QString const links_namespace_end(QString("%1:;").arg(get_name(name_t::SNAP_NAME_LINKS_NAMESPACE)));

    // TBD: now that we have an '*index*' row with all the pages of
    //      a website sorted "as expected", we may be able revise
    //      the following algorithm to avoid reading all the branches
    //      of all the websites...
    //
    auto row_predicate(std::make_shared<QtCassandra::QCassandraRowPredicate>());
    row_predicate->setCount(100);
    for(;;)
    {
        uint32_t const count(branch_table->readRows(row_predicate));
        if(count == 0)
        {
            // no more branches to process
            //
            break;
        }
        QtCassandra::QCassandraRows const rows(branch_table->rows());
        for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            QString const key(QString::fromUtf8(o.key().data()));
            if(!key.startsWith(site_key))
            {
                // not this website, try another key
                //
                continue;
            }

            // within each row, check all the columns
            //
            QtCassandra::QCassandraRow::pointer_t row(*o);
            row->clearCache();

            auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
            column_predicate->setCount(100);
            column_predicate->setIndex(); // behave like an index
            column_predicate->setStartCellKey(links_namespace_start); // limit the loading to links at least
            column_predicate->setEndCellKey(links_namespace_end);

            // loop until all cells are handled
            //
            for(;;)
            {
                row->readCells(column_predicate);
                QtCassandra::QCassandraCells const cells(row->cells());
                if(cells.isEmpty())
                {
                    // no more rows here
                    //
                    break;
                }

                // handle one batch
                //
                for(QtCassandra::QCassandraCells::const_iterator c(cells.begin());
                        c != cells.end();
                        ++c)
                {
                    QtCassandra::QCassandraCell::pointer_t cell(*c);

                    QString const cell_name(cell->columnName());
                    int const pos(cell_name.indexOf('-'));
                    int const branch_pos(cell_name.indexOf('#', pos + 1));
                    if(pos != -1
                    || branch_pos != -1)
                    {
                        // okay, this looks like a multi-link
                        // now check for the corresponding entry in the
                        // links table
                        //
                        QString const link_name(cell_name.mid(links_namespace_start.length(), pos - links_namespace_start.length()));
                        // here 'key' already includes the '#<id>'
                        QString const link_key(QString("%1/%2").arg(key).arg(link_name));

                        bool exists(false);
                        if(links_table->exists(link_key))
                        {
                            // the row exists, is there an entry for this link?
                            //
                            QtCassandra::QCassandraRow::pointer_t link_row(links_table->row(link_key));

                            // the column name in that row is the value of 'k'
                            // in the current cell value
                            //
                            link_info info;
                            info.from_data(cell->value().stringValue());
                            // build the key with branch here (we do not have a source so we need to do it this way)
                            QString const key_with_branch(QString("%1%2").arg(info.key()).arg(cell_name.mid(branch_pos)));
                            if(link_row->exists(key_with_branch))
                            {
                                QString const expected_name(link_row->cell(key_with_branch)->value().stringValue());
                                exists = cell_name == expected_name;
                            }
                        }

                        if(!exists)
                        {
                            // this is a spurius cell, get rid of it
                            SNAP_LOG_ERROR("found dangling link \"")(cell_name)("\" in row \"")(key)("\".");
                            row->dropCell(cell_name);
                        }
                    }
                }
            }
        }
    }
}



SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
