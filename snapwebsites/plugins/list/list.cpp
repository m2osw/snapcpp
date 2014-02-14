// Snap Websites Server -- advanced handling of lists
// Copyright (C) 2014  Made to Order Software Corp.
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

#include "list.h"

#include "not_reached.h"
#include "snap_expr.h"

#include <iostream>

#include <sys/time.h>

#include "poison.h"


SNAP_PLUGIN_START(list, 1, 0)

/** \brief Get a fixed list name.
 *
 * The list plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_LIST_LAST_UPDATED:
        return "list::last_updated";

    case SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT: // text format
        return "list::original_item_key_script";

    case SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT: // text format
        return "list::original_test_script";

    case SNAP_NAME_LIST_PAGELIST: // --action pagelist
        return "pagelist";

    case SNAP_NAME_LIST_SELECTOR: // all, public, children, type, ...
        return "list::selector";

    case SNAP_NAME_LIST_STANDALONE: // when present in list table as a column name of a site row: signals a website managed as a standalone site
        return "*standalone*";

    case SNAP_NAME_LIST_STANDALONELIST: // --action standalonelist
        return "standalonelist";

    case SNAP_NAME_LIST_STOP: // STOP signal
        return "STOP";

    case SNAP_NAME_LIST_TABLE:
        return "list";

    case SNAP_NAME_LIST_ITEM_KEY_SCRIPT: // compiled
        return "list::item_key_script";

    case SNAP_NAME_LIST_TEST_SCRIPT: // compiled
        return "list::test_script";

    case SNAP_NAME_LIST_TYPE:
        return "list::type";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_OUTPUT_...");

    }
    NOTREACHED();
}










/** \brief Initialize the list plugin.
 *
 * This function is used to initialize the list plugin object.
 */
list::list()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the list plugin.
 *
 * Ensure the list object is clean before it is gone.
 */
list::~list()
{
}


/** \brief Initialize the list.
 *
 * This function terminates the initialization of the list plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void list::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(list, "server", server, register_backend_action, _1);
    SNAP_LISTEN(list, "layout", layout::layout, generate_page_content, _1, _2, _3, _4);
    SNAP_LISTEN(list, "content", content::content, create_content, _1, _2, _3);
    SNAP_LISTEN(list, "content", content::content, modified_content, _1);
}


/** \brief Get a pointer to the list plugin.
 *
 * This function returns an instance pointer to the list plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the list plugin.
 */
