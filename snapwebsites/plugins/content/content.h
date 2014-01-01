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

#include "../layout/layout.h"
#include "../javascript/javascript.h"
#include "../links/links.h"
#include "../path/path.h"

namespace snap
{
namespace content
{


enum name_t {
    SNAP_NAME_CONTENT_ACCEPTED,
    SNAP_NAME_CONTENT_ATTACHMENT,
    SNAP_NAME_CONTENT_ATTACHMENT_FILENAME,
    SNAP_NAME_CONTENT_ATTACHMENT_MIME_TYPE,
    SNAP_NAME_CONTENT_ATTACHMENT_PATH_END,
    SNAP_NAME_CONTENT_BODY,
    SNAP_NAME_CONTENT_CHILDREN,
    SNAP_NAME_CONTENT_COMPRESSOR_UNCOMPRESSED,
    SNAP_NAME_CONTENT_CONTENT_TYPES,
    SNAP_NAME_CONTENT_CONTENT_TYPES_NAME,
    SNAP_NAME_CONTENT_COPYRIGHTED,
    SNAP_NAME_CONTENT_CREATED,
    SNAP_NAME_CONTENT_FILES_COMPRESSOR,
    SNAP_NAME_CONTENT_FILES_CREATED,
    SNAP_NAME_CONTENT_FILES_CREATION_TIME,
    SNAP_NAME_CONTENT_FILES_DATA,
    SNAP_NAME_CONTENT_FILES_DATA_COMPRESSED,
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
    SNAP_NAME_CONTENT_SHORT_TITLE,
    SNAP_NAME_CONTENT_SINCE,
    SNAP_NAME_CONTENT_SUBMITTED,
    SNAP_NAME_CONTENT_TABLE,         // Cassandra Table used for content (pages, comments, tags, vocabularies, etc.)
    SNAP_NAME_CONTENT_TITLE,
    SNAP_NAME_CONTENT_UNTIL,
    SNAP_NAME_CONTENT_UPDATED
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




class field_search
{
public:
    enum command_t
    {
        COMMAND_UNKNOWN,                // (cannot be used)

        // retrieve from Cassandra
        COMMAND_FIELD_NAME,             // + field name
        COMMAND_MODE,                   // + mode (int)

        COMMAND_SELF,                   // no parameters
        COMMAND_PATH,                   // + path
        COMMAND_CHILDREN,               // + depth (int)
        COMMAND_PARENTS,                // + path (limit)
        COMMAND_LINK,                   // + link name

        COMMAND_DEFAULT_VALUE,          // + data
        COMMAND_DEFAULT_VALUE_OR_NULL,  // + data (ignore if data is null)

        // save in temporary XML for XSLT
        COMMAND_ELEMENT,                // + QDomElement
        COMMAND_CHILD_ELEMENT,          // + child name
        COMMAND_PARENT_ELEMENT,         // no parameters
        COMMAND_ELEMENT_ATTR,           // + QDomElement
        COMMAND_RESULT,                 // + search_result_t
        COMMAND_SAVE,                   // + child name
        COMMAND_SAVE_INT64,             // + child name
        COMMAND_SAVE_INT64_DATE,        // + child name
        COMMAND_SAVE_XML,               // + child name

        // other types of commands
        COMMAND_LABEL,                  // + label number
        COMMAND_GOTO,                   // + label number
        COMMAND_IF_FOUND,               // + label number
        COMMAND_IF_NOT_FOUND,           // + label number
        COMMAND_RESET,                  // no parameters
        COMMAND_WARNING                 // + warning message
    };
    typedef controlled_vars::limited_auto_init<command_t, COMMAND_UNKNOWN, COMMAND_DEFAULT_VALUE, COMMAND_UNKNOWN> safe_command_t;

    enum mode_t
    {
        SEARCH_MODE_FIRST,            // default mode: only return first parameter found
        SEARCH_MODE_EACH,             // return a list of QCassandraValue's of all the entire tree
        SEARCH_MODE_PATHS             // return a list of paths (for debug purposes usually)
    };
    typedef controlled_vars::limited_auto_init<mode_t, SEARCH_MODE_FIRST, SEARCH_MODE_PATHS, SEARCH_MODE_FIRST> safe_mode_t;

