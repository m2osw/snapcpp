// Snap Websites Server -- content management (pages, tags, everything!)
// Copyright (C) 2011-2013  Made to Order Software Corp.
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
#ifndef SNAP_CONTENT_H
#define SNAP_CONTENT_H

#include "../links/links.h"
#include "../path/path.h"
#include "../layout/layout.h"
#include "../javascript/javascript.h"

namespace snap
{
namespace content
{

enum name_t {
    SNAP_NAME_CONTENT_ACCEPTED,
    SNAP_NAME_CONTENT_BODY,
    SNAP_NAME_CONTENT_CHILDREN,
    SNAP_NAME_CONTENT_CONTENT_TYPES,
    SNAP_NAME_CONTENT_CONTENT_TYPES_NAME,
    SNAP_NAME_CONTENT_COPYRIGHTED,
    SNAP_NAME_CONTENT_CREATED,
    SNAP_NAME_CONTENT_ISSUED,
    SNAP_NAME_CONTENT_LONG_TITLE,
    SNAP_NAME_CONTENT_MODIFIED,
    SNAP_NAME_CONTENT_PAGE_CONTENT_TYPE,
    SNAP_NAME_CONTENT_PARENT,
    SNAP_NAME_CONTENT_SHORT_TITLE,
    SNAP_NAME_CONTENT_SINCE,
    SNAP_NAME_CONTENT_SUBMITTED,
    SNAP_NAME_CONTENT_TABLE,         // Cassandra Table used for content (pages, comments, tags, vocabularies, etc.)
    SNAP_NAME_CONTENT_TITLE,
    SNAP_NAME_CONTENT_UNTIL,
    SNAP_NAME_CONTENT_UPDATED
};
const char *get_name(name_t name);


class content_exception : public snap_exception
{
public:
    content_exception(const char *what_msg) : snap_exception("Content: " + std::string(what_msg)) {}
    content_exception(const std::string& what_msg) : snap_exception("Content: " + what_msg) {}
    content_exception(const QString& what_msg) : snap_exception("Content: " + what_msg.toStdString()) {}
};

class content_exception_invalid_content_xml : public content_exception
{
public:
    content_exception_invalid_content_xml(const char *what_msg) : content_exception(what_msg) {}
    content_exception_invalid_content_xml(const std::string& what_msg) : content_exception(what_msg) {}
    content_exception_invalid_content_xml(const QString& what_msg) : content_exception(what_msg) {}
};

class content_exception_parameter_not_defined : public content_exception
{
public:
    content_exception_parameter_not_defined(const char *what_msg) : content_exception(what_msg) {}
    content_exception_parameter_not_defined(const std::string& what_msg) : content_exception(what_msg) {}
    content_exception_parameter_not_defined(const QString& what_msg) : content_exception(what_msg) {}
};

class content_exception_content_already_defined : public content_exception
{
public:
    content_exception_content_already_defined(const char *what_msg) : content_exception(what_msg) {}
    content_exception_content_already_defined(const std::string& what_msg) : content_exception(what_msg) {}
    content_exception_content_already_defined(const QString& what_msg) : content_exception(what_msg) {}
};

class content_exception_circular_dependencies : public content_exception
{
public:
    content_exception_circular_dependencies(const char *what_msg) : content_exception(what_msg) {}
    content_exception_circular_dependencies(const std::string& what_msg) : content_exception(what_msg) {}
    content_exception_circular_dependencies(const QString& what_msg) : content_exception(what_msg) {}
};


class content : public plugins::plugin, public path::path_execute, public layout::layout_content, public javascript::javascript_dynamic_plugin
{
public:
                        content();
                        ~content();

    static content *    instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_content_table();
    QtCassandra::QCassandraValue get_content_parameter(const QString& path, const QString& name);

    void                on_bootstrap(snap_child *snap);
    virtual bool        on_path_execute(const QString& url);
    void                on_save_content();
    void                on_create_content(const QString& path);
    virtual void        on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate);
    void                on_generate_page_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate);

    SNAP_SIGNAL(new_content, (const QString& path), (path));
    SNAP_SIGNAL(create_content, (const QString& path, const QString& owner), (path, owner));

    void                output() const;

    // add content for addition to the database
    void                add_xml(const QString& plugin_name);
    void                add_content(const QString& path, const QString& plugin_owner);
    void                add_param(const QString& path, const QString& name, const QString& data);
    void                set_param_overwrite(const QString& path, const QString& name, bool overwrite);
    void                add_link(const QString& path, const links::link_info& source, const links::link_info& destination);

    virtual int         js_property_count() const;
    virtual QVariant    js_property_get(const QString& name) const;
    virtual QString     js_property_name(int index) const;
    virtual QVariant    js_property_get(int index) const;

private:
    // from the <param> tags
    struct content_param
    {
        QString                     f_name;
        QString                     f_data;
        controlled_vars::fbool_t    f_overwrite;
    };
    typedef QMap<QString, content_param>    content_params_t;

    struct content_link
    {
        links::link_info            f_source;
        links::link_info            f_destination;
    };
    typedef QVector<content_link>   content_links_t;

    struct content_block
    {
        QString                             f_path;
        QString                             f_owner;
        content_params_t                    f_params;
        content_links_t                     f_links;
        controlled_vars::fbool_t            f_saved;
    };
    typedef QMap<QString, content_block>    content_block_map_t;

    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);

    zpsnap_child_t                                  f_snap;
    QSharedPointer<QtCassandra::QCassandraTable>    f_content_table;
    content_block_map_t                             f_blocks;
};

class content_box_execute
{
public:
    virtual             ~content_box_execute() {} // ensure proper virtual tables
    virtual bool        on_content_box_execute(content *c, const QString& path, QDomElement& box) = 0;
};


} // namespace content
} // namespace snap
#endif
// SNAP_CONTENT_H
// vim: ts=4 sw=4 et
