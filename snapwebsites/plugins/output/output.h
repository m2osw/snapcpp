// Snap Websites Server -- content management (pages, tags, everything!)
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

#include "../layout/layout.h"
#include "../content/content.h"
#include <controlled_vars/controlled_vars_ptr_no_init.h>

namespace snap
{
namespace output
{


enum name_t
{
    SNAP_NAME_CONTENT_ACCEPTED,
    SNAP_NAME_CONTENT_ATTACHMENT,
    //SNAP_NAME_CONTENT_ATTACHMENT_FILENAME,
    SNAP_NAME_CONTENT_ATTACHMENT_JAVASCRIPTS,
    //SNAP_NAME_CONTENT_ATTACHMENT_MIME_TYPE,
    SNAP_NAME_CONTENT_ATTACHMENT_OWNER,
    SNAP_NAME_CONTENT_ATTACHMENT_PATH_END,
    SNAP_NAME_CONTENT_ATTACHMENT_REVISION_CONTROL_LAST_BRANCH,
    SNAP_NAME_CONTENT_ATTACHMENT_REVISION_CONTROL_LAST_REVISION,
    SNAP_NAME_CONTENT_ATTACHMENT_REVISION_CONTROL_CURRENT,
    SNAP_NAME_CONTENT_ATTACHMENT_REVISION_CONTROL_CURRENT_WORKING_VERSION,
    SNAP_NAME_CONTENT_ATTACHMENT_REVISION_FILENAME,
    SNAP_NAME_CONTENT_ATTACHMENT_REVISION_FILENAME_WITH_VAR,
    SNAP_NAME_CONTENT_ATTACHMENT_REVISION_MIME_TYPE,
    SNAP_NAME_CONTENT_BODY,
    SNAP_NAME_CONTENT_BRANCH,
    SNAP_NAME_CONTENT_CHILDREN,
    SNAP_NAME_CONTENT_COMPRESSOR_UNCOMPRESSED,
    SNAP_NAME_CONTENT_CONTENT_TYPES,
    SNAP_NAME_CONTENT_CONTENT_TYPES_NAME,
    SNAP_NAME_CONTENT_COPYRIGHTED,
    SNAP_NAME_CONTENT_CREATED,
    SNAP_NAME_CONTENT_CURRENT_VERSION,
    SNAP_NAME_CONTENT_DATA_TABLE,
    SNAP_NAME_CONTENT_FILES_COMPRESSOR,
    SNAP_NAME_CONTENT_FILES_CREATED,
    SNAP_NAME_CONTENT_FILES_CREATION_TIME,
    SNAP_NAME_CONTENT_FILES_DATA,
    SNAP_NAME_CONTENT_FILES_DATA_COMPRESSED,
    SNAP_NAME_CONTENT_FILES_DEPENDENCY,
    SNAP_NAME_CONTENT_FILES_FILENAME,
    SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT,
    SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH,
    SNAP_NAME_CONTENT_FILES_MIME_TYPE,
    SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE,
    SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME,
    SNAP_NAME_CONTENT_FILES_NEW,
    SNAP_NAME_CONTENT_FILES_REFERENCE,
    SNAP_NAME_CONTENT_FILES_SECURE,
    SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK,
    SNAP_NAME_CONTENT_FILES_SECURITY_REASON,
    SNAP_NAME_CONTENT_FILES_SIZE,
    SNAP_NAME_CONTENT_FILES_SIZE_COMPRESSED,
    SNAP_NAME_CONTENT_FILES_TABLE,
    SNAP_NAME_CONTENT_FILES_UPDATED,
    SNAP_NAME_CONTENT_FINAL,
    SNAP_NAME_CONTENT_ISSUED,
    SNAP_NAME_CONTENT_LONG_TITLE,
    SNAP_NAME_CONTENT_MODIFIED,
    SNAP_NAME_CONTENT_PAGE_TYPE,
    SNAP_NAME_CONTENT_PARENT,
    SNAP_NAME_CONTENT_PRIMARY_OWNER,
    SNAP_NAME_CONTENT_REVISION_CONTROL,
    SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH,
    SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH_KEY,
    SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION,
    SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY,
    SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH,
    SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH_KEY,
    SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION,
    SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY,
    SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH,
    SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION,
    SNAP_NAME_CONTENT_SHORT_TITLE,
    SNAP_NAME_CONTENT_SINCE,
    SNAP_NAME_CONTENT_SUBMITTED,
    SNAP_NAME_CONTENT_TABLE,         // Cassandra Table used for content (pages, comments, tags, vocabularies, etc.)
    SNAP_NAME_CONTENT_TITLE,
    SNAP_NAME_CONTENT_UNTIL,
    SNAP_NAME_CONTENT_UPDATED,
    SNAP_NAME_CONTENT_VARIABLE_REVISION
};
char const *get_name(name_t name) __attribute__ ((const));


class content_exception : public snap_exception
{
public:
    content_exception(char const *what_msg) : snap_exception("Content: " + std::string(what_msg)) {}
    content_exception(std::string const& what_msg) : snap_exception("Content: " + what_msg) {}
    content_exception(QString const& what_msg) : snap_exception("Content: " + what_msg.toStdString()) {}
};

class content_exception_invalid_content_xml : public content_exception
{
public:
    content_exception_invalid_content_xml(char const *what_msg) : content_exception(what_msg) {}
    content_exception_invalid_content_xml(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_invalid_content_xml(QString const& what_msg) : content_exception(what_msg) {}
};

class content_exception_parameter_not_defined : public content_exception
{
public:
    content_exception_parameter_not_defined(char const *what_msg) : content_exception(what_msg) {}
    content_exception_parameter_not_defined(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_parameter_not_defined(QString const& what_msg) : content_exception(what_msg) {}
};

class content_exception_content_already_defined : public content_exception
{
public:
    content_exception_content_already_defined(char const *what_msg) : content_exception(what_msg) {}
    content_exception_content_already_defined(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_content_already_defined(QString const& what_msg) : content_exception(what_msg) {}
};

class content_exception_circular_dependencies : public content_exception
{
public:
    content_exception_circular_dependencies(char const *what_msg) : content_exception(what_msg) {}
    content_exception_circular_dependencies(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_circular_dependencies(QString const& what_msg) : content_exception(what_msg) {}
};

class content_exception_type_mismatch : public content_exception
{
public:
    content_exception_type_mismatch(char const *what_msg) : content_exception(what_msg) {}
    content_exception_type_mismatch(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_type_mismatch(QString const& what_msg) : content_exception(what_msg) {}
};

class content_exception_invalid_sequence : public content_exception
{
public:
    content_exception_invalid_sequence(char const *what_msg) : content_exception(what_msg) {}
    content_exception_invalid_sequence(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_invalid_sequence(QString const& what_msg) : content_exception(what_msg) {}
};

class content_exception_invalid_name : public content_exception
{
public:
    content_exception_invalid_name(char const *what_msg) : content_exception(what_msg) {}
    content_exception_invalid_name(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_invalid_name(QString const& what_msg) : content_exception(what_msg) {}
};

class content_exception_unexpected_revision_type : public content_exception
{
public:
    content_exception_unexpected_revision_type(char const *what_msg) : content_exception(what_msg) {}
    content_exception_unexpected_revision_type(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_unexpected_revision_type(QString const& what_msg) : content_exception(what_msg) {}
};





class output : public plugins::plugin, public content::path_execute, public layout::layout_content
{
public:
                        output();
                        ~output();

