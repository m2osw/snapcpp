// Snap Websites Server -- handle the output of attachments
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
#pragma once

#include "../path/path.h"


namespace snap
{
namespace attachment
{


//enum name_t
//{
//    SNAP_NAME_ATTACHMENT_...
//};
//char const *get_name(name_t name) __attribute__ ((const));


class attachment_exception : public snap_exception
{
public:
    attachment_exception(char const *       what_msg) : snap_exception("attachment", what_msg) {}
    attachment_exception(std::string const& what_msg) : snap_exception("attachment", what_msg) {}
    attachment_exception(QString const&     what_msg) : snap_exception("attachment", what_msg) {}
};

class attachment_exception_invalid_content_xml : public attachment_exception
{
public:
    attachment_exception_invalid_content_xml(char const *       what_msg) : attachment_exception(what_msg) {}
    attachment_exception_invalid_content_xml(std::string const& what_msg) : attachment_exception(what_msg) {}
    attachment_exception_invalid_content_xml(QString const&     what_msg) : attachment_exception(what_msg) {}
};







class attachment : public plugins::plugin, public path::path_execute
{
public:
                        attachment();
                        ~attachment();

    static attachment * instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    void                on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info);
    virtual bool        on_path_execute(content::path_info_t& ipath);
    void                on_page_cloned(content::content::cloned_tree_t const& tree);
    void                on_copy_branch_cells(QtCassandra::QCassandraCells& source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, snap_version::version_number_t const destination_branch);

private:
    void                content_update(int64_t variables_timestamp);

    zpsnap_child_t      f_snap;
};


} // namespace attachment
} // namespace snap
// vim: ts=4 sw=4 et
