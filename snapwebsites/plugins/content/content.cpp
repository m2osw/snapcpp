// Snap Websites Server -- all the user content and much of the system content
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

#include "content.h"
#include "plugins.h"
#include "log.h"
#include "compression.h"
#include "not_reached.h"
#include "dom_util.h"
#include "snap_magic.h"
#include <iostream>
#include <openssl/md5.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QFile>
#pragma GCC diagnostic pop
#include "poison.h"


SNAP_PLUGIN_START(content, 1, 0)

/** \brief Get a fixed content name.
 *
 * The content plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name) {
    case SNAP_NAME_CONTENT_ACCEPTED:
        return "content::accepted";

    case SNAP_NAME_CONTENT_ATTACHMENT:
        return "content::attachment";

    case SNAP_NAME_CONTENT_ATTACHMENT_PATH_END:
        return "path";

    case SNAP_NAME_CONTENT_ATTACHMENT_FILENAME:
        return "content::attachment::filename";

    case SNAP_NAME_CONTENT_ATTACHMENT_MIME_TYPE:
        return "content::attachment::mime_type";

    case SNAP_NAME_CONTENT_BODY:
        return "content::body";

    case SNAP_NAME_CONTENT_CHILDREN:
        return "content::children";

    case SNAP_NAME_CONTENT_COMPRESSOR_UNCOMPRESSED:
        return "uncompressed";

    case SNAP_NAME_CONTENT_CONTENT_TYPES:
        return "Content Types";

    case SNAP_NAME_CONTENT_CONTENT_TYPES_NAME:
        return "content_types";

    case SNAP_NAME_CONTENT_COPYRIGHTED:
        return "content::copyrighted";

    case SNAP_NAME_CONTENT_CREATED:
        return "content::created";

    case SNAP_NAME_CONTENT_FILES_COMPRESSOR:
        return "content::files::compressor";

    case SNAP_NAME_CONTENT_FILES_CREATED:
        return "content::files::created";

    case SNAP_NAME_CONTENT_FILES_CREATION_TIME:
        return "content::files::creation_time";

    case SNAP_NAME_CONTENT_FILES_DATA:
        return "content::files::data";

    case SNAP_NAME_CONTENT_FILES_DATA_COMPRESSED:
        return "content::files::data::compressed";

    case SNAP_NAME_CONTENT_FILES_FILENAME:
        return "content::files::filename";

    case SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT:
        return "content::files::image_height";

    case SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH:
        return "content::files::image_width";

    case SNAP_NAME_CONTENT_FILES_MIME_TYPE:
        return "content::files::mime_type";

    case SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME:
        return "content::files::modification_time";

    case SNAP_NAME_CONTENT_FILES_NEW:
        return "new";

    case SNAP_NAME_CONTENT_FILES_REFERENCE:
        return "content::files::reference";

    case SNAP_NAME_CONTENT_FILES_SECURE: // -1 -- unknown, 0 -- unsecure, 1 -- secure
        return "content::files::secure";

    case SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK:
        return "content::files::secure::last_check";

    case SNAP_NAME_CONTENT_FILES_SECURITY_REASON:
        return "content::files::security_reason";

    case SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE:
        return "content::files::original_mime_type";

    case SNAP_NAME_CONTENT_FILES_SIZE:
        return "content::files::size";

    case SNAP_NAME_CONTENT_FILES_SIZE_COMPRESSED:
        return "content::files::size::compressed";

    case SNAP_NAME_CONTENT_FILES_TABLE:
        return "files";

    case SNAP_NAME_CONTENT_FILES_UPDATED:
        return "content::files::updated";

    case SNAP_NAME_CONTENT_FINAL:
        return "content::final";

    case SNAP_NAME_CONTENT_ISSUED:
        return "content::issued";

    case SNAP_NAME_CONTENT_LONG_TITLE:
        return "content::long_title";

    case SNAP_NAME_CONTENT_MODIFIED:
        return "content::modified";

    case SNAP_NAME_CONTENT_PAGE_TYPE:
        return "content::page_type";

    case SNAP_NAME_CONTENT_PARENT:
        return "content::parent";

    case SNAP_NAME_CONTENT_SHORT_TITLE:
        return "content::short_title";

    case SNAP_NAME_CONTENT_SINCE:
        return "content::since";

    case SNAP_NAME_CONTENT_SUBMITTED:
        return "content::submitted";

    case SNAP_NAME_CONTENT_TABLE: // pages, tags, comments, etc.
        return "content";

    case SNAP_NAME_CONTENT_TITLE:
        return "content::title";

    case SNAP_NAME_CONTENT_UNTIL:
        return "content::until";

    case SNAP_NAME_CONTENT_UPDATED:
        return "content::updated";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_CONTENT_...");

    }
    NOTREACHED();
}


/** \class field_search
 * \brief Retrieve one or more parameters from one or more path.
 *
 * This function is used to search for a parameter in one or more paths
 * in your existing database tree.
 *
 * In many cases, the parameter exists in the specified path (i.e. the
 * "modified" parameter). In some other cases, the parameter only
 * exists in a child, a parent, the template, or a settings page.
 * This function is very easy to use and it will return said parameter
 * from wherever it is first found.
 *
 * If you are creating an administrative screen (and in some other
 * circumstances) it may be useful to find all instances of the parameter.
 * In that case you can request all instances. Note that this case is
 * considered SLOW and it should not be used lightly while generating
 * a page!
 *
 * The following shows you an example of a tree that this function can
 * search. Say that the input path represents B. If your search setup
 * asks for SELF, its CHILDREN with a depth limit of 2, a template (assuming
 * its template is D,) its type found using LINK (and assuming its type is
 * F) and the PARENTS of that type with a limit on C then the search can
 * check the following nodes in that order:
 *
 * \li B
 * \li E (switched to children)
 * \li H (switched to children; last time because depth is limited to 2)
 * \li I
 * \li J
 * \li D (switched to template)
 * \li F (switched to page type)
 * \li C (switched to parent, stop on C)
 *
 * Pages A, K and G are therefore ignored.
 *
 *                +-------+       +------+       +-------+
 *          +---->| B     |+----->| E    |+-+--->| H     |
 *          |     +-------+       +------+  |    +-------+
 *          |                               |
 *          |                               |
 *          |                     +------+  |    +-------+     +------+
 *          |     +-------+  +--->| F    |  +--->| I     |+--->| K    |
 *          +---->| C     |+-+    +------+  |    +-------+     +------+
 *  +----+  |     +-------+  |              |
 *  | A  |+-+                |              |
 *  +----+  |                |    +------+  |
 *          |                +--->| G    |  |    +-------+
 *          |     +-------+       +------+  +--->| J     |
 *          +---->| D     |                      +-------+
 *                +-------+
 *
 * View: http://www.asciiflow.com/#1357940162213390220
 * Edit: http://www.asciiflow.com/#1357940162213390220/1819073096
 *
 * This type of search can be used to gather pretty much all the
 * necessary parameters used in a page to display that page.
 *
 * Note that this function is not used by the permissions because in
 * that case all permission links defined in a page are sought. Whereas
 * here we're interested in the content of a field in a page.
 *
 * Note that when searching children we first search all the children at
 * a given depth, then repeat the search at the next level. So in our
 * example, if we had a search depth of 3, we would end up searching
 * K after J, not between I and J.
 *
 * Since the opt_info_t object is like a mini program, it is possible
 * to do things such as change the name of the field being sought as
 * the different parts of the tree are searched. So a parameter named
 * "created" in SELF, could change to "modified" when searching the
 * PARENT, and "primary-date" when searching the TYPE. It may, however,
 * not be a good idea as in most situations you probably want to use
 * just and only "modified". This being said, when you try to determine
 * the modification date, you could try the "modified" date first, then
 * try the "updated" and finally "created" and since "created" is
 * mandatory you know you'll always find it (and it not, there is no
 * other valid default):
 *
 * \code
 * QStringList result(opt_info_t()
 *      (PARAMETER_OPTION_FIELD_NAME, "modified")
 *      (PARAMETER_OPTION_SELF, path)
 *      (PARAMETER_OPTION_FIELD_NAME, "updated")
 *      (PARAMETER_OPTION_SELF, path)
 *      (PARAMETER_OPTION_FIELD_NAME, "created")
 *      (PARAMETER_OPTION_SELF, path)
 *      .run(PARAMETER_OPTION_MODE_FIRST)); // run
 * \endcode
 *
 * In this example notice that we just lose the opt_info_t object. It is
 * temporarily created on the stack, initialized, used to gather the
 * first match, then return a list of strings our of which we expect
 * either nothing (empty list) or one entry (the first parameter found.)
 */


/** \class cmd_info_t
 * \brief Instructions about the search to perform.
 *
 * This sub-class is used by the parameters_t class as an instruction:
 * what to search next to find a given parameter.
 */


/** \brief Create an empty cmd_info_t object.
 *
 * To be able to create cmd_info_t objects in a vector we have to create
 * a constructor with no parameters. This creates an invalid command
 * object.
 */
field_search::cmd_info_t::cmd_info_t()
    //: f_cmd(COMMAND_UNKNOWN) -- auto-init
    //, f_value() -- auto-init
    //, f_element() -- auto-init
    //, f_result(NULL) -- auto-init
{
}


/** \brief Initialize an opt_info_t object.
 *
 * This function initializes the opt_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    //, f_value(str_value)
    //, f_element() -- auto-init
    //, f_result(NULL) -- auto-init
{
    switch(cmd)
    {
    case COMMAND_PARENT_ELEMENT:
    case COMMAND_RESET:
    case COMMAND_SELF:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for an instruction without parameters").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an opt_info_t object.
 *
 * This function initializes the opt_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] opt  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] str_value  The string value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QString const& str_value)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    , f_value(str_value)
    //, f_element() -- auto-init
    //, f_result(NULL) -- auto-init
{
    switch(cmd)
    {
    case COMMAND_FIELD_NAME:
    case COMMAND_PATH:
    case COMMAND_PARENTS:
    case COMMAND_LINK:
    case COMMAND_DEFAULT_VALUE:
    case COMMAND_DEFAULT_VALUE_OR_NULL:
    case COMMAND_CHILD_ELEMENT:
    case COMMAND_ELEMENT_ATTR:
    case COMMAND_SAVE:
    case COMMAND_SAVE_INT64:
    case COMMAND_SAVE_INT64_DATE:
    case COMMAND_SAVE_XML:
    case COMMAND_WARNING:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a string (%2)").arg(static_cast<int>(cmd)).arg(str_value));

    }
}


/** \brief Initialize an opt_info_t object.
 *
 * This function initializes the opt_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] opt  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] int_value  The integer value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, int64_t int_value)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    , f_value(int_value)
    //, f_element() -- auto-init
    //, f_result(NULL) -- auto-init
{
    switch(cmd)
    {
    case COMMAND_MODE:
    case COMMAND_CHILDREN:
    case COMMAND_DEFAULT_VALUE:
    case COMMAND_DEFAULT_VALUE_OR_NULL:
    case COMMAND_LABEL:
    case COMMAND_GOTO:
    case COMMAND_IF_FOUND:
    case COMMAND_IF_NOT_FOUND:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a string (%2)").arg(static_cast<int>(cmd)).arg(int_value));

    }
}


/** \brief Initialize an opt_info_t object.
 *
 * This function initializes the opt_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] opt  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] value  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QtCassandra::QCassandraValue& value)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    , f_value(value)
    //, f_element() -- auto-init
    //, f_result(NULL) -- auto-init
{
    switch(cmd)
    {
    case COMMAND_DEFAULT_VALUE:
    case COMMAND_DEFAULT_VALUE_OR_NULL:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a QCassandraValue").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an opt_info_t object.
 *
 * This function initializes the opt_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] opt  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] element  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QDomElement element)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    //, f_value() -- auto-init
    , f_element(element)
    //, f_result(NULL) -- auto-init
{
    switch(cmd)
    {
    case COMMAND_ELEMENT:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a QCassandraValue").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an opt_info_t object.
 *
 * This function initializes the opt_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] opt  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in,out] result  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, search_result_t& result)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    //, f_value() -- auto-init
    //, f_element(element)
    , f_result(&result)
{
    switch(cmd)
    {
    case COMMAND_RESULT:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a search_result_t").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize a field search object.
 *
 * This constructor saves the snap child pointer in the field_search so
 * it can be referenced later to access pages.
 */
field_search::field_search(char const *filename, char const *func, int line, snap_child *snap)
    : f_filename(filename)
    , f_function(func)
    , f_line(line)
    , f_snap(snap)
    //, f_program() -- auto-init
{
}


/** \brief Generate the data and then destroy the field_search object.
 *
 * The destructor makes sure that the program runs once, then it cleans
 * up the object. This allows you to create a tempoary field_search object
 * on the stack and at the time it gets deleted, it runs the program.
 */
