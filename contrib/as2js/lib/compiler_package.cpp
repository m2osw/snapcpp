/* compiler_package.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

/*

Copyright (c) 2005-2017 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include    "as2js/compiler.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"
#include    "as2js/parser.h"

// private classes
#include    "db.h"
#include    "rc.h"

#include    <algorithm>

#include    <dirent.h>
#include    <cstring>


namespace as2js
{


namespace
{


// The following globals are read only once and you can compile
// many times without having to reload them.
//
// the resource file information
//
rc_t                        g_rc;

// the global imports (those which are automatic and
// define the intrinsic functions and types of the language)
//
Node::pointer_t             g_global_import;

// the system imports (this is specific to the system you
// are using this compiler for; it defines the system)
//
Node::pointer_t             g_system_import;

// the native imports (this is specific to your system
// environment, it defines objects in your environment)
//
Node::pointer_t             g_native_import;

// the database handling all the packages and their name
// so we can quickly find which package to import when
// a given name is used
//
Database::pointer_t         g_db;

// whether the database was loaded (true) or not (false)
//
bool                        g_db_loaded = false;

// Search for a named element:
// <package name>{.<package name>}.<class, function, variable name>
// TODO: add support for '*' in <package name>
Database::Element::pointer_t find_element(String const& package_name, String const& element_name, char const *type)
{
    Database::package_vector_t packages(g_db->find_packages(package_name));
    for(auto pt(packages.begin()); pt != packages.end(); ++pt)
    {
        Database::element_vector_t elements((*pt)->find_elements(element_name));
        for(auto et(elements.begin()); et != elements.end(); ++et)
        {
            if(!type
            || (*et)->get_type() == type)
            {
                return *et;
            }
        }
    }

    return Database::Element::pointer_t();
}


void add_element(String const& package_name, String const& element_name, Node::pointer_t element, char const *type)
{
    Database::Package::pointer_t p(g_db->add_package(package_name));
    Database::Element::pointer_t e(p->add_element(element_name));
    e->set_type(type);
    e->set_filename(element->get_position().get_filename());
    e->set_line(element->get_position().get_line());
}


}
// no name namespace







/** \brief Get the filename of a package.
 *
 */
String Compiler::get_package_filename(char const *package_info)
{
    for(int cnt(0); *package_info != '\0';)
    {
        ++package_info;
        if(package_info[-1] == ' ')
        {
            ++cnt;
            if(cnt >= 3)
            {
                break;
            }
        }
    }
    if(*package_info != '"')
    {
        return "";
    }
    ++package_info;
    char const *name = package_info;
    while(*package_info != '"' && *package_info != '\0')
    {
        ++package_info;
    }

    String result;
    result.from_utf8(name, package_info - name);

    return result;
}


/** \brief Find a module, load it if necessary.
 *
 * If the module was already loaded, return a pointer to the existing
 * tree of nodes.
 *
 * If the module was not yet loaded, try to load it. If the file cannot
 * be found or the file cannot be compiled, a fatal error is emitted
 * and the process stops.
 *
 * \note
 * At this point this function either returns true because it found
 * the named module, or it throws exception_exit. So the
 * \p result parameter is always set on a normal return.
 *
 * \param[in] filename  The name of the module to be loaded
 * \param[in,out] result  The shared pointer to the resulting root node.
 *
 * \return true if the module was found.
 */
bool Compiler::find_module(String const& filename, Node::pointer_t& result)
{
    // module already loaded?
    module_map_t::const_iterator existing_module(f_modules.find(filename));
    if(existing_module != f_modules.end())
    {
        result = existing_module->second;
        return true;
    }

    // we could not find this module, try to load it
    Input::pointer_t in;
    if(f_input_retriever)
    {
        in = f_input_retriever->retrieve(filename);
    }
    if(!in)
    {
        in.reset(new FileInput());
        // 'in' is just a 'class Input' so we need to cast
        if(!static_cast<FileInput&>(*in).open(filename))
        {
            Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_NOT_FOUND, in->get_position());
            msg << "cannot open module file \"" << filename << "\".";
            throw exception_exit(1, "cannot open module file");
        }
    }

    // Parse the file in result
    Parser::pointer_t parser(new Parser(in, f_options));
    result = parser->parse();
    parser.reset();

#if 0
//std::cerr << "+++++\n \"" << filename << "\" module:\n" << *result << "\n+++++\n";
std::cerr << "+++++++++++++++++++++++++++ Loading \"" << filename << "\" +++++++++++++++++++++++++++++++\n";
#endif

    if(!result)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_CANNOT_COMPILE, in->get_position());
        msg << "could not compile module file \"" << filename << "\".";
        throw exception_exit(1, "could not compile module file");
    }

    // save the newly loaded module
    f_modules[filename] = result;

    return true;
}



