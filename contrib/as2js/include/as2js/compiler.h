#ifndef AS2JS_COMPILER_H
#define AS2JS_COMPILER_H
/* compiler.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/optimizer.h"
#include    "as2js/stream.h"
#include    "as2js/options.h"


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

                                Compiler(Options::pointer_t options);
    virtual                     ~Compiler();

    InputRetriever::pointer_t   set_input_retriever(InputRetriever::pointer_t retriever);
    int                         compile(Node::pointer_t& root);

private:
    typedef uint32_t                                search_error_t;
    typedef uint32_t                                search_flag_t;

    static search_error_t const SEARCH_ERROR_NONE                   = 0x00000000;
    static search_error_t const SEARCH_ERROR_PRIVATE                = 0x00000001;
    static search_error_t const SEARCH_ERROR_PROTECTED              = 0x00000002;
    static search_error_t const SEARCH_ERROR_PROTOTYPE              = 0x00000004;
    static search_error_t const SEARCH_ERROR_WRONG_PRIVATE          = 0x00000008;
    static search_error_t const SEARCH_ERROR_WRONG_PROTECTED        = 0x00000010;
    static search_error_t const SEARCH_ERROR_PRIVATE_PACKAGE        = 0x00000020;
    static search_error_t const SEARCH_ERROR_EXPECTED_STATIC_MEMBER = 0x00000040;

    static search_flag_t const  SEARCH_FLAG_NO_PARSING              = 0x00000001;    // avoid parsing variables
    static search_flag_t const  SEARCH_FLAG_GETTER                  = 0x00000002;    // accept getters (reading)
    static search_flag_t const  SEARCH_FLAG_SETTER                  = 0x00000004;    // accept setters (writing)
    static search_flag_t const  SEARCH_FLAG_PACKAGE_MUST_EXIST      = 0x00000008;    // weather the package has to exist

    typedef std::map<String, Node::pointer_t>   module_map_t;

    // automate the restoration of the error flags
    class RestoreFlags
    {
    public:
                RestoreFlags(Compiler *compiler)
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
        Compiler *      f_compiler;
        int             f_org_flags;
    };


    // functions used to load the internal imports
    void                internal_imports();
    Node::pointer_t     load_module(char const *module, char const *file);
    void                load_internal_packages(char const *module);
    void                read_db();
    void                write_db();
    bool                find_module(String const& filename, Node::pointer_t& result);
    void                find_packages_add_database_entry(String const& package_name, Node::pointer_t& element, char const *type);
    void                find_packages_save_package_elements(Node::pointer_t package, String const& package_name);
    void                find_packages_directive_list(Node::pointer_t list);
    void                find_packages(Node::pointer_t program);
    String              get_package_filename(char const *package_info);

    void                add_variable(Node::pointer_t variable);
    bool                are_objects_derived_from_one_another(Node::pointer_t derived_class, Node::pointer_t super_class, Node::pointer_t& /*out*/ the_super_class);
    void                assignment_operator(Node::pointer_t expr);
    bool                best_param_match(Node::pointer_t& /*in,out*/ best, Node::pointer_t match);
    bool                best_param_match_derived_from(Node::pointer_t& /*in,out*/ best, Node::pointer_t match);
    void                binary_operator(Node::pointer_t& expr);
    void                break_continue(Node::pointer_t& break_node);
    void                call_add_missing_params(Node::pointer_t call, Node::pointer_t params);
    void                can_instantiate_type(Node::pointer_t expr);
    void                case_directive(Node::pointer_t& case_node);
    void                catch_directive(Node::pointer_t& catch_node);
    bool                check_field(Node::pointer_t link, Node::pointer_t field, int& funcs, Node::pointer_t& /*out*/ resolution, Node::pointer_t params, int const search_flags);
    bool                check_final_functions(Node::pointer_t& function_node, Node::pointer_t& class_node);
    bool                check_function(Node::pointer_t function_node, Node::pointer_t& /*out*/ resolution, String const& name, Node::pointer_t params, int const search_flags);
    int                 check_function_with_params(Node::pointer_t function_node, Node::pointer_t params);
    bool                check_import(Node::pointer_t& child, Node::pointer_t& /*out*/ resolution, String const& name, Node::pointer_t params, int const search_flags);
    void                check_member(Node::pointer_t ref, Node::pointer_t field, Node::pointer_t field_name);
    bool                check_name(Node::pointer_t list, int idx, Node::pointer_t& /*out*/ resolution, Node::pointer_t id, Node::pointer_t params, int const search_flags);
    void                check_super_validity(Node::pointer_t expr);
    void                check_this_validity(Node::pointer_t expr);
    bool                check_unique_functions(Node::pointer_t function_node, Node::pointer_t class_node, bool const all_levels);
    void                class_directive(Node::pointer_t& class_node);
    Node::pointer_t     class_of_member(Node::pointer_t parent);
    bool                compare_parameters(Node::pointer_t& lfunction, Node::pointer_t& rfunction);
    void                declare_class(Node::pointer_t class_node);
    void                default_directive(Node::pointer_t& default_node);
    bool                define_function_type(Node::pointer_t func);
    void                directive(Node::pointer_t& directive);
    Node::pointer_t     directive_list(Node::pointer_t directive_list);
    void                do_directive(Node::pointer_t& do_node);
    void                enum_directive(Node::pointer_t& enum_node);
    void                expression(Node::pointer_t expr, Node::pointer_t params = Node::pointer_t());
    bool                expression_new(Node::pointer_t expr);
    void                extend_class(Node::pointer_t class_node, bool const extend, Node::pointer_t extend_name);
    void                finally(Node::pointer_t& finally_node);
    bool                find_any_field(Node::pointer_t link, Node::pointer_t field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags);
    Node::depth_t       find_class(Node::pointer_t class_type, Node::pointer_t type, Node::depth_t depth);
    bool                find_external_package(Node::pointer_t import, String const& name, Node::pointer_t& /*out*/ program_node);
    bool                find_field(Node::pointer_t link, Node::pointer_t field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags);
    bool                find_final_functions(Node::pointer_t& function, Node::pointer_t& super);
    bool                find_in_extends(Node::pointer_t link, Node::pointer_t field, int& funcs, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags);
    void                find_labels(Node::pointer_t function, Node::pointer_t node);
    bool                find_member(Node::pointer_t member, Node::pointer_t& /*out*/ resolution, Node::pointer_t params, int search_flags);
    bool                find_overloaded_function(Node::pointer_t class_node, Node::pointer_t function);
    Node::pointer_t     find_package(Node::pointer_t list, String const& name);
    bool                find_package_item(Node::pointer_t program, Node::pointer_t import, Node::pointer_t& /*out*/ resolution, String const& name, Node::pointer_t params, int const search_flags);
    void                for_directive(Node::pointer_t& for_node);
    bool                funcs_name(int& funcs, Node::pointer_t resolution, bool const increment = true);
    void                function(Node::pointer_t function_node);
    bool                get_attribute(Node::pointer_t node, Node::attribute_t const a);
    unsigned long       get_attributes(Node::pointer_t& node);
    search_error_t      get_err_flags() const { return f_err_flags; }
    void                goto_directive(Node::pointer_t& goto_node);
    bool                has_abstract_functions(Node::pointer_t class_node, Node::pointer_t list, Node::pointer_t& /*out*/ func);
    void                identifier_to_attrs(Node::pointer_t node, Node::pointer_t a);
    void                if_directive(Node::pointer_t& if_node);
    void                import(Node::pointer_t& import);
    bool                is_constructor(Node::pointer_t func, Node::pointer_t& the_class);
    bool                is_derived_from(Node::pointer_t derived_class, Node::pointer_t super_class);
    bool                is_dynamic_class(Node::pointer_t class_node);
    bool                is_function_abstract(Node::pointer_t function);
    bool                is_function_overloaded(Node::pointer_t class_node, Node::pointer_t function);
    void                link_type(Node::pointer_t type);
    Node::depth_t       match_type(Node::pointer_t t1, Node::pointer_t t2);
    void                node_to_attrs(Node::pointer_t node, Node::pointer_t a);
    void                object_literal(Node::pointer_t expr);
    void                parameters(Node::pointer_t parameters_node);
    void                prepare_attributes(Node::pointer_t node);
    void                print_search_errors(const Node::pointer_t name);
    void                program(Node::pointer_t program_node);
    bool                replace_constant_variable(Node::pointer_t& /*in,out*/ replace, Node::pointer_t resolution);
    bool                resolve_call(Node::pointer_t call);
    bool                resolve_field(Node::pointer_t object, Node::pointer_t field, Node::pointer_t& resolution, Node::pointer_t params, int const search_flags);
    void                resolve_internal_type(Node::pointer_t parent, char const *type, Node::pointer_t& /*out*/ resolution);
    void                resolve_member(Node::pointer_t expr, Node::pointer_t params, int const search_flags);
    bool                resolve_name(Node::pointer_t list, Node::pointer_t id, Node::pointer_t& /*out*/ resolution, Node::pointer_t params, int const search_flags);
    Node::pointer_t     return_directive(Node::pointer_t return_node);
    bool                select_best_func(Node::pointer_t params, Node::pointer_t& /*out*/ resolution);
    void                set_err_flags(search_error_t flags) { f_err_flags = flags; }
    bool                special_identifier(Node::pointer_t expr);
    void                switch_directive(Node::pointer_t& switch_node);
    void                throw_directive(Node::pointer_t& throw_node);
    void                try_directive(Node::pointer_t& try_node);
    void                type_expr(Node::pointer_t expr);
    void                unary_operator(Node::pointer_t expr);
    void                use_namespace(Node::pointer_t& use_namespace_node);
    void                var(Node::pointer_t var_node);
    void                variable(Node::pointer_t variable_node, bool side_effects_only);
    void                variable_to_attrs(Node::pointer_t node, Node::pointer_t var);
    void                while_directive(Node::pointer_t& while_node);
    void                with(Node::pointer_t& with_node);

    time_t                      f_time = 0;     // time when the compiler is created
    Options::pointer_t          f_options;
    Node::pointer_t             f_program;
    InputRetriever::pointer_t   f_input_retriever;
    search_error_t              f_err_flags = 0;// when searching a name and it doesn't get resolve, emit these errors
    Node::pointer_t             f_scope;        // with() and use namespace list
    module_map_t                f_modules;      // already loaded files (external modules)
};





}
// namespace as2js
#endif
// #ifndef AS2JS_COMPILER_H

// vim: ts=4 sw=4 et