field_search::~field_search()
{
    run();
}


/** \brief Add a command with no parameter.
 *
 * The following commands support this scheme:
 *
 * \li COMMAND_PARENT_ELEMENT
 * \li COMMAND_RESET
 *
 * \param[in] cmd  The command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (field_search::command_t cmd)
{
    field_search::cmd_info_t inst(cmd);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a "char const *".
 *
 * The following commands support the "char const *" value:
 *
 * \li COMMAND_FIELD_NAME
 * \li COMMAND_PATH
 * \li COMMAND_PARENTS
 * \li COMMAND_LINK
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 * \li COMMAND_CHILD_ELEMENT
 * \li COMMAND_ELEMENT_ATTR
 * \li COMMAND_SAVE
 * \li COMMAND_SAVE_INT64
 * \li COMMAND_SAVE_INT64_DATE
 * \li COMMAND_SAVE_XML
 * \li COMMAND_WARNING
 *
 * \param[in] cmd  The command.
 * \param[in] str_value  The string attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (field_search::command_t cmd, char const *str_value)
{
    field_search::cmd_info_t inst(cmd, QString(str_value));
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QString.
 *
 * The following commands support the QString value:
 *
 * \li COMMAND_FIELD_NAME
 * \li COMMAND_PATH
 * \li COMMAND_PARENTS
 * \li COMMAND_LINK
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 * \li COMMAND_CHILD_ELEMENT
 * \li COMMAND_ELEMENT_ATTR
 *
 * \param[in] cmd  The command.
 * \param[in] str_value  The string attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (field_search::command_t cmd, QString const& str_value)
{
    field_search::cmd_info_t inst(cmd, str_value);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a 64 bit integer.
 *
 * The following commands support the integer:
 *
 * \li COMMAND_CHILDREN
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 * \li COMMAND_LABEL
 * \li COMMAND_GOTO
 * \li COMMAND_IF_FOUND
 * \li COMMAND_IF_NOT_FOUND
 *
 * \param[in] cmd  The command.
 * \param[in] value  The integer attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (field_search::command_t cmd, int64_t int_value)
{
    field_search::cmd_info_t inst(cmd, int_value);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QCassandraValue.
 *
 * The following commands support the QCassandraValue:
 *
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 *
 * \param[in] cmd  The command.
 * \param[in] value  The value attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (field_search::command_t cmd, QtCassandra::QCassandraValue value)
{
    field_search::cmd_info_t inst(cmd, value);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QDomElement.
 *
 * The following commands support the QDomElement:
 *
 * \li COMMAND_ELEMENT
 *
 * \param[in] cmd  The command.
 * \param[in] element  The element attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, QDomElement element)
{
    field_search::cmd_info_t inst(cmd, element);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a search_result_t reference.
 *
 * The following commands support the result reference:
 *
 * \li COMMAND_RESULT
 *
 * \param[in] cmd  The command.
 * \param[in] element  The element attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, search_result_t& result)
{
    field_search::cmd_info_t inst(cmd, result);
    f_program.push_back(inst);
    return *this;
}


/** \brief Run the search commands.
 *
 * This function runs the search commands over the data found in Cassandra.
 * It is somewhat similar to an XPath only it applies to a tree in Cassandra
 * instead of an XML tree.
 *
 * By default, you are expected to search for the very first instance of
 * the parameter sought. It is possible to transform the search in order
 * to search all the parameters that match.
 *
 * \return An array of QCassandraValue's.
 */
void field_search::run()
{
    struct auto_search
    {
        auto_search(char const *filename, char const *func, int line, snap_child *snap, cmd_info_vector_t& program)
            : f_filename(filename)
            , f_function(func)
            , f_line(line)
            , f_snap(snap)
            , f_program(program)
            //, f_mode(SEARCH_MODE_FIRST) -- auto-init
            , f_site_key(f_snap->get_site_key_with_slash())
            //, f_field_name("") -- auto-init
            //, f_self("") -- auto-init
            , f_content_table(content::content::instance()->get_content_table())
            //, f_result() -- auto-init
        {
        }

        void cmd_mode(int64_t mode)
        {
            f_mode = static_cast<int>(mode); // XXX fix, should be a cast to mode_t
        }

        void cmd_field_name(QString const& field_name)
        {
            if(field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("COMMAND_FIELD_NAME cannot be set to an empty string");
            }
            f_field_name = field_name;
        }

        void cmd_self(QString const& self)
        {
            // verify that a field name is defined
            if(f_field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("the field_search cannot check COMMAND_SELF without first being given a COMMAND_FIELD_NAME");
            }

            if(f_content_table->exists(self)
            && f_content_table->row(self)->exists(f_field_name))
            {
                f_found_self = true;

                // found a field, add it to result
                if(SEARCH_MODE_PATHS == f_mode)
                {
                    // save the path(s) only
                    f_result.push_back(self);
                }
                else
                {
                    // save the value
                    f_result.push_back(f_content_table->row(self)->cell(f_field_name)->value());
                }
            }
        }

        void cmd_path(QString const& path)
        {
            f_found_self = false;

            // get the self path and add the site key if required
            // (it CAN be empty in case we are trying to access the home page
            f_self = path;
            if(f_self.isEmpty() || !f_self.startsWith(f_site_key))
            {
                // path does not yet include the site key
                f_snap->canonicalize_path(f_self);
                f_self = f_site_key + f_self;
            }
        }

        void cmd_children(int64_t depth)
        {
            // invalid depth?
            if(depth < 0)
            {
                throw content_exception_invalid_sequence("COMMAND_CHILDREN expects a depth of 0 or more");
            }
            if(depth == 0 || !f_found_self)
            {
                // no depth or no self
                return;
            }

            QStringList children;
            children += f_self;

            for(int i(0); i < children.size(); ++i)
            {
                // first loop through all the children of self for f_field_name
                links::link_info info(get_name(SNAP_NAME_CONTENT_CHILDREN), false, children[i]);
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                links::link_info child_info;
                while(link_ctxt->next_link(child_info))
                {
                    QString const child(child_info.key());
                    cmd_self(child);
                    if(!f_result.isEmpty() && SEARCH_MODE_FIRST == f_mode)
                    {
                        return;
                    }

                    if(depth >= 2)
                    {
                        // record this child as its children will have to be tested
                        children += child;
                    }
                }
                --depth;
            }
        }

        void cmd_parents(QString limit_path)
        {
            // verify that a field name is defined
            if(f_field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("the field_search cannot check COMMAND_PARENTS without first being given a COMMAND_FIELD_NAME");
            }
            if(!f_found_self)
            {
                return;
            }

            // fix the parent limit
            if(!limit_path.startsWith(f_site_key) || limit_path.isEmpty())
            {
                // path does not yet include the site key
                f_snap->canonicalize_path(limit_path);
                limit_path = f_site_key + limit_path;
            }

            if(f_self.startsWith(limit_path))
            {
                // we could use the parent link from each page, but it is
                // a lot faster to compute it each time (no db access)
                QStringList parts(f_self.right(f_self.length() - f_site_key.length()).split('/'));
                while(!parts.isEmpty())
                {
                    parts.pop_back();
                    QString self(parts.join("/"));
                    cmd_self(f_site_key + self);
                    if((!f_result.isEmpty() && SEARCH_MODE_FIRST == f_mode)
                    || self == limit_path)
                    {
                        return;
                    }
                }
            }
        }

        void cmd_link(QString const& link_name)
        {
            if(!f_found_self)
            {
                // no self, no link to follow
                return;
            }

            bool const unique_link(true);
            links::link_info info(link_name, unique_link, f_self);
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
            links::link_info type_info;
            if(link_ctxt->next_link(type_info))
            {
                f_self = type_info.key();
                cmd_self(f_self);
            }
            else
            {
                // no such link
                f_self.clear();
                f_found_self = false;
            }
        }

        void cmd_default_value(QtCassandra::QCassandraValue const& value, bool keep_null)
        {
            if(!value.nullValue() || keep_null)
            {
                f_result.push_back(value);
            }
        }

        void cmd_element(QDomElement element)
        {
            f_element = element;
        }

        void cmd_child_element(QString const& child_name)
        {
            if(!f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QDomElement child(doc.createElement(child_name));
                f_element.appendChild(child);
                f_element = child;
            }
        }

        void cmd_parent_element()
        {
            if(!f_element.isNull())
            {
                f_element = f_element.parentNode().toElement();
            }
        }

        void cmd_element_attr(QString const& attr)
        {
            if(!f_element.isNull())
            {
                QStringList a(attr.split('='));
                if(a.size() == 1)
                {
                    // checked="checked"
                    a += a[0];
                }
                f_element.setAttribute(a[0], a[1]);
            }
        }

        void cmd_reset(bool status)
        {
            f_saved = status;
            f_result.clear();
        }

        void cmd_result(search_result_t& result)
        {
            result = f_result;
        }

        void cmd_save(QString const& child_name)
        {
            if(!f_result.isEmpty() && !f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QStringList children(child_name.split('/'));
                QDomElement parent(f_element);
                while(children.size() != 1)
                {
                    // TODO write a clean parser seeking in the string
                    //      it would make it faster (i.e. no intermediate
                    //      list of strings)
                    QStringList child_attr(children[0].split('['));
                    QDomElement child(doc.createElement(child_attr[0]));
                    parent.appendChild(child);
                    while(child_attr.size() > 1)
                    {
                        // remove the ']' if present
                        if(child_attr[1].right(1) != "]")
                        {
                            throw content_exception_invalid_sequence("invalid attribute definition, missing ']'");
                        }
                        child_attr[1].remove(child_attr[1].length() - 1, 1);
                        QStringList attr_value(child_attr[1].split('='));
                        if(attr_value.size() == 1)
                        {
                            attr_value += attr_value[0];
                        }
                        child.setAttribute(attr_value[0], attr_value[1]);
                        child_attr.removeAt(1);
                    }
                    parent = child;
                    children.removeAt(0);
                }
                QDomElement last_child(doc.createElement(children[0]));
                parent.appendChild(last_child);
                QDomText text(doc.createTextNode(f_result[0].stringValue()));
                last_child.appendChild(text);
                cmd_reset(true);
            }
        }

        void cmd_save_int64(QString const& child_name)
        {
            if(!f_result.isEmpty() && !f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QDomElement child(doc.createElement(child_name));
                f_element.appendChild(child);
                QDomText text(doc.createTextNode(QString("%1").arg(f_result[0].int64Value())));
                child.appendChild(text);
                cmd_reset(true);
            }
        }

        void cmd_save_int64_date(QString const& child_name)
        {
            if(!f_result.isEmpty() && !f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QDomElement child(doc.createElement(child_name));
                f_element.appendChild(child);
                QDomText text(doc.createTextNode(f_snap->date_to_string(f_result[0].int64Value())));
                child.appendChild(text);
                cmd_reset(true);
            }
        }

        void cmd_save_xml(QString const& child_name)
        {
            if(!f_result.isEmpty() && !f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QDomElement child(doc.createElement(child_name));
                f_element.appendChild(child);

                // parse the XML (XHTML) string
                content::content::insert_html_string_to_xml_doc(child, f_result[0].stringValue());

                cmd_reset(true);
            }
        }

        void cmd_if_found(int& i, int64_t label, bool equal)
        {
            if(f_result.isEmpty() == equal)
            {
                cmd_goto(i, label);
            }
        }

        void cmd_goto(int& i, int64_t label)
        {
            int const max(f_program.size());
            for(int j(0); j < max; ++j)
            {
                if(f_program[j].get_command() == COMMAND_LABEL
                && f_program[j].get_int64() == label)
                {
                    // NOTE: the for() loop will do a ++i which is fine
                    //       since we're giving the label position here
                    i = j;
                    return;
                }
            }
            throw content_exception_invalid_sequence(QString("found unknown label %1 at %2").arg(label).arg(i));
        }

        void cmd_warning(QString const& warning_msg)
        {
            // XXX only problem is we do not get the right filename,
            //     line number, function name on this one...
            if(!f_saved)
            {
                SNAP_LOG_WARNING("in ")(f_filename)(":")(f_function)(":")(f_line)(": ")(warning_msg)(" (path: \"")(f_self)("\" and field name: \"")(f_field_name)("\")");
                f_saved = false;
            }
        }

        void run()
        {
            int const max(f_program.size());
            for(int i(0); i < max; ++i)
            {
                switch(f_program[i].get_command())
                {
                case COMMAND_RESET:
                    cmd_reset(false);
                    break;

                case COMMAND_MODE:
                    cmd_mode(f_program[i].get_int64());
                    break;

                case COMMAND_FIELD_NAME:
                    cmd_field_name(f_program[i].get_string());
                    break;

                case COMMAND_SELF:
                    cmd_self(f_self);
                    break;

                case COMMAND_PATH:
                    cmd_path(f_program[i].get_string());
                    break;

                case COMMAND_CHILDREN:
                    cmd_children(f_program[i].get_int64());
                    break;

                case COMMAND_PARENTS:
                    cmd_parents(f_program[i].get_string());
                    break;

                case COMMAND_LINK:
                    cmd_link(f_program[i].get_string());
                    break;

                case COMMAND_DEFAULT_VALUE:
                    cmd_default_value(f_program[i].get_value(), true);
                    break;

                case COMMAND_DEFAULT_VALUE_OR_NULL:
                    cmd_default_value(f_program[i].get_value(), false);
                    break;

                case COMMAND_ELEMENT:
                    cmd_element(f_program[i].get_element());
                    break;

                case COMMAND_CHILD_ELEMENT:
                    cmd_child_element(f_program[i].get_string());
                    break;

                case COMMAND_PARENT_ELEMENT:
                    cmd_parent_element();
                    break;

                case COMMAND_ELEMENT_ATTR:
                    cmd_element_attr(f_program[i].get_string());
                    break;

                case COMMAND_RESULT:
                    cmd_result(*f_program[i].get_result());
                    break;

                case COMMAND_SAVE:
                    cmd_save(f_program[i].get_string());
                    break;

                case COMMAND_SAVE_INT64:
                    cmd_save_int64(f_program[i].get_string());
                    break;

                case COMMAND_SAVE_INT64_DATE:
                    cmd_save_int64_date(f_program[i].get_string());
                    break;

                case COMMAND_SAVE_XML:
                    cmd_save_xml(f_program[i].get_string());
                    break;

                case COMMAND_LABEL:
                    // this is a nop
                    break;

                case COMMAND_IF_FOUND:
                    cmd_if_found(i, f_program[i].get_int64(), false);
                    break;

                case COMMAND_IF_NOT_FOUND:
                    cmd_if_found(i, f_program[i].get_int64(), true);
                    break;

                case COMMAND_GOTO:
                    cmd_goto(i, f_program[i].get_int64());
                    break;

                case COMMAND_WARNING:
                    cmd_warning(f_program[i].get_string());
                    break;

                default:
                    throw content_exception_invalid_sequence(QString("encountered an unknown instruction (%1)").arg(static_cast<int>(f_program[i].get_command())));

                }
                if(!f_result.isEmpty() && SEARCH_MODE_FIRST == f_mode)
                {
                    return;
                }
            }
        }

        char const *                                    f_filename;
        char const *                                    f_function;
        int                                             f_line;
        zpsnap_child_t                                  f_snap;
        cmd_info_vector_t&                              f_program;
        safe_mode_t                                     f_mode;
        QString                                         f_site_key;
        QString                                         f_field_name;
        QString                                         f_self;
        QSharedPointer<QtCassandra::QCassandraTable>    f_content_table;
        QDomElement                                     f_element;
        controlled_vars::fbool_t                        f_found_self;
        controlled_vars::fbool_t                        f_saved;
        field_search::search_result_t                   f_result;
    } search(f_filename, f_function, f_line, f_snap, f_program);

    search.run();
}


