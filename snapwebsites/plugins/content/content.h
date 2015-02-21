// Snap Websites Server -- content management (pages, tags, everything!)
// Copyright (C) 2011-2015  Made to Order Software Corp.
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

#include "../links/links.h"

#include <controlled_vars/controlled_vars_ptr_no_init.h>


namespace snap
{
namespace content
{


enum name_t
{
    SNAP_NAME_CONTENT_ACCEPTED,
    SNAP_NAME_CONTENT_ATTACHMENT,
    SNAP_NAME_CONTENT_ATTACHMENT_FILENAME,
    SNAP_NAME_CONTENT_ATTACHMENT_JAVASCRIPTS,
    SNAP_NAME_CONTENT_ATTACHMENT_MIME_TYPE,
    SNAP_NAME_CONTENT_ATTACHMENT_PATH_END,
    SNAP_NAME_CONTENT_ATTACHMENT_PLUGIN,
    SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE,
    SNAP_NAME_CONTENT_BODY,
    SNAP_NAME_CONTENT_BRANCH,
    SNAP_NAME_CONTENT_BRANCH_TABLE,         // Name of Cassandra table used for branch information
    SNAP_NAME_CONTENT_CHILDREN,
    SNAP_NAME_CONTENT_CLONE,
    SNAP_NAME_CONTENT_CLONED,
    SNAP_NAME_CONTENT_COMPRESSOR_UNCOMPRESSED,
    SNAP_NAME_CONTENT_CONTENT_TYPES,
    SNAP_NAME_CONTENT_CONTENT_TYPES_NAME,
    SNAP_NAME_CONTENT_COPYRIGHTED,
    SNAP_NAME_CONTENT_CREATED,
    SNAP_NAME_CONTENT_CURRENT_VERSION,
    SNAP_NAME_CONTENT_DESCRIPTION,
    SNAP_NAME_CONTENT_ERROR_FILES,
    SNAP_NAME_CONTENT_FILES_COMPRESSOR,
    SNAP_NAME_CONTENT_FILES_CREATED,
    SNAP_NAME_CONTENT_FILES_CREATION_TIME,
    SNAP_NAME_CONTENT_FILES_DATA,
    SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED,
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
    SNAP_NAME_CONTENT_FILES_SIZE_GZIP_COMPRESSED,
    SNAP_NAME_CONTENT_FILES_TABLE,
    SNAP_NAME_CONTENT_FILES_UPDATED,
    SNAP_NAME_CONTENT_FINAL,
    SNAP_NAME_CONTENT_FORCERESETSTATUS,
    SNAP_NAME_CONTENT_ISSUED,
    SNAP_NAME_CONTENT_LONG_TITLE,
    SNAP_NAME_CONTENT_MINIMAL_LAYOUT_NAME,
    SNAP_NAME_CONTENT_MODIFIED,
    SNAP_NAME_CONTENT_ORIGINAL_PAGE,
    SNAP_NAME_CONTENT_OUTPUT_PLUGIN,
    SNAP_NAME_CONTENT_PAGE,
    SNAP_NAME_CONTENT_PAGE_TYPE,
    SNAP_NAME_CONTENT_PARENT,
    SNAP_NAME_CONTENT_PREVENT_DELETE,
    SNAP_NAME_CONTENT_PRIMARY_OWNER,
    SNAP_NAME_CONTENT_PROCESSING_TABLE,
    SNAP_NAME_CONTENT_RESETSTATUS,
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
    SNAP_NAME_CONTENT_REVISION_TABLE,       // Name of Cassandra table used for revision information
    SNAP_NAME_CONTENT_SECRET_TABLE,         // Name of Cassandra table used for secret content
    SNAP_NAME_CONTENT_SHORT_TITLE,
    SNAP_NAME_CONTENT_SINCE,
    SNAP_NAME_CONTENT_STATUS,
    SNAP_NAME_CONTENT_STATUS_CHANGED,
    SNAP_NAME_CONTENT_SUBMITTED,
    SNAP_NAME_CONTENT_TABLE,         // Name of Cassandra table used for content tree main info
    SNAP_NAME_CONTENT_TAG,
    SNAP_NAME_CONTENT_TITLE,
    SNAP_NAME_CONTENT_TRASHCAN,
    SNAP_NAME_CONTENT_UNTIL,
    SNAP_NAME_CONTENT_UPDATED,
    SNAP_NAME_CONTENT_VARIABLE_REVISION
};
char const *get_name(name_t name) __attribute__ ((const));


class content_exception : public snap_exception
{
public:
    content_exception(char const *       what_msg) : snap_exception("content", what_msg) {}
    content_exception(std::string const& what_msg) : snap_exception("content", what_msg) {}
    content_exception(QString const&     what_msg) : snap_exception("content", what_msg) {}
};

class content_exception_content_invalid_state : public content_exception
{
public:
    content_exception_content_invalid_state(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_content_invalid_state(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_content_invalid_state(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_content_not_initialized : public content_exception
{
public:
    content_exception_content_not_initialized(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_content_not_initialized(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_content_not_initialized(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_invalid_content_xml : public content_exception
{
public:
    content_exception_invalid_content_xml(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_invalid_content_xml(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_invalid_content_xml(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_parameter_not_defined : public content_exception
{
public:
    content_exception_parameter_not_defined(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_parameter_not_defined(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_parameter_not_defined(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_content_already_defined : public content_exception
{
public:
    content_exception_content_already_defined(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_content_already_defined(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_content_already_defined(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_circular_dependencies : public content_exception
{
public:
    content_exception_circular_dependencies(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_circular_dependencies(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_circular_dependencies(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_type_mismatch : public content_exception
{
public:
    content_exception_type_mismatch(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_type_mismatch(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_type_mismatch(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_invalid_sequence : public content_exception
{
public:
    content_exception_invalid_sequence(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_invalid_sequence(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_invalid_sequence(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_invalid_name : public content_exception
{
public:
    content_exception_invalid_name(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_invalid_name(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_invalid_name(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_unexpected_revision_type : public content_exception
{
public:
    content_exception_unexpected_revision_type(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_unexpected_revision_type(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_unexpected_revision_type(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_data_missing : public content_exception
{
public:
    content_exception_data_missing(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_data_missing(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_data_missing(QString const&     what_msg) : content_exception(what_msg) {}
};

class content_exception_io_error : public content_exception
{
public:
    content_exception_io_error(char const *       what_msg) : content_exception(what_msg) {}
    content_exception_io_error(std::string const& what_msg) : content_exception(what_msg) {}
    content_exception_io_error(QString const&     what_msg) : content_exception(what_msg) {}
};






class content;

class path_info_t
{
public:
    class status_t
    {
    public:
        // hermetic type to save the status in the database
        typedef uint32_t            status_type;

        // error state, if not NO then it has priority over the general state and working state
        enum class error_t
        {
            NO_ERROR,
            UNDEFINED,
            UNSUPPORTED
        };
        typedef controlled_vars::limited_auto_enum_init<error_t, error_t::NO_ERROR, error_t::UNSUPPORTED, error_t::NO_ERROR> safe_error_t;

        // general state
        enum class state_t
        {
            UNKNOWN_STATE,
            CREATE,
            NORMAL,
            HIDDEN,
            MOVED,
            DELETED
        };
        typedef controlled_vars::limited_auto_enum_init<state_t, state_t::UNKNOWN_STATE, state_t::DELETED, state_t::UNKNOWN_STATE> safe_state_t;

        // working state
        enum class working_t
        {
            UNKNOWN_WORKING,
            NOT_WORKING,
            CREATING,
            CLONING,
            REMOVING,
            UPDATING
        };
        typedef controlled_vars::limited_auto_enum_init<working_t, working_t::UNKNOWN_WORKING, working_t::UPDATING, working_t::NOT_WORKING> safe_working_t;

                                    status_t();
                                    status_t(status_type current_status);
        void                        set_status(status_type current_status);
        status_type                 get_status() const;
        bool                        valid_transition(status_t destination) const;

        // error status
        void                        set_error(error_t error);
        error_t                     get_error() const;
        bool                        is_error() const;

        // reset the error, set the state and working values
        void                        reset_state(state_t state, working_t working);

        // general status, may include a working status (see below)
        void                        set_state(state_t state);
        state_t                     get_state() const;
        bool                        is_unknown() const;

        // working status used along the general status
        void                        set_working(working_t working);
        working_t                   get_working() const;
        bool                        is_working() const;

    private:
        safe_error_t                f_error;
        safe_state_t                f_state;
        safe_working_t              f_working;
    };

    class raii_status_t
    {
    public:
                                    raii_status_t(path_info_t& ipath, status_t now, status_t end);
                                    ~raii_status_t();

    private:
        path_info_t&                f_ipath;
        status_t const              f_end;
    };

    typedef std::vector<path_info_t *>              vector_path_info_t;
    typedef std::map<std::string, path_info_t *>    map_path_info_t;

                                    path_info_t();

    void                            set_path(QString const& path);
    void                            set_real_path(QString const& path);
    void                            set_main_page(bool main_page);
    void                            set_parameter(QString const& name, QString const& value);

    void                            force_branch(snap_version::version_number_t branch);
    void                            force_revision(snap_version::version_number_t revision);
    void                            force_extended_revision(QString const& revision, QString const& filename);
    void                            force_locale(QString const& locale);

    void                            get_parent(path_info_t& parent_ipath) const;
    void                            get_child(path_info_t& parent_ipath, QString const& child) const;

    snap_child *                    get_snap() const;
    QString                         get_key() const;
    QString                         get_real_key() const;
    QString                         get_cpath() const;
    QString                         get_real_cpath() const;
    bool                            is_main_page() const;
    QString                         get_parameter(QString const& name) const;
    status_t                        get_status() const;
    void                            set_status(status_t const& status);

    bool                            get_working_branch() const;
    snap_version::version_number_t  get_branch(bool create_new_if_required = false, QString const& locale = "") const;
    bool                            has_branch() const;
    snap_version::version_number_t  get_revision() const;
    bool                            has_revision() const;
    QString                         get_locale() const;
    QString                         get_branch_key() const;
    QString                         get_revision_key() const;
    QString                         get_extended_revision() const;

private:
    typedef QMap<QString, QString>  parameters_t;

    void                            clear(bool keep_parameters = false);

    // auto-initialized
    content *                       f_content_plugin;
    zpsnap_child_t                  f_snap;
    controlled_vars::fbool_t        f_initialized;

    // user specified
    QString                         f_key;
    QString                         f_real_key;
    QString                         f_cpath;
    QString                         f_real_cpath;
    controlled_vars::fbool_t        f_main_page;
    parameters_t                    f_parameters;

    // generated internally
    mutable snap_version::version_number_t  f_branch;
    mutable snap_version::version_number_t  f_revision;
    QString                                 f_revision_string;
    mutable QString                         f_locale;
    mutable QString                         f_branch_key;
    mutable QString                         f_revision_key;
};


class field_search
{
public:
    enum command_t
    {
        COMMAND_UNKNOWN,                // (cannot be used)

        // retrieve from Cassandra
        COMMAND_FIELD_NAME,             // + field name
        COMMAND_FIELD_NAME_WITH_VARS,   // + field name
        COMMAND_MODE,                   // + mode (int)
        COMMAND_BRANCH_PATH,            // + true if main page, false otherwise
        COMMAND_REVISION_PATH,          // + true if main page, false otherwise

        COMMAND_TABLE,                  // + table name
        COMMAND_SELF,                   // no parameters
        COMMAND_PATH,                   // + path
        COMMAND_PATH_INFO_GLOBAL,       // + path_info_t
        COMMAND_PATH_INFO_BRANCH,       // + path_info_t
        COMMAND_PATH_INFO_REVISION,     // + path_info_t
        COMMAND_CHILDREN,               // + depth (int)
        COMMAND_PARENTS,                // + path (limit)
        COMMAND_LINK,                   // + link name

        COMMAND_DEFAULT_VALUE,          // + data
        COMMAND_DEFAULT_VALUE_OR_NULL,  // + data (ignore if data is null)

        // save in temporary XML for XSLT
        COMMAND_ELEMENT,                // + QDomElement
        COMMAND_PATH_ELEMENT,           // + path to child element
        COMMAND_CHILD_ELEMENT,          // + child name
        COMMAND_NEW_CHILD_ELEMENT,      // + child name
        COMMAND_PARENT_ELEMENT,         // no parameters
        COMMAND_ELEMENT_TEXT,           // no parameters
        COMMAND_ELEMENT_ATTR,           // + QDomElement
        COMMAND_RESULT,                 // + search_result_t
        COMMAND_LAST_RESULT_TO_VAR,     // + var name
        COMMAND_SAVE,                   // + child name
        COMMAND_SAVE_FLOAT64,           // + child name
        COMMAND_SAVE_INT64,             // + child name
        COMMAND_SAVE_INT64_DATE,        // + child name
        COMMAND_SAVE_XML,               // + child name

        // other types of commands
        COMMAND_LABEL,                  // + label number
        COMMAND_GOTO,                   // + label number
        COMMAND_IF_FOUND,               // + label number
        COMMAND_IF_NOT_FOUND,           // + label number
        COMMAND_IF_ELEMENT_NULL,        // + label number
        COMMAND_IF_NOT_ELEMENT_NULL,    // + label number
        COMMAND_RESET,                  // no parameters
        COMMAND_WARNING                 // + warning message
    };
    typedef controlled_vars::limited_auto_enum_init<command_t, COMMAND_UNKNOWN, COMMAND_DEFAULT_VALUE, COMMAND_UNKNOWN> safe_command_t;

    enum mode_t
    {
        SEARCH_MODE_FIRST,            // default mode: only return first parameter found
        SEARCH_MODE_EACH,             // return a list of QCassandraValue's of all the entire tree
        SEARCH_MODE_PATHS             // return a list of paths (for debug purposes usually)
    };
    typedef controlled_vars::limited_auto_enum_init<mode_t, SEARCH_MODE_FIRST, SEARCH_MODE_PATHS, SEARCH_MODE_FIRST> safe_mode_t;

    typedef QVector<QtCassandra::QCassandraValue> search_result_t;
    typedef QMap<QString, QString> variables_t;

    class cmd_info_t
    {
    public:
                            cmd_info_t();
                            cmd_info_t(command_t cmd);
                            cmd_info_t(command_t cmd, QString const& str_value);
                            cmd_info_t(command_t cmd, int64_t int_value);
                            cmd_info_t(command_t cmd, QtCassandra::QCassandraValue& value);
                            cmd_info_t(command_t cmd, QDomElement element);
                            cmd_info_t(command_t cmd, QDomDocument doc);
                            cmd_info_t(command_t cmd, search_result_t& result);
                            cmd_info_t(command_t cmd, path_info_t const& ipath);

        command_t           get_command() const { return f_cmd; }
        QString const       get_string()  const { return f_value.stringValue(); }
        //int32_t             get_int32() const { return f_value.int32Value(); }
        int64_t             get_int64() const { return f_value.int64Value(); }
        QtCassandra::QCassandraValue const& get_value() const { return f_value; }
        QDomElement         get_element() const { return f_element; }
        search_result_t *   get_result() const { return f_result; }
        path_info_t const & get_ipath() const { return f_path_info; }

    private:
        safe_command_t                  f_cmd;
        QtCassandra::QCassandraValue    f_value;
        QDomElement                     f_element;
        controlled_vars::ptr_auto_init<search_result_t> f_result;
        path_info_t                     f_path_info;
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
    field_search&       operator () (command_t cmd, QDomDocument doc);
    field_search&       operator () (command_t cmd, search_result_t& result);
    field_search&       operator () (command_t cmd, path_info_t& ipath);

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



typedef QVector<QString>            dependency_list_t;

class attachment_file
{
public:
                                    attachment_file(snap_child *snap);
                                    attachment_file(snap_child *snap, snap_child::post_file_t const& file);

    void                            set_multiple(bool multiple);
    void                            set_parent_cpath(QString const& cpath);
    void                            set_field_name(QString const& field_name);
    void                            set_attachment_cpath(QString const& cpath);
    void                            set_attachment_owner(QString const& owner);
    void                            set_attachment_type(QString const& type);
    void                            set_creation_time(int64_t time);
    void                            set_update_time(int64_t time);
    void                            set_dependencies(dependency_list_t& dependencies);

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
    QString const&                  get_parent_cpath() const;
    QString const&                  get_field_name() const;
    QString const&                  get_attachment_cpath() const;
    QString const&                  get_attachment_owner() const;
    QString const&                  get_attachment_type() const;
    int64_t                         get_creation_time() const;
    int64_t                         get_update_time() const;
    dependency_list_t const&        get_dependencies() const;
    QString const&                  get_name() const;

private:
    zpsnap_child_t                  f_snap;
    snap_child::post_file_t         f_file;
    controlled_vars::fbool_t        f_multiple;
    controlled_vars::fbool_t        f_has_cpath;
    QString                         f_parent_cpath;
    QString                         f_field_name;
    QString                         f_attachment_cpath;
    QString                         f_attachment_owner;
    QString                         f_attachment_type;
    mutable QString                 f_name;
    controlled_vars::zint64_t       f_creation_time;
    controlled_vars::zint64_t       f_update_time;
    dependency_list_t               f_dependencies;
};


class permission_flag
{
public:
                    permission_flag() {}
    bool            allowed() const { return f_allowed; }
    QString const&  reason() const { return f_reason; }
    void            not_permitted(QString const& reason = "");

private:
    // prevent copies or a user could reset the flag!
                    permission_flag(permission_flag const& rhs) = delete;
                    permission_flag& operator = (permission_flag const& rhs) = delete;

    controlled_vars::tbool_t    f_allowed;
    QString                     f_reason;
};






class content : public plugins::plugin, public server::backend_action, public links::links_cloned
{
public:
    enum param_type_t
    {
        PARAM_TYPE_STRING,
        PARAM_TYPE_FLOAT32,
        PARAM_TYPE_FLOAT64,
        PARAM_TYPE_INT8,
        PARAM_TYPE_INT64
    };

    enum : signed char
    {
        // WARNING: these are saved in the database which is why we directly
        //          assign values DO NOT CHANGE THE VALUES
        CONTENT_SECURE_UNDEFINED = -1,  // not checked yet
        CONTENT_SECURE_INSECURE = 0,    // a plugin said it was not safe to use
        CONTENT_SECURE_SECURE = 1       // all plugins are go!
    };

    struct content_attachment
    {
        QString             f_owner;
        QString             f_field_name;
        QString             f_type;
        QString             f_path;
        QString             f_mime_type;
        QString             f_filename;
        dependency_list_t   f_dependencies;
    };

    typedef controlled_vars::limited_auto_enum_init<param_type_t, PARAM_TYPE_STRING, PARAM_TYPE_INT64, PARAM_TYPE_STRING> safe_param_type_t;

    enum param_revision_t
    {
        PARAM_REVISION_GLOBAL,
        PARAM_REVISION_BRANCH,
        PARAM_REVISION_REVISION
    };

    struct clone_info_t
    {
        path_info_t             f_ipath;
        path_info_t::status_t   f_processing_state;
        path_info_t::status_t   f_done_state;
    };

    struct cloned_branch_t
    {
        snap_version::version_number_t          f_branch;
        snap_version::version_numbers_vector_t  f_revisions;
    };
    typedef std::vector<cloned_branch_t>    cloned_branches_t;

    struct cloned_page_t
    {
        path_info_t                             f_source;
        path_info_t                             f_destination;
        cloned_branches_t                       f_branches;
    };
    typedef std::vector<cloned_page_t>      cloned_pages_t;

    struct cloned_tree_t
    {
        cloned_tree_t(clone_info_t& source, clone_info_t& destination)
            : f_source(source)
            , f_destination(destination)
            //, f_pages() -- auto-init
        {
        }

        clone_info_t&                           f_source;
        clone_info_t&                           f_destination;
        cloned_pages_t                          f_pages;
    };

                        content();
    virtual             ~content();

    static content *    instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);
    QtCassandra::QCassandraTable::pointer_t get_content_table();
    QtCassandra::QCassandraTable::pointer_t get_secret_table();
    QtCassandra::QCassandraTable::pointer_t get_files_table();
    QtCassandra::QCassandraTable::pointer_t get_branch_table();
    QtCassandra::QCassandraTable::pointer_t get_revision_table();
    QtCassandra::QCassandraTable::pointer_t get_processing_table();
    QtCassandra::QCassandraValue get_content_parameter(path_info_t& path, QString const& param_name, param_revision_t revision_type);

    // revision control
    snap_child *        get_snap();
    void                invalid_revision_control(QString const& version);
    snap_version::version_number_t get_current_branch(QString const& key, bool working_branch);
    snap_version::version_number_t get_current_user_branch(QString const& key, QString const& locale, bool working_branch);
    snap_version::version_number_t get_current_revision(QString const& key, snap_version::version_number_t const branch, QString const& locale, bool working_branch);
    snap_version::version_number_t get_current_revision(QString const& key, QString const& locale, bool working_branch);
    snap_version::version_number_t get_new_branch(QString const& key, QString const& locale);
    snap_version::version_number_t get_new_revision(QString const& key, snap_version::version_number_t const branch, QString const& locale, bool repeat, snap_version::version_number_t const old_branch = static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED));
    void                copy_branch(QString const& key, snap_version::version_number_t const source_branch, snap_version::version_number_t const destination_branch);
    static void         copy_branch_cells_as_is(QtCassandra::QCassandraCells& source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, QString const& plugin_namespace);
    QString             get_branch_key(QString const& key, bool working_branch);
    void                initialize_branch(QString const& key);
    QString             generate_branch_key(QString const& key, snap_version::version_number_t branch);
    void                set_branch(QString const& key, snap_version::version_number_t branch, bool working_branch);
    QString             set_branch_key(QString const& key, snap_version::version_number_t branch, bool working_branch);
    QString             get_revision_key(QString const& key, snap_version::version_number_t branch, QString const& locale, bool working_branch);
    QString             generate_revision_key(QString const& key, snap_version::version_number_t branch, snap_version::version_number_t revision, QString const& locale);
    QString             generate_revision_key(QString const& key, QString const& revision, QString const& locale);
    void                set_current_revision(QString const& key, snap_version::version_number_t branch, snap_version::version_number_t revision, QString const& locale, bool working_branch);
    QString             set_revision_key(QString const& key, snap_version::version_number_t branch, snap_version::version_number_t revision, QString const& locale, bool working_branch);
    QString             set_revision_key(QString const& key, snap_version::version_number_t branch, QString const& revision, QString const& locale, bool working_branch);
    path_info_t         get_path_info(QString const& cpath, bool main_page);
    QString             get_user_key(QString const& key, snap_version::version_number_t branch, int64_t identifier);
    virtual void        repair_link_of_cloned_page(QString const& clone, snap_version::version_number_t branch_number, links::link_info const& source, links::link_info const& destination, bool const cloning);
    bool                clone_page(clone_info_t& source, clone_info_t& destination);
    bool                move_page(path_info_t& ipath_source, path_info_t& ipath_destination);
    bool                trash_page(path_info_t& ipath);

    void                on_bootstrap(snap_child *snap);
    void                on_execute(QString const& uri_path);
    //virtual bool        on_path_execute(path_info_t& ipath);
    void                on_save_content();
    //virtual void        on_generate_main_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    //void                on_generate_page_content(layout::layout *l, QString const& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                on_register_backend_action(server::backend_action_map_t& actions);
    virtual void        on_backend_action(QString const& action);
    void                on_backend_process();
    void                on_load_file(snap_child::post_file_t& file, bool& found);
    void                on_cell_is_secure(QString const& table, QString const& row, QString const& cell, server::secure_field_flag_t& secure);

    SNAP_SIGNAL(new_content, (path_info_t& path), (path));
    SNAP_SIGNAL_WITH_MODE(create_content, (path_info_t& path, QString const& owner, QString const& type), (path, owner, type), START_AND_DONE);
    SNAP_SIGNAL(create_attachment, (attachment_file& file, snap_version::version_number_t branch_number, QString const& locale), (file, branch_number, locale));
    SNAP_SIGNAL(modified_content, (path_info_t& ipath), (ipath));
    SNAP_SIGNAL_WITH_MODE(check_attachment_security, (attachment_file const& file, permission_flag& secure, bool const fast), (file, secure, fast), NEITHER);
    SNAP_SIGNAL(process_attachment, (QByteArray const& key, attachment_file const& file), (key, file));
    SNAP_SIGNAL(page_cloned, (cloned_tree_t const& tree), (tree));
    SNAP_SIGNAL(copy_branch_cells, (QtCassandra::QCassandraCells& source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, snap_version::version_number_t const destination_branch), (source_cells, destination_row, destination_branch));

    //void                output() const;

    // add content for addition to the database
    void                add_xml(QString const& plugin_name);
    void                add_xml_document(QDomDocument& dom, QString const& plugin_name);
    void                add_content(QString const& path, QString const& plugin_owner);
    void                add_param(QString const& path, QString const& name, param_revision_t revision_type, QString const& locale, QString const& data);
    void                set_param_overwrite(QString const& path, const QString& name, bool overwrite);
    void                set_param_type(QString const& path, const QString& name, param_type_t param_type);
    void                add_link(QString const& path, links::link_info const& source, links::link_info const& destination);
    void                add_attachment(QString const& path, content_attachment const& attachment);
    bool                load_attachment(QString const& key, attachment_file& file, bool load_data = true);
    void                add_javascript(QDomDocument doc, QString const& name);
    void                add_css(QDomDocument doc, QString const& name);

    bool                is_final(QString const& key);

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

    void        initial_update(int64_t variables_timestamp);
    void        content_update(int64_t variables_timestamp);
    void        backend_action_reset_status(bool const force);
    void        backend_process_status();
    void        backend_process_files();

    zpsnap_child_t                                  f_snap;
    QtCassandra::QCassandraTable::pointer_t         f_content_table;
    QtCassandra::QCassandraTable::pointer_t         f_secret_table;
    QtCassandra::QCassandraTable::pointer_t         f_processing_table;
    QtCassandra::QCassandraTable::pointer_t         f_branch_table;
    QtCassandra::QCassandraTable::pointer_t         f_revision_table;
    QtCassandra::QCassandraTable::pointer_t         f_files_table;
    content_block_map_t                             f_blocks;
    controlled_vars::zint32_t                       f_file_index;
    controlled_vars::fbool_t                        f_updating;
    QMap<QString, bool>                             f_added_javascripts;
    javascript_ref_map_t                            f_javascripts;
    QMap<QString, bool>                             f_added_css;
};

//class content_box_execute
//{
//public:
//    virtual             ~content_box_execute() {} // ensure proper virtual tables
//    virtual bool        on_content_box_execute(content *c, QString const& path, QDomElement& box) = 0;
//};


} // namespace content
} // namespace snap
// vim: ts=4 sw=4 et