list *list::instance()
{
    return g_plugin_list_factory.instance();
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
QString list::description() const
{
    return "Generate lists of pages using a set of parameters as defined"
          " by the system (some lists are defined internally) and the end"
          " users.";
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
int64_t list::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2014, 2, 4, 16, 29, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the list plugin.
 *
 * This function is the first update for the list plugin. It creates
 * the list table.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void list::initial_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    get_list_table();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void list::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Initialize the list table.
 *
 * This function creates the list table if it doesn't exist yet. Otherwise
 * it simple initializes the f_list_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The list table is used to record all the pages of a website so they can
 * get sorted. As time passes older pages get removed as they are expected
 * to already be part of the list as required. Pages that are created or
 * modified are re-added to the list table so lists that include them can
 * be updated on the next run of the backend.
 *
 * New lists are created using a different scheme which is to find pages
 * using the list definitions to find said pages (i.e. all the pages link
 * under a given type, all the children of a given page, etc.)
 *
 * The table is defined as one row per website. The site_key_with_path()
 * is used as the row key. Within each row, you have one column per page
 * that was created or updated in the last little bit (until the backend
 * receives the time to work on all the lists concerned by such data.)
 * However, we need to time those entries so the column key is defined as
 * a 64 bit number representing the start date (as the
 * f_snap->get_start_date() returns) and the full key of the page that
 * was modified. This means the exact same page may appear multiple times
 * in the table. The backend is capable of ignoring duplicates.
 *
 * The content of the row is simple a boolean (signed char) set to 1.
 *
 * \return The pointer to the list table.
 */
QtCassandra::QCassandraTable::pointer_t list::get_list_table()
{
    if(!f_list_table)
    {
        f_list_table = f_snap->create_table(get_name(SNAP_NAME_LIST_TABLE), "Website list table.");
    }
    return f_list_table;
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
 * Note that this is NOT the HTML output. It is the <page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  A fallback path in case ipath is not satisfactory.
 */
void list::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    static_cast<void>(ipath);
    static_cast<void>(page);
    static_cast<void>(body);
    static_cast<void>(ctemplate);
}


/** \brief Generate the page common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The body being generated.
 */
void list::on_generate_page_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    static_cast<void>(ipath);
    static_cast<void>(page);
    static_cast<void>(body);
    static_cast<void>(ctemplate);
}


/** \brief Signal that a page was created.
 *
 * This function is called whenever the content plugin creates a new page.
 * At that point the page may not yet be complete so we could not handle
 * the possible list updates.
 *
 * So instead the function saves the full key to the page that was just
 * created so lists that include this page can be updated by the backend
 * as required.
 *
 * \param[in,out] ipath  The path to the page being modified.
 * \param[in] owner  The plugin owner of the page.
 * \param[in] type  The type of the page.
 */
void list::on_create_content(content::path_info_t& ipath, QString const& owner, QString const& type)
{
    static_cast<void>(owner);
    static_cast<void>(type);

    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t data_table(content_plugin->get_data_table());

    // if a list is defined in this content, make sure to mark the
    // row as having a list with the last updated data set to zero
    //
    // Note: the exists() call is going to be very fast since the data will
    //       be in memory if true (if false, we still send a network request
    //       to Cassandra... but you never know in case the cache was reset!)
    //       this is going to be faster than such a test in the backend loop
    //       and replacing that with the test of the last update is going to
    //       make it a lot faster overall.
    QString const branch_key(ipath.get_branch_key());
    if(data_table->row(branch_key)->exists(get_name(SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT)))
    {
        QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_data_table());

        // zero marks the list as brand new so we use a different
        // algorithm to check the data in that case (i.e. the list of
        // rows in the list table is NOT complete!)
        QString const key(ipath.get_key());
        int64_t const zero(0);
        content_table->row(key)->cell(get_name(SNAP_NAME_LIST_LAST_UPDATED))->setValue(zero);
    }

    on_modified_content(ipath); // same as on_modified_content()
}


/** \brief Signal that a page was modified.
 *
 * This function is called whenever a plugin modified a page and then called
 * the modified_content() signal of the content plugin.
 *
 * This function saves the full key to the page that was just modified so
 * lists that include this page can be updated by the backend as required.
 *
 * \param[in,out] ipath  The path to the page being modified.
 */
void list::on_modified_content(content::path_info_t& ipath)
{
    // if the same page is modified multiple times then we overwrite the
    // same entry multiple times
    QString site_key(f_snap->get_site_key_with_slash());
    QtCassandra::QCassandraTable::pointer_t list_table(get_list_table());
    int64_t const start_date(f_snap->get_start_date());
    QByteArray key;
    key.append(start_date);
    key.append(ipath.get_key());
    bool const modified(true);
    list_table->row(site_key)->cell(key)->setValue(modified);
}


/** \brief Register the pagelist action.
 *
 * This function registers this plugin as supporting the "pagelist" action.
 * This is used by the backend to continuously and as fast as possible build
 * lists of pages. It understands PINGs so one can wake this backend up as
 * soon as required.
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void list::on_register_backend_action(server::backend_action_map_t& actions)
{
    actions[get_name(SNAP_NAME_LIST_PAGELIST)] = this;
    actions[get_name(SNAP_NAME_LIST_STANDALONELIST)] = this;
}


/** \brief Start the page list server.
 *
 * When running the backend the user can ask to run the pagelist
 * server (--action pagelist). This function captures those events.
 * It loops until stopped with a STOP message via the UDP address/port.
 * Note that Ctrl-C won't work because it does not support killing
 * both: the parent and child processes (we do a fork() to create
 * this child.)
 *
 * The loop updates all the lists as required, then it
 * falls asleep until the next UDP PING event received via the
 * pagelist_udp_signal IP:Port information.
 *
 * Note that because the UDP signals are not 100% reliable, the
 * server actually sleeps for 5 minutes and checks for new pages
 * whether a PING signal was received or not.
 *
 * The lists data is found in the Cassandra cluster and never
 * sent along the UDP signal. This means the UDP signals do not need
 * to be secure.
 *
 * \note
 * The \p action parameter is here because some plugins may
 * understand multiple actions in which case we need to know
 * which action is waking us up.
 *
 * \param[in] action  The action this function is being called with.
 */