/** \brief This function is used by the FIELD_SEARCH macro.
 *
 * This function creates a field_search object and initializes it
 * with the information specified by the FIELD_SEARCH macro. The
 * result is a field_search that we can use to instantly run a
 * search program.
 *
 * \param[in] filename  The name of the file where the FIELD_SEARCH macro is used.
 * \param[in] func  The name of the function using the FIELD_SEARCH macro.
 * \param[in] line  The line where the FIELD_SEARCH macro can be found.
 * \param[in] snap  A pointer to the snap server.
 */
field_search create_field_search(char const *filename, char const *func, int line, snap_child *snap)
{
    field_search fs(filename, func, line, snap);
    return fs;
}




/** \brief Create a structure used to setup an attachment file.
 *
 * This contructor is used whenever loading an attachment from the
 * database. In this case the file is setup from the database
 * information.
 *
 * \param[in] snap  A pointer to the snap_child object.
 */
attachment_file::attachment_file(snap_child *snap)
    : f_snap(snap)
    //, f_file()
    //, f_multiple(false) -- auto-init
    //, f_cpath("") -- auto-init
    //, f_field_name("") -- auto-init
    //, f_attachment_owner("") -- auto-init
    //, f_attachment_type("") -- auto-init
{
}


/** \brief Create a structure used to setup an attachment file.
 *
 * Create and properly initialize this structure and then you can call
 * the create_attachment() function which takes this structure as a parameter
 * to create a new file in the database.
 *
 * To finish the initialization of this structure you must call the
 * following functions:
 *
 * \li set_cpath()
 * \li set_field_name()
 * \li set_attachment_owner()
 * \li set_attachment_type()
 *
 * By default the attachment file structure is set to work on unique
 * files. Call the set_multiple() function to make sure that the
 * user does not overwrite previous attachments.
 *
 * \warning
 * Each attachment file structure can really only be used once (it is
 * used for throw away objects.) The get_name() function, for example,
 * generates the name internally and it is not possible to change it
 * afterward.
 *
 * \warning
 * Calling the get_name() function fails with a throw if some of
 * the mandatory parameters were not properly set.
 *
 * \param[in] snap  A pointer to the snap_child object.
 * \param[in] file  The file to attach to this page.
 */
attachment_file::attachment_file(snap_child *snap, snap_child::post_file_t const& file)
    : f_snap(snap)
    , f_file(file)
    //, f_multiple(false) -- auto-init
    //, f_cpath("") -- auto-init
    //, f_field_name("") -- auto-init
    //, f_attachment_owner("") -- auto-init
    //, f_attachment_type("") -- auto-init
{
}


/** \brief Whether multiple files can be saved under this one name.
 *
 * This function is used to mark the attachment as unique (false) or
 * not (true). If unique, saving the attachment again with a different
 * files removes the existing file first.
 *
 * When multiple is set to true, saving a new file adds it to the list
 * of existing files. The list may be empty too.
 *
 * Multiple adds a unique number at the end of each field name
 * which gives us a full name such as:
 *
 * \code
 * "content::attachment::<owner>::<field name>::path::<server_name>_<unique number>"
 * \endcode
 *
 * By default a file is expected to be unique (multiple is set to false).
 *
 * \param[in] multiple  Whether the field represents a multi-file attachment.
 *
 * \sa get_multiple()
 * \sa get_name()
 */
void attachment_file::set_multiple(bool multiple)
{
    f_multiple = multiple;
}


/** \brief Set the path where the attachment is being added.
 *
 * This is the path to the parent page to which this attachment is
 * being added. A path is mandatory so you will have to call this
 * function, although the empty path is allowed (it represents the
 * home page so be careful!)
 *
 * \note
 * The class marks whether you set the path or not. If not, trying
 * to use it (get_cpath() function called) generates an exception
 * because it is definitively a mistake.
 *
 * \param[in] cpath  The path to the page receiving the attachment.
 *
 * \sa get_cpath()
 */
void attachment_file::set_cpath(QString const& cpath)
{
    f_cpath = cpath;
    f_has_cpath = true;
}


/** \brief Set the name of the field for the attachment.
 *
 * When saving a file as an attachment, we want to save the reference
 * in the parent as such. This makes it a lot easier to find the
 * attachments attached to a page.
 *
 * Note that to retreive the full name to the field, make sure to
 * call the get_name() function, the get_field_name() will return
 * just and only the \<field name> part, not the whole name.
 *
 * \code
 * // name of the field in the database:
 * "content::attachment::<owner>::<field name>::path"
 *
 * // or, if multiple is set to true:
 * "content::attachment::<owner>::<field name>::path::<server_name>_<unique number>"
 * \endcode
 *
 * \param[in] field_name  The name of the field used to save this attachment.
 *
 * \sa get_field_name()
 * \sa get_name()
 */
void attachment_file::set_field_name(QString const& field_name)
{
    f_field_name = field_name;
}


/** \brief Set the owner of this attachment.
 *
 * This name represents the plugin owner of the attachment. It must be
 * a valid plugin name as it is saved as the owner of the attachment.
 * This allows the plugin to specially handle the attachment when the
 * client wants to retrieve it.
 *
 * Note that this name is also used in the name of field holding the
 * path to the attachment.
 *
 * \param[in] owner  The name of the plugin that owns that attachment.
 *
 * \sa get_attachment_owner()
 */
void attachment_file::set_attachment_owner(QString const& owner)
{
    f_attachment_owner = owner;
}


/** \brief Define the type of the attachment page.
 *
 * When adding an attachment to the database, a new page is created as
 * a child of the page where the attachment is added. This allows us
 * to easily do all sorts of things with attachments. This new page being
 * content it needs to have a type and this type represents that type.
 *
 * In most cases the type is set to the parent by default.
 *
 * \param[in] type  The type of the page.
 *
 * \sa get_attachment_type()
 */
void attachment_file::set_attachment_type(QString const& type)
{
    f_attachment_type = type;
}


/** \brief Set the creation time of the attachment.
 *
 * The first time the user POSTs an attachment, it saves the start date of
 * the HTTP request as the creation date. The loader sets the date back in
 * the attachment.
 *
 * \param[in] time  The time when the attachment was added to the database.
 *
 * \sa get_creation_time()
 */
void attachment_file::set_creation_time(int64_t time)
{
    f_creation_time = time;
}


/** \brief Set the modification time of the attachment.
 *
 * Each time the user POSTs an attachment, it saves the start date of the
 * HTTP request as the modification date. The loader sets the date back in
 * the attachment.
 *
 * \param[in] time  The time when the attachment was last modified.
 *
 * \sa get_update_time()
 */
void attachment_file::set_update_time(int64_t time)
{
    f_update_time = time;
}


/** \brief Set the name of the field the attachment comes from.
 *
 * This function is used by the load_attachment() function to set the
 * name of the file attachment as if it had been sent by a POST.
 *
 * \param[in] name  The name of the field used to upload this attachment.
 */
void attachment_file::set_file_name(QString const& name)
{
    f_file.set_name(name);
}


/** \brief Set the name of the file.
 *
 * This function sets the name of the file as it was sent by the POST
 * sending the attachment.
 *
 * \param[in] filename  The name of the file as sent by the POST message.
 */
void attachment_file::set_file_filename(QString const& filename)
{
    f_file.set_filename(filename);
}


/** \brief Set the mime_type of the file.
 *
 * This function can be used to setup the MIME type of the file when
 * the data if the file is not going to be set in the attachment file.
 * (It is useful NOT to load the data if you are not going to use it
 * anyway!)
 *
 * The original MIME type is the one sent by the browser at
 * the time the attachment was POSTed.
 *
 * \param[in] mime_type  The original MIME time as sent by the client.
 */
void attachment_file::set_file_mime_type(QString const& mime_type)
{
    f_file.set_mime_type(mime_type);
}


/** \brief Set the original mime_type of the file.
 *
 * This function can be used to setup the original MIME type of the
 * file. The original MIME type is the one sent by the browser at
 * the time the attachment was POSTed.
 *
 * \param[in] mime_type  The original MIME time as sent by the client.
 */
void attachment_file::set_file_original_mime_type(QString const& mime_type)
{
    f_file.set_original_mime_type(mime_type);
}


/** \brief Set the creation time.
 *
 * This function sets the creating time of the file.
 *
 * \param[in] ctime  The creation time.
 */
void attachment_file::set_file_creation_time(time_t ctime)
{
    f_file.set_creation_time(ctime);
}


/** \brief Set the modification time.
 *
 * This function sets the creation time of the file.
 *
 * \param[in] mtime  The last modification time.
 */
void attachment_file::set_file_modification_time(time_t mtime)
{
    f_file.set_modification_time(mtime);
}


/** \brief Set the data of the file.
 *
 * This function sets the data of the file. This is the actual file
 * content.
 *
 * \param[in] data  The last modification time.
 */
void attachment_file::set_file_data(QByteArray const& data)
{
    f_file.set_data(data);
}


/** \brief Set the size of the file.
 *
 * This function sets the size of the file. This is particularly
 * useful if you do not want to load the data but still want to
 * get the size for display purposes.
 *
 * \param[in] size  Set the size of the file.
 */
void attachment_file::set_file_size(int size)
{
    f_file.set_size(size);
}


/** \brief Set the image width.
 *
 * This function is used to set the width of the image.
 *
 * \param[in] width  The image width.
 */
void attachment_file::set_file_image_width(int width)
{
    f_file.set_image_width(width);
}


/** \brief Set the image height.
 *
 * This function is used to set the height of the image.
 *
 * \param[in] height  The image height.
 */
void attachment_file::set_file_image_height(int height)
{
    f_file.set_image_height(height);
}


/** \brief Set the index of the field.
 *
 * This function is used to set the field index within the form.
 *
 * \param[in] index  The index of the field.
 */