    typedef QVector<QtCassandra::QCassandraValue> search_result_t;

    class cmd_info_t
    {
    public:
                            cmd_info_t();
                            cmd_info_t(command_t cmd);
                            cmd_info_t(command_t cmd, QString const& str_value);
                            cmd_info_t(command_t cmd, int64_t int_value);
                            cmd_info_t(command_t cmd, QtCassandra::QCassandraValue& value);
                            cmd_info_t(command_t cmd, QDomElement element);
                            cmd_info_t(command_t cmd, search_result_t& result);

        command_t           get_command() const { return f_cmd; }
        QString const       get_string()  const { return f_value.stringValue(); }
        //int32_t             get_int32() const { return f_value.int32Value(); }
        int64_t             get_int64() const { return f_value.int64Value(); }
        QtCassandra::QCassandraValue const& get_value() const { return f_value; }
        QDomElement         get_element() const { return f_element; }
        search_result_t *   get_result() const { return f_result; }

    private:
        safe_command_t                  f_cmd;
        QtCassandra::QCassandraValue    f_value;
        QDomElement                     f_element;
        controlled_vars::ptr_auto_init<search_result_t> f_result;
    };
    typedef QVector<cmd_info_t> cmd_info_vector_t;

                        field_search(char const *filename, char const *func, int line, snap_child *snap);
                        ~field_search();

    field_search&       operator () (command_t cmd);
    field_search&       operator () (command_t cmd, char const *str_value);
    field_search&       operator () (command_t cmd, QString const& str_value);
    field_search&       operator () (command_t cmd, int64_t int_value);
    field_search&       operator () (command_t cmd, QtCassandra::QCassandraValue value);
    field_search&       operator () (command_t cmd, QDomElement element);
    field_search&       operator () (command_t cmd, search_result_t& result);

private:
    void                run();

    char const *        f_filename;
    char const *        f_function;
    int                 f_line;
    zpsnap_child_t      f_snap;
    cmd_info_vector_t   f_program;
};

field_search create_field_search(char const *filename, char const *func, int line, snap_child *snap);

#define FIELD_SEARCH    snap::content::create_field_search(__FILE__, __func__, __LINE__, f_snap)


class attachment_file
{
public:
                                    attachment_file(snap_child *snap);
                                    attachment_file(snap_child *snap, snap_child::post_file_t const& file);

    void                            set_multiple(bool multiple);
    void                            set_cpath(QString const& cpath);
    void                            set_field_name(QString const& field_name);
    void                            set_attachment_owner(QString const& owner);
    void                            set_attachment_type(QString const& type);
    void                            set_creation_time(int64_t time);
    void                            set_update_time(int64_t time);

    void                            set_file_name(QString const& name);
    void                            set_file_filename(QString const& filename);
    void                            set_file_mime_type(QString const& mime_type);
    void                            set_file_original_mime_type(QString const& mime_type);
    void                            set_file_creation_time(time_t ctime);
    void                            set_file_modification_time(time_t mtime);
    void                            set_file_data(QByteArray const& data);
    void                            set_file_size(int size);
    void                            set_file_index(int index);
    void                            set_file_image_width(int width);
    void                            set_file_image_height(int height);

    bool                            get_multiple() const;
    snap_child::post_file_t const&  get_file() const;
    QString const&                  get_cpath() const;
    QString const&                  get_field_name() const;
    QString const&                  get_attachment_owner() const;
    QString const&                  get_attachment_type() const;
    int64_t                         get_creation_time() const;
    int64_t                         get_update_time() const;
    QString const&                  get_name() const;

private:
    zpsnap_child_t                  f_snap;
    snap_child::post_file_t         f_file;
    controlled_vars::fbool_t        f_multiple;
    controlled_vars::fbool_t        f_has_cpath;
    QString                         f_cpath;
    QString                         f_field_name;
    QString                         f_attachment_owner;
    QString                         f_attachment_type;
    mutable QString                 f_name;
    int64_t                         f_creation_time;
    int64_t                         f_update_time;
};



class content : public plugins::plugin, public path::path_execute, public layout::layout_content, public javascript::javascript_dynamic_plugin
{
public:
    enum param_type_t
    {
        PARAM_TYPE_STRING,
        PARAM_TYPE_FLOAT,
        PARAM_TYPE_INT8,
        PARAM_TYPE_INT64
    };