void list::on_backend_action(QString const& action)
{
    QtCassandra::QCassandraTable::pointer_t list_table(get_list_table());
    if(action == get_name(SNAP_NAME_LIST_PAGELIST))
    {
        QSharedPointer<udp_client_server::udp_server> udp_signals(f_snap->udp_get_server("sendmail_udp_signal"));
        char const *stop(get_name(SNAP_NAME_LIST_STOP));
        // loop until stopped
        for(;;)
        {
            // work as long as there is work to do
            int did_work(1);
            while(did_work != 0)
            {
                did_work = 0;
                QtCassandra::QCassandraRowPredicate row_predicate;
                row_predicate.setCount(1000);
                for(;;)
                {
                    list_table->clearCache();
                    uint32_t count(list_table->readRows(row_predicate));
                    if(count == 0)
                    {
                        break;
                    }
                    QtCassandra::QCassandraRows const& rows(list_table->rows());
                    for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                            o != rows.end(); ++o)
                    {
                        // do not work on standalone websites
                        if(!(*o)->exists(get_name(SNAP_NAME_LIST_STANDALONE)))
                        {
                            QString const key(QString::fromUtf8(o.key().data()));
                            did_work |= generate_all_lists(key);
                        }
                    }
                }
            }

            // sleep till next PING (but max. 5 minutes)
            char buf[256];
            int r(udp_signals->timed_recv(buf, sizeof(buf), 5 * 60 * 1000)); // wait for up to 5 minutes (x 60 seconds)
            if(r != -1 || errno != EAGAIN)
            {
                if(r < 1 || r >= static_cast<int>(sizeof(buf) - 1))
                {
                    perror("udp_signals->timed_recv():");
                    std::cerr << "error: an error occured in the UDP recv() call, returned size: " << r << std::endl;
                    exit(1);
                }
                buf[r] = '\0';
                if(strcmp(buf, stop) == 0)
                {
                    // clean STOP
                    return;
                }
                // should we check that we really received a PING?
                //char const *ping(get_name(SNAP_NAME_SENDMAIL_PING));
                //if(strcmp(buf, ping) != 0)
                //{
                //    continue
                //}
            }
        }
    }
    else if(action == get_name(SNAP_NAME_LIST_STANDALONELIST))
    {
        // mark the site as a standalone website for its list management
        QString const site_key(f_snap->get_site_key_with_slash());
        int8_t const standalone(1);
        list_table->row(site_key)->cell(get_name(SNAP_NAME_LIST_STANDALONE))->setValue(standalone);
    }
    else
    {
        // unknown action (we should not have been called with that name!)
        throw snap_logic_exception(QString("list::on_backend_action(\"%1\") called with an unknown action...").arg(action));
    }
}


/** \brief Implementation of the backend process signal.
 *
 * This function captures the backend processing signal which is sent
 * by the server whenever the backend tool is run against a site.
 *
 * The list plugin refreshes lists of pages on websites when it receives
 * this signal.
 *
 * This backend may end up taking a lot of processing time and may need to
 * run very quickly (i.e. within seconds when a new page is created or a
 * page is modified). For this reason we also offer an action which supports
 * the PING signal.
 *
 * This backend process will actually NOT run if the PROCESS_LISTS parameter
 * is not defined on the command line:
 *
 * \code
 * snapbackend [--config snapserver.conf] --param PROCESS_LISTS=1
 * \endcode
 *
 * At this time the value used with PROCESS_LIST is not tested, however, it
 * is strongly recommended you use 1.
 */
void list::on_backend_process()
{
    // only process if the user clearly specified that we should do so;
    // we should never run in parallel with a background backend, hence
    // this flag (see the on_backend_action() function)
    QString const process_lists(f_snap->get_server_parameter("PROCESS_LISTS"));
    if(!process_lists.isEmpty())
    {
        // we ignore the result in this case, the backend will
        // run again soon and take care of the additional data
        // accordingly (with the action we process as much as
        // possible all in one go)
        generate_all_lists(f_snap->get_site_key_with_slash());
    }
}


