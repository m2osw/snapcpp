/* compiler.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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
$include    "rc.h"                 // this is a 100% private class


namespace as2js
{

namespace
{



// The following globals are read only once and you can compile
// many times without having to reload them.
//
// the resource file information
//
rc_t                g_rc;

// the global imports (those which are automatic and
// define the intrinsic functions and types of the language)
//
Node::pointer_t     g_global_import;

// the system imports (this is specific to the system you
// are using this compiler for; it defines the system)
//
Node::pointer_t     g_system_import;

// the native imports (this is specific to your system
// environment, it defines objects in your environment)
//
Node::pointer_t     g_native_import;



}
// no name namespace


/**********************************************************************/
/**********************************************************************/
/***  COMPILER CREATOR  ***********************************************/
/**********************************************************************/
/**********************************************************************/

Compiler *Compiler::CreateCompiler(InputRetriever *input)
{
    return new IntCompiler(input);
}


const char *Compiler::Version(void)
{
    return TO_STR(SSWF_VERSION);
}


/**********************************************************************/
/**********************************************************************/
/***  INTERNAL COMPILER (Basics)  *************************************/
/**********************************************************************/
/**********************************************************************/




Compiler::Compiler(InputRetriever *input)
{
    //f_default_error_stream -- auto-init
    f_error_stream = &f_default_error_stream;
    //f_optimizer -- auto-init
    f_optimizer.SetErrorStream(f_default_error_stream);
    f_options = 0;
    f_input_retriever = input;
    //f_program -- auto-init
    f_time = time(NULL);
    f_err_flags = 0;
    //f_scope -- auto-init
    f_db = 0;
    f_db_size = 0;
    f_db_data = 0;
    f_db_count = 0;
    f_db_max = 0;
    f_db_packages = 0;
    f_modules = 0;

    InternalImports();
}


IntCompiler::~IntCompiler()
{
    if(f_db != 0) {
        fclose(f_db);
    }
    delete [] f_db_data;

    // delete packages we added in this session
    for(size_t idx = 0; idx < f_db_count; ++idx) {
        if(f_db_packages[idx] < f_db_data
        || f_db_packages[idx] > f_db_data + f_db_size) {
            delete [] f_db_packages[idx];
        }
    }

    delete [] f_db_packages;
}


InputRetriever *IntCompiler::SetInputRetriever(InputRetriever *retriever)
{
    InputRetriever *old(f_input_retriever);

    f_input_retriever = retriever;

    return old;
}


void IntCompiler::SetErrorStream(ErrorStream& error_stream)
{
    f_error_stream = &error_stream;
    f_optimizer.SetErrorStream(error_stream);
}


void IntCompiler::SetOptions(Options& options)
{
    f_options = &options;
    f_optimizer.SetOptions(options);
}





bool Compiler::isspace(int c)
{
    return c == ' ' || c == '\t';
}


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
 * If the module was already loaded, return a copy of the existing
 * tree of nodes.
 *
 * If the module was not yet loaded, try to load it. If the file cannot
 * be found or the file cannot be compiled, a fatal error is emitted
 * and the process stops.
 *
 * \param[in] filename  The name of the module to be loaded
 * \param[in,out] result  The shared pointer to the resulting root node.
 */