void attachment_file::set_file_index(int index)
{
    f_file.set_index(index);
}


/** \brief Return whether the attachment is unique or not.
 *
 * This function returns the flag as set by the set_multiple().
 * If true it means that as many attachments as necessary can
 * be added under the same field name. Otherwise only one
 * attachment can be added.
 *
 * \return Whether multiple attachments can be saved.
 *
 * \sa set_multiple()
 * \sa get_name()
 */
bool attachment_file::get_multiple() const
{
    return f_multiple;
}


/** \brief Return the file structure.
 *
 * When receiving a file, in most cases it is via an upload so we
 * use that structure directly to avoid copying all that data all
 * the time.
 *
 * This function returns a reference so you can directly use a
 * reference instead of a copy.
 *
 * \note
 * The only way to setup the file is via the constructor.
 *
 * \return A reference to the file representing this attachment.
 *
 * \sa attachment_file()
 */
snap_child::post_file_t const& attachment_file::get_file() const
{
    return f_file;
}


/** \brief Path to the parent of the file.
 *
 * This path represents the parent receiving this attachment.
 *
 * \return The path to the parent of the attachment.
 *
 * \sa set_cpath()
 */
QString const& attachment_file::get_cpath() const
{
    if(!f_has_cpath)
    {
        throw content_exception_invalid_name("the cpath parameter of a attachment_file object was never set");
    }
    return f_cpath;
}


/** \brief Retrieve the name of the field.
 *
 * This function retrieves the raw name of the field. For the complete
 * name, make sure to use the get_name() function instead.
 *
 * \exception content_exception_invalid_name
 * This function generates the invalid name exception if the owner was
 * not defined an the parameter is still empty at the time it is to be
 * used.
 *
 * \return The field name to use for this attachment file.
 *
 * \sa set_field_name()
 */
QString const& attachment_file::get_field_name() const
{
    if(f_field_name.isEmpty())
    {
        throw content_exception_invalid_name("the field name of a attachment_file object cannot be empty");
    }
    return f_field_name;
}


/** \brief Retrieve the owner of the attachment page.
 *
 * This function returns the name of the plugin that becomes the attachment
 * owner in the content table. The owner has rights over the content to
 * display it, allow the client to download it, etc.
 *
 * \exception content_exception_invalid_name
 * This function generates the invalid name exception if the owner was
 * not defined an the parameter is still empty at the time it is to be
 * used.
 *
 * \return The name of the plugin that owns this attachment file.
 *
 * \sa set_attachment_owner()
 */
QString const& attachment_file::get_attachment_owner() const
{
    if(f_attachment_owner.isEmpty())
    {
        throw content_exception_invalid_name("the attachment owner of a attachment_file object cannot be empty");
    }
    return f_attachment_owner;
}


/** \brief Retrieve the type of the attachment page.
 *
 * This function returns the type to use for the page we are to create for
 * this attachment. This is one of the .../content-types/\<name> types.
 *
 * \exception content_exception_invalid_name
 * This function generates the invalid name exception if the type was
 * not defined and the parameter is still empty at the time it is to be
 * used.
 *
 * \return The name of the type to use for the attachment page.
 *
 * \sa set_attachment_type()
 */
QString const& attachment_file::get_attachment_type() const
{
    if(f_attachment_type.isEmpty())
    {
        throw content_exception_invalid_name("the attachment type of a attachment_file object cannot be empty");
    }
    return f_attachment_type;
}


/** \brief Set the creation time of the attachment.
 *
 * The first time the user POSTs an attachment, it saves the start date of
 * the HTTP request as the creation date. The loader sets the date back in
 * the attachment.
 *
 * \param[in] time  The time when the attachment was added to the database.
 *
 * \sa set_creation_time()
 */
int64_t attachment_file::get_creation_time() const
{
    return f_creation_time;
}


/** \brief Get the modification time of the attachment.
 *
 * Each time the user POSTs an attachment, it saves the start date of the
 * HTTP request as the modification date. The loader sets the date back in
 * the attachment.
 *
 * \param[in] time  The time when the attachment was last modified.
 *
 * \sa set_update_time()
 */
int64_t attachment_file::get_update_time() const
{
    return f_update_time;
}


/** \brief Generate the full field name.
 *
 * The name of the field in the parent page in the content is defined
 * as follow:
 *
 * \code
 * // name of the field in the database:
 * "content::attachment::<owner>::<field name>::path"
 *
 * // or, if multiple is set to true:
 * "content::attachment::<owner>::<field name>::path::<server_name>_<unique number>"
 * \endcode
 *
 * To make sure that everyone always uses the same name each time, we
 * created this function and you'll automatically get the right name
 * every time.
 *
 * \warning
 * After the first call this function always returns exactly the same
 * name. This is because we cache the name so it can be called any
 * number of time and it will quickly return with the name.
 *
 * \return The full name of the field.
 *
 * \sa set_multiple()
 * \sa set_field_name()
 * \sa set_attachment_owner()
 */
QString const& attachment_file::get_name() const
{
    if(f_name.isEmpty())
    {
        if(f_multiple)
        {
            f_name = QString("%1::%2::%3::%4::%5")
                    .arg(snap::content::get_name(SNAP_NAME_CONTENT_ATTACHMENT))
                    .arg(get_attachment_owner())
                    .arg(get_field_name())
                    .arg(f_snap->get_unique_number())
                    .arg(snap::content::get_name(SNAP_NAME_CONTENT_ATTACHMENT_PATH_END))
                    ;
        }
        else
        {
            f_name = QString("%1::%2::%3::%4")
                    .arg(snap::content::get_name(SNAP_NAME_CONTENT_ATTACHMENT))
                    .arg(get_attachment_owner())
                    .arg(get_field_name())
                    .arg(snap::content::get_name(SNAP_NAME_CONTENT_ATTACHMENT_PATH_END))
                    ;
        }
    }
    return f_name;
}







/** \brief Useful function that transforms a QString to XML.
 *
 * When inserting a string in the XML document when that string may include
 * HTML code, call this function, it will first convert the string to XML
 * then insert the result as children of the \p child element.
 *
 * \param[in,out] child  DOM element receiving the result as children nodes.
 * \param[in] xml  The input XML string.
 */
void content::insert_html_string_to_xml_doc(QDomElement child, QString const& xml)
{
    // parsing the XML can be slow, try to avoid that if possible
    if(xml.contains('<'))
    {
        QDomDocument xml_doc("wrapper");
        xml_doc.setContent("<wrapper>" + xml + "</wrapper>", true, NULL, NULL, NULL);

        // copy the result in a fragment of our document
        QDomDocumentFragment frag(child.ownerDocument().createDocumentFragment());
        frag.appendChild(child.ownerDocument().importNode(xml_doc.documentElement(), true));

        // copy the fragment nodes at the right place
        QDomNodeList children(frag.firstChild().childNodes());
        QDomNode previous(children.at(0));
        child.appendChild(children.at(0));
        while(!children.isEmpty())
        {
            QDomNode l(children.at(0));
            child.insertAfter(children.at(0), previous);
            previous = l;
        }
    }
    else
    {
        QDomText text(child.ownerDocument().createTextNode(xml));
        child.appendChild(text);
    }
}


void content::secure_flag::not_secure(QString const& new_reason)
{
    f_secure = false;

    if(!f_reason.isEmpty())
    {
        f_reason += "\n";
    }
    // TBD: should we prevent "\n" in "reason"?
    f_reason += new_reason;
}


/** \brief Initialize the content plugin.
 *
 * This function is used to initialize the content plugin object.
 */
content::content()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Clean up the content plugin.
 *
 * Ensure the content object is clean before it is gone.
 */
content::~content()
{
}


/** \brief Initialize the content.
 *
 * This function terminates the initialization of the content plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void content::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN0(content, "server", server, save_content);
    SNAP_LISTEN0(content, "server", server, backend_process);
    SNAP_LISTEN(content, "layout", layout::layout, generate_page_content, _1, _2, _3, _4, _5);

    if(plugins::exists("javascript"))
    {
        javascript::javascript::instance()->register_dynamic_plugin(this);
    }
}


/** \brief Get a pointer to the content plugin.
 *
 * This function returns an instance pointer to the content plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the content plugin.
 */