/** \brief This function regenerates all the lists of all the websites.
 *
 * This function reads the complete list of all the lists as defined in the
 * lists table for each website defined in there.
 *
 * The process can take a very long time, especially if you have a large
 * number of websites with a lot of activity. For this reason the system
 * allows you to run this process on a backend server with the --action
 * command line option.
 *
 * The process is to:
 *
 * 1. go through all the rows of the list table (one row per website)
 * 2. go through all the columns of each row of the list table
 *    (one column per page that changed since the last update; note that
 *    it can continue to grow as we work on the list!)
 * 3. if the last update(s) happened more than LIST_PROCESSING_LATENCY
 *    then that specific page is processed and any list that include
 *    this page get updated appropriately
 * 4. entries that were processed between now and now + latency are
 *    ignored in this run (this way we avoid some problems where a client
 *    is still working on that page and thus the resulting sort of the
 *    list is not going to be accurate)
 *    TBD -- we may want to preprocess these and reprocess them at least
 *    LIST_PROCESSING_LATENCY later to make sure that the sort is correct;
 *    that way lists are still, in most cases, updated really quickly
 * 5. once we got a page that needs to be checked, we look whether this
 *    page is part of a list, if not then there is nothing to do
 *
 * \todo
 * At a later time we want to also add a way to mark a website as "standalone"
 * meaning that its lists are managed by a dedicated process (possibly even
 * a dedicated server.)
 *
 * \param[in] site_key  The site we want to process, if empty, process all
 *                      sites.
 *
 * \return 1 if the function changed anything, 0 otherwise
 */
int list::generate_all_lists(QString const& site_key)
{
    QtCassandra::QCassandraTable::pointer_t list_table(get_list_table());
    QtCassandra::QCassandraRow::pointer_t list_row(list_table->row(site_key));

    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setCount(100); // do one round then exit
    column_predicate.setIndex(); // behave like an index

    list_row->clearCache();
    list_row->readCells(column_predicate);
    const QtCassandra::QCassandraCells& cells(list_row->cells());
    if(cells.isEmpty())
    {
        return 0;
    }

    int did_work(0);

    // handle one batch
    for(QtCassandra::QCassandraCells::const_iterator c(cells.begin());
            c != cells.end();
            ++c)
    {
        // we cannot just use f_snap->get_start_date() since in the backend
        // that date does not get updated...
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        int64_t start_date = static_cast<int64_t>(tv.tv_sec) * static_cast<int64_t>(1000000)
                           + static_cast<int64_t>(tv.tv_usec);

        // the cell
        QtCassandra::QCassandraCell::pointer_t cell(*c);
        // the key is start date and a string representing the row key
        // in the content table
        QByteArray const& key(cell->columnKey());
        int64_t const page_start_date(QtCassandra::int64Value(key, 0));
        if(page_start_date + LIST_PROCESSING_LATENCY < start_date)
        {
            QString const row_key(QtCassandra::stringValue(key, sizeof(int64_t)));
            if(generate_all_lists_for_page(row_key) == 0)
            {
                did_work = 1;
            }
        }
    }

    // clear our cache
    f_check_expressions.clear();
    f_item_key_expressions.clear();

    return did_work;
}


