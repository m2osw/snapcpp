// Snap Websites Server -- tests for the links
// Copyright (C) 2012-2017  Made to Order Software Corp.
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

// TODO: remove dependency on content (because content includes links...)
//       it may be that content and links should be merged (yuck!) TBD
#include "../content/content.h"

#include <snapwebsites/log.h>
#include <snapwebsites/not_reached.h>

#include <iostream>

#include <snapwebsites/poison.h>


SNAP_PLUGIN_EXTENSION_START(links)


SNAP_TEST_PLUGIN_SUITE(links)
    SNAP_TEST_PLUGIN_TEST(links, test_unique_unique_create_delete)
    SNAP_TEST_PLUGIN_TEST(links, test_multiple_multiple_create_delete)
SNAP_TEST_PLUGIN_SUITE_END()


SNAP_TEST_PLUGIN_TEST_IMPL(links, test_unique_unique_create_delete)
{
    content::content * content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    QtCassandra::QCassandraTable::pointer_t links_table(get_links_table());

    content::path_info_t source;
    content::path_info_t destination;

    bool const source_unique(true);
    bool const destination_unique(true);

    QString const source_name("test_plugin_suite::test_unique_source");
    QString const destination_name("test_plugin_suite::test_unique_destination");

    source.set_path("js");
    destination.set_path("admin");

    snap_version::version_number_t source_branch(source.get_branch());
    snap_version::version_number_t destination_branch(destination.get_branch());

    QString const source_field_name(QString("links::%1#%2").arg(source_name).arg(source_branch));
    QString const destination_field_name(QString("links::%1#%2").arg(destination_name).arg(destination_branch));

    QString const source_multilink_name(QString("%1/%2").arg(source.get_branch_key()).arg(source_field_name));
    QString const destination_multilink_name(QString("%1/%2").arg(destination.get_branch_key()).arg(destination_field_name));

    // first verify that those two pages exist
    SNAP_TEST_PLUGIN_SUITE_ASSERT(content_table->exists(source.get_key()))
    SNAP_TEST_PLUGIN_SUITE_ASSERT(content_table->exists(destination.get_key()))

    // second, check whether the already link exists, if so delete it
    if(branch_table->row(source.get_branch_key())->exists(source_field_name))
    {
        branch_table->row(source.get_branch_key())->dropCell(source_field_name);
        SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(source.get_branch_key())->exists(source_field_name))
    }
    if(branch_table->row(destination.get_branch_key())->exists(destination_field_name))
    {
        branch_table->row(destination.get_branch_key())->dropCell(destination_field_name);
        SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(destination.get_branch_key())->exists(destination_field_name))
    }

    // third, check that there are no multi-link definitions either
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!links_table->exists(source_multilink_name))
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!links_table->exists(destination_multilink_name))

    // now get ready to create the link
    link_info source_info(source_name, source_unique, source.get_key(), source.get_branch());
    link_info destination_info(destination_name, destination_unique, destination.get_key(), destination.get_branch());

    create_link(source_info, destination_info);

    // now those two fields must exist or we have a problem
    SNAP_TEST_PLUGIN_SUITE_ASSERT(branch_table->row(source.get_branch_key())->exists(source_field_name))
    SNAP_TEST_PLUGIN_SUITE_ASSERT(branch_table->row(destination.get_branch_key())->exists(destination_field_name))

    // but the multi-link must still not have been created
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!links_table->exists(source_multilink_name))
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!links_table->exists(destination_multilink_name))

    // delete the link, we expect both to get removed
    delete_link(source_info);

    // got deleted, it must be gone now
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(source.get_branch_key())->exists(source_field_name))
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(destination.get_branch_key())->exists(destination_field_name))
}


