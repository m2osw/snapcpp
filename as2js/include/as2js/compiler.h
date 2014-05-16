#ifndef AS2JS_COMPILER_H
#define AS2JS_COMPILER_H
/* compiler.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/optimizer.h"
#include    "as2js/stream.h"


namespace as2js
{




// Once a program was parsed, you need to compile it. This
// mainly means resolving the references (i.e. identifiers)
// which may generate the loading of libraries specified in
// import instructions (note that some import instructions
// are automatic for the global and native environments.)
//
// The code, after you ran the parser looks like this:
//
//    Compiler *compiler = Compiler::CreateCompiler();
//    // this is the same options as for the parser
//    compiler->SetOptions(options);
//    error_count = compiler->Compile(root);
//
// The Compile() function returns the number of errors
// encountered while compiling. The root parameter is
// what was returned by the Parse() function of the
// Parser object.
class Compiler
{
public:
    typedef std::shared_ptr<Compiler>   pointer_t;

                                Compiler();
    virtual                     ~Compiler();

    void                        set_options(Options::pointer_t options);
    int                         compile(Node::pointer_t& root);

private:
    enum search_error_t
    {
        MATCH_ANY_ANCESTOR                  = 0x0001,

        SEARCH_ERROR_PRIVATE                = 0x00000001,
        SEARCH_ERROR_PROTECTED              = 0x00000002,
        SEARCH_ERROR_PROTOTYPE              = 0x00000004,
        SEARCH_ERROR_WRONG_PRIVATE          = 0x00000008,
        SEARCH_ERROR_WRONG_PROTECTED        = 0x00000010,
        SEARCH_ERROR_PRIVATE_PACKAGE        = 0x00000020,
        SEARCH_ERROR_EXPECTED_STATIC_MEMBER = 0x00000040
    };

    enum search_flag_t
    {
        SEARCH_FLAG_NO_PARSING              = 0x00000001,    // avoid parsing variables
        SEARCH_FLAG_GETTER                  = 0x00000002,    // accept getters (reading)
        SEARCH_FLAG_SETTER                  = 0x00000004,    // accept setters (writing)
        SEARCH_FLAG_PACKAGE_MUST_EXIST      = 0x00000008     // weather the package has to exist
    };

    typedef std::map<String, Node>          module_map_t;

    // automate the restoration of the error flags
    class RestoreFlags
    {
    public:
                RestoreFlags(Compiler::pointer_t compiler)
                {
                    f_compiler = compiler;
                    f_org_flags = f_compiler->get_err_flags();
                    f_compiler->set_err_flags(0);
                }
                ~RestoreFlags()
                {
                    f_compiler->set_err_flags(f_org_flags);
                }

    private:
        Compiler::pointer_t f_compiler;
        int                 f_org_flags;
    };


    // functions used to load the internal imports
    void                internal_imports();
    Node::pointer_t     load_module(char const *module, char const *file);
    void                load_internal_packages(char const *module);
    void                read_db();
    void                write_db();
    static bool         isspace(int c);
    bool                find_module(String const& filename, Node::pointer_t& n);
    const char *        find_element(String const& package_name, String const& element_name, Node::pointer_t element, char const *type);
    void                find_packages_add_database_entry(const String& package_name, Node::pointer_t& element, char const *type);
    void                find_packages_save_package_Elements(Node::pointer_t& package, String const& package_name);
    void                find_packages_directive_list(Node::pointer_t& list);
    void                find_packages(Node::pointer_t& program);
    String              get_package_filename(char const *package_info);

    void                add_variable(Node::pointer_t& variable);
    bool                are_objects_derived_from_one_another(Node::pointer_t& derived_class, Node::pointer_t& super_class, Node::pointer_t data);
    void                assignment_operator(Node::pointer_t& expr);
    bool                best_param_match(Node::pointer_t& best, Node::pointer_t& match);
    bool                best_param_match_derived_from(Node::pointer_t& best, Node::pointer_t& match);
    void                binary_operator(Node::pointer_t& expr);
    void                break_continue(Node::pointer_t& break_node);
    void                call_add_missing_params(Node::pointer_t& call, Node::pointer_t& params);
    void                can_instantiate_type(Node::pointer_t& expr);
    void                case_directive(Node::pointer_t& case_node);
    void                catch_directive(Node::pointer_t& catch_node);
    bool                checkField(Node::pointer_t& link, Node::pointer_t& field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int search_flags);
    bool                checkFinalFunctions(Node::pointer_t& function, Node::pointer_t& class_node);
    bool                checkFunction(Node::pointer_t& func, Node::pointer_t& resolution, const String& name, Node::pointer_t params, int search_flags);
    int                 checkFunctionWithParams(Node::pointer_t& func, Node::pointer_t params);
    bool                checkImport(Node::pointer_t& child, Node::pointer_t& resolution, String const& name, Node::pointer_t params, int search_flags);
    void                checkMember(Node::pointer_t& obj, Node::pointer_t& field, Node::pointer_t& field_name);
    bool                checkName(Node::pointer_t& list, int idx, Node::pointer_t& resolution, Node::pointer_t& id, Node::pointer_t params, int search_flags);
    void                checkSuperValidity(Node::pointer_t& expr);
    void                checkThisValidity(Node::pointer_t& expr);
    bool                checkUniqueFunctions(Node::pointer_t& function, Node::pointer_t& class_node, bool all_levels);
    void                class_directive(Node::pointer_t& class_node);
    Node::pointer_t     class_of_member(Node::pointer_t parent, Node::pointer_t data);
    bool                compare_parameters(Node::pointer_t& lfunction, Node::pointer_t& rfunction);
    void                declare_class(Node::pointer_t& class_node);
    void                default_directive(Node::pointer_t& default_node);
    bool                define_function_type(Node::pointer_t& func);
    void                directive(Node::pointer_t& directive);
    Node::pointer_t     directive_list(Node::pointer_t& directive_list);
    void                do_directive(Node::pointer_t& do_node);
    void                enum_directive(Node::pointer_t& enum_node);
    void                expression(Node::pointer_t expr, Node::pointer_t params = Node::pointer_t());
    bool                expression_new(Node::pointer_t expr);
    void                extendClass(Node::pointer_t& class_node, Node::pointer_t& extend_name);
    void                finally(Node::pointer_t& finally_node);
    bool                findAnyField(Node::pointer_t& link, Node::pointer_t& field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int search_flags);
    int                 findClass(Node::pointer_t& class_type, Node::pointer_t& type, int depth);
    bool                findExternalPackage(Node::pointer_t& import, const String& name, Node::pointer_t& program);
    bool                findField(Node::pointer_t& link, Node::pointer_t& field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int search_flags);
    bool                findFinalFunctions(Node::pointer_t& function, Node::pointer_t& super);
    bool                findInExtends(Node::pointer_t& link, Node::pointer_t& field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int search_flags);
    void                find_labels(Node::pointer_t& function, Node::pointer_t node);
    bool                findMember(Node::pointer_t& member, Node::pointer_t& resolution, Node::pointer_t params, int search_flags);
    bool                findOverloadedFunction(Node::pointer_t& class_node, Node::pointer_t& function);
    Node::pointer_t     findPackage(Node::pointer_t& list, const String& name);
    bool                findPackageItem(Node::pointer_t& program, Node::pointer_t& import, Node::pointer_t& resolution, const String& name, Node::pointer_t params, int search_flags);
    void                for_directive(Node::pointer_t& for_node);
    bool                funcsName(int& funcs, Node::pointer_t& resolution, bool increment = true);
    void                function(Node::pointer_t& parameters);
    unsigned long       getAttributes(Node::pointer_t& node);
    int                 get_err_flags() const { return f_err_flags; }
    void                goto_directive(Node::pointer_t& goto_node);
    bool                hasAbstractFunctions(Node::pointer_t& class_node, Node::pointer_t& list, Node::pointer_t& func);
    void                identifierToAttrs(Node::pointer_t& node, Node::pointer_t& a, unsigned long& attrs);
    void                if_directive(Node::pointer_t& if_node);
    void                import(Node::pointer_t& import);
    bool                isConstructor(Node::pointer_t& func);
    bool                isDerivedFrom(Node::pointer_t& derived_class, Node::pointer_t& super_class);
    bool                isDynamicClass(Node::pointer_t& class_node);
    bool                isFunctionAbstract(Node::pointer_t& function);
    bool                isFunctionOverloaded(Node::pointer_t& class_node, Node::pointer_t& function);
    void                linkType(Node::pointer_t& type);
    int                 matchType(Node::pointer_t& t1, Node::pointer_t t2, int match);
    void                nodeToAttrs(Node::pointer_t& node, Node::pointer_t& a, unsigned long& attrs);
    void                objectLiteral(Node::pointer_t& expr);
    void                parameters(Node::pointer_t& parameters);
    void                printSearchErrors(const Node::pointer_t& name);
    void                program(Node::pointer_t& program);
    bool                replaceConstantVariable(Node::pointer_t& replace, Node::pointer_t& resolution);
    bool                resolveCall(Node::pointer_t& call);
    bool                resolveField(Node::pointer_t& object, Node::pointer_t& field, Node::pointer_t& resolution, Node::pointer_t params, int search_flags);
    void                resolveInternalType(Node::pointer_t& parent, const char *type, Node::pointer_t& resolution);
    void                resolveMember(Node::pointer_t& expr, Node::pointer_t params, int search_flags);
    bool                resolveName(Node::pointer_t list, Node::pointer_t& id, Node::pointer_t& resolution, Node::pointer_t params, int search_flags);
    Node::pointer_t     return_directive(Node::pointer_t& return_node);
    bool                selectBestFunc(Node::pointer_t *params, Node::pointer_t& resolution);
    void                setAttr(Node::pointer_t& node, unsigned long& list_attrs, unsigned long set, unsigned long group, const char names);
    void                set_err_flags(int flags) { f_err_flags = flags; }
    bool                specialIdentifier(Node::pointer_t& expr);
    void                switch_directive(Node::pointer_t& switch_node);
    void                throw_directive(Node::pointer_t& throw_node);
    void                try_directive(Node::pointer_t& try_node);
    void                typeExpr(Node::pointer_t& expr);
    void                unaryOperator(Node::pointer_t& expr);
    void                useNamespace(Node::pointer_t& use_namespace);
    void                var(Node::pointer_t& var);
    void                variable(Node::pointer_t& variable, bool side_effects_only);
    void                variableToAttrs(Node::pointer_t& node, Node::pointer_t& var, unsigned long& attrs);
    void                while_directive(Node::pointer_t& while_node);
    void                with(Node::pointer_t& with);

    Optimizer::pointer_t        f_optimizer;
    Options::pointer_t          f_options;
    Node::pointer_t             f_program;
    time_t                      f_time;         // time when the compiler is created
    int                         f_err_flags;    // when searching a name and it doesn't get resolve, emit these errors
    Node::pointer_t             f_scope;        // with() and use namespace list
    FILE *                      f_db;
    size_t                      f_db_size;
    char *                      f_db_data;
    size_t                      f_db_count;     // valid entries
    size_t                      f_db_max;       // total # of slots available
    char **                     f_db_packages;
    module_map_t                f_modules;      // already loaded files (external modules)
};





}
// namespace as2js
#endif
// #ifndef AS2JS_COMPILER_H

// vim: ts=4 sw=4 et
