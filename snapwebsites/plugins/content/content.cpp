// Snap Websites Server -- all the user content and much of the system content
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

#include "content.h"
#include "plugins.h"
#include "log.h"
#include "not_reached.h"
#include "dom_util.h"
#include <iostream>
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

    case SNAP_NAME_CONTENT_BODY:
        return "content::body";

    case SNAP_NAME_CONTENT_CHILDREN:
        return "content::children";

    case SNAP_NAME_CONTENT_CONTENT_TYPES:
        return "Content Types";

    case SNAP_NAME_CONTENT_CONTENT_TYPES_NAME:
        return "content_types";

    case SNAP_NAME_CONTENT_COPYRIGHTED:
        return "content::copyrighted";

    case SNAP_NAME_CONTENT_CREATED:
        return "content::created";

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
    SNAP_PLUGIN_UPDATE(2013, 12, 7, 16, 18, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the content plugin.
 *
 * This function is the first update for the content plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void content::initial_update(int64_t variables_timestamp)
{
    get_content_table();
}
#pragma GCC diagnostic pop


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
void content::content_update(int64_t variables_timestamp)
{
    add_xml(get_plugin_name());
}


/** \brief Initialize the content table.
 *
 * This function creates the content table if it doesn't exist yet. Otherwise
 * it simple initializes the f_content_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * \return The pointer to the content table.
 */
QSharedPointer<QtCassandra::QCassandraTable> content::get_content_table()
{
    return f_snap->create_table(get_name(SNAP_NAME_CONTENT_TABLE), "Website content table.");
}
#pragma GCC diagnostic pop


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

    if(content_table->exists(key))
    {
        // the row already exists, this is considered created.
        // (we may later want to have a repair_content signal
        // which we could run as an action from the backend...)
        // however, if it were created by an add_xml() call,
        // then the on_create_content() of all the other plugins
        // should probably be called (i.e. f_updating is true then)
        return f_updating;
    }
    QSharedPointer<QtCassandra::QCassandraRow> row(content_table->row(key));

    // save the owner
    QString const primary_owner(path::get_name(path::SNAP_NAME_PATH_PRIMARY_OWNER));
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
QString content::js_property_name(int index) const
{
    return "modified";
}
#pragma GCC diagnostic pop

QVariant content::js_property_get(int index) const
{
    if(index == 0)
    {
        return "content::modified";
    }
    return QVariant();
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
