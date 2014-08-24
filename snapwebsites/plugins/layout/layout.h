// Snap Websites Server -- layout management
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
#pragma once

#include "../content/content.h"

namespace snap
{
namespace layout
{

enum name_t
{
    SNAP_NAME_LAYOUT_ADMIN_LAYOUTS,
    SNAP_NAME_LAYOUT_BODY_XSL,
    SNAP_NAME_LAYOUT_BOX,
    SNAP_NAME_LAYOUT_BOXES,
    SNAP_NAME_LAYOUT_CONTENT,
    SNAP_NAME_LAYOUT_LAYOUT,
    SNAP_NAME_LAYOUT_REFERENCE,
    SNAP_NAME_LAYOUT_TABLE,
    SNAP_NAME_LAYOUT_THEME,
    SNAP_NAME_LAYOUT_THEME_XSL
};
char const *get_name(name_t name) __attribute__ ((const));


class layout_exception : public snap_exception
{
public:
    layout_exception(char const *       what_msg) : snap_exception("layout", what_msg) {}
    layout_exception(std::string const& what_msg) : snap_exception("layout", what_msg) {}
    layout_exception(QString const&     what_msg) : snap_exception("layout", what_msg) {}
};

class layout_exception_invalid_xslt_data : public layout_exception
{
public:
    layout_exception_invalid_xslt_data(char const *       what_msg) : layout_exception(what_msg) {}
    layout_exception_invalid_xslt_data(std::string const& what_msg) : layout_exception(what_msg) {}
    layout_exception_invalid_xslt_data(QString const&     what_msg) : layout_exception(what_msg) {}
};


class layout_content
{
public:
    virtual ~layout_content() {} // ensure proper virtual tables
    virtual void on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate) = 0;
};


class layout_boxes
{
public:
    virtual ~layout_boxes() {} // ensure proper virtual tables
    virtual void on_generate_boxes_content(content::path_info_t& page_ipath, content::path_info_t& ipath, QDomElement& page, QDomElement& boxes, QString const& ctemplate) = 0;
};


class layout : public plugins::plugin
{
public:
                        layout();
                        ~layout();

    static layout *     instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QtCassandra::QCassandraTable::pointer_t get_layout_table();

    void                on_bootstrap(snap_child *snap);
    void                on_load_file(snap_child::post_file_t& file, bool& found);

    QString             get_layout(content::path_info_t& ipath, const QString& column_name, bool use_qs_theme);
    QDomDocument        create_document(content::path_info_t& ipath, plugin *content_plugin);
    QString             apply_layout(content::path_info_t& ipath, layout_content *plugin, const QString& ctemplate = "");
    QString             define_layout(content::path_info_t& ipath, QString const& name, QString const& key, QString const& default_filename, QString& layout_name);
    void                create_body(QDomDocument& doc, content::path_info_t& ipath, QString const& xsl, layout_content *content_plugin, const QString& ctemplate = "", bool handle_boxes = false, QString const& layout_name = "");
    QString             apply_theme(QDomDocument doc, QString const& xsl, QString const& theme_name);
    void                replace_includes(QString& xsl);
    int64_t             install_layout(QString const& layout_name, int64_t const last_updated);
    void                add_layout_from_resources(QString const& name);
    void                extract_js_and_css(QDomDocument& doc, QDomDocument& doc_output);

    SNAP_SIGNAL(generate_header_content, (content::path_info_t& ipath, QDomElement& header, QDomElement& metadata, const QString& ctemplate), (ipath, header, metadata, ctemplate));
    SNAP_SIGNAL_WITH_MODE(generate_page_content, (content::path_info_t& ipath, QDomElement& page, QDomElement& body, const QString& ctemplate), (ipath, page, body, ctemplate), NEITHER);
    SNAP_SIGNAL_WITH_MODE(filtered_content, (content::path_info_t& ipath, QDomDocument& doc, QString const& xsl), (ipath, doc, xsl), NEITHER);

private:
    //void content_update(int64_t variables_timestamp);
    int64_t             do_layout_updates(int64_t const last_updated);

    void                generate_boxes(content::path_info_t& ipath, QString const& layout_name, QDomDocument doc);

    zpsnap_child_t                             f_snap;
    QtCassandra::QCassandraTable::pointer_t    f_content_table;
};

class layout_box_execute
{
public:
    virtual ~layout_box_execute() {} // ensure proper virtual tables
    virtual bool on_layout_box_execute(const QString& path, QDomElement& box) = 0;
};


} // namespace layout
} // namespace snap
// vim: ts=4 sw=4 et