/** \brief Load a module as specified by \p module and \p file.
 *
 * This function loads the specified module. The filename is defined
 * as the path found in the .rc file, followed by the module name,
 * followed by the file name:
 *
 * \code
 * <rc.path>/<module>/<file>
 * \endcode
 *
 * The function always returns a pointer. If the module cannot be
 * loaded, an error is generated and the compiler exists with a
 * fatal error.
 *
 * \param[in] module  The name of the module to be loaded.
 * \param[in] file  The name of the file to load from that module.
 *
 * \return The pointer to the module loaded.
 */
Node::pointer_t Compiler::load_module(char const *module, char const *file)
{
    // create the path to the module
    String path(g_rc.get_scripts());
    path += "/";
    path += module;
    path += "/";
    path += file;

    Node::pointer_t result;
    find_module(path, result);
    return result;
}




void Compiler::find_packages_add_database_entry(String const & package_name, Node::pointer_t & element, char const * type)
{
    // here, we totally ignore internal, private
    // and false entries right away
    if(get_attribute(element, Node::attribute_t::NODE_ATTR_PRIVATE)
    || get_attribute(element, Node::attribute_t::NODE_ATTR_FALSE)
    || get_attribute(element, Node::attribute_t::NODE_ATTR_INTERNAL))
    {
        return;
    }

    add_element(package_name, element->get_string(), element, type);
}



// This function searches a list of directives for class, functions
// and variables which are defined in a package. Their names are
// then saved in the import database for fast search.
void Compiler::find_packages_save_package_elements(Node::pointer_t package, String const& package_name)
{
    size_t const max_children(package->get_children_size());
    for(size_t idx = 0; idx < max_children; ++idx)
    {
        Node::pointer_t child(package->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_DIRECTIVE_LIST)
        {
            find_packages_save_package_elements(child, package_name); // recursive
        }
        else if(child->get_type() == Node::node_t::NODE_CLASS)
        {
            find_packages_add_database_entry(
                    package_name,
                    child,
                    "class"
                );
        }
        else if(child->get_type() == Node::node_t::NODE_FUNCTION)
        {
            // we do not save prototypes, that is tested later
            char const * type;
            if(child->get_flag(Node::flag_t::NODE_FUNCTION_FLAG_GETTER))
            {
                type = "getter";
            }
            else if(child->get_flag(Node::flag_t::NODE_FUNCTION_FLAG_SETTER))
            {
                type = "setter";
            }
            else
            {
                type = "function";
            }
            find_packages_add_database_entry(
                    package_name,
                    child,
                    type
                );
        }
        else if(child->get_type() == Node::node_t::NODE_VAR)
        {
            size_t const vcnt(child->get_children_size());
            for(size_t v(0); v < vcnt; ++v)
            {
                Node::pointer_t variable_node(child->get_child(v));
                // we do not save the variable type,
                // it would not help resolution
                find_packages_add_database_entry(
                        package_name,
                        variable_node,
                        "variable"
                    );
            }
        }
        else if(child->get_type() == Node::node_t::NODE_PACKAGE)
        {
            // sub packages
            Node::pointer_t list(child->get_child(0));
            String name(package_name);
            name += ".";
            name += child->get_string();
            find_packages_save_package_elements(list, name); // recursive
        }
    }
}


// this function searches the tree for packages (it stops at classes,
// functions, and other such blocks)
void Compiler::find_packages_directive_list(Node::pointer_t list)
{
    size_t const max_children(list->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(list->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_DIRECTIVE_LIST)
        {
            find_packages_directive_list(child);
        }
        else if(child->get_type() == Node::node_t::NODE_PACKAGE)
        {
            // Found a package! Save all the functions
            // variables and classes in the database
            // if not there yet.
            Node::pointer_t directive_list_node(child->get_child(0));
            find_packages_save_package_elements(directive_list_node, child->get_string());
        }
    }
}


void Compiler::find_packages(Node::pointer_t program_node)
{
    if(program_node->get_type() != Node::node_t::NODE_PROGRAM)
    {
        return;
    }

    find_packages_directive_list(program_node);
}


void Compiler::load_internal_packages(char const *module)
{
    // TODO: create sub-class to handle the directory

    std::string path(g_rc.get_scripts().to_utf8());
    path += "/";
    path += module;
    DIR *dir(opendir(path.c_str()));
    if(dir == nullptr)
    {
        // could not read this directory
        Position pos;
        pos.set_filename(path);
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INSTALLATION, pos);
        msg << "cannot read directory \"" << path << "\".\n";
        throw exception_exit(1, "cannot read directory");
    }

    for(;;)
    {
        struct dirent *ent(readdir(dir));
        if(!ent)
        {
            // no more files in directory
            break;
        }
        char const *s = ent->d_name;
        char const *e = nullptr;  // extension position
        while(*s != '\0')
        {
            if(*s == '.')
            {
                e = s;
            }
            s++;
        }
        // only interested by .js files except
        // the as2js_init.js file
        if(e == nullptr || strcmp(e, ".js") != 0
        || strcmp(ent->d_name, "as2js_init.js") == 0)
        {
            continue;
        }
        // we got a file of interest
        // TODO: we want to keep this package in RAM since
        //       we already parsed it!
        Node::pointer_t p(load_module(module, ent->d_name));
        // now we can search the package in the actual code
        find_packages(p);
    }

    // avoid leaks
    closedir(dir);
}