content *content::instance()
{
    return g_plugin_content_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString content::description() const
{
    return "Manage nearly all the content of your website. This plugin handles"
        " your pages, the website taxonomy (tags, categories, permissions...)"
        " and much much more.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t content::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2013, 12, 25, 11, 19, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the content plugin.
 *
 * This function is the first update for the content plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void content::initial_update(int64_t variables_timestamp)
{
    (void)variables_timestamp;
    get_content_table();
    get_files_table();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void content::content_update(int64_t variables_timestamp)
{
    (void)variables_timestamp;
    add_xml(get_plugin_name());
}


/** \brief Initialize the content table.
 *
 * This function creates the content table if it doesn't exist yet. Otherwise
 * it simple initializes the f_content_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The content table is the one that includes the entire content of all
 * the websites. Since tables can grow as big as we want, this is not a
 * concern. The content table looks like a tree although each row represents
 * one leaf at any one level (the keys are the site key with slash + path).
 *
 * \return The pointer to the content table.
 */
QSharedPointer<QtCassandra::QCassandraTable> content::get_content_table()
{
    if(f_content_table.isNull())
    {
        f_content_table = f_snap->create_table(get_name(SNAP_NAME_CONTENT_TABLE), "Website content table.");
    }
    return f_content_table;
}


/** \brief Initialize the files table.
 *
 * This function creates the files table if it doesn't exist yet. Otherwise
 * it simple initializes the f_files_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The table is used to list all the files from all the websites managed
 * by this Snap! server. Note that the files are listed for all the
 * websites, by website & filename, when new and need to be checked
 * (anti-virus, etc.) and maybe a few other things later.
 *
 * \li Rows are MD5 sums of the files, this is used as the key in the
 *     content table
 * \li '*new*' includes MD5 sums of files to be checked (anti-virus, ...)
 * \li '*index*' lists of files by 'site key + filename'
 *
 * \return The pointer to the files table.
 */
QSharedPointer<QtCassandra::QCassandraTable> content::get_files_table()
{
    if(f_files_table.isNull())
    {
        f_files_table = f_snap->create_table(get_name(SNAP_NAME_CONTENT_FILES_TABLE), "List of all the files ever uploaded to all the websites.");
    }
    return f_files_table;
}


/** \brief Execute a page: generate the complete output of that page.
 *
 * This function displays the page that the user is trying to view. It is
 * supposed that the page permissions were already checked and thus that
 * its contents can be displayed to the current user.
 *
 * Note that the path was canonicalized by the path plugin and thus it does
 * not require any further corrections.
 *
 * \param[in] cpath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool content::on_path_execute(const QString& cpath)
{
    f_snap->output(layout::layout::instance()->apply_layout(cpath, this));

    return true;
}


/** \brief Create a page at the specified path.
 *
 * This function creates a page in the database at the specified path.
 * The page will be ready to be used once all the plugins had a chance
 * to run their own on_create_content() function.
 *
 * Note that if the page (as in, the row as defined by the path) already
 * exists then the function returns immediately.
 *
 * The full key for the page makes use of the site key which cannot already
 * be included in the path.
 *
 * The type of a new page must be specified. By default, the type is set
 * to "page". Specific modules may offer additional types. The three offered
 * by the content plugin are:
 *
 * \li "page" -- a standard user page.
 * \li "administration-page" -- in general any page under /admin.
 * \li "system-page" -- a page created by the content.xml which is not under
 *                      /admin.
 *
 * The page type MUST be just the type. It may be a path since a type
 * of page may be a sub-type of an basic type. For example, a "blog"
 * type would actually be a page and thus the proper type to pass to
 * this function is "page/blog" and not a full path or just "blog".
 * We force you in this way so any plugin can test the type without
 * having to frantically test all sorts of cases.
 *
 * \param[in] path  The path of the new page.
 * \param[in] owner  The name of the plugin that is to own this page.
 * \param[in] type  The type of page.
 *
 * \return true if the signal is to be propagated.
 */
bool content::create_content_impl(QString const& path, QString const& owner, QString const& type)
{
    QSharedPointer<QtCassandra::QCassandraTable> content_table(get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(site_key + path);

    QString const primary_owner(path::get_name(path::SNAP_NAME_PATH_PRIMARY_OWNER));
    QSharedPointer<QtCassandra::QCassandraRow> row(content_table->row(key));
    if(row->exists(primary_owner))
    {
        // the row already exists, this is considered created.
        // (we may later want to have a repair_content signal
        // which we could run as an action from the backend...)
        // however, if it were created by an add_xml() call,
        // then the on_create_content() of all the other plugins
        // should probably be called (i.e. f_updating is true then)
        return f_updating;
    }

    // save the owner
    row->cell(primary_owner)->setValue(owner);

    // add the different basic content dates setup
    uint64_t const start_date(f_snap->get_uri().option("start_date").toLongLong());
    row->cell(QString(get_name(SNAP_NAME_CONTENT_CREATED)))->setValue(start_date);
    row->cell(QString(get_name(SNAP_NAME_CONTENT_UPDATED)))->setValue(start_date);
    row->cell(QString(get_name(SNAP_NAME_CONTENT_MODIFIED)))->setValue(start_date);

    // link the page to its type (very important for permissions)
    {
        // TODO we probably should test whether that content-types exists
        //      because if not it's certainly completely invalid (i.e. the
        //      programmer mistyped the type [again])
        QString const destination_key(site_key + "types/taxonomy/system/content-types/" + (type.isEmpty() ? "page" : type));
        QString const link_name(get_name(SNAP_NAME_CONTENT_PAGE_TYPE));
        QString const link_to(get_name(SNAP_NAME_CONTENT_PAGE_TYPE));
        bool const source_unique(true);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, key);
        links::link_info destination(link_to, destination_unique, destination_key);
        links::links::instance()->create_link(source, destination);
    }

    // link this entry to its parent automatically
    // first we need to remove the site key from the path
    QStringList parts(path.split('/', QString::SkipEmptyParts));
    while(parts.count() > 0)
    {
        QString src(parts.join("/"));
        src = site_key + src;
        parts.pop_back();
        QString dst(parts.join("/"));
        dst = site_key + dst;
        links::link_info source(get_name(SNAP_NAME_CONTENT_PARENT), true, src);
        links::link_info destination(get_name(SNAP_NAME_CONTENT_CHILDREN), false, dst);
// TODO only repeat if the parent did not exist, otherwise we assume the
//      parent created its own parent/children link already.
//printf("parent/children [%s]/[%s]\n", src.toUtf8().data(), dst.toUtf8().data());
        links::links::instance()->create_link(source, destination);
    }

    return true;
}


/** \brief Create a page which represents an attachment (A file).
 *
 * This function creates a page that represents an attachment with the
 * specified file, owner, and type.
 *
 * This function prepares the file and sends a create_content() event
 * first.
 *
 * Note that the MIME type of the file is generated using the magic
 * database. The \p attachment_type parameter is the one saved in the
 * page referencing that file. However, only the one generated by magic
 * is official.
 *
 * \note
 * It is important to understand that we only save each file only ONCE
 * in the database. This is accomplished by create_attachment() by computing
 * the MD5 sum of the file and then checking whether the file was
 * previously already loaded. If so, then the existing copy is used
 * (even if it was uploaded by someone else.)
 *
 * Possible cases when creating an attachment:
 *
 * \li The file does not yet exist in the files table; in that case we
 *     simply create it
 *
 * \li If the file already existed, we do not add it again (obviously)
 *     and we can check whether it was already attached to that very
 *     same page; if so then we have nothing else to do (files have
 *     links of all the pages were they are attachments)
 *
 * \param[in] file  The file to save in the Cassandra database.
 */
bool content::create_attachment_impl(attachment_file const& file)
{
    // verify that the row specified by file::get_cpath() exists
    QSharedPointer<QtCassandra::QCassandraTable> content_table(get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(site_key + file.get_cpath());
    if(!content_table->exists(key))
    {
        // the parent row does not even exist yet...
        return false;
    }
    QSharedPointer<QtCassandra::QCassandraRow> parent_row(content_table->row(key));

    snap_child::post_file_t const& post_file(file.get_file());

    // create the path to the new attachment itself
    QString filename(post_file.get_filename());
    int last_slash(filename.lastIndexOf('/'));
    if(last_slash != -1)
    {
        filename = filename.mid(last_slash + 1);
    }
    QString const attachment_cpath(file.get_cpath() + "/" + filename);
    QString const attachment_key(site_key + attachment_cpath);

    QString const name(file.get_name());

    // compute the MD5 sum of the file
    // TBD should we forbid the saving of empty files?
    unsigned char md[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<unsigned char const *>(post_file.get_data().data()), post_file.get_size(), md);
    QByteArray md5(reinterpret_cast<char const *>(md), sizeof(md));

    // check whether the file already exists in the database
    QSharedPointer<QtCassandra::QCassandraRow> attachment_row;
    QSharedPointer<QtCassandra::QCassandraTable> files_table(get_files_table());
    if(!files_table->exists(md5))
    {
        // the file does not exist yet, add it
        //
        // 1. create the row with the file data, the compression used,
        //    and size; also add it to the list of new cells
        files_table->row(md5)->cell(get_name(SNAP_NAME_CONTENT_FILES_DATA))->setValue(post_file.get_data());
        files_table->row(get_name(SNAP_NAME_CONTENT_FILES_NEW))->cell(md5)->setValue(true);

        QSharedPointer<QtCassandra::QCassandraRow> file_row(files_table->row(md5));

        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_COMPRESSOR))->setValue(get_name(SNAP_NAME_CONTENT_COMPRESSOR_UNCOMPRESSED));
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SIZE))->setValue(static_cast<int32_t>(post_file.get_size()));

        // Note we save the following mainly for completness because it is
        // not really usable (i.e. two people who are to upload the same file
        // with the same filename, the same original MIME type, the same
        // creation/modification dates... close to impossible!)
        //
        // 2. link back to the row where the file is saved in the content table
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_FILENAME))->setValue(filename);

        // 3. save the computed MIME type
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_MIME_TYPE))->setValue(post_file.get_mime_type());

        // 4. save the original MIME type
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE))->setValue(post_file.get_original_mime_type());

        // 5. save the creation date if available (i.e. if not zero)
        if(post_file.get_creation_time() != 0)
        {
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_CREATION_TIME))->setValue(static_cast<int64_t>(post_file.get_creation_time()));
        }

        // 6. save the modification date if available (i.e. if not zero)
        if(post_file.get_modification_time() != 0)
        {
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME))->setValue(static_cast<int64_t>(post_file.get_modification_time()));
        }

        // 7. save the date when the file was uploaded
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_CREATED))->setValue(f_snap->get_start_date());

        // 8. save the date when the file was last updated
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_UPDATED))->setValue(f_snap->get_start_date());

        // 9. if the file is an image save the width & height
        int32_t width(post_file.get_image_width());
        int32_t height(post_file.get_image_height());
        if(width > 0 && height > 0)
        {
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH))->setValue(width);
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT))->setValue(height);
        }

        // 10. save the description
        // At this point we do not have that available, we could use the
        // comment/description from the file if there is such, but those
        // are often "broken" (i.e. version of the camera used...)

        // TODO should we also save a SHA1 of the files so people downloading
        //      can be given the SHA1 even if the file is saved compressed?

        // 11. Some additional fields
        signed char sflag(CONTENT_SECURE_UNDEFINED);
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE))->setValue(sflag);
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK))->setValue(static_cast<int64_t>(0));
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURITY_REASON))->setValue(QString());
    }

    // make a full reference back to the attachment (which may not yet
    // exist at this point, we do that next)
    signed char ref(1);
    files_table->row(md5)->cell(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE) + ("::" + attachment_key))->setValue(ref);

    // if the field exists and that attach is unique (i.e. supports only
    // one single file), then we want to delete the existing page unless
    // the user uploaded a file with the exact same filename
    if(content_table->exists(attachment_key))
    {
        if(!file.get_multiple())
        {
            // it exists, check the filename first
            if(parent_row->exists(name))
            {
                // check the filename
                QString old_attachment_key(parent_row->cell(name)->value().stringValue());
                if(!old_attachment_key.isEmpty() && old_attachment_key != attachment_key)
                {
                    // that's not the same filename, drop it
                    // WE CANNOT JUST DROP A ROW, it breaks all the links, etc.
                    // TODO: implement a delete_content() function which
                    //       does all the necessary work (and actually move
                    //       the content to the trashcan)
                    //content_table->dropRow(old_attachment_key, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());

                    // TODO: nothing should be deleted in our system, instead
                    //       it should be put in a form of trashcan; in this
                    //       case it could remain an attachment, only moved
                    //       to a special "old attachments" list

                    // TBD if I'm correct, the md5 reference was already dropped
                    //     in the previous if() blocks...
                }
            }
        }

        attachment_row = content_table->row(attachment_key);
        if(attachment_row->exists(get_name(SNAP_NAME_CONTENT_ATTACHMENT)))
        {
            // the MD5 is saved in there, get it and compare
            QtCassandra::QCassandraValue existing_ref(attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT))->value());
            if(!existing_ref.nullValue())
            {
                if(existing_ref.binaryValue() == md5)
                {
                    // this is the exact same file, do nearly nothing
                    // (i.e. the file may already exist but the path
                    //       may not be there anymore)
                    parent_row->cell(name)->setValue(attachment_key);

                    // TBD -- should the 'updated' flag really be true since there is
                    //        no visible change to talk about?
                    modified_content(attachment_cpath, true);

                    // TBD -- should it be true here to let the other plugins
                    //        do their own work?
                    return false;
                }

                // not the same file, we've got to remove the reference
                // from the existing file since it's going to be moved
                // to a new file (i.e. the current md5 points to a
                // different file)
                //
                // TODO: nothing should be just dropped in our system,
                //       instead it should be moved to some form of
                //       trashcan; in this case we'd use a new name
                //       for the reference although if the whole row
                //       is to be "dropped" (see below) then we should
                //       not even have to drop this cell at all because
                //       it will remain there, only under a different
                //       name...
                files_table->row(existing_ref.binaryValue())->dropCell(attachment_cpath);
            }
        }

        // it is not there yet, so go on...
        //
        // TODO: we want to check all the attachments and see if any
        //       one of them is the same file (i.e. user uploading the
        //       same file twice with two different file names...)

        files_table->row(md5)->cell(get_name(SNAP_NAME_CONTENT_FILES_UPDATED))->setValue(f_snap->get_start_date());
    }

    // yes that path may already exists, no worries since the create_content()
    // function checks that and returns quickly if it does exist
    create_content(attachment_cpath, file.get_attachment_owner(), file.get_attachment_type());

    // if it is already filename it won't hurt too much to set it again
    parent_row->cell(name)->setValue(attachment_key);

    // get the attachment row anew, just in case it was not taken yet
    attachment_row = content_table->row(attachment_key);

    // in this case 'post' represents the filename as sent by the
    // user, the binary data is in the corresponding file
    attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT_FILENAME))->setValue(filename);

    // save the file reference
    attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT))->setValue(md5);

    // save the MIME type (this is the one returned by the magic library)
    attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT_MIME_TYPE))->setValue(post_file.get_mime_type());

    // XXX we could also save the modification and creation times, but the
    //     likelihood that these exist is so small that I'll skip at this
    //     time; we do save them in the files table

    // mark that attachment as final (i.e. cannot create children below an attachment)
    signed char final(1);
    attachment_row->cell(get_name(SNAP_NAME_CONTENT_FINAL))->setValue(final);

    return true;
}


/** \brief Load an attachment previously saved with create_attachment().
 *
 * The function checks that the attachment exists and is in good condition
 * and if so, loads it in the specified file parameter.
 *
 * \param[in] key  The key to the attachment to load.
 * \param[in] file  The file will be loaded in this structure.
 * \param[in] load_data  Whether the data should be loaded (true by default.)
 *
 * \return true if the attachment was successfully loaded.
 */
bool content::load_attachment(QString const& key, attachment_file& file, bool load_data)
{
    QSharedPointer<QtCassandra::QCassandraTable> content_table(get_content_table());
    if(!content_table->exists(key))
    {
        // the parent row does not even exist yet...
        return false;
    }
    QSharedPointer<QtCassandra::QCassandraRow> attachment_row(content_table->row(key));

    QtCassandra::QCassandraValue md5_value(attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT))->value());

    QSharedPointer<QtCassandra::QCassandraTable> files_table(get_files_table());
    if(!files_table->exists(md5_value.binaryValue()))
    {
        // file not available?!
        return false;
    }
    QSharedPointer<QtCassandra::QCassandraRow> file_row(files_table->row(md5_value.binaryValue()));

    if(!file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_DATA)))
    {
        // no data available
        return false;
    }

    // TODO handle the compression of the file...
    //file.set_file_compressor(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_COMPRESSOR))->value()->stringValue());

    if(load_data)
    {
        file.set_file_data(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_DATA))->value().binaryValue());

        // TODO if compressed, we may have (want) to decompress here?
    }
    else
    {
        // since we're not loading the data, we want to get some additional
        // information on the side: the verified MIME type and the file size
        if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_MIME_TYPE)))
        {
            // This one gets set automatically when we set the data so we only
            // load it if the data is not getting loaded
            file.set_file_mime_type(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_MIME_TYPE))->value().stringValue());
        }
        if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_SIZE)))
        {
            // since we're not loading the data, we get the size parameter
            // like this (later we may want to always do that once we save
            // files compressed in the database!)
            file.set_file_size(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SIZE))->value().int32Value());
        }
    }

    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_FILENAME)))
    {
        file.set_file_filename(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_FILENAME))->value().stringValue());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE)))
    {
        file.set_file_original_mime_type(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE))->value().stringValue());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_CREATION_TIME)))
    {
        file.set_file_creation_time(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_CREATION_TIME))->value().int64Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME)))
    {
        file.set_file_creation_time(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME))->value().int64Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_CREATED)))
    {
        file.set_creation_time(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_CREATED))->value().int64Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_UPDATED)))
    {
        file.set_update_time(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_UPDATED))->value().int64Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH)))
    {
        file.set_file_image_width(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH))->value().int32Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT)))
    {
        file.set_file_image_height(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT))->value().int32Value());
    }

    return true;
}