    static output *     instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    virtual bool        on_path_execute(path_info_t& ipath);
    virtual void        on_generate_main_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                on_generate_page_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);

    void                output() const;

    // add content for addition to the database
    void                add_xml(QString const& plugin_name);
    void                add_xml_document(QDomDocument& dom, QString const& plugin_name);
    void                add_content(QString const& path, QString const& plugin_owner);
    void                add_param(QString const& path, QString const& name, param_revision_t revision_type, QString const& locale, QString const& data);
    void                set_param_overwrite(QString const& path, const QString& name, bool overwrite);
    void                set_param_type(QString const& path, const QString& name, param_type_t param_type);
    void                add_link(QString const& path, links::link_info const& source, links::link_info const& destination);
    void                add_attachment(QString const& path, content_attachment const& attachment);
    static void         insert_html_string_to_xml_doc(QDomElement child, QString const& xml);
    bool                load_attachment(QString const& key, attachment_file& file, bool load_data = true);
    void                add_javascript(layout::layout *l, QString const& path, QDomElement& header, QDomElement& metadata, QString const& name);

private:
    // from the <param> tags
    struct content_param
    {
        QString                     f_name;
        QMap<QString, QString>      f_data; // [locale] = <html>
        param_revision_t            f_revision_type;
        controlled_vars::fbool_t    f_overwrite;
        safe_param_type_t           f_type;
    };
    typedef QMap<QString, content_param>    content_params_t;

    struct content_link
    {
        links::link_info            f_source;
        links::link_info            f_destination;
    };
    typedef QVector<content_link>   content_links_t;

    typedef QVector<content_attachment>    content_attachments_t;

    struct content_block
    {
        QString                     f_path;
        QString                     f_owner;
        content_params_t            f_params;
        content_links_t             f_links;
        content_attachments_t       f_attachments;
        controlled_vars::fbool_t    f_saved;
    };
    typedef QMap<QString, content_block>    content_block_map_t;

    struct javascript_ref_t
    {
        //QByteArray                  f_md5;
        QString                     f_name;
        QString                     f_filename;
    };
    typedef QVector<javascript_ref_t> javascript_ref_map_t;

    void initial_update(int64_t variables_timestamp);
    void content_update(int64_t variables_timestamp);

    zpsnap_child_t                                  f_snap;
    QSharedPointer<QtCassandra::QCassandraTable>    f_content_table;
    QSharedPointer<QtCassandra::QCassandraTable>    f_data_table;
    QSharedPointer<QtCassandra::QCassandraTable>    f_files_table;
    content_block_map_t                             f_blocks;
    controlled_vars::zint32_t                       f_file_index;
    controlled_vars::fbool_t                        f_updating;
    QMap<QString, bool>                             f_added_javascripts;
    javascript_ref_map_t                            f_javascripts;
    controlled_vars::zint64_t                       f_last_modified;
};


} // namespace output
} // namespace snap
// vim: ts=4 sw=4 et
