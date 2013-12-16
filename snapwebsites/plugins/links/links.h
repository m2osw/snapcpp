// Snap Websites Server -- links
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
#ifndef SNAP_LINKS_H
#define SNAP_LINKS_H

#include "snapwebsites.h"

namespace snap
{
namespace links
{

enum name_t {
    SNAP_NAME_LINKS_TABLE,         // Cassandra Table used for content (pages, comments, tags, vocabularies, etc.)
    SNAP_NAME_LINKS_NAMESPACE
};
const char *get_name(name_t name) __attribute__ ((const));


class links_exception : public snap_exception
{
public:
    links_exception(const char *what_msg) : snap_exception("Links: " + std::string(what_msg)) {}
    links_exception(const std::string& what_msg) : snap_exception("Links: " + what_msg) {}
    links_exception(const QString& what_msg) : snap_exception("Links: " + what_msg) {}
};

class links_exception_missing_links_table : public links_exception
{
public:
    links_exception_missing_links_table(const char *what_msg) : links_exception(what_msg) {}
    links_exception_missing_links_table(const std::string& what_msg) : links_exception(what_msg) {}
    links_exception_missing_links_table(const QString& what_msg) : links_exception(what_msg) {}
};

class links_exception_missing_content_table : public links_exception
{
public:
    links_exception_missing_content_table(const char *what_msg) : links_exception(what_msg) {}
    links_exception_missing_content_table(const std::string& what_msg) : links_exception(what_msg) {}
    links_exception_missing_content_table(const QString& what_msg) : links_exception(what_msg) {}
};

class links_exception_invalid_name : public links_exception
{
public:
    links_exception_invalid_name(const char *what_msg) : links_exception(what_msg) {}
    links_exception_invalid_name(const std::string& what_msg) : links_exception(what_msg) {}
    links_exception_invalid_name(const QString& what_msg) : links_exception(what_msg) {}
};

class links_exception_invalid_db_data : public links_exception
{
public:
    links_exception_invalid_db_data(const char *what_msg) : links_exception(what_msg) {}
    links_exception_invalid_db_data(const std::string& what_msg) : links_exception(what_msg) {}
    links_exception_invalid_db_data(const QString& what_msg) : links_exception(what_msg) {}
};



class link_info
{
public:
    link_info(const QString& new_name = "", bool unique = false, const QString& new_key = "")
        : f_unique(unique)
        , f_name(new_name)
        , f_key(new_key)
    {
        // empty is valid on construction
        if(!new_name.isEmpty())
        {
            verify_name(new_name);
        }
    }

    void set_name(const QString& new_name, bool unique = false)
    {
        verify_name(new_name);
        f_unique = unique;
        f_name = new_name;
    }
    void set_key(const QString& new_key)
    {
        f_key = new_key;
    }

    bool is_unique() const
    {
        return f_unique;
    }
    const QString& name() const
    {
        return f_name;
    }
    const QString& key() const
    {
        return f_key;
    }

    QString data() const;
    void from_data(const QString& db_data);

    static void verify_name(const QString& name);

private:
    controlled_vars::fbool_t    f_unique;
    QString                     f_name;
    QString                     f_key;
};

class link_context
{
public:
    bool next_link(link_info& info);

private:
    friend class links;

    link_context(::snap::snap_child *snap, const link_info& info);

    zpsnap_child_t                                  f_snap;
    link_info                                       f_info;
    QSharedPointer<QtCassandra::QCassandraRow>      f_row;
    QtCassandra::QCassandraColumnRangePredicate     f_column_predicate;
    QtCassandra::QCassandraCells::const_iterator    f_cell_iterator;
    QString                                         f_link;
};

class links : public plugins::plugin
{
public:
                        links();
                        ~links();

    static links *      instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_links_table();

    void                on_bootstrap(::snap::snap_child *snap);

    // should those be events?
    void                create_link(const link_info& src, const link_info& dst);
    void                delete_link(const link_info& info);

    QSharedPointer<link_context> new_link_context(const link_info& info);

private:
    void                initial_update(int64_t variables_timestamp);
    void                init_tables();

    zpsnap_child_t                                  f_snap;
    QSharedPointer<QtCassandra::QCassandraTable>    f_links_table;
    QSharedPointer<QtCassandra::QCassandraTable>    f_content_table;
};

} // namespace links
} // namespace snap
#endif
// SNAP_LINKS_H
// vim: ts=4 sw=4 et
