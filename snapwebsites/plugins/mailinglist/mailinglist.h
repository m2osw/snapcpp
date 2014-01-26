// Snap Websites Server -- mailing list system
// Copyright (C) 2013-2014  Made to Order Software Corp.
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

#include "snapwebsites.h"
#include "plugins.h"
#include "snap_child.h"
#include <QtSerialization/QSerializationReader.h>
#include <QMap>
#include <QVector>
#include <QByteArray>

namespace snap
{
namespace mailinglist
{

class mailinglist_exception : public snap_exception {};
class mailinglist_exception_no_magic : public mailinglist_exception {};
class mailinglist_exception_invalid_argument : public mailinglist_exception {};

enum name_t
{
    SNAP_NAME_MAILINGLIST_TABLE
};
const char *get_name(name_t name) __attribute__ ((const));


class mailinglist : public plugins::plugin
{
public:
    class list
    {
    public:
        static const int LIST_MAJOR_VERSION = 1;
        static const int LIST_MINOR_VERSION = 0;

        list(mailinglist *parent, const QString& list_name);
        virtual ~list();

        QString name() const;
        virtual QString next();

    private:
        mailinglist *                                   f_parent;
        const QString                                   f_name;
        QSharedPointer<QtCassandra::QCassandraTable>    f_table;
        QSharedPointer<QtCassandra::QCassandraRow>      f_row;
        QtCassandra::QCassandraColumnRangePredicate     f_column_predicate;
        QtCassandra::QCassandraCells                    f_cells;
        QtCassandra::QCassandraCells::const_iterator    f_c;
        controlled_vars::fbool_t                        f_done;
    };

    mailinglist();
    ~mailinglist();

    static mailinglist *instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_mailinglist_table();

    void                on_bootstrap(snap_child *snap);
    void                on_name_to_list(const QString& name, QSharedPointer<list>& emails);

    SNAP_SIGNAL(name_to_list, (const QString& name, QSharedPointer<list>& emails), (name, emails));

private:
    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);

    zpsnap_child_t      f_snap;
};

} // namespace mailinglist
} // namespace snap
// vim: ts=4 sw=4 et