/** \brief Tell the system that data was updated.
 *
 * This signal should be called any time you modify something in a page.
 *
 * This very function takes care of updating the content::modified and
 * content:updated as required:
 *
 * \li content::modified -- if anything changes in a page, this date
 *                          is changed; in other words, any time this
 *                          function is called, this date is set to
 *                          the current date
 *
 * \li content::updated -- if the content gets updated then this date
 *                         is expected to change; "content" here means
 *                         the title, body, or "any" important content
 *                         that is shown to the user (i.e. a small
 *                         change in a field that is not displayed or
 *                         is not directly considered content as part of
 *                         the main body of the page should not change
 *                         this date)
 *
 * This signal also gives other modules a chance to update their own
 * data (i.e. the sitemap.xml needs to update this page information.)
 *
 * Since the other plugins may make use of your plugin changes, you have
 * to call this signal last.
 *
 * \note
 * The function returns false and generates a warning (in your log) in the
 * event the process cannot find the specified path.
 *
 * \param[in] path  The path to the page being udpated.
 * \param[in] updated  Set to true if a user visible piece of content was
 *                     modified (i.e. title, body...); meta data changes
 *                     are not reported here and updated should be false
 *                     for those.
 *
 * \return true if the event should be propagated.
 */
bool content::modified_content_impl(QString const& path, bool updated)
{
    QSharedPointer<QtCassandra::QCassandraTable> content_table(get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(site_key + path);

    if(!content_table->exists(key))
    {
        // the row doesn't exist?!
        SNAP_LOG_WARNING("Page \"")(key)("\" does not exist. We cannot do anything about it being modified.");;
        return false;
    }
    QSharedPointer<QtCassandra::QCassandraRow> row(content_table->row(key));

    uint64_t const start_date(f_snap->get_uri().option("start_date").toLongLong());
    if(updated)
    {
        row->cell(QString(get_name(SNAP_NAME_CONTENT_UPDATED)))->setValue(start_date);
    }
    row->cell(QString(get_name(SNAP_NAME_CONTENT_MODIFIED)))->setValue(start_date);

    return true;
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the <page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in] l  The layout pointer.
 * \param[in] path  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void content::on_generate_main_content(layout::layout *l, QString const& cpath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    // if the content is the main page then define the titles and body here
    FIELD_SEARCH
        (field_search::COMMAND_MODE, field_search::SEARCH_MODE_EACH)
        (field_search::COMMAND_ELEMENT, body)
        (field_search::COMMAND_PATH, cpath)

        // /snap/page/body/titles
        (field_search::COMMAND_CHILD_ELEMENT, "titles")
        // /snap/page/body/titles/title
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_TITLE))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_IF_FOUND, 1)
            (field_search::COMMAND_PATH, ctemplate)
            (field_search::COMMAND_SELF)
            (field_search::COMMAND_PATH, cpath)
        (field_search::COMMAND_LABEL, 1)
        (field_search::COMMAND_SAVE, "title")
        // /snap/page/body/titles/short-title
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_SHORT_TITLE))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_IF_FOUND, 2)
            (field_search::COMMAND_PATH, ctemplate)
            (field_search::COMMAND_SELF)
            (field_search::COMMAND_PATH, cpath)
        (field_search::COMMAND_LABEL, 2)
        (field_search::COMMAND_SAVE, "short-title")
        // /snap/page/body/titles/long-title
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_LONG_TITLE))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_IF_FOUND, 3)
            (field_search::COMMAND_PATH, ctemplate)
            (field_search::COMMAND_SELF)
            (field_search::COMMAND_PATH, cpath)
        (field_search::COMMAND_LABEL, 3)
        (field_search::COMMAND_SAVE, "long-title")
        (field_search::COMMAND_PARENT_ELEMENT)

        // /snap/page/body/content
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_BODY))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_IF_FOUND, 10)
            (field_search::COMMAND_PATH, ctemplate)
            (field_search::COMMAND_SELF)
            //(field_search::COMMAND_PATH, cpath) -- uncomment if we go on
        (field_search::COMMAND_LABEL, 10)
        (field_search::COMMAND_SAVE_XML, "content")

        // generate!
        ;
}
#pragma GCC diagnostic pop


/** \brief Generate the page common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in] l  The layout pointer.
 * \param[in] cpath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The body being generated.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void content::on_generate_page_content(layout::layout *l, const QString& cpath, QDomElement& page, QDomElement& body, const QString& ctemplate)
{
    // create information mainly used in the HTML <head> tag
    QString up;
    int const p(cpath.lastIndexOf('/'));
    if(p == -1)
    {
        // in this case it is an equivalent to top
        up = f_snap->get_site_key();
    }
    else
    {
        up = f_snap->get_site_key_with_slash() + cpath.mid(0, p);
    }

    FIELD_SEARCH
        (field_search::COMMAND_MODE, field_search::SEARCH_MODE_EACH)
        (field_search::COMMAND_ELEMENT, body)
        (field_search::COMMAND_PATH, cpath)

        // /snap/page/body/created
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_CREATED))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_SAVE_INT64_DATE, "created")
        (field_search::COMMAND_WARNING, "field missing")

        // /snap/page/body/modified
        // XXX should it be mandatory or just use "created" as the default?
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_MODIFIED))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_SAVE_INT64_DATE, "modified")
        (field_search::COMMAND_WARNING, "field missing")

        // /snap/page/body/updated
        // XXX should it be mandatory or just use "created" as the default?
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_UPDATED))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_SAVE_INT64_DATE, "updated")
        (field_search::COMMAND_WARNING, "field missing")

        // /snap/page/body/accepted
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_ACCEPTED))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_SAVE_INT64_DATE, "accepted")

        // /snap/page/body/submitted
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_SUBMITTED))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_SAVE_INT64_DATE, "submitted")

        // /snap/page/body/since
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_SINCE))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_SAVE_INT64_DATE, "since")

        // /snap/page/body/until
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_UNTIL))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_SAVE_INT64_DATE, "until")

        // /snap/page/body/copyrighted
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_COPYRIGHTED))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_SAVE_INT64_DATE, "copyrighted")

        // /snap/page/body/issued
        (field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_CONTENT_ISSUED))
        (field_search::COMMAND_SELF)
        (field_search::COMMAND_SAVE_INT64_DATE, "issued")

        // /snap/page/body/navigation/link[@rel="top"][@title="Index"][@href="<site key>"]
        // /snap/page/body/navigation/link[@rel="up"][@title="Up"][@href="<path/..>"]
        (field_search::COMMAND_DEFAULT_VALUE_OR_NULL, cpath)
        (field_search::COMMAND_IF_NOT_FOUND, 1)
            //(field_search::COMMAND_RESET) -- uncomment if we go on with other things
            (field_search::COMMAND_CHILD_ELEMENT, "navigation")

            // Index
            (field_search::COMMAND_CHILD_ELEMENT, "link")
            (field_search::COMMAND_ELEMENT_ATTR, "rel=top")
            (field_search::COMMAND_ELEMENT_ATTR, "title=Index") // TODO: translate
            (field_search::COMMAND_ELEMENT_ATTR, "href=" + f_snap->get_site_key())
            (field_search::COMMAND_PARENT_ELEMENT)

            // Up
            (field_search::COMMAND_CHILD_ELEMENT, "link")
            (field_search::COMMAND_ELEMENT_ATTR, "rel=up")
            (field_search::COMMAND_ELEMENT_ATTR, "title=Up") // TODO: translate
            (field_search::COMMAND_ELEMENT_ATTR, "href=" + up)
            //(field_search::COMMAND_PARENT_ELEMENT) -- uncomment if we go on with other things

            //(field_search::COMMAND_PARENT_ELEMENT) -- uncomment if we go on with other things
        (field_search::COMMAND_LABEL, 1)

        // generate!
        ;

//QDomDocument doc(page.ownerDocument());
//printf("content XML [%s]\n", doc.toString().toUtf8().data());
}
#pragma GCC diagnostic pop


/** \brief Retreive a content page parameter.
 *
 * This function reads a column from the content of the page using the
 * content key as defined by the canonalization process. The function
 * cannot be called before the content::on_path_execute() function is
 * called and the key properly initialized.
 *
 * The table is opened once and remains opened so calling this function
 * many times is not a problem. Also the libQtCassandra library caches
 * all the data. Reading the same field multiple times is not a concern
 * at all.
 *
 * If the value is undefined, the result is a null value.
 *
 * \note
 * The path should be canonicalized before the call although we call
 * the remove_slashes() function on it cleanup starting and ending
 * slashes (because the URI object returns paths such as "/login" and
 * the get_content_parameter() requires just "login" to work right.)
 *
 * \param[in] path  The canonicalized path being managed.
 * \param[in] param_name  The name of the parameter to retrieve.
 *
 * \return The content of the row as a Cassandra value.
 */
QtCassandra::QCassandraValue content::get_content_parameter(QString path, const QString& param_name)
{
    f_snap->canonicalize_path(path);
    // "" represents the home page
    //if(path.isEmpty())
    //{
    //    // an empty value is considered to be a null value
    //    QtCassandra::QCassandraValue value;
    //    return value;
    //}

    QSharedPointer<QtCassandra::QCassandraTable> content_table(get_content_table());

    QString key(f_snap->get_site_key_with_slash() + path);
//printf("get content for [%s] . [%s]\n", key.toUtf8().data(), param_name.toUtf8().data());
    if(!content_table->exists(key))
    {
        // an empty value is considered to be a null value
        QtCassandra::QCassandraValue value;
        return value;
    }
    if(!content_table->row(key)->exists(param_name))
    {
      // an empty value is considered to be a null value
      QtCassandra::QCassandraValue value;
      return value;
    }

    return content_table->row(key)->cell(param_name)->value();
}

/** \brief Prepare a set of content to add to the database.
 *
 * In most cases, plugins call this function in one of their do_update()
 * functions to add their content.xml file to the database.
 *
 * This function expects a plugin name as input to add the
 * corresponding content.xml file of that plugin. The data is search in
 * the resources (it is expected to be added there by the plugin).
 * The resource path is built as follow:
 *
 * \code
 * ":/plugins/" + plugin_name + "/content.xml"
 * \endcode
 *
 * The content is not immediately added to the database because
 * of dependency issues. At the time all the content is added
 * using this function, the order in which it is added is not
 * generally proper (i.e. the taxonomy "/types" may be
 * added after the content "/types/taxonomy/system/content-types"
 * which would then fail.)
 *
 * The content plugin saves this data when it receives the
 * save_content signal.
 *
 * To dynamically add content (opposed to adding information
 * from an XML file) you want to call the add_param() and
 * add_link() functions as required.
 *
 * \param[in] plugin_name  The name of the plugin loading this data.
 *
 * \sa on_save_content()
 * \sa add_param()
 * \sa add_link()
 */
