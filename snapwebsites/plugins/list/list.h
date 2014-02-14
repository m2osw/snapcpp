// Snap Websites Server -- list management (sort criteria)
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

#include "../layout/layout.h"

#include "snap_expr.h"


namespace snap
{
namespace list
{


enum name_t
{
    SNAP_NAME_LIST_LAST_UPDATED,
    SNAP_NAME_LIST_ORIGINAL_ITEM_KEY_SCRIPT,
    SNAP_NAME_LIST_ORIGINAL_TEST_SCRIPT,
    SNAP_NAME_LIST_PAGELIST,
    SNAP_NAME_LIST_SELECTOR,
    SNAP_NAME_LIST_STANDALONE,
    SNAP_NAME_LIST_STANDALONELIST,
    SNAP_NAME_LIST_STOP,
    SNAP_NAME_LIST_TABLE,
    SNAP_NAME_LIST_ITEM_KEY_SCRIPT,
    SNAP_NAME_LIST_TEST_SCRIPT,
    SNAP_NAME_LIST_TYPE
};
char const *get_name(name_t name) __attribute__ ((const));


class list_exception : public snap_exception
{
public:
    list_exception(char const *what_msg)        : snap_exception("list", std::string(what_msg)) {}
    list_exception(std::string const& what_msg) : snap_exception("list", what_msg) {}
    list_exception(QString const& what_msg)     : snap_exception("list", what_msg.toStdString()) {}
};

class list_exception_unknown_function : public list_exception
{
public:
    list_exception_unknown_function(char const *what_msg)        : list_exception(what_msg) {}
    list_exception_unknown_function(std::string const& what_msg) : list_exception(what_msg) {}
    list_exception_unknown_function(QString const& what_msg)     : list_exception(what_msg) {}
};

class list_exception_invalid_number_of_parameters : public list_exception
{
public:
    list_exception_invalid_number_of_parameters(char const *what_msg)        : list_exception(what_msg) {}
    list_exception_invalid_number_of_parameters(std::string const& what_msg) : list_exception(what_msg) {}
    list_exception_invalid_number_of_parameters(QString const& what_msg)     : list_exception(what_msg) {}
};

class list_exception_invalid_parameter_type : public list_exception
{
public:
    list_exception_invalid_parameter_type(char const *what_msg)        : list_exception(what_msg) {}
    list_exception_invalid_parameter_type(std::string const& what_msg) : list_exception(what_msg) {}
    list_exception_invalid_parameter_type(QString const& what_msg)     : list_exception(what_msg) {}
};









class list : public plugins::plugin, public server::backend_action, public layout::layout_content
{
public:
    static int const LIST_PROCESSING_LATENCY = 10 * 1000000; // 10 seconds in micro-seconds

                        list();
                        ~list();

    static list *       instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QtCassandra::QCassandraTable::pointer_t get_list_table();

    void                on_bootstrap(snap_child *snap);
    void                on_register_backend_action(server::backend_action_map_t& actions);
    virtual void        on_backend_action(QString const& action);
    void                on_backend_process();
    virtual void        on_generate_main_content(content::path_info_t& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                on_generate_page_content(content::path_info_t& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                on_create_content(content::path_info_t& ipath, QString const& owner, QString const& type);
    void                on_modified_content(content::path_info_t& ipath);

private:
    void                initial_update(int64_t variables_timestamp);
    void                content_update(int64_t variables_timestamp);
    int                 generate_all_lists(QString const& site_key);
    int                 generate_all_lists_for_page(QString const& site_key);
    bool                run_list_check(content::path_info_t& ipath);
    bool                run_list_item_key(content::path_info_t& ipath);

    zpsnap_child_t                          f_snap;
    QtCassandra::QCassandraTable::pointer_t f_list_table;
    snap_expr::expr::expr_map_t             f_check_expressions;
    snap_expr::expr::expr_map_t             f_item_key_expressions;
};


} // namespace list
} // namespace snap
// vim: ts=4 sw=4 et
