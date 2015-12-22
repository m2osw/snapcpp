// Snap Websites Server -- all the user content and much of the system content
// Copyright (C) 2011-2015  Made to Order Software Corp.
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
 * \brief The implementation of the content plugin class cloning functionality.
 *
 * A page can be cloned for various reasons:
 *
 * * change the path to the page, in this case you want to move the page
 * * to delete the page, this is generally done by moving the page to
 *   the trashcan (so this is a move page too!)
 */

#include "content.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_EXTENSION_START(content)


/** \brief Destroy a page.
 *
 * \warning
 * This function DESTROYS a page RECURSIVELY. So the specified page and
 * all the children of that page will ALL get DESTROYED.
 *
 * This function can be used to DESTROY a page.
 *
 * 99.99% of the time, you should use the trash_page() function which
 * will safely move the existing page to the trashcan and destroy the
 * data only at a later time.
 *
 * In many other systesm, this function would probably be just called
 * delete_page(). However, in our case, we wanted to clearly stress
 * out the fact that this function is to be used as a last resort in
 * very very few cases.
 *
 * For example, you are a programmer and you created 1,000 pages by
 * mistake and just want to get rid of them without having to delete
 * the whole database and restart populating your database. That's
 * an acceptable use case of this function.
 *
 * \warning
 * DO NOT USE THIS FUNCTION. This function destroys a page and may
 * create all sorts of problems as a result. Many pages are necessary
 * for all sorts of reasons and just destroying them may generate
 * side effects in the code that are totally unexpected. Look into
 * using trash_page() instead.
 *
 * \bug
 * There is no locking mechanism. If some other process accesses the
 * page while it is being deleted, unexpected behavior may result.
 *
 * \bug
 * The deletions scans the ENTIRE revision and branch tables to find
 * all the entries to delete for a given page. This is SLOW.
 *
 * \bug
 * The deletion of children uses recursion on the stack. A website
 * with a very large number of children could use a lot of memory
 * for this process.
 *
 * \note
 * This signal is used by the content plugin itself to make the trashed
 * pages disappear after a certain amount of time. This applies to
 * both: the original page and the page in the trashcan. It may first
 * apply to the original page quickly (within a day or two) and then
 * to the trashed page after some time (we may actually add a minimum
 * amount of time the page would stay in the trashcan such as 2 months
 * and then it gets destroyed.) By default, trash is never deleted.
 * It is kept in the trashcan forever (which is the safest thing we
 * can do.)
 *
 * \param[in] ipath  The path to the page to destroy.
 *
 * \return This function always returns true.
 */
bool content::destroy_page_impl(path_info_t & ipath)
{
    links::links *links_plugin(links::links::instance());

    // here we check whether we have children, because if we do
    // we have to delete the children first
    {
        links::link_info link_info(get_name(name_t::SNAP_NAME_CONTENT_CHILDREN), false, ipath.get_key(), ipath.get_branch());
        QSharedPointer<links::link_context> link_ctxt(links_plugin->new_link_context(link_info));
        links::link_info link_child_info;
        while(link_ctxt->next_link(link_child_info))
        {
            path_info_t child_ipath;
            child_ipath.set_path(link_child_info.key());
            destroy_page(child_ipath);
        }
    }

    // the links plugin cannot include content.h (at least not the
    // header) so we have to implement the deletion of all the links
    // on this page here
    {
        links::link_info_pair::vector_t all_links(links_plugin->list_of_links(ipath.get_key()));
        for(auto l : all_links)
        {
            links_plugin->delete_this_link(l.source(), l.destination());
        }
    }

    return true;
}


/** \brief Finish up the destruction of a page.
 *
 * This function is called once all the other plugins were called and
 * deleted the data that they are responsible for.
 *
 * \param[in] ipath  The path of the page being deleted.
 */
void content::destroy_page_done(path_info_t & ipath)
{
    // here we actually drop the page data: all the revisions, branches
    // and the main content page

    QString const key(ipath.get_key());
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());

    // if you have problems with the deletion of some parts of that page
    // (i.e. some things did not get deleted) then you will want to use
    // a manual process... look into using cassview to delete the remains
    // and fix the corresponding plugins for next time.
    if(!content_table->exists(key))
    {
        return;
    }

    // Revisions
    {
        snap_string_list revision_keys;
        QtCassandra::QCassandraTable::pointer_t revision_table(get_revision_table());
        QtCassandra::QCassandraRowPredicate row_predicate;
        row_predicate.setCount(1000);
        for(;;)
        {
            revision_table->clearCache();
            uint32_t const count(revision_table->readRows(row_predicate));
            if(count == 0)
            {
                // no more revisions to process
                break;
            }
            QtCassandra::QCassandraRows const rows(revision_table->rows());
            for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                    o != rows.end(); ++o)
            {
                // within each row, check all the columns
                QtCassandra::QCassandraRow::pointer_t row(*o);
                QString const revision_key(QString::fromUtf8(o.key().data()));
                if(!revision_key.startsWith(key))
                {
                    // not this page, try another key
                    continue;
                }

                // this was part of this page
                revision_keys.push_back(revision_key);
            }
        }

        // now do the deletion (we avoid that in the loop to make sure the
        // loop works as expected)
        int const max_keys(revision_keys.size());
        for(int idx(0); idx < max_keys; ++idx)
        {
            revision_table->dropRow(revision_keys[idx]);
        }
    }

    // Branches
    {
        snap_string_list branch_keys;
        QtCassandra::QCassandraTable::pointer_t branch_table(get_branch_table());
        QtCassandra::QCassandraRowPredicate row_predicate;
        row_predicate.setCount(1000);
        for(;;)
        {
            branch_table->clearCache();
            uint32_t const count(branch_table->readRows(row_predicate));
            if(count == 0)
            {
                // no more revisions to process
                break;
            }
            QtCassandra::QCassandraRows const rows(branch_table->rows());
            for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                    o != rows.end(); ++o)
            {
                // within each row, check all the columns
                QtCassandra::QCassandraRow::pointer_t row(*o);
                QString const branch_key(QString::fromUtf8(o.key().data()));
                if(!branch_key.startsWith(key))
                {
                    // not this page, try another key
                    continue;
                }

                // this was part of this page
                branch_keys.push_back(branch_key);
            }
        }

        // now do the deletion (we avoid that in the loop to make sure the
        // loop works as expected)
        int const max_keys(branch_keys.size());
        for(int idx(0); idx < max_keys; ++idx)
        {
            branch_table->dropRow(branch_keys[idx]);
        }
    }

    // Finally, get rid of the content row
    {
        content_table->dropRow(ipath.get_key());
    }
}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
