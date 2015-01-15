// Snap Websites Server -- tests for the links
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

#include "links.h"

// TODO: remove dependency on content (because content includes links...)
//       it may be that content and links should be merged (yuck!) TBD
#include "../content/content.h"

#include "log.h"
#include "not_reached.h"

#include <iostream>

#include "poison.h"

namespace snap
{
namespace links
{

SNAP_TEST_PLUGIN_SUITE(links)
    SNAP_TEST_PLUGIN_TEST(links, test_unique_unique_create_delete)
SNAP_TEST_PLUGIN_SUITE_END()


SNAP_TEST_PLUGIN_TEST_IMPL(links, test_unique_unique_create_delete)
{
    content::path_info_t source;
    content::path_info_t destination;

    source.set_path("js");
    destination.set_path("admin");

    bool const source_unique(true);
    bool const destination_unique(true);

    QString const source_name("test_unique_source");
    QString const destination_name("test_unique_destination");

    link_info source_info(source_name, source_unique, source.get_key(), source.get_branch());
    link_info destination_info(destination_name, destination_unique, destination.get_key(), destination.get_branch());

    create_link(source_info, destination_info);
}


} // namespace links
} // namespace snap

// vim: ts=4 sw=4 et