void content::add_xml(const QString& plugin_name)
{
    if(!plugins::verify_plugin_name(plugin_name))
    {
        // invalid plugin name
        throw content_exception_invalid_content_xml("add_xml() called with an invalid plugin name: \"" + plugin_name + "\"");
    }
    QString const filename(":/plugins/" + plugin_name + "/content.xml");
    QFile xml_content(filename);
    if(!xml_content.open(QFile::ReadOnly))
    {
        // file not found
        throw content_exception_invalid_content_xml("add_xml() cannot open file: \"" + filename + "\"");
    }
    QDomDocument dom;
    if(!dom.setContent(&xml_content, false))
    {
        // invalid XML
        throw content_exception_invalid_content_xml("add_xml() cannot read the XML of content file: \"" + filename + "\"");
    }
    QDomNodeList content_nodes(dom.elementsByTagName("content"));
    int const max(content_nodes.size());
    for(int i(0); i < max; ++i)
    {
        QDomNode content_node(content_nodes.at(i));
        if(!content_node.isElement())
        {
            // we're only interested in elements
            continue;
        }
        QDomElement content_element(content_node.toElement());
        if(content_element.isNull())
        {
            // somehow this is not an element
            continue;
        }

        QString owner(content_element.attribute("owner"));
        if(owner.isEmpty())
        {
            owner = plugin_name;
        }

        QString path(content_element.attribute("path"));
        if(path.isEmpty())
        {
            throw content_exception_invalid_content_xml("all <content> tags supplied to add_xml() must include a valid \"path\" attribute");
        }
        f_snap->canonicalize_path(path);
        QString const key(f_snap->get_site_key_with_slash() + path);

        // create a new entry for the database
        add_content(key, owner);

        QDomNodeList children(content_element.childNodes());
        bool found_content_type(false);
        int const cmax(children.size());
        for(int c(0); c < cmax; ++c)
        {
            // grab <param> and <link> tags
            QDomNode child(children.at(c));
            if(!child.isElement())
            {
                // we're only interested by elements
                continue;
            }
            QDomElement element(child.toElement());
            if(element.isNull())
            {
                // somehow this is not really an element
                continue;
            }

            // <param name=... overwrite=... force-namespace=...> data </param>
            if(element.tagName() == "param")
            {
                QString param_name(element.attribute("name"));
                if(param_name.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <param> tags supplied to add_xml() must include a valid \"name\" attribute");
                }

                // 1) prepare the buffer
                // the parameter value can include HTML (should be in a [CDATA[...]] in that case)
                QString buffer;
                QTextStream data(&buffer);
                // we have to save all the element children because
                // saving the element itself would save the <param ...> tag
                // also if the whole is a <![CDATA[...]]> entry, remove it
                // (but keep sub-<![CDATA[...]]> if any.)
                QDomNodeList values(element.childNodes());
                int lmax(values.size());
                if(lmax == 1)
                {
                    QDomNode n(values.at(0));
                    if(n.isCDATASection())
                    {
                        QDomCDATASection raw_data(n.toCDATASection());
                        data << raw_data.data();
                    }
                    else
                    {
                        // not a CDATA section, save as is
                        n.save(data, 0);
                    }
                }
                else
                {
                    // save all the children
                    for(int l(0); l < lmax; ++l)
                    {
                        values.at(l).save(data, 0);
                    }
                }

                // 2) prepare the name
                QString fullname;
                // It seems to me that if the developer included any namespace
                // then it was meant to be defined that way
                //if(param_name.left(plugin_name.length() + 2) == plugin_name + "::")
                if(param_name.contains("::")) // already includes a namespace
                {
                    // plugin namespace already defined
                    fullname = param_name;
                }
                else
                {
                    // plugin namespace not defined
                    if(element.attribute("force-namespace") == "no")
                    {
                        // but developer said no namespace needed (?!)
                        fullname = param_name;
                    }
                    else
                    {
                        // this is the default!
                        fullname = plugin_name + "::" + param_name;
                    }
                }

                // add the resulting parameter
                add_param(key, fullname, buffer);

                // check whether we allow overwrites
                if(element.attribute("overwrite") == "yes")
                {
                    set_param_overwrite(key, fullname, true);
                }

                // check whether a data type was defined
                QString type(element.attribute("type"));
                if(!type.isEmpty())
                {
                    param_type_t param_type;
                    if(type == "string")
                    {
                        param_type = PARAM_TYPE_STRING;
                    }
                    else if(type == "float")
                    {
                        param_type = PARAM_TYPE_FLOAT;
                    }
                    else if(type == "int8")
                    {
                        param_type = PARAM_TYPE_INT8;
                    }
                    else if(type == "int64")
                    {
                        param_type = PARAM_TYPE_INT64;
                    }
                    else
                    {
                        throw content_exception_invalid_content_xml(QString("unknown type in <param type=\"%1\"> tags").arg(type));
                    }
                    set_param_type(key, fullname, param_type);
                }
            }
            // <link name=... to=... [mode="1/*:1/*"]> destination path </link>
            else if(element.tagName() == "link")
            {
                QString link_name(element.attribute("name"));
                if(link_name.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <link> tags supplied to add_xml() must include a valid \"name\" attribute");
                }
                if(link_name == plugin_name)
                {
                    throw content_exception_invalid_content_xml("the \"name\" attribute of a <link> tags cannot be set to the plugin name (" + plugin_name + ")");
                }
                if(!link_name.contains("::"))
                {
                    // force the owner in the link name
                    link_name = plugin_name + "::" + link_name;
                }
                if(link_name == "content::page_type")
                {
                    found_content_type = true;
                }
                QString link_to(element.attribute("to"));
                if(link_to.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <link> tags supplied to add_xml() must include a valid \"to\" attribute");
                }
                if(link_to == plugin_name)
                {
                    throw content_exception_invalid_content_xml("the \"to\" attribute of a <link> tags cannot be set to the plugin name (" + plugin_name + ")");
                }
                if(!link_to.contains("::"))
                {
                    // force the owner in the link name
                    link_to = plugin_name + "::" + link_to;
                }
                bool source_unique(true);
                bool destination_unique(true);
                QString mode(element.attribute("mode"));
                if(!mode.isEmpty() && mode != "1:1")
                {
                    if(mode == "1:*")
                    {
                        destination_unique = false;
                    }
                    else if(mode == "*:1")
                    {
                        source_unique = false;
                    }
                    else if(mode == "*:*")
                    {
                        destination_unique = false;
                        source_unique = false;
                    }
                    else
                    {
                        throw content_exception_invalid_content_xml("<link> tags mode attribute must be one of \"1:1\", \"1:*\", \"*:1\", or \"*:*\"");
                    }
                }
                // the destination URL is defined in the <link> content
                QString destination_path(element.text());
                f_snap->canonicalize_path(destination_path);
                QString destination_key(f_snap->get_site_key_with_slash() + destination_path);
                links::link_info source(link_name, source_unique, key);
                links::link_info destination(link_to, destination_unique, destination_key);
                add_link(key, source, destination);
            }
        }
        if(!found_content_type)
        {
            QString const link_name("content::page_type");
            QString const link_to("content::page_page");
            bool const source_unique(true);
            bool const destination_unique(false);
            QString const destination_path(path.left(6) == "admin/"
                    ? "types/taxonomy/system/content-types/administration-page"
                    : "types/taxonomy/system/content-types/system-page");
            QString const destination_key(f_snap->get_site_key_with_slash() + destination_path);
            links::link_info source(link_name, source_unique, key);
            links::link_info destination(link_to, destination_unique, destination_key);
            add_link(key, source, destination);
        }
    }
}


/** \brief Prepare to add content to the database.
 *
 * This function creates a new block of data to be added to the database.
 * Each time one wants to add content to the database, one must call
 * this function first. At this time the plugin_owner cannot be changed.
 * If that happens (i.e. two plugins trying to create the same piece of
 * content) then the system raises an exception.
 *
 * \exception content_exception_content_already_defined
 * This exception is raised if the block already exists and the owner
 * of the existing block doesn't match the \p plugin_owner parameter.
 *
 * \param[in] path  The path of the content being added.
 * \param[in] plugin_owner  The name of the plugin managing this content.
 */
void content::add_content(const QString& path, const QString& plugin_owner)
{
    if(!plugins::verify_plugin_name(plugin_owner))
    {
        // invalid plugin name
        throw content_exception_invalid_name("install_content() called with an invalid plugin name: \"" + plugin_owner + "\"");
    }

    content_block_map_t::iterator b(f_blocks.find(path));
    if(b != f_blocks.end())
    {
        if(b->f_owner != plugin_owner)
        {
            // cannot change owner!?
            throw content_exception_content_already_defined("adding block \"" + path + "\" with owner \"" + b->f_owner + "\" cannot be changed to \"" + plugin_owner + "\"");
        }
        // it already exists, we're all good
    }
    else
    {
        // create the new block
        content_block block;
        block.f_path = path;
        block.f_owner = plugin_owner;
        f_blocks.insert(path, block);
    }

    f_snap->new_content();
}


/** \brief Add a parameter to the content to be saved in the database.
 *
 * This function is used to add a parameter to the database.
 * A parameter is composed of a name and a block of data that may be of any
 * type (HTML, XML, picture, etc.)
 *
 * Other parameters can be attached to parameters using set_param_...()
 * functions, however, the add_param() function must be called first to
 * create the parameter.
 *
 * Note that the data added in this way is NOT saved in the database until
 * the save_content signal is sent.
 *
 * \warning
 * This function does NOT save the data immediately (if called after the
 * update, then it is saved after the execute() call returns!) Instead
 * the function prepares the data so it can be saved later. This is useful
 * if you expect many changes and dependencies may not all be available at
 * the time you add the content but will be at a later time. If you already
 * have all the data, you may otherwise directly call the Cassandra function
 * to add the data to the content table.
 *
 * \bug
 * At this time the data of a parameter is silently overwritten if this
 * function is called multiple times with the same path and name.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised when this funtion is called before the
 * add_content() is called (i.e. the block of data referenced by
 * \p path is not defined yet.)
 *
 * \param[in] path  The path of this parameter (i.e. /types/taxonomy)
 * \param[in] name  The name of this parameter (i.e. "Website Taxonomy")
 * \param[in] data  The data of this parameter.
 *
 * \sa add_param()
 * \sa add_link()
 * \sa on_save_content()
 */
void content::add_param(const QString& path, const QString& name, const QString& data)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" was found");
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        content_param param;
        param.f_name = name;
        param.f_data = data;
        b->f_params.insert(name, param);
    }
    else
    {
        // replace the data
        // TBD: should we generate an error because if defined by several
        //      different plugins then we cannot ensure which one is going
        //      to make it to the database! At the same time, we cannot
        //      know whether we're overwriting a default value.
        p->f_data = data;
    }
}


/** \brief Set the overwrite flag to a specific parameter.
 *
 * The parameter must first be added with the add_param() function.
 * By default this is set to false as defined in the DTD of the
 * content XML format. This means if the attribute is not defined
 * then there is no need to call this function.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised if the path or the name parameters do
 * not match any block or parameter in that block.
 *
 * \param[in] path  The path of this parameter.
 * \param[in] name  The name of the parameter to modify.
 * \param[in] overwrite  The new value of the overwrite flag.
 *
 * \sa add_param()
 */
void content::set_param_overwrite(const QString& path, const QString& name, bool overwrite)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        throw content_exception_parameter_not_defined("no param with name \"" + path + "\" found in block \"" + path + "\"");
    }

    p->f_overwrite = overwrite;
}


/** \brief Set the type to a specific value.
 *
 * The parameter must first be added with the add_param() function.
 * By default the type of a parameter is "string". However, some
 * parameters are integers and this function can be used to specify
 * such. Note that it is important to understand that if you change
 * the type in the content.xml then when reading the data you'll have
 * to use the correct type.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised if the path or the name parameters do
 * not match any block or parameter in that block.
 *
 * \param[in] path  The path of this parameter.
 * \param[in] name  The name of the parameter to modify.
 * \param[in] type  The new type for this parameter.
 *
 * \sa add_param()
 */
void content::set_param_type(const QString& path, const QString& name, param_type_t param_type)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        throw content_exception_parameter_not_defined("no param with name \"" + path + "\" found in block \"" + path + "\"");
    }

    p->f_type = static_cast<int>(param_type); // XXX fix cast
}


/** \brief Add a link to the specified content.
 *
 * This function links the specified content (defined by path) to the
 * specified destination.
 *
 * The source parameter defines the name of the link, the path (has to
 * be the same as path) and whether the link is unique.
 *
 * The path must already represent a block as defined by the add_content()
 * function call otherwise the function raises an exception.
 *
 * Note that the link is not searched. If it is already defined in
 * the array of links, it will simply be written twice to the
 * database.
 *
 * \warning
 * This function does NOT save the data immediately (if called after the
 * update, then it is saved after the execute() call returns!) Instead
 * the function prepares the data so it can be saved later. This is useful
 * if you expect many changes and dependencies may not all be available at
 * the time you add the content but will be at a later time. If you already
 * have all the data, you may otherwise directly call the
 * links::create_link() function.
 *
 * \exception content_exception_parameter_not_defined
 * The add_content() function was not called prior to this call.
 *
 * \param[in] path  The path where the link is added (source URI, site key + path.)
 * \param[in] source  The link definition of the source.
 * \param[in] destination  The link definition of the destination.
 *
 * \sa add_content()
 * \sa add_xml()
 * \sa add_param()
 * \sa create_link()
 */
void content::add_link(const QString& path, const links::link_info& source, const links::link_info& destination)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    content_link link;
    link.f_source = source;
    link.f_destination = destination;
    b->f_links.push_back(link);
}


/** \brief Signal received when the system request that we save content.
 *
 * This function is called by the snap_child() after the update if any one of
 * the plugins requested content to be saved to the database (in most cases
 * from their content.xml file, although it could be created dynamically.)
 *
 * It may be called again after the execute() if anything more was saved
 * while processing the page.
 */
