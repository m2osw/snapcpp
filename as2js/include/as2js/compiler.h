#ifndef AS2JS_COMPILER_H
#define AS2JS_COMPILER_H
/* compiler.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

/*

Copyright (c) 2005-2009 Made to Order Software Corp.

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


#include    "as2js/as2js.h"
#include    "as2js/optimizer.h"

namespace as2js
{




class IntCompiler : public Compiler
{
public:
                IntCompiler(InputRetriever *input);
    virtual            ~IntCompiler();

    virtual InputRetriever *SetInputRetriever(InputRetriever *retriever);
    virtual void        SetErrorStream(ErrorStream& error_stream);
    virtual void        SetOptions(Options& options);
    virtual int        Compile(NodePtr& root);

private:
    enum {
        MATCH_ANY_ANCESTOR = 0x0001,

        SEARCH_ERROR_PRIVATE            = 0x00000001,
        SEARCH_ERROR_PROTECTED            = 0x00000002,
        SEARCH_ERROR_PROTOTYPE            = 0x00000004,
        SEARCH_ERROR_WRONG_PRIVATE        = 0x00000008,
        SEARCH_ERROR_WRONG_PROTECTED        = 0x00000010,
        SEARCH_ERROR_PRIVATE_PACKAGE        = 0x00000020,
        SEARCH_ERROR_EXPECTED_STATIC_MEMBER    = 0x00000040,
        SEARCH_ERROR_last = 0
    };

    enum {
        SEARCH_FLAG_NO_PARSING            = 0x00000001,    // avoid parsing variables
        SEARCH_FLAG_GETTER            = 0x00000002,    // accept getters (reading)
        SEARCH_FLAG_SETTER            = 0x00000004,    // accept setters (writing)
        SEARCH_FLAG_PACKAGE_MUST_EXIST        = 0x00000008,    // weather the package has to exist
        SEARCH_FLAG_last = 0
    };

    class rc_t {
    public:
                rc_t(void)
                {
                    f_f = 0;
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
        FILE *        f_f;
        char        f_filename[256];
        String        f_path;
        String        f_db;
    };

    struct module_t {
        String        f_filename;
        NodePtr        f_node;
    };

    // automate the restoration of the error flags
    class RestoreFlags
    {
    public:
                RestoreFlags(IntCompiler *compiler)
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
        IntCompiler *    f_compiler;
        int        f_org_flags;
    };


    // functions used to load the internal imports
    void            InternalImports(void);
    NodePtr            LoadModule(const char *module, const char *file);
    void            LoadInternalPackages(const char *module);
    void            ReadDB(void);
    void            WriteDB(void);
    static bool        isspace(int c);
    bool            FindModule(const String& filename, NodePtr& n);
    const char *        FindElement(const String& package_name, const String& element_name, NodePtr *element, const char *type);
    void            FindPackages_AddDatabaseEntry(const String& package_name, NodePtr& element, const char *type);
    void            FindPackages_SavePackageElements(NodePtr& package, const String& package_name);
    void            FindPackages_DirectiveList(NodePtr& list);
    void            FindPackages(NodePtr& program);
    String            GetPackageFilename(const char *package_info);

    void            AddVariable(NodePtr& variable);
    bool            AreObjectsDerivedFromOneAnother(NodePtr& derived_class, NodePtr& super_class, Data *& data);
    void            AssignmentOperator(NodePtr& expr);
    bool            BestParamMatch(NodePtr& best, NodePtr& match);
    bool            BestParamMatchDerivedFrom(NodePtr& best, NodePtr& match);
    void            BinaryOperator(NodePtr& expr);
    void            BreakContinue(NodePtr& break_node);
    void            CallAddMissingParams(NodePtr& call, NodePtr& params);
    void            CanInstantiateType(NodePtr& expr);
    void            Case(NodePtr& case_node);
    void            Catch(NodePtr& catch_node);
    bool            CheckField(NodePtr& link, NodePtr& field, int& funcs, NodePtr& resolution, NodePtr *params, int search_flags);
    bool            CheckFinalFunctions(NodePtr& function, NodePtr& class_node);
    bool            CheckFunction(NodePtr& func, NodePtr& resolution, const String& name, NodePtr *params, int search_flags);
    int            CheckFunctionWithParams(NodePtr& func, NodePtr *params);
    bool            CheckImport(NodePtr& child, NodePtr& resolution, const String& name, NodePtr *params, int search_flags);
    void            CheckMember(NodePtr& obj, NodePtr& field, NodePtr& field_name);
    bool            CheckName(NodePtr& list, int idx, NodePtr& resolution, NodePtr& id, NodePtr *params, int search_flags);
    void            CheckSuperValidity(NodePtr& expr);
    void            CheckThisValidity(NodePtr& expr);
    bool            CheckUniqueFunctions(NodePtr& function, NodePtr& class_node, bool all_levels);
    void            Class(NodePtr& class_node);
    NodePtr            ClassOfMember(NodePtr parent, Data *& data);
    bool            CompareParameters(NodePtr& lfunction, NodePtr& rfunction);
    void            DeclareClass(NodePtr& class_node);
    void            Default(NodePtr& default_node);
    bool            DefineFunctionType(NodePtr& func);
    void            Directive(NodePtr& directive);
    NodePtr            DirectiveList(NodePtr& directive_list);
    void            Do(NodePtr& do_node);
    void            Enum(NodePtr& enum_node);
    void            Expression(NodePtr& expr, NodePtr *params = 0);
    bool            ExpressionNew(NodePtr& expr);
    void            ExtendClass(NodePtr& class_node, NodePtr& extend_name);
    void            Finally(NodePtr& finally_node);
    bool            FindAnyField(NodePtr& link, NodePtr& field, int& funcs, NodePtr& resolution, NodePtr *params, int search_flags);
    int            FindClass(NodePtr& class_type, NodePtr& type, int depth);
    bool            FindExternalPackage(NodePtr& import, const String& name, NodePtr& program);
    bool            FindField(NodePtr& link, NodePtr& field, int& funcs, NodePtr& resolution, NodePtr *params, int search_flags);
    bool            FindFinalFunctions(NodePtr& function, NodePtr& super);
    bool            FindInExtends(NodePtr& link, NodePtr& field, int& funcs, NodePtr& resolution, NodePtr *params, int search_flags);
    void            FindLabels(NodePtr& function, NodePtr& node);
    bool            FindMember(NodePtr& member, NodePtr& resolution, NodePtr *params, int search_flags);
    bool            FindOverloadedFunction(NodePtr& class_node, NodePtr& function);
    NodePtr            FindPackage(NodePtr& list, const String& name);
    bool            FindPackageItem(NodePtr& program, NodePtr& import, NodePtr& resolution, const String& name, NodePtr *params, int search_flags);
    void            For(NodePtr& for_node);
    bool            FuncsName(int& funcs, NodePtr& resolution, bool increment = true);
    void            Function(NodePtr& parameters);
    unsigned long        GetAttributes(NodePtr& node);
    int            GetErrFlags(void) const { return f_err_flags; }
    void            Goto(NodePtr& goto_node);
    bool            HasAbstractFunctions(NodePtr& class_node, NodePtr& list, NodePtr& func);
    void            IdentifierToAttrs(NodePtr& node, NodePtr& a, unsigned long& attrs);
    void            If(NodePtr& if_node);
    void            Import(NodePtr& import);
    bool            IsConstructor(NodePtr& func);
    bool            IsDerivedFrom(NodePtr& derived_class, NodePtr& super_class);
    bool            IsDynamicClass(NodePtr& class_node);
    bool            IsFunctionAbstract(NodePtr& function);
    bool            IsFunctionOverloaded(NodePtr& class_node, NodePtr& function);
    void            LinkType(NodePtr& type);
    int            MatchType(NodePtr& t1, NodePtr t2, int match);
    void            NodeToAttrs(NodePtr& node, NodePtr& a, unsigned long& attrs);
    void            ObjectLiteral(NodePtr& expr);
    void            Offsets(NodePtr& node);        // that should probably be in the NodePtr realm
    void            Parameters(NodePtr& parameters);
    void            PrintSearchErrors(const NodePtr& name);
    void            Program(NodePtr& program);
    bool            ReplaceConstantVariable(NodePtr& replace, NodePtr& resolution);
    bool            ResolveCall(NodePtr& call);
    bool            ResolveField(NodePtr& object, NodePtr& field, NodePtr& resolution, NodePtr *params, int search_flags);
    void            ResolveInternalType(NodePtr& parent, const char *type, NodePtr& resolution);
    void            ResolveMember(NodePtr& expr, NodePtr *params, int search_flags);
    bool            ResolveName(NodePtr list, NodePtr& id, NodePtr& resolution, NodePtr *params, int search_flags);
    NodePtr            Return(NodePtr& return_node);
    bool            SelectBestFunc(NodePtr *params, NodePtr& resolution);
    void            SetAttr(NodePtr& node, unsigned long& list_attrs, unsigned long set, unsigned long group, const char *names);
    void            SetErrFlags(int flags) { f_err_flags = flags; }
    bool            SpecialIdentifier(NodePtr& expr);
    void            Switch(NodePtr& switch_node);
    void            Throw(NodePtr& throw_node);
    void            Try(NodePtr& try_node);
    void            TypeExpr(NodePtr& expr);
    void            UnaryOperator(NodePtr& expr);
    void            UseNamespace(NodePtr& use_namespace);
    void            Var(NodePtr& var);
    void            Variable(NodePtr& variable, bool side_effects_only);
    void            VariableToAttrs(NodePtr& node, NodePtr& var, unsigned long& attrs);
    void            While(NodePtr& while_node);
    void            With(NodePtr& with);

// The following globals are read only once and you can compile
// many times without having to reload them.
    // the resource file information
    static rc_t        g_rc;

    // the global imports (those which are automatic and
    // define the intrinsic functions and types of the language)
    static NodePtr        g_global_import;

    // the system imports (this is specific to the system you
    // are using this compiler for; it defines the system)
    static NodePtr        g_system_import;

    // the native imports (this is specific to your system
    // environment, it defines objects in your environment)
    static NodePtr        g_native_import;

    const char *        f_home;        // $HOME value
    ErrorStream        f_default_error_stream;
    ErrorStream *        f_error_stream;
    IntOptimizer        f_optimizer;
    Options *        f_options;
    InputRetriever *    f_input_retriever;
    NodePtr            f_program;
    time_t            f_time;        // time when the compiler is created
    int            f_err_flags;    // when searching a name and it doesn't get resolve, emit these errors
    NodePtr            f_scope;    // with() and use namespace list
    FILE *            f_db;
    size_t            f_db_size;
    char *            f_db_data;
    size_t            f_db_count;    // valid entries
    size_t            f_db_max;    // total # of slots available
    char **            f_db_packages;
    size_t            f_mod_count;
    size_t            f_mod_max;
    module_t *        f_modules;    // already loaded files (modules)
};





}
// namespace as2js
#endif
// #ifndef AS2JS_COMPILER_H

// vim: ts=4 sw=4 et