void Compiler::import(Node::pointer_t& import_node)
{
    // If we have the IMPLEMENTS flag set, then we must make sure
    // that the corresponding package is compiled.
    if(!import_node->get_flag(Node::flag_t::NODE_IMPORT_FLAG_IMPLEMENTS))
    {
        return;
    }

    // find the package
    Node::pointer_t package;

    // search in this program
    package = find_package(f_program, import_node->get_string());
    if(!package)
    {
        // not in this program, search externals
        Node::pointer_t program_node;
        String any_name("*");
        if(find_external_package(import_node, any_name, program_node))
        {
            // got externals, search those now
            package = find_package(program_node, import_node->get_string());
        }
        if(!package)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_FOUND, import_node->get_position());
            msg << "cannot find package '" << import_node->get_string() << "'.";
            return;
        }
    }

    // make sure it is compiled (once)
    bool const was_referenced(package->get_flag(Node::flag_t::NODE_PACKAGE_FLAG_REFERENCED));
    package->set_flag(Node::flag_t::NODE_PACKAGE_FLAG_REFERENCED, true);
    if(!was_referenced)
    {
        directive_list(package);
    }
}






Node::pointer_t Compiler::find_package(Node::pointer_t list, String const& name)
{
    NodeLock ln(list);
    size_t const max_children(list->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        Node::pointer_t child(list->get_child(idx));
        if(child->get_type() == Node::node_t::NODE_DIRECTIVE_LIST)
        {
            Node::pointer_t package(find_package(child, name));  // recursive
            if(package)
            {
                return package;
            }
        }
        else if(child->get_type() == Node::node_t::NODE_PACKAGE)
        {
            if(child->get_string() == name)
            {
                // found it!
                return child;
            }
        }
    }

    // not found
    return Node::pointer_t();
}


bool Compiler::find_external_package(Node::pointer_t import_node, String const& name, Node::pointer_t& program_node)
{
    // search a package which has an element named 'name'
    // and has a name which match the identifier specified in 'import'
    Database::Element::pointer_t e(find_element(import_node->get_string(), name, nullptr));
    if(!e)
    {
        // not found!
        return false;
    }

    String const filename(e->get_filename());

    // found it, lets get a node for it
    find_module(filename, program_node);

    // at this time this will not happen because if the find_module()
    // function fails, it throws exception_exit(1, ...);
    if(!program_node)
    {
        return false;
    }

    return true;
}


bool Compiler::check_import(Node::pointer_t& import_node, Node::pointer_t& resolution, String const& name, Node::pointer_t params, int search_flags)
{
    // search for a package within this program
    // (I am not too sure, but according to the spec. you can very well
    // have a package within any script file)
    if(find_package_item(f_program, import_node, resolution, name, params, search_flags))
    {
        return true;
    }

    Node::pointer_t program_node;
    if(!find_external_package(import_node, name, program_node))
    {
        return false;
    }

    return find_package_item(program_node, import_node, resolution, name, params, search_flags | SEARCH_FLAG_PACKAGE_MUST_EXIST);
}


bool Compiler::find_package_item(Node::pointer_t program_node, Node::pointer_t import_node, Node::pointer_t& resolution, String const& name, Node::pointer_t params, int const search_flags)
{
    Node::pointer_t package_node(find_package(program_node, import_node->get_string()));

    if(!package_node)
    {
        if((search_flags & SEARCH_FLAG_PACKAGE_MUST_EXIST) != 0)
        {
            // this is a bad error! we should always find the
            // packages in this case (i.e. when looking using the
            // database.)
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INTERNAL_ERROR, import_node->get_position());
            msg << "cannot find package '" << import_node->get_string() << "' in any of the previously registered packages.";
            throw exception_exit(1, "cannot find package");
        }
        return false;
    }

    if(package_node->get_children_size() == 0)
    {
        return false;
    }

    // setup labels (only the first time around)
    if(!package_node->get_flag(Node::flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS))
    {
        package_node->set_flag(Node::flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS, true);
        Node::pointer_t child(package_node->get_child(0));
        find_labels(package_node, child);
    }

    // search the name of the class/function/variable we're
    // searching for in this package:

    // TODO: Hmmm... could we have the actual node instead?
    Node::pointer_t id(package_node->create_replacement(Node::node_t::NODE_IDENTIFIER));
    id->set_string(name);

    int funcs = 0;
    if(!find_field(package_node, id, funcs, resolution, params, search_flags))
    {
        return false;
    }

    // TODO: Can we have an empty resolution here?!
    if(resolution)
    {
        if(get_attribute(resolution, Node::attribute_t::NODE_ATTR_PRIVATE))
        {
            // it is private, we cannot use this item
            // from outside whether it is in the
            // package or a sub-class
            return false;
        }

        if(get_attribute(resolution, Node::attribute_t::NODE_ATTR_INTERNAL))
        {
            // it is internal we can only use it from
            // another package
            Node::pointer_t parent(import_node);
            for(;;)
            {
                parent = parent->get_parent();
                if(!parent)
                {
                    return false;
                }
                if(parent->get_type() == Node::node_t::NODE_PACKAGE)
                {
                    // found the package mark
                    break;
                }
                if(parent->get_type() == Node::node_t::NODE_ROOT
                || parent->get_type() == Node::node_t::NODE_PROGRAM)
                {
                    return false;
                }
            }
        }
    }

    // make sure it is compiled (once)
    bool const was_referenced(package_node->get_flag(Node::flag_t::NODE_PACKAGE_FLAG_REFERENCED));
    package_node->set_flag(Node::flag_t::NODE_PACKAGE_FLAG_REFERENCED, true);
    if(!was_referenced)
    {
        directive_list(package_node);
    }

    return true;
}


