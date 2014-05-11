#ifndef AS2JS_COMPILER_H
#define AS2JS_COMPILER_H
/* compiler.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

/*

Copyright (c) 2005-2014 Made to Order Software Corp.

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

//#include    "as2js/as2js.h"
#include    "as2js/optimizer.h"


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

                                Compiler(InputRetriever *input);
    virtual                     ~Compiler();

    static Compiler *           CreateCompiler(InputRetriever *retriever);
    static const char *         Version(void);

    virtual InputRetriever *    SetInputRetriever(InputRetriever *retriever);
    virtual void                SetErrorStream(ErrorStream& error_stream);
    virtual void                SetOptions(Options& options);
    virtual int                 Compile(Node::pointer_t& root);

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
        SEARCH_ERROR_EXPECTED_STATIC_MEMBER = 0x00000040,

        SEARCH_ERROR_last = 0
    };

    enum search_flag_t
    {
        SEARCH_FLAG_NO_PARSING              = 0x00000001,    // avoid parsing variables
        SEARCH_FLAG_GETTER                  = 0x00000002,    // accept getters (reading)
        SEARCH_FLAG_SETTER                  = 0x00000004,    // accept setters (writing)
        SEARCH_FLAG_PACKAGE_MUST_EXIST      = 0x00000008,    // weather the package has to exist

        SEARCH_FLAG_last = 0
    };

    class rc_t
    {
    public:
                rc_t()
                    : f_f(0);
                {
                    f_filename[0] = '\0';
                }
                ~rc_t()
                {
                    Close();
                }
        void        FindRC(const String& home, bool accept_if_missing);
        void        ReadRC(void);
        void        Close(void)
                {
                    if(f_f != 0) {
                        fclose(f_f);
                        f_f = 0;
                    }
                }

        const String&    GetPath(void) const
                {
                    return f_path;
                }
        const String&    GetDB(void) const
                {
                    return f_db;
                }

    private:
        FILE *          f_f;
        String          f_filename[256];
        String          f_path;
        String          f_db;
    };

    struct module_t
    {
        String          f_filename;
        Node::pointer_t f_node;
    };

    // automate the restoration of the error flags
    class RestoreFlags
    {
    public:
                RestoreFlags(Compiler *compiler)
                {
                    f_compiler = compiler;
                    f_org_flags = f_compiler->GetErrFlags();
                    f_compiler->SetErrFlags(0);
                }
                ~RestoreFlags()
                {
                    f_compiler->SetErrFlags(f_org_flags);
                }

    private:
        Compiler *  f_compiler;
        int         f_org_flags;
    };


    // functions used to load the internal imports
    void                InternalImports(void);
    Node::pointer_t     LoadModule(const char *module, const char *file);
    void                LoadInternalPackages(const char *module);
    void                ReadDB(void);
    void                WriteDB(void);
    static bool         isspace(int c);
    bool                FindModule(const String& filename, Node::pointer_t& n);
    const char *        FindElement(const String& package_name, const String& element_name, Node::pointer_t *element, const char *type);
    void                FindPackages_AddDatabaseEntry(const String& package_name, Node::pointer_t& element, const char *type);
    void                FindPackages_SavePackageElements(Node::pointer_t& package, const String& package_name);
    void                FindPackages_DirectiveList(Node::pointer_t& list);
    void                FindPackages(Node::pointer_t& program);
    String              GetPackageFilename(const char *package_info);

    void                AddVariable(Node::pointer_t& variable);
    bool                AreObjectsDerivedFromOneAnother(Node::pointer_t& derived_class, Node::pointer_t& super_class, Data *& data);
    void                AssignmentOperator(Node::pointer_t& expr);
    bool                BestParamMatch(Node::pointer_t& best, Node::pointer_t& match);
    bool                BestParamMatchDerivedFrom(Node::pointer_t& best, Node::pointer_t& match);
    void                BinaryOperator(Node::pointer_t& expr);
    void                BreakContinue(Node::pointer_t& break_node);
    void                CallAddMissingParams(Node::pointer_t& call, Node::pointer_t& params);
    void                CanInstantiateType(Node::pointer_t& expr);
    void                Case(Node::pointer_t& case_node);
    void                Catch(Node::pointer_t& catch_node);
    bool                CheckField(Node::pointer_t& link, Node::pointer_t& field, int& funcs, Node::pointer_t& resolution, Node::pointer_t *params, int search_flags);
    bool                CheckFinalFunctions(Node::pointer_t& function, Node::pointer_t& class_node);
    bool                CheckFunction(Node::pointer_t& func, Node::pointer_t& resolution, const String& name, Node::pointer_t *params, int search_flags);
    int                 CheckFunctionWithParams(Node::pointer_t& func, Node::pointer_t *params);
    bool                CheckImport(Node::pointer_t& child, Node::pointer_t& resolution, const String& name, Node::pointer_t *params, int search_flags);
    void                CheckMember(Node::pointer_t& obj, Node::pointer_t& field, Node::pointer_t& field_name);
    bool                CheckName(Node::pointer_t& list, int idx, Node::pointer_t& resolution, Node::pointer_t& id, Node::pointer_t *params, int search_flags);
    void                CheckSuperValidity(Node::pointer_t& expr);
    void                CheckThisValidity(Node::pointer_t& expr);
    bool                CheckUniqueFunctions(Node::pointer_t& function, Node::pointer_t& class_node, bool all_levels);
    void                Class(Node::pointer_t& class_node);
    Node::pointer_t     ClassOfMember(Node::pointer_t parent, Data *& data);
    bool                CompareParameters(Node::pointer_t& lfunction, Node::pointer_t& rfunction);
    void                DeclareClass(Node::pointer_t& class_node);
    void                Default(Node::pointer_t& default_node);
    bool                DefineFunctionType(Node::pointer_t& func);
    void                Directive(Node::pointer_t& directive);
    Node::pointer_t     DirectiveList(Node::pointer_t& directive_list);
    void                Do(Node::pointer_t& do_node);
    void                enum_directive(Node::pointer_t& enum_node);
    void                expression(Node::pointer_t expr, Node::pointer_t params = Node::pointer_t());
    bool                expression_new(Node::pointer_t expr);
    void                ExtendClass(Node::pointer_t& class_node, Node::pointer_t& extend_name);
    void                Finally(Node::pointer_t& finally_node);
    bool                FindAnyField(Node::pointer_t& link, Node::pointer_t& field, int& funcs, Node::pointer_t& resolution, Node::pointer_t *params, int search_flags);
    int                 FindClass(Node::pointer_t& class_type, Node::pointer_t& type, int depth);
    bool                FindExternalPackage(Node::pointer_t& import, const String& name, Node::pointer_t& program);
    bool                FindField(Node::pointer_t& link, Node::pointer_t& field, int& funcs, Node::pointer_t& resolution, Node::pointer_t *params, int search_flags);
    bool                FindFinalFunctions(Node::pointer_t& function, Node::pointer_t& super);
    bool                FindInExtends(Node::pointer_t& link, Node::pointer_t& field, int& funcs, Node::pointer_t& resolution, Node::pointer_t *params, int search_flags);
    void                FindLabels(Node::pointer_t& function, Node::pointer_t& node);
    bool                FindMember(Node::pointer_t& member, Node::pointer_t& resolution, Node::pointer_t *params, int search_flags);
    bool                FindOverloadedFunction(Node::pointer_t& class_node, Node::pointer_t& function);
    Node::pointer_t     FindPackage(Node::pointer_t& list, const String& name);
    bool                FindPackageItem(Node::pointer_t& program, Node::pointer_t& import, Node::pointer_t& resolution, const String& name, Node::pointer_t *params, int search_flags);
    void                For(Node::pointer_t& for_node);
    bool                FuncsName(int& funcs, Node::pointer_t& resolution, bool increment = true);
    void                Function(Node::pointer_t& parameters);
    unsigned long       GetAttributes(Node::pointer_t& node);
    int                 GetErrFlags(void) const { return f_err_flags; }
    void                Goto(Node::pointer_t& goto_node);
    bool                HasAbstractFunctions(Node::pointer_t& class_node, Node::pointer_t& list, Node::pointer_t& func);
    void                IdentifierToAttrs(Node::pointer_t& node, Node::pointer_t& a, unsigned long& attrs);
    void                If(Node::pointer_t& if_node);
    void                Import(Node::pointer_t& import);
    bool                IsConstructor(Node::pointer_t& func);
    bool                IsDerivedFrom(Node::pointer_t& derived_class, Node::pointer_t& super_class);
    bool                IsDynamicClass(Node::pointer_t& class_node);
    bool                IsFunctionAbstract(Node::pointer_t& function);
    bool                IsFunctionOverloaded(Node::pointer_t& class_node, Node::pointer_t& function);
    void                LinkType(Node::pointer_t& type);
    int                 MatchType(Node::pointer_t& t1, Node::pointer_t t2, int match);
    void                NodeToAttrs(Node::pointer_t& node, Node::pointer_t& a, unsigned long& attrs);
    void                ObjectLiteral(Node::pointer_t& expr);
    void                Parameters(Node::pointer_t& parameters);
    void                PrintSearchErrors(const Node::pointer_t& name);
    void                Program(Node::pointer_t& program);
    bool                ReplaceConstantVariable(Node::pointer_t& replace, Node::pointer_t& resolution);
    bool                ResolveCall(Node::pointer_t& call);
    bool                ResolveField(Node::pointer_t& object, Node::pointer_t& field, Node::pointer_t& resolution, Node::pointer_t *params, int search_flags);
    void                ResolveInternalType(Node::pointer_t& parent, const char *type, Node::pointer_t& resolution);
    void                ResolveMember(Node::pointer_t& expr, Node::pointer_t *params, int search_flags);
    bool                ResolveName(Node::pointer_t list, Node::pointer_t& id, Node::pointer_t& resolution, Node::pointer_t *params, int search_flags);
    Node::pointer_t     Return(Node::pointer_t& return_node);
    bool                SelectBestFunc(Node::pointer_t *params, Node::pointer_t& resolution);
    void                SetAttr(Node::pointer_t& node, unsigned long& list_attrs, unsigned long set, unsigned long group, const char *names);
    void                SetErrFlags(int flags) { f_err_flags = flags; }
    bool                SpecialIdentifier(Node::pointer_t& expr);
    void                Switch(Node::pointer_t& switch_node);
    void                Throw(Node::pointer_t& throw_node);
    void                Try(Node::pointer_t& try_node);
    void                TypeExpr(Node::pointer_t& expr);
    void                UnaryOperator(Node::pointer_t& expr);
    void                UseNamespace(Node::pointer_t& use_namespace);
    void                Var(Node::pointer_t& var);
    void                Variable(Node::pointer_t& variable, bool side_effects_only);
    void                VariableToAttrs(Node::pointer_t& node, Node::pointer_t& var, unsigned long& attrs);
    void                While(Node::pointer_t& while_node);
    void                With(Node::pointer_t& with);

    // The following globals are read only once and you can compile
    // many times without having to reload them.
    //
    // the resource file information
    static rc_t             g_rc;

    // the global imports (those which are automatic and
    // define the intrinsic functions and types of the language)
    static Node::pointer_t  g_global_import;

    // the system imports (this is specific to the system you
    // are using this compiler for; it defines the system)
    static Node::pointer_t  g_system_import;

    // the native imports (this is specific to your system
    // environment, it defines objects in your environment)
    static Node::pointer_t  g_native_import;

    String                  f_home;        // $HOME value
    ErrorStream             f_default_error_stream;
    ErrorStream *           f_error_stream;
    Optimizer::pointer_t    f_optimizer;
    Options::pointer_t      f_options;
    InputRetriever *        f_input_retriever;
    Node::pointer_t         f_program;
    time_t                  f_time;        // time when the compiler is created
    int                     f_err_flags;    // when searching a name and it doesn't get resolve, emit these errors
    Node::pointer_t         f_scope;    // with() and use namespace list
    FILE *                  f_db;
    size_t                  f_db_size;
    char *                  f_db_data;
    size_t                  f_db_count;    // valid entries
    size_t                  f_db_max;    // total # of slots available
    char **                 f_db_packages;
    size_t                  f_mod_count;
    size_t                  f_mod_max;
    module_t *              f_modules;    // already loaded files (modules)
};





}
// namespace as2js
#endif
// #ifndef AS2JS_COMPILER_H

// vim: ts=4 sw=4 et
