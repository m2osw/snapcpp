// Snap Websites Server -- server side javascript environment
// Copyright (C) 2011-2014  Made to Order Software Corp.
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
#ifndef SNAP_JAVASCRIPT_H
#define SNAP_JAVASCRIPT_H

#include "../links/links.h"
#include "../path/path.h"

namespace snap
{
namespace javascript
{

enum name_t
{
    SNAP_NAME_JAVASCRIPT_MINIMIZED,
    SNAP_NAME_JAVASCRIPT_MINIMIZED_COMPRESSED
};
char const *get_name(name_t name) __attribute__ ((const));


//class javascript_exception : public snap_exception {};
//class javascript_exception_circular_dependencies : public javascript_exception {};

class javascript_dynamic_plugin
{
public:
    virtual             ~javascript_dynamic_plugin() {}
    virtual int         js_property_count() const = 0;
    virtual QVariant    js_property_get(QString const& name) const = 0;
    virtual QString     js_property_name(int index) const = 0;
    virtual QVariant    js_property_get(int index) const = 0;
};


class javascript_filename
{
public:
    typedef QVector<controlled_vars::zuint32_t> version_t;

                        javascript_filename(QString const& filename, QString const& extension);

    bool                valid() const { return f_valid; }
    QString             error() const { return f_error; }
    QString             filename() const { return f_filename; }
    QString             name() const { return f_name; }
    version_t           version() const { return f_version; }

    int                 compare(javascript_filename const& rhs) const;
    bool                operator == (javascript_filename const& rhs) const;
    bool                operator != (javascript_filename const& rhs) const;
    bool                operator <  (javascript_filename const& rhs) const;
    bool                operator <= (javascript_filename const& rhs) const;
    bool                operator >  (javascript_filename const& rhs) const;
    bool                operator >= (javascript_filename const& rhs) const;

private:
    controlled_vars::fbool_t    f_valid;
    QString                     f_error;
    QString                     f_filename;
    QString                     f_name;
    version_t                   f_version;
};


class javascript : public plugins::plugin
{
public:
                        javascript();
                        ~javascript();

    static javascript * instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    void                on_process_attachment(QSharedPointer<QtCassandra::QCassandraTable> files_table, QByteArray const& key, snap_child::post_file_t const& file);
    void                on_check_attachment_security(snap_child::post_file_t const& file, server::permission_flag& secure, bool const fast);

    void                register_dynamic_plugin(javascript_dynamic_plugin *p);

    QVariant            evaluate_script(const QString& script);

private:
    friend class javascript_plugins_iterator;
    friend class plugins_class;

    void                initial_update(int64_t variables_timestamp);
    void                content_update(int64_t variables_timestamp);

    zpsnap_child_t                          f_snap;
    QVector<javascript_dynamic_plugin *>    f_dynamic_plugins;
};



} // namespace javascript
} // namespace snap
#endif
// SNAP_JAVSCRIPT_H
// vim: ts=4 sw=4 et