void Compiler::internal_imports()
{
    if(!g_native_import)
    {
        // read the resource file
        g_rc.init_rc(static_cast<bool>(f_input_retriever));

        // TBD: at this point we only have native scripts
        //      we need browser scripts, for sure...
        //      and possibly some definitions of extensions such as jQuery
        //      however, at this point we do not have a global or system
        //      set of modules
        //g_global_import = load_module("global", "as_init.js");
        //g_system_import = load_module("system", "as_init.js");
        g_native_import = load_module("native", "as_init.js");
    }

    if(!g_db)
    {
        g_db.reset(new Database);
    }
    if(!g_db->load(g_rc.get_db()))
    {
        Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_UNEXPECTED_DATABASE);
        msg << "Failed reading the compiler database. You may need to delete it and try again or fix the resource file to point to the right file.";
        return;
    }

    if(!g_db_loaded)
    {
        g_db_loaded = true;

        // global defines the basic JavaScript classes such
        // as Object and String.
        //load_internal_packages("global");

        // the system defines Browser classes such as XMLNode
        //load_internal_packages("system");

        // the ECMAScript low level definitions
        load_internal_packages("native");

        // this saves the internal packages info for fast query
        // on next invocations
        g_db->save();
    }
}


bool Compiler::check_name(Node::pointer_t list, int idx, Node::pointer_t& resolution, Node::pointer_t id, Node::pointer_t params, int const search_flags)
{
    if(static_cast<size_t>(idx) >= list->get_children_size())
    {
        throw exception_internal_error(std::string("Compiler::check_name() index too large for this list."));
    }

    Node::pointer_t child(list->get_child(idx));

    // turned off?
    //if(get_attribute(child, Node::attribute_t::NODE_ATTR_FALSE))
    //{
    //    return false;
    //}

    bool result = false;
//std::cerr << "  +--> compiler_package.cpp: check_name() processing a child node type: \"" << child->get_type_name() << "\" ";
//if(child->get_type() == Node::node_t::NODE_CLASS
//|| child->get_type() == Node::node_t::NODE_PACKAGE
//|| child->get_type() == Node::node_t::NODE_IMPORT
//|| child->get_type() == Node::node_t::NODE_ENUM
//|| child->get_type() == Node::node_t::NODE_FUNCTION)
//{
//    std::cerr << " \"" << child->get_string() << "\"";
//}
//std::cerr << "\n";
    switch(child->get_type())
    {
    case Node::node_t::NODE_VAR:    // a VAR is composed of VARIABLEs
        {
            NodeLock ln(child);
            size_t const max_children(child->get_children_size());
            for(size_t j(0); j < max_children; ++j)
            {
                Node::pointer_t variable_node(child->get_child(j));
                if(variable_node->get_string() == id->get_string())
                {
                    // that is a variable!
                    // make sure it was parsed
                    if((search_flags & SEARCH_FLAG_NO_PARSING) == 0)
                    {
                        variable(variable_node, false);
                    }
                    if(params)
                    {
                        // check whether we are in a call
                        // because if we are the resolution
                        // is the "()" operator instead
                    }
                    resolution = variable_node;
                    result = true;
                    break;
                }
            }
        }
        break;

    case Node::node_t::NODE_PARAM:
//std::cerr << "  +--> param = " << child->get_string() << " against " << id->get_string() << "\n";
        if(child->get_string() == id->get_string())
        {
            resolution = child;
            resolution->set_flag(Node::flag_t::NODE_PARAM_FLAG_REFERENCED, true);
            return true;
        }
        break;

    case Node::node_t::NODE_FUNCTION:
//std::cerr << "  +--> name = " << child->get_string() << "\n";
        {
            Node::pointer_t the_class;
            if(is_constructor(child, the_class))
            {
                // this is a special case as the function name is the same
                // as the class name and the type resolution is thus the
                // class and not the function and we have to catch this
                // special case otherwise we get a never ending loop
                if(the_class->get_string() == id->get_string())
                {
                    // just in case we replace the child pointer so we
                    // avoid potential side effects of having a function
                    // declaration in the child pointer
                    child = the_class;
                    resolution = the_class;
                    result = true;
//std::cerr << "  +--> this was a class! => " << child->get_string() << "\n";
                }
            }
            else
            {
                result = check_function(child, resolution, id->get_string(), params, search_flags);
            }
        }
        break;

    case Node::node_t::NODE_CLASS:
    case Node::node_t::NODE_INTERFACE:
        if(child->get_string() == id->get_string())
        {
            // That is a class name! (good for a typedef, etc.)
            resolution = child;
            Node::pointer_t type(resolution->get_type_node());
//std::cerr << "  +--> so we got a type of CLASS or INTERFACE for " << id->get_string()
//          << " ... [" << (type ? "has a current type ptr" : "no current type ptr") << "]\n";
            if(!type)
            {
                // A class (interface) represents itself as far as type goes (TBD)
                resolution->set_type_node(child);
            }
            resolution->set_flag(Node::flag_t::NODE_IDENTIFIER_FLAG_TYPED, true);
            result = true;
        }
        break;

    case Node::node_t::NODE_ENUM:
    {
        // first we check whether the name of the enum is what
        // is being referenced (i.e. the type)
        if(child->get_string() == id->get_string())
        {
            resolution = child;
            resolution->set_flag(Node::flag_t::NODE_ENUM_FLAG_INUSE, true);
            return true;
        }

        // inside an enum we have references to other
        // identifiers of that enum and these need to be
        // checked here
        size_t const max(child->get_children_size());
        for(size_t j(0); j < max; ++j)
        {
            Node::pointer_t entry(child->get_child(j));
            if(entry->get_type() == Node::node_t::NODE_VARIABLE)
            {
                if(entry->get_string() == id->get_string())
                {
                    // this cannot be a function, right? so the following
                    // call is probably not really useful
                    resolution = entry;
                    resolution->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_INUSE, true);
                    return true;
                }
            }
        }
    }
        break;

    case Node::node_t::NODE_PACKAGE:
        if(child->get_string() == id->get_string())
        {
            // That is a package... we have to see packages
            // like classes, to search for more, you need
            // to search inside this package and none other.
            resolution = child;
            return true;
        }
#if 0
        // TODO: auto-import? this works, but I do not think we
        //       want an automatic import of even internal packages?
        //       do we?
        //
        //       At this point I would say that we do for the
        //       internal packages only. That being said, the
        //       Google closure compiler does that for all
        //       browser related declarations.
        //
        // if it is not the package itself, it could be an
        // element inside the package
        {
            int funcs(0);
            if(!find_field(child, id, funcs, resolution, params, search_flags))
            {
                break;
            }
        }
        result = true;
//std::cerr << "Found inside package! [" << id->get_string() << "]\n";
        if(!child->get_flag(Node::flag_t::NODE_PACKAGE_FLAG_REFERENCED))
        {
//std::cerr << "Compile package now!\n";
            directive_list(child);
            child->set_flag(Node::flag_t::NODE_PACKAGE_FLAG_REFERENCED);
        }
#endif
        break;

    case Node::node_t::NODE_IMPORT:
        return check_import(child, resolution, id->get_string(), params, search_flags);

    default:
        // ignore anything else for now
        break;

    }

    if(!result)
    {
        return false;
    }

    if(!resolution)
    {
        // this is kind of bad since we cannot test for
        // the scope...
        return true;
    }