int list::generate_all_lists_for_page(QString const& site_key)
{
    int did_work(0);

    content::path_info_t ipath;
    ipath.set_path(site_key);
    links::link_info info(get_name(SNAP_NAME_LIST_TYPE), false, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
    links::link_info child_info;
    while(link_ctxt->next_link(child_info))
    {
        QString const key(child_info.key());
        content::path_info_t list_ipath;
        list_ipath.set_path(key);
        //QtCassandra::QCassandraRow::pointer_t row(content_table->row(list_ipath.get_branch_key()));
        bool const included(run_list_check(list_ipath));
        QString const item_key(run_list_item_key(list_ipath));
        if(included)
        {
            // the check script says to include this item in this list;
            // first we need to check to find under which key it was
            // included if it is already there because it may have
            // changed
        }
        else
        {
            // the check script says that this path is not included in this
            // list; the item may be included from earlier so we have to
            // make sure it gets removed if still there
        }
    }

    return did_work;
}


/** \brief Retrieve the test script of a list.
 *
 * This function is used to extract the test script of a list object.
 * The test script is saved in the list::test_script field of a page,
 * on a per branch basis. This function makes use of the branch
 * defined in the ipath.
 *
 * \param[in,out] ipath  The ipath used to find the list.
 *
 * \return true if the page is to be included.
 */
bool list::run_list_check(content::path_info_t& ipath)
{
    QString const branch_key(ipath.get_branch_key());
    snap_expr::expr::expr_pointer_t e(nullptr);
    if(!f_check_expressions.contains(branch_key))
    {
        e = snap_expr::expr::expr_pointer_t(new snap_expr::expr);
        QByteArray program;
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t data_table(content_plugin->get_data_table());
        QtCassandra::QCassandraValue compiled_script(data_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_TEST_SCRIPT))->value());
        if(compiled_script.nullValue())
        {
            QtCassandra::QCassandraValue script(data_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT))->value());
            if(script.nullValue())
            {
                // no list here?!
                // TODO: generate an error
                return false;
            }
            if(!e->compile(script.stringValue()))
            {
                // script could not be compiled (invalid script!)
                // TODO: generate an error

                // create a default script so we do not try to compile the
                // broken script over and over again
                if(!e->compile("0"))
                {
                    // TODO: generate a double error!
                    //       this should really not happen
                    //       because "0" is definitively a valid script
                    return false;
                }
            }
            // save the result for next time
            data_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_TEST_SCRIPT))->setValue(e->serialize());
        }
        else
        {
            e->unserialize(compiled_script.binaryValue());
        }
        f_check_expressions[branch_key] = e;
    }
    else
    {
        e = f_check_expressions[branch_key];
    }

    // run the script with this path
    snap_expr::variable_t result;
    snap_expr::variable_t::variable_map_t variables;
    snap_expr::variable_t var_path(ipath.get_cpath());
    variables["path"] = var_path;
    snap_expr::functions_t functions;
    e->execute(result, variables, functions);

    return result.is_true();
}


/** \brief Generate the test script of a list.
 *
 * This function is used to extract the test script of a list object.
 * The test script is saved in the list::test_script field of a page,
 * on a per branch basis. This function makes use of the branch
 * defined in the ipath.
 *
 * \param[in,out] ipath  The ipath used to find the list.
 *
 * \return true if the page is to be included.
 */
bool list::run_list_item_key(content::path_info_t& ipath)
{
    QString const branch_key(ipath.get_branch_key());
    snap_expr::expr::expr_pointer_t e(nullptr);
    if(!f_item_key_expressions.contains(branch_key))
    {
        e = snap_expr::expr::expr_pointer_t(new snap_expr::expr);
        QByteArray program;
        content::content *content_plugin(content::content::instance());
        QtCassandra::QCassandraTable::pointer_t data_table(content_plugin->get_data_table());
        QtCassandra::QCassandraValue compiled_script(data_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_ITEM_KEY_SCRIPT))->value());
        if(compiled_script.nullValue())
        {
            QtCassandra::QCassandraValue script(data_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT))->value());
            if(script.nullValue())
            {
                // no list here?!
                // TODO: generate an error
                return false;
            }
            if(!e->compile(script.stringValue()))
            {
                // script could not be compiled (invalid script!)
                // TODO: generate an error

                // create a default script so we do not try to compile the
                // broken script over and over again
                if(!e->compile("---"))
                {
                    // TODO: generate a double error!
                    //       this should really not happen
                    //       because "0" is definitively a valid script
                    return false;
                }
            }
            // save the result for next time
            data_table->row(branch_key)->cell(get_name(SNAP_NAME_LIST_ITEM_KEY_SCRIPT))->setValue(e->serialize());
        }
        else
        {
            e->unserialize(compiled_script.binaryValue());
        }
        f_item_key_expressions[branch_key] = e;
    }
    else
    {
        e = f_item_key_expressions[branch_key];
    }

    // run the script with this path
    snap_expr::variable_t result;
    snap_expr::variable_t::variable_map_t variables;
    snap_expr::variable_t var_path(ipath.get_cpath());
    variables["path"] = var_path;
    snap_expr::functions_t functions;
    e->execute(result, variables, functions);

    return result.is_true();
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