void content::on_save_content()
{
    // anything to save?
    if(f_blocks.isEmpty())
    {
        return;
    }

    QString const site_key(f_snap->get_site_key_with_slash());
    QSharedPointer<QtCassandra::QCassandraTable> content_table(get_content_table());
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        // now do the actual save
        // connect this entry to the corresponding plugin
        // (unless that field is already defined!)
        QString primary_owner(path::get_name(path::SNAP_NAME_PATH_PRIMARY_OWNER));
        if(content_table->row(d->f_path)->cell(primary_owner)->value().nullValue())
        {
            content_table->row(d->f_path)->cell(primary_owner)->setValue(d->f_owner);
        }
        // if != then another plugin took ownership which is fine...
        //else if(content_table->row(d->f_path)->cell(primary_owner)->value().stringValue() != d->f_owner) {
        //}

        // make sure we have our different basic content dates setup
        uint64_t start_date(f_snap->get_uri().option("start_date").toLongLong());
        if(content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_CREATED)))->value().nullValue())
        {
            // do not overwrite the created date
            content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_CREATED)))->setValue(start_date);
        }
        if(content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_UPDATED)))->value().nullValue())
        {
            // updated changes only because of a user action (i.e. Save)
            content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_UPDATED)))->setValue(start_date);
        }
        // always overwrite the modified date
        content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_MODIFIED)))->setValue(start_date);

        // save the parameters (i.e. cells of data defined by the developer)
        for(content_params_t::iterator p(d->f_params.begin());
                p != d->f_params.end(); ++p)
        {
            // make sure no parameter is defined as path::primary_owner
            // because we are 100% in control of that one!
            // (we may want to add more as time passes)
            if(p->f_name == primary_owner)
            {
                throw content_exception_invalid_content_xml("content::on_save_content() cannot accept a parameter named \"path::primary_owner\" as it is reserved");
            }

            // we just saved the path::primary_owner so the row exists now
            //if(content_table->exists(d->f_block.f_path)) ...

            // unless the developer said to overwrite the data, skip
            // the save if the data alerady exists
            if(p->f_overwrite
            || content_table->row(d->f_path)->cell(p->f_name)->value().nullValue())
            {
                bool ok(true);
                switch(p->f_type)
                {
                case PARAM_TYPE_STRING:
                    content_table->row(d->f_path)->cell(p->f_name)->setValue(p->f_data);
                    break;

                case PARAM_TYPE_FLOAT:
                    {
                    float const v(p->f_data.toFloat(&ok));
                    content_table->row(d->f_path)->cell(p->f_name)->setValue(v);
                    }
                    break;

                case PARAM_TYPE_INT8:
                    {
                    int const v(p->f_data.toInt(&ok));
                    ok = ok && v >= -128 && v <= 127; // verify overflows
                    content_table->row(d->f_path)->cell(p->f_name)->setValue(static_cast<signed char>(v));
                    }
                    break;

                case PARAM_TYPE_INT64:
                    content_table->row(d->f_path)->cell(p->f_name)->setValue(static_cast<int64_t>(p->f_data.toLongLong(&ok)));
                    break;

                }
                if(!ok)
                {
                    throw content_exception_invalid_content_xml(QString("content::on_save_content() tried to convert %1 to a number and failed.").arg(p->f_data));
                }
            }
        }

        // link this entry to its parent automatically
        // first we need to remove the site key from the path
        QString path(d->f_path.mid(site_key.length()));
        QStringList parts(path.split('/', QString::SkipEmptyParts));
        while(parts.count() > 0)
        {
            QString src(parts.join("/"));
            src = site_key + src;
            parts.pop_back();
            QString dst(parts.join("/"));
            dst = site_key + dst;
            links::link_info source(get_name(SNAP_NAME_CONTENT_PARENT), true, src);
            links::link_info destination(get_name(SNAP_NAME_CONTENT_CHILDREN), false, dst);
// TODO only repeat if the parent did not exist, otherwise we assume the
//      parent created its own parent/children link already.
//printf("parent/children [%s]/[%s]\n", src.toUtf8().data(), dst.toUtf8().data());
            links::links::instance()->create_link(source, destination);
        }
    }

    // link the nodes together (on top of the parent/child links)
    // this is done as a second step so we're sure that all the source and
    // destination rows exist at the time we create the links
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        for(content_links_t::iterator l(d->f_links.begin());
                l != d->f_links.end(); ++l)
        {
//printf("developer link: [%s]/[%s]\n", l->f_source.key().toUtf8().data(), l->f_destination.key().toUtf8().data());
            links::links::instance()->create_link(l->f_source, l->f_destination);
        }
    }

    // allow other plugins to add their own stuff dynamically
    // (note that this is working only comme-ci comme-ca since all
    // the other plugins should anyway have workable defaults; however,
    // once in a while, defaults are not enough; for example the shorturl
    // needs to generate a shorturl, there is real default other than:
    // that page has no shorturl.)
    f_updating = true;
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        QtCassandra::QCassandraValue type(get_content_parameter(d->f_path, get_name(SNAP_NAME_CONTENT_PAGE_TYPE)));
        QString path(d->f_path);
        if(path.startsWith(site_key))
        {
            path = path.mid(site_key.length());
            create_content(path, d->f_owner, type.stringValue());
        }
        // else -- if the path doesn't start with site_key we've got a problem
    }
    f_updating = false;

    // we're done with that set of data
    f_blocks.clear();
}


int content::js_property_count() const
{
    return 1;
}


QVariant content::js_property_get(const QString& name) const
{
    if(name == "modified")
    {
        return "content::modified";
    }
    return QVariant();
}


QString content::js_property_name(int index) const
{
	if(index == 0)
	{
		return "modified";
	}
	return "";
}


QVariant content::js_property_get(int index) const
{
    if(index == 0)
    {
        return "content::modified";
    }
    return QVariant();
}



/** \brief Process new attachments.
 *
 * As user upload new files to the server, we want to have them
 * processed in different ways. This backend process does part of
 * that work and allows other plugins to check files out to make
 * sure they are fine.
 *
 * Type of processes we are expecting to run against files:
 *
 * \li The Anti-Virus plugin checks that the file is not viewed as a
 *     virus using external tools such as clamscan. This is expected
 *     to be checked within the check_attachment_security() signal.
 *
 * \li The JavaScript plugin checks the syntax of all JavaScript files.
 *     It also minimizes them and save that minimized version.
 *
 * \li The Layout plugin checks the syntax of all the CSS files and
 *     it also minimizes them and save that minimized version.
 *
 * \li The layout plugin tries to fully load all Images, play movies,
 *     etc. to make sure that the files are valid. If that process
 *     fails, then the file is marked as invalid.
 *
 * When serving a file that is an attachment, plugins that own those
 * files are given a chance to server the attachment themselves. If
 * they do, then the default code doesn't get used at all. This allows
 * plugins such as the JavaScript plugin to send their compressed and
 * minimized version of the file instead of the source version.
 *
 * \important
 * This function generates two signals: check_attachment_security()
 * and process_attachment(). If your plugin can check the file for
 * security reason, implement the check_attachment_security(). In
 * all other cases, use the process_attachment(). It is important to
 * do that work in the right function because attempting to load a
 * virus or some other bad file make cause havoc on the server.
 *
 * \todo
 * The security checks may need to be re-run on all the files once
 * in a while since brand new viruses may not be detected when they
 * first get uploaded. Once signal on that one could be to count the
 * number of time a file gets uploaded, if the counter increases
 * outregiously fast, it's probably not a good sign.
 */
void content::on_backend_process()
{
    QSharedPointer<QtCassandra::QCassandraTable> files_table(get_files_table());
    QSharedPointer<QtCassandra::QCassandraRow> new_row(files_table->row(get_name(SNAP_NAME_CONTENT_FILES_NEW)));
    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setCount(100); // should this be a parameter?
    column_predicate.setIndex(); // behave like an index
    for(;;)
    {
        new_row->clearCache();
        new_row->readCells(column_predicate);
        const QtCassandra::QCassandraCells& new_cells(new_row->cells());
        if(new_cells.isEmpty())
        {
            break;
        }
        // handle one batch
        for(QtCassandra::QCassandraCells::const_iterator nc(new_cells.begin());
                nc != new_cells.end();
                ++nc)
        {
            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            QSharedPointer<QtCassandra::QCassandraCell> new_cell(*nc);
            if(!new_cell->value().nullValue())
            {
                QByteArray file_key(new_cell->columnKey());

                QSharedPointer<QtCassandra::QCassandraRow> file_row(files_table->row(file_key));
                QtCassandra::QCassandraColumnRangePredicate reference_column_predicate;
                reference_column_predicate.setStartColumnName(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE));
                reference_column_predicate.setEndColumnName(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE) + QString(";"));
                reference_column_predicate.setCount(100);
                reference_column_predicate.setIndex(); // behave like an index
                bool first(true); // load the image only once for now
                secure_flag secure;
                for(;;)
                {
                    file_row->clearCache();
                    file_row->readCells(reference_column_predicate);
                    const QtCassandra::QCassandraCells& content_cells(file_row->cells());
                    if(content_cells.isEmpty())
                    {
                        break;
                    }
                    // handle one batch
                    for(QtCassandra::QCassandraCells::const_iterator cc(content_cells.begin());
                            cc != content_cells.end();
                            ++cc)
                    {
                        // get the email from the database
                        // we expect empty values once in a while because a dropCell() is
                        // not exactly instantaneous in Cassandra
                        QSharedPointer<QtCassandra::QCassandraCell> content_cell(*cc);
                        if(!content_cell->value().nullValue())
                        {
                            QByteArray attachment_key(content_cell->columnKey().data() + (strlen(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE)) + 2),
                                                      content_cell->columnKey().size() - (strlen(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE)) + 2));

                            if(first)
                            {
                                first = false;

                                attachment_file file(f_snap);
                                if(!load_attachment(attachment_key, file, true))
                                {
                                    signed char const sflag(CONTENT_SECURE_UNDEFINED);
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE))->setValue(sflag);
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK))->setValue(f_snap->get_start_date());
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURITY_REASON))->setValue(QString("Attachment could not be loaded."));

                                    // TODO generate an email about the error...
                                }
                                else
                                {
                                    check_attachment_security(file, secure);

                                    // always save the secure flag
                                    signed char const sflag(secure.secure() ? CONTENT_SECURE_SECURE : CONTENT_SECURE_UNSECURE);
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE))->setValue(sflag);
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK))->setValue(f_snap->get_start_date());
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURITY_REASON))->setValue(secure.reason());

                                    if(secure.secure())
                                    {
                                        // only process the attachment further if it is
                                        // considered secure
                                        process_attachment(file_key, file);
                                    }
                                }
                            }
                            if(!secure.secure())
                            {
                                // TODO: warning the author that his file was
                                //       quanranteened and will not be served
                                //...sendmail()...
                            }
                        }
                    }
                }
            }
            // we're done with that file, remove it from the list of new files
            new_row->dropCell(new_cell->columnKey());
        }
    }
}


/** \brief Check whether the attachment is considered secure.
 *
 * Before processing an attachment further we want to know whether it is
 * secure. This event allows different plugins to check the security of
 * each file.
 *
 * Once a process decides that a file is not secure, the secure flag is
 * false and it cannot be reset back to true.
 *
 * \param[in] file  The file being processed.
 * \param[in,out] secure  Whether the file is secure.
 */
bool content::check_attachment_security_impl(attachment_file const& file, secure_flag& secure)
{
    return true;
}


/** \brief Check the attachment for one thing or another.
 *
 * The startup function generates a compressed version of the file using
 * gzip as the compression mode.
 *
 * \param[in] key  The key of the file in the files table.
 * \param[in] file  The file being processed.
 */
bool content::process_attachment_impl(QByteArray const& file_key, attachment_file const& file)
{
    QSharedPointer<QtCassandra::QCassandraTable> files_table(get_files_table());
    QSharedPointer<QtCassandra::QCassandraRow> file_row(files_table->row(file_key));
    if(!file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_DATA_COMPRESSED)))
    {
        QString compressor_name("gzip");
        QByteArray compressed_file(compression::compress(compressor_name, file.get_file().get_data(), 100, false));
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_DATA_COMPRESSED))->setValue(compressed_file);
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SIZE_COMPRESSED))->setValue(compressed_file.size());
    }

    // TODO: actually the JS plugin cannot save in the files table
    //       unless we pass files_table to it; so we'll have to update
    //       this call for the plugin!
    //
    // The JavaScript plugin does not know about us (content plugin),
    // however, we know about it so we can ask it to do this job
    // by calling it directly
    javascript::javascript::instance()->on_process_attachment(files_table, file_key, file.get_file());

    return true;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