void Compiler::find_module(String const& filename, Node::pointer_t& result)
{
    // module already loaded?
    module_map_t::const_iterator existing_module(std::find(f_modules.begin(), f_modules.end(), filename));
    if(existing_module != f_modules.end())
    {
        result = *existing_module;
        return true;
    }

    // we could not find this module, try to load the it
    Input::pointer_t in;
    if(f_input_retriever)
    {
        in = f_input_retriever->retrieve(filename);
    }
    if(!in)
    {
        in.reset(new FileInput());
        if(!in->open(filename))
        {
            Message msg(MESSAGE_LEVEL_FATAL, AS_ERR_NOT_FOUND, f_lexer->get_input()->get_position());
            msg << "cannot open module file \"" << filename << "\".";
            exit(1);
        }
    }

    // Parse the file in result
    Parser::pointer_t parser(new Parser());
    parser->set_options(f_options);
    parser->set_input(in);
    result = parser->parse();
    parser.reset();

#if 0
fprintf(stderr, "%s module:\n", fn);
result.Display(stderr);
#endif

    if(!result)
    {
        Message msg(MESSAGE_LEVEL_FATAL, AS_ERR_CANNOT_COMPILE, f_lexer->get_input()->get_position());
        msg << "could not compile module file \"" << filename << "\".";
        exit(1);
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




void Compiler::find_packages_add_database_entry(String const& package_name, Node::pointer_t& element, char const *type)
{
    // here, we totally ignore internal, private
    // and false entries right away
    if(get_attribute(element, Node::NODE_ATTR_PRIVATE)
    || get_attribute(element, Node::NODE_ATTR_FALSE)
    || get_attribute(element, Node::NODE_ATTR_INTERNAL))
    {
        return;
    }

    find_element(package_name, element->get_string(), element, type);
}



// This function searches a list of directives for class, functions
// and variables which are defined in a package. Their names are
// then saved in the import database for fast search.
void IntCompiler::FindPackages_SavePackageElements(NodePtr& package, const String& package_name)
{
    int max = package.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = package.GetChild(idx);
        Data& data = child.GetData();
        if(data.f_type == NODE_DIRECTIVE_LIST) {
            FindPackages_SavePackageElements(child, package_name);
        }
        else if(data.f_type == NODE_CLASS) {
            FindPackages_AddDatabaseEntry(
                    package_name,
                    child,
                    "class"
                );
        }
        else if(data.f_type == NODE_FUNCTION) {
            // we don't save prototypes, that's tested later
            int flg = data.f_int.Get();
            const char *type;
            if((flg & NODE_FUNCTION_FLAG_GETTER) != 0) {
                type = "getter";
            }
            else if((flg & NODE_FUNCTION_FLAG_SETTER) != 0) {
                type = "setter";
            }
            else {
                type = "function";
            }
            FindPackages_AddDatabaseEntry(
                    package_name,
                    child,
                    type
                );
        }
        else if(data.f_type == NODE_VAR) {
            int vcnt = child.GetChildCount();
            for(int v = 0; v < vcnt; ++v) {
                NodePtr& variable = child.GetChild(v);
                // we don't save the variable type,
                // it wouldn't help resolution
                FindPackages_AddDatabaseEntry(
                        package_name,
                        variable,
                        "variable"
                    );
            }
        }
        else if(data.f_type == NODE_PACKAGE) {
            // sub packages
            NodePtr& list = child.GetChild(0);
            String name = package_name;
            name += ".";
            name += data.f_str;
            FindPackages_SavePackageElements(list, name);
        }
    }
}


// this function searches the tree for packages (it stops at classes,
// functions, and other such blocks)
void IntCompiler::FindPackages_DirectiveList(NodePtr& list)
{
    int max = list.GetChildCount();
    for(int idx = 0; idx < max; ++idx) {
        NodePtr& child = list.GetChild(idx);
        Data& data = child.GetData();
        if(data.f_type == NODE_DIRECTIVE_LIST) {
            FindPackages_DirectiveList(child);
        }
        else if(data.f_type == NODE_PACKAGE) {
            // Found a package! Save all the functions
            // variables and classes in the database
            // if not there yet.
            NodePtr& directive_list = child.GetChild(0);
            FindPackages_SavePackageElements(directive_list, data.f_str);
        }
    }
}


void IntCompiler::FindPackages(NodePtr& program)
{
    Data& data = program.GetData();
    if(data.f_type != NODE_PROGRAM) {
        return;
    }

    FindPackages_DirectiveList(program);
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
        Message msg(MESSAGE_LEVEL_ERROR, AS_ERR_INSTALLATION, f_lexer->get_input()->get_position());
        msg << "cannot read directory \"" << filename << "\".\n"
        exit(1);
    }

    for(;;)
    {
        struct dirent *ent(readdir);
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
        // we've got a file of interest
        // TODO: we want to keep this package in RAM since
        //       we already parsed it!
        Node::pointer_t p(load_module(module, ent->d_name));
        // now we can search the package in the actual code
        find_packages(p);
    }

    // avoid leaks
    closedir(dir);
}


void Compiler::InternalImports()
{
    if(!g_global_import)
    {
        // read the resource file
        g_rc.init_rc(static_cast<bool>(f_input_retriever));

        g_global_import = load_module("global", "as_init.js");
        g_system_import = load_module("system", "as_init.js");
        g_native_import = load_module("native", "as_init.js");
    }

    f_db->load(g_rc.get_db());

    if(f_db_count == 0)
    {
        // global defines the basic JavaScript classes such
        // as Object and String.
        load_internal_packages("global");
        // the system defines Browser classes such as XMLNode
        load_internal_packages("system");
        // ???
        load_internal_packages("native");

        // this saves the internal packages info
        WriteDB();
    }
}




}
// namespace as2js

// vim: ts=4 sw=4 et
