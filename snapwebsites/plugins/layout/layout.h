// Snap Websites Server -- layout management
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
#ifndef SNAP_LAYOUT_H
#define SNAP_LAYOUT_H

#include "../javascript/javascript.h"

namespace snap
{
namespace layout
{

enum name_t
{
    SNAP_NAME_LAYOUT_TABLE,
    SNAP_NAME_LAYOUT_THEME,
    SNAP_NAME_LAYOUT_LAYOUT
};
const char *get_name(name_t name);


class layout;
class layout_content
{
public:
    virtual ~layout_content() {} // ensure proper virtual tables
    virtual void on_generate_main_content(layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate) = 0;
};


class layout : public plugins::plugin //, public javascript::javascript_dynamic_plugin
{
public:
                        layout();
                        ~layout();

    static layout *     instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_layout_table();

    void                on_bootstrap(snap_child *snap);
    void                on_get_dynamic_plugins(javascript::javascript *js);

    QString             get_layout(const QString& cpath, const QString& column_name);
    QString             apply_layout(const QString& cpath, layout_content *plugin, const QString& ctemplate = "");
    QDomDocument        create_body(const QString& cpath, layout_content *content_plugin, const QString& ctemplate = "");
    QString             apply_theme(QDomDocument doc, const QString& cpath, layout_content *content_plugin);

    SNAP_SIGNAL(generate_header_content, (layout *l, const QString& path, QDomElement& header, QDomElement& metadata, const QString& ctemplate), (l, path, header, metadata, ctemplate));
    SNAP_SIGNAL(generate_page_content, (layout *l, const QString& path, QDomElement& page, QDomElement& body, const QString& ctemplate), (l, path, page, body, ctemplate));
    //SNAP_SIGNAL(generate_box_content, (layout *l, const QString& path, QDomElement& box), (l, path, box));

    //virtual int js_property_count() const;
    //virtual QVariant js_property_get(const QString& name) const;
    //virtual QString js_property_name(int index) const;
    //virtual QVariant js_property_get(int index) const;

private:
    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);
    bool parse_layout_rules(const QString& script, QByteArray& result);

    zpsnap_child_t                                  f_snap;
    QSharedPointer<QtCassandra::QCassandraTable>    f_content_table;

    // output document in XML format while building the output
    //QDomDocument                                    f_doc;
    //QDomElement                                     f_header;
    //QDomElement                                     f_page;
    //QVector<QDomElement>                            f_boxes;
};

class layout_box_execute
{
public:
    virtual ~layout_box_execute() {} // ensure proper virtual tables
    virtual bool on_layout_box_execute(layout *l, const QString& path, QDomElement& box) = 0;
};


} // namespace layout
} // namespace snap
#endif
// SNAP_LAYOUT_H
// vim: ts=4 sw=4 et