//std::cerr << "  +--> yeah! resolved ID " << reinterpret_cast<long *>(resolution.get()) << "\n";
//std::cerr << "  +--> check_name(): private?\n";
    if(get_attribute(resolution, Node::attribute_t::NODE_ATTR_PRIVATE))
    {
//std::cerr << "  +--> check_name(): resolved private...\n";
        // Note that an interface and a package
        // can also have private members
        Node::pointer_t the_resolution_class(class_of_member(resolution));
        if(!the_resolution_class)
        {
            f_err_flags |= SEARCH_ERROR_PRIVATE;
            resolution.reset();
            return false;
        }
        if(the_resolution_class->get_type() == Node::node_t::NODE_PACKAGE)
        {
            f_err_flags |= SEARCH_ERROR_PRIVATE_PACKAGE;
            resolution.reset();
            return false;
        }
        if(the_resolution_class->get_type() != Node::node_t::NODE_CLASS
        && the_resolution_class->get_type() != Node::node_t::NODE_INTERFACE)
        {
            f_err_flags |= SEARCH_ERROR_WRONG_PRIVATE;
            resolution.reset();
            return false;
        }
        Node::pointer_t the_id_class(class_of_member(id));
        if(!the_id_class)
        {
            f_err_flags |= SEARCH_ERROR_PRIVATE;
            resolution.reset();
            return false;
        }
        if(the_id_class != the_resolution_class)
        {
            f_err_flags |= SEARCH_ERROR_PRIVATE;
            resolution.reset();
            return false;
        }
    }

    if(get_attribute(resolution, Node::attribute_t::NODE_ATTR_PROTECTED))
    {
//std::cerr << "  +--> check_name(): resolved protected...\n";
        // Note that an interface can also have protected members
        Node::pointer_t the_super_class;
        if(!are_objects_derived_from_one_another(id, resolution, the_super_class))
        {
            if(the_super_class
            && the_super_class->get_type() != Node::node_t::NODE_CLASS
            && the_super_class->get_type() != Node::node_t::NODE_INTERFACE)
            {
                f_err_flags |= SEARCH_ERROR_WRONG_PROTECTED;
            }
            else
            {
                f_err_flags |= SEARCH_ERROR_PROTECTED;
            }
            resolution.reset();
            return false;
        }
    }

    if(child->get_type() == Node::node_t::NODE_FUNCTION && params)
    {
std::cerr << "  +--> check_name(): resolved function...\n";
        if(check_function_with_params(child, params) < 0)
        {
            resolution.reset();
            return false;
        }
    }

    return true;
}