SNAP_TEST_PLUGIN_TEST_IMPL(links, test_multiple_multiple_create_delete)
{
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    QtCassandra::QCassandraTable::pointer_t links_table(get_links_table());

    content::path_info_t source;
    content::path_info_t destination;

    bool const source_unique(false);
    bool const destination_unique(false);

    QString const source_name("test_plugin_suite::test_multiple_source");
    QString const destination_name("test_plugin_suite::test_multiple_destination");

    source.set_path("js");
    destination.set_path("admin");

    snap_version::version_number_t source_branch(source.get_branch());
    snap_version::version_number_t destination_branch(destination.get_branch());

    QString const source_field_name(QString("links::%1#%2").arg(source_name).arg(source_branch));
    QString const destination_field_name(QString("links::%1#%2").arg(destination_name).arg(destination_branch));

    QString const source_field_multiname_start(QString("links::%1").arg(source_name));
    QString const destination_field_multiname_start(QString("links::%1").arg(destination_name));

    QString const source_hash_branch(QString("#%1").arg(source_branch));
    QString const destination_hash_branch(QString("#%1").arg(destination_branch));

    QString const source_multilink_name(QString("%1/%2").arg(source.get_branch_key()).arg(source_name));
    QString const destination_multilink_name(QString("%1/%2").arg(destination.get_branch_key()).arg(destination_name));

    QString source_multilink_unique_name;
    QString destination_multilink_unique_name;

    // 1. verify that those two pages exist
    SNAP_TEST_PLUGIN_SUITE_ASSERT(content_table->exists(source.get_key()))
    SNAP_TEST_PLUGIN_SUITE_ASSERT(content_table->exists(destination.get_key()))

    // 2. check whether the already link exists, if so delete it

    // 2.1 check with "unique" field names, although these should really not exist!
    if(branch_table->row(source.get_branch_key())->exists(source_field_name))
    {
        branch_table->row(source.get_branch_key())->dropCell(source_field_name);
        SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(source.get_branch_key())->exists(source_field_name))
    }
    if(branch_table->row(destination.get_branch_key())->exists(destination_field_name))
    {
        branch_table->row(destination.get_branch_key())->dropCell(destination_field_name);
        SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(destination.get_branch_key())->exists(destination_field_name))
    }

    // 2.2 check with multiple field names
    // 2.2.1 check the source
    {
        QtCassandra::QCassandraRow::pointer_t row(branch_table->row(source.get_branch_key()));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setStartCellKey(QString("%1-").arg(source_field_multiname_start));
        column_predicate->setEndCellKey(QString("%1.").arg(source_field_multiname_start));
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                QString const key(QString::fromUtf8(cell_iterator.key()));
                row->dropCell(key);
                SNAP_TEST_PLUGIN_SUITE_ASSERT(!row->exists(key))
            }
        }
    }

    // 2.2.2 check the destination
    {
        QtCassandra::QCassandraRow::pointer_t row(branch_table->row(destination.get_branch_key()));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setStartCellKey(QString("%1-").arg(destination_field_multiname_start));
        column_predicate->setEndCellKey(QString("%1.").arg(destination_field_multiname_start));
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                QString const key(QString::fromUtf8(cell_iterator.key()));
                row->dropCell(key);
                SNAP_TEST_PLUGIN_SUITE_ASSERT(!row->exists(key))
            }
        }
    }

    // 2.3 check links table with multiple field names
    if(links_table->exists(source_multilink_name))
    {
        links_table->dropRow(source_multilink_name);
    }
    if(links_table->exists(destination_multilink_name))
    {
        links_table->dropRow(destination_multilink_name);
    }

    // now get ready to create the link
    link_info source_info(source_name, source_unique, source.get_key(), source.get_branch());
    link_info destination_info(destination_name, destination_unique, destination.get_key(), destination.get_branch());

    create_link(source_info, destination_info);

    // the two unique fields should still not exist
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(source.get_branch_key())->exists(source_field_name))
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(destination.get_branch_key())->exists(destination_field_name))

    // however, we have got ONE multi-link now
    // search for it, and then verify it exists in the links table as expected
    {
        QtCassandra::QCassandraRow::pointer_t row(branch_table->row(source.get_branch_key()));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setStartCellKey(QString("%1-").arg(source_field_multiname_start));
        column_predicate->setEndCellKey(QString("%1.").arg(source_field_multiname_start));
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                // we have to make sure it is the right branch
                QString const key(QString::fromUtf8(cell_iterator.key()));
                if(key.endsWith(source_hash_branch))
                {
                    // there has to be only one
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(source_multilink_unique_name.isEmpty())
                    source_multilink_unique_name = key;
                    link_info info;
                    info.from_data(cell_iterator.value()->value().stringValue());
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(info.branch() == destination_branch)
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(info.name() == destination_name)
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(!info.is_unique())
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(info.key() == destination.get_key())
                }
            }
        }
        SNAP_TEST_PLUGIN_SUITE_ASSERT(!source_multilink_unique_name.isEmpty())
    }
    {
        QtCassandra::QCassandraRow::pointer_t row(branch_table->row(destination.get_branch_key()));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setStartCellKey(QString("%1-").arg(destination_field_multiname_start));
        column_predicate->setEndCellKey(QString("%1.").arg(destination_field_multiname_start));
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                // we have to make sure it is the right branch
                QString const key(QString::fromUtf8(cell_iterator.key()));
                if(key.endsWith(source_hash_branch))
                {
                    // there has to be only one
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(destination_multilink_unique_name.isEmpty())
                    destination_multilink_unique_name = key;
                    link_info info;
                    info.from_data(cell_iterator.value()->value().stringValue());
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(info.branch() == source_branch)
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(info.name() == source_name)
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(!info.is_unique())
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(info.key() == source.get_key())
                }
            }
        }
        SNAP_TEST_PLUGIN_SUITE_ASSERT(!destination_multilink_unique_name.isEmpty())
    }

    // in this case we must have those fields
    SNAP_TEST_PLUGIN_SUITE_ASSERT(links_table->exists(source_multilink_name))
    SNAP_TEST_PLUGIN_SUITE_ASSERT(links_table->exists(destination_multilink_name))

    // check for the links in the links table now
    // there must be only one, the key of the cell is the URI and the
    // value is the field name as we just read from the branch table
    {
        bool found(false);
        QtCassandra::QCassandraRow::pointer_t row(links_table->row(source_multilink_name));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                SNAP_TEST_PLUGIN_SUITE_ASSERT(!found)
                found = true;

                // there has to be only one
                SNAP_TEST_PLUGIN_SUITE_ASSERT(destination.get_key() == QString::fromUtf8(cell_iterator.key()))
                SNAP_TEST_PLUGIN_SUITE_ASSERT(source_multilink_unique_name == cell_iterator.value()->value().stringValue())
            }
        }
        SNAP_TEST_PLUGIN_SUITE_ASSERT(found)
    }
    {
        bool found(false);
        QtCassandra::QCassandraRow::pointer_t row(links_table->row(destination_multilink_name));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setCount(100);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                SNAP_TEST_PLUGIN_SUITE_ASSERT(!found)
                found = true;

                // there has to be only one
                SNAP_TEST_PLUGIN_SUITE_ASSERT(source.get_key() == QString::fromUtf8(cell_iterator.key()))
                SNAP_TEST_PLUGIN_SUITE_ASSERT(destination_multilink_unique_name == cell_iterator.value()->value().stringValue())
            }
        }
        SNAP_TEST_PLUGIN_SUITE_ASSERT(found)
    }

    // delete the link, we expect both to get removed
    delete_link(source_info);

    // the unique entry are still not there
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(source.get_branch_key())->exists(source_field_name))
    SNAP_TEST_PLUGIN_SUITE_ASSERT(!branch_table->row(destination.get_branch_key())->exists(destination_field_name))

    // now check that all the multi-link data was indeed removed as expected
    {
        QtCassandra::QCassandraRow::pointer_t row(branch_table->row(source.get_branch_key()));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setStartCellKey(QString("%1-").arg(source_field_multiname_start));
        column_predicate->setEndCellKey(QString("%1.").arg(source_field_multiname_start));
        column_predicate->setCount(3);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                // we have to make sure it is the right branch
                QString const key(QString::fromUtf8(cell_iterator.key()));
                if(key.endsWith(source_hash_branch))
                {
                    // there has to be only one
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(false)
                }
            }
        }
    }
    {
        QtCassandra::QCassandraRow::pointer_t row(branch_table->row(destination.get_branch_key()));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setStartCellKey(QString("%1-").arg(destination_field_multiname_start));
        column_predicate->setEndCellKey(QString("%1.").arg(destination_field_multiname_start));
        column_predicate->setCount(3);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                // we have to make sure it is the right branch
                QString const key(QString::fromUtf8(cell_iterator.key()));
                if(key.endsWith(source_hash_branch))
                {
                    // there has to be only one
                    SNAP_TEST_PLUGIN_SUITE_ASSERT(false)
                }
            }
        }
    }

    // those rows show disappear, but by this time we cannot be
    // sure they are; instead we just verify that the columns are
    // all gone below
    //SNAP_TEST_PLUGIN_SUITE_ASSERT(links_table->exists(source_multilink_name))
    //SNAP_TEST_PLUGIN_SUITE_ASSERT(links_table->exists(destination_multilink_name))

    // check for the links in the links table now
    // there must be only one, the key of the cell is the URI and the
    // value is the field name as we just read from the branch table
    {
        QtCassandra::QCassandraRow::pointer_t row(links_table->row(source_multilink_name));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setCount(3);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                SNAP_TEST_PLUGIN_SUITE_ASSERT(false)
            }
        }
    }
    {
        QtCassandra::QCassandraRow::pointer_t row(links_table->row(destination_multilink_name));
        row->clearCache();
        auto column_predicate = std::make_shared<QtCassandra::QCassandraCellRangePredicate>();
        column_predicate->setCount(3);
        column_predicate->setIndex(); // behave like an index
        for(;;)
        {
            // we MUST clear the cache in case we read the same list of links twice
            row->readCells(column_predicate);
            QtCassandra::QCassandraCells const cells(row->cells());
            if(cells.empty())
            {
                // all columns read
                break;
            }
            for(QtCassandra::QCassandraCells::const_iterator cell_iterator(cells.begin()); cell_iterator != cells.end(); ++cell_iterator)
            {
                SNAP_TEST_PLUGIN_SUITE_ASSERT(false)
            }
        }
    }

}


SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et