    class secure_flag
    {
    public:
                        secure_flag() {}
        bool            secure() const { return f_secure; }
        QString const&  reason() const { return f_reason; }
        void            not_secure(QString const& new_reason);

    private:
        // prevent copies or a user could reset the flag!
        secure_flag(secure_flag const& rhs);
        secure_flag& operator = (secure_flag const& rhs);

        controlled_vars::tbool_t    f_secure;
        QString                     f_reason;
    };

    enum : signed char
    {
        CONTENT_SECURE_UNDEFINED = -1,  // not checked yet
        CONTENT_SECURE_UNSECURE = 0,    // a plugin said it was not safe to use
        CONTENT_SECURE_SECURE = 1       // all plugins are go!
    };

    typedef controlled_vars::limited_auto_init<param_type_t, PARAM_TYPE_STRING, PARAM_TYPE_INT64, PARAM_TYPE_STRING> safe_param_type_t;

                        content();
                        ~content();

    static content *    instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QSharedPointer<QtCassandra::QCassandraTable> get_content_table();
    QSharedPointer<QtCassandra::QCassandraTable> get_files_table();
    QtCassandra::QCassandraValue get_content_parameter(QString path, QString const& name);

    void                on_bootstrap(snap_child *snap);
    virtual bool        on_path_execute(QString const& url);
    void                on_save_content();
    void                on_create_content(QString const& path);
    virtual void        on_generate_main_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                on_generate_page_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                on_backend_process();

    SNAP_SIGNAL(new_content, (QString const& path), (path));
    SNAP_SIGNAL(create_content, (QString const& path, QString const& owner, QString const& type), (path, owner, type));
    SNAP_SIGNAL(create_attachment, (attachment_file const& file), (file));
    SNAP_SIGNAL(modified_content, (QString const& path, bool updated), (path, updated));
    SNAP_SIGNAL(check_attachment_security, (attachment_file const& file, secure_flag& secure), (file, secure));
    SNAP_SIGNAL(process_attachment, (QByteArray const& key, attachment_file const& file), (key, file));

    void                output() const;

    // add content for addition to the database
    void                add_xml(QString const& plugin_name);
    void                add_content(QString const& path, QString const& plugin_owner);
    void                add_param(QString const& path, QString const& name, QString const& data);
    void                set_param_overwrite(QString const& path, const QString& name, bool overwrite);
    void                set_param_type(QString const& path, const QString& name, param_type_t param_type);
    void                add_link(QString const& path, links::link_info const& source, links::link_info const& destination);
    static void         insert_html_string_to_xml_doc(QDomElement child, QString const& xml);
    bool                load_attachment(QString const& key, attachment_file& file, bool load_data = true);

    virtual int         js_property_count() const;
    virtual QVariant    js_property_get(QString const& name) const;
    virtual QString     js_property_name(int index) const;
    virtual QVariant    js_property_get(int index) const;

private:
    // from the <param> tags
    struct content_param
    {
        QString                     f_name;
        QString                     f_data;
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
    QSharedPointer<QtCassandra::QCassandraTable>    f_files_table;
    content_block_map_t                             f_blocks;
    controlled_vars::fbool_t                        f_updating;
};

class content_box_execute
{
public:
    virtual             ~content_box_execute() {} // ensure proper virtual tables
    virtual bool        on_content_box_execute(content *c, QString const& path, QDomElement& box) = 0;
};


} // namespace content
} // namespace snap
#endif
// SNAP_CONTENT_H
// vim: ts=4 sw=4 et