void Compiler::resolve_internal_type(Node::pointer_t parent, char const *type, Node::pointer_t& resolution)
{
    // create a temporary identifier
    Node::pointer_t id(parent->create_replacement(Node::node_t::NODE_IDENTIFIER));
    id->set_string(type);

    // TBD: identifier ever needs a parent?!
    //int const idx(parent->get_children_size());
//std::cerr << "Do some invalid append now?\n";
    //parent->append_child(id);
//std::cerr << "Done the invalid append?!\n";

    // search for the identifier which is an internal type name
    bool r;
    {
        // TODO: we should be able to start the search from the native
        //       definitions since this is only used for native types
        //       (i.e. Object, Boolean, etc.)
        NodeLock ln(parent);
        r = resolve_name(parent, id, resolution, Node::pointer_t(), 0);
    }

    // get rid of the temporary identifier
    //parent->delete_child(idx);

    if(!r)
    {
        // if the compiler cannot find an internal type, that is really bad!
        Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_INTERNAL_ERROR, parent->get_position());
        msg << "cannot find internal type \"" << type << "\".";
        throw exception_exit(1, "cannot find internal type");
    }

    return;
}


bool Compiler::resolve_name(
            Node::pointer_t list,
            Node::pointer_t id,
            Node::pointer_t& resolution,
            Node::pointer_t params,
            int const search_flags
        )
{
//std::cerr << " +++ resolve_name()\n";
    RestoreFlags restore_flags(this);

    // just in case the caller is reusing the same node
    resolution.reset();

    // resolution may includes a member (a.b) and the resolution is the
    // last field name
    Node::node_t id_type(id->get_type());
    if(id_type == Node::node_t::NODE_MEMBER)
    {
        if(id->get_children_size() != 2)
        {
            throw exception_internal_error(std::string("compiler_package:Compiler::resolve_name() called with a MEMBER which does not have exactly two children."));
        }
        // child 0 is the variable name, child 1 is the field name
        Node::pointer_t name(id->get_child(0));
        if(!resolve_name(list, name, resolution, params, search_flags))  // recursive
        {
            // we could not find 'name' so we are hosed anyway
            // the callee should already have generated an error
            return false;
        }
        list = resolution;
        resolution.reset();
        id = id->get_child(1);
        id_type = id->get_type();
    }

    // in some cases we may want to resolve a name specified in a string
    // (i.e. test["me"])
    if(id_type != Node::node_t::NODE_IDENTIFIER
    && id_type != Node::node_t::NODE_VIDENTIFIER
    && id_type != Node::node_t::NODE_STRING)
    {
        throw exception_internal_error(std::string("compiler_package:Compiler::resolve_name() was called with an 'identifier node' which is not a NODE_[V]IDENTIFIER or NODE_STRING, it is ") + id->get_type_name());
    }

    // already typed?
    {
        Node::pointer_t type(id->get_type_node());
        if(type)
        {
            resolution = type;
            return true;
        }
    }

    //
    // Search for the parent list of directives; in that list, search
    // for the identifier; if not found, try again with the parent
    // of that list of directives (unless we find an import in which
    // case we first try the import)
    //
    // Note that the currently effective with()'s and use namespace's
    // are defined in the f_scope variable. This is used here to know
    // whether the name matches an entry or not.
    //

    // a list of functions whenever the name resolves to a function
    int funcs(0);

    Node::pointer_t parent(list->get_parent());
    if(parent->get_type() == Node::node_t::NODE_WITH)
    {
        // we are currently defining the WITH object, skip the
        // WITH itself!
        list = parent;
    }
    int module(0);        // 0 is user module being compiled
    for(;;)
    {
        // we will start searching at this offset; first backward
        // and then forward
        size_t offset(0);

        // This function should never be called from program()
        // also, 'id' cannot be a directive list (it has to be an
        // identifier, a member or a string!)
        //
        // For these reasons, we can start the following loop with
        // a get_parent() in all cases.
        //
        if(module == 0)
        {
            // when we were inside the function parameter
            // list we do not want to check out the function
            // otherwise we could have a forward search of
            // the parameters which we disallow (only backward
            // search is allowed in that list)
            if(list->get_type() == Node::node_t::NODE_PARAMETERS)
            {
                list = list->get_parent();
                if(!list)
                {
                    throw exception_internal_error("compiler_package:Compiler::resolve_name() got a NULL parent without finding NODE_ROOT first (NODE_PARAMETERS).");
                }
            }

            for(bool more(true); more; )
            {
                offset = list->get_offset();
                list = list->get_parent();
                if(!list)
                {
                    throw exception_internal_error("compiler_package:Compiler::resolve_name() got a nullptr parent without finding NODE_ROOT first.");
                }
                switch(list->get_type())
                {
                case Node::node_t::NODE_ROOT:
                    throw exception_internal_error("compiler_package:Compiler::resolve_name() found the NODE_ROOT while searching for a parent.");

                case Node::node_t::NODE_EXTENDS:
                case Node::node_t::NODE_IMPLEMENTS:
                    list = list->get_parent();
                    if(!list)
                    {
                        throw exception_internal_error("compiler_package:Compiler::resolve_name() got a nullptr parent without finding NODE_ROOT first (NODE_EXTENDS/NODE_IMPLEMENTS).");
                    }
                    break;

                case Node::node_t::NODE_DIRECTIVE_LIST:
                case Node::node_t::NODE_FOR:
                case Node::node_t::NODE_WITH:
                //case Node::node_t::NODE_PACKAGE: -- not necessary, the first item is a NODE_DIRECTIVE_LIST
                case Node::node_t::NODE_PROGRAM:
                case Node::node_t::NODE_FUNCTION:
                case Node::node_t::NODE_PARAMETERS:
                case Node::node_t::NODE_ENUM:
                case Node::node_t::NODE_CATCH:
                case Node::node_t::NODE_CLASS:
                case Node::node_t::NODE_INTERFACE:
                    more = false;
                    break;

                default:
                    break;

                }
            }
        }

        if(list->get_type() == Node::node_t::NODE_PROGRAM
        || module != 0)
        {
            // not resolved
            switch(module)
            {
            case 0:
                module = 1;
                if(g_global_import
                && g_global_import->get_children_size() > 0)
                {
                    list = g_global_import->get_child(0);
                    break;
                }
                /*FALLTHROUGH*/
            case 1:
                module = 2;
                if(g_system_import
                && g_system_import->get_children_size() > 0)
                {
                    list = g_system_import->get_child(0);
                    break;
                }
                /*FALLTHROUGH*/
            case 2:
                module = 3;
                if(g_native_import
                && g_native_import->get_children_size() > 0)
                {
                    list = g_native_import->get_child(0);
                    break;
                }
                /*FALLTHROUGH*/
            case 3:
                // no more default list of directives...
                module = 4;
                break;

            }
            offset = 0;
        }
        if(module == 4)
        {
            // did not find a variable and such, but
            // we may have found a function (see below
            // after the forever loop breaking here)
            break;
        }

        NodeLock ln(list);
        size_t const max_children(list->get_children_size());
        switch(list->get_type())
        {
        case Node::node_t::NODE_DIRECTIVE_LIST:
        {
            // okay! we have got a list of directives
            // backward loop up first since in 99% of cases that
            // will be enough...
            if(offset >= max_children)
            {
                throw exception_internal_error("somehow an offset is out of range");
            }
            size_t idx(offset);
            while(idx > 0)
            {
                idx--;
                if(check_name(list, idx, resolution, id, params, search_flags))
                {
                    if(funcs_name(funcs, resolution))
                    {
                        return true;
                    }
                }
            }

            // forward look up is also available in ECMAScript...
            // (necessary in case function A calls function B
            // and function B calls function A).
            for(idx = offset; idx < max_children; ++idx)
            {
                if(check_name(list, idx, resolution, id, params, search_flags))
                {
                    // TODO: if it is a variable it needs
                    //       to be a constant...
                    if(funcs_name(funcs, resolution))
                    {
                        return true;
                    }
                }
            }
        }
            break;

        case Node::node_t::NODE_FOR:
        {
            // the first member of a for can include variable
            // definitions
            if(max_children > 0 && check_name(list, 0, resolution, id, params, search_flags))
            {
                if(funcs_name(funcs, resolution))
                {
                    return true;
                }
            }
        }
            break;

#if 0
        case Node::node_t::NODE_PACKAGE:
            // From inside a package, we have an implicit
            //    IMPORT <package name>;
            //
            // This is required to enable a multiple files
            // package definition which ease the development
            // of really large packages.
            if(check_import(list, resolution, id->get_string(), params, search_flags))
            {
                return true;
            }
            break;
#endif

        case Node::node_t::NODE_WITH:
        {
            if(max_children != 2)
            {
                break;
            }
            // ha! we found a valid WITH instruction, let's
            // search for this name in the corresponding
            // object type instead (i.e. a field of the object)
            Node::pointer_t type(list->get_child(0));
            if(type)
            {
                Node::pointer_t link(type->get_instance());
                if(link)
                {
                    if(resolve_field(link, id, resolution, params, search_flags))
                    {
                        // Mark this identifier as a
                        // reference to a WITH object
                        id->set_flag(Node::flag_t::NODE_IDENTIFIER_FLAG_WITH, true);

                        // TODO: we certainly want to compare
                        //       all the field functions and the
                        //       other functions... at this time,
                        //       err if we get a field function
                        //       and others are ignored!
                        if(funcs != 0)
                        {
                            throw exception_internal_error("at this time we do not support functions here (under a with)");
                        }
                        return true;
                    }
                }
            }
        }
            break;

        case Node::node_t::NODE_FUNCTION:
        {
            // if identifier is marked as a type, then skip testing
            // the function parameters since those cannot be type
            // declarations
            if(!id->get_attribute(Node::attribute_t::NODE_ATTR_TYPE))
            {
                // search the list of parameters for a corresponding name
                for(size_t idx(0); idx < max_children; ++idx)
                {
                    Node::pointer_t parameters_node(list->get_child(idx));
                    if(parameters_node->get_type() == Node::node_t::NODE_PARAMETERS)
                    {
                        NodeLock parameters_ln(parameters_node);
                        size_t const cnt(parameters_node->get_children_size());
                        for(size_t j(0); j < cnt; ++j)
                        {
                            if(check_name(parameters_node, j, resolution, id, params, search_flags))
                            {
                                if(funcs_name(funcs, resolution))
                                {
                                    return true;
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
            break;

        case Node::node_t::NODE_PARAMETERS:
        {
            // Wow! I cannot believe I am implementing this...
            // So we will be able to reference the previous
            // parameters in the default value of the following
            // parameters; and that makes sense, it is available
            // in C++ templates, right?!
            // And guess what, that is just this little loop.
            // That is it. Big deal, hey?! 8-)
            if(offset >= max_children)
            {
                throw exception_internal_error("somehow an offset is out of range");
            }
            size_t idx(offset);
            while(idx > 0)
            {
                idx--;
                if(check_name(list, idx, resolution, id, params, search_flags))
                {
                    if(funcs_name(funcs, resolution))
                    {
                        return true;
                    }
                }
            }
        }
            break;

        case Node::node_t::NODE_CATCH:
        {
            // a catch can have a parameter of its own
            Node::pointer_t parameters_node(list->get_child(0));
            if(parameters_node->get_children_size() > 0)
            {
                if(check_name(parameters_node, 0, resolution, id, params, search_flags))
                {
                    if(funcs_name(funcs, resolution))
                    {
                        return true;
                    }
                }
            }
        }
            break;

        case Node::node_t::NODE_ENUM:
            // first we check whether the name of the enum is what
            // is being referenced (i.e. the type)
            if(id->get_string() == list->get_string())
            {
                resolution = list;
                resolution->set_flag(Node::flag_t::NODE_ENUM_FLAG_INUSE, true);
                return true;
            }

            // inside an enum we have references to other
            // identifiers of that enum and these need to be
            // checked here
            //
            // And note that these are not in any way affected
            // by scope attributes
            for(size_t idx(0); idx < max_children; ++idx)
            {
                Node::pointer_t entry(list->get_child(idx));
                if(entry->get_type() == Node::node_t::NODE_VARIABLE)
                {
                    if(id->get_string() == entry->get_string())
                    {
                        // this cannot be a function, right? so the following
                        // call is probably not really useful
                        resolution = entry;
                        if(funcs_name(funcs, resolution))
                        {
                            resolution->set_flag(Node::flag_t::NODE_VARIABLE_FLAG_INUSE, true);
                            return true;
                        }
                    }
                }
                // else -- probably a NODE_TYPE
            }
            break;

        case Node::node_t::NODE_CLASS:
        case Node::node_t::NODE_INTERFACE:
            // // if the ID is a type and the name is the same as the
            // // class name, then we are found what we were looking for
            // if(id->get_attribute(Node::attribute_t::NODE_ATTR_TYPE)
            // && id->get_string() == list->get_string())
            // {
            //     resolution = list;
            //     return true;
            // }
            // We want to search the extends and implements declarations as well
            if(find_in_extends(list, id, funcs, resolution, params, search_flags))
            {
                if(funcs_name(funcs, resolution))
                {
                    return true;
                }
            }
            break;

        default:
            // this could happen if our tree was to change and we do not
            // properly update this function
            throw exception_internal_error("compiler_package: unhandled type in Compiler::resolve_name()");

        }
    }

    resolution.reset();

    if(funcs != 0)
    {
        if(select_best_func(params, resolution))
        {
            return true;
        }
    }

    print_search_errors(id);

    return false;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
