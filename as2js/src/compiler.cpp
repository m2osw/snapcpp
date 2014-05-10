/* compiler.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/compiler.h"



namespace as2js
{



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


IntCompiler::rc_t    IntCompiler::g_rc;
NodePtr            IntCompiler::g_global_import;
NodePtr            IntCompiler::g_system_import;
NodePtr            IntCompiler::g_native_import;


IntCompiler::IntCompiler(InputRetriever *input)
{
    // TODO: under MS-Windows, we may want to use something such
    //     as a SHGetFolder() call
    f_home = getenv("HOME");

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
    f_mod_count = 0;
    f_mod_max = 0;
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





bool IntCompiler::isspace(int c)
{
    return c == ' ' || c == '\t';
}




void IntCompiler::rc_t::FindRC(const String& home, bool accept_if_missing)
{
    // first try to find a place with a .rc
    // (at this time, these directories are kind of random...)
    static const char *directories[] = {
        // Mainly for Unices
        ".",
        "include/sswf/scripts",
        "@include/sswf/scripts",    // For Win32
        ".sswf",
        "~/.sswf",
        "~/sswf",
        "/etc",
        "/etc/sswf",
        "/usr/include/sswf/scripts",
        "/usr/share/sswf/scripts",
        0
    };

    const char **dir, *d;

    for(dir = directories; *dir != 0; ++dir) {
        d = *dir;
        if(*d == '@') {
            // by default I assume I cannot apply this function properly
            f_filename[0] = '\0';
#ifdef WIN32
            char        fullpath[MAX_PATH * 5];

            fullpath[0] = '\0';
            DWORD sz = GetModuleFileNameA(NULL, fullpath, MAX_PATH * 5);
            if(sz < MAX_PATH * 5) {
                char *s = fullpath + strlen(fullpath);
                while(s > fullpath) {
                    if(s[-1] == '\\') {
                        s--;
                        while(s > fullpath) {
                            if(s[-1] == '\\') {
                                s[0] = '\0';
                                snprintf(f_filename, sizeof(f_filename),
                                    "%s%s\\sswf.rc", fullpath, *dir + 1);
                                break;
                            }
                            s--;
                        }
                        break;
                    }
                    s--;
                }
            }
#endif
        }
        else if(*d == '~') {
            if(home.IsEmpty()) {
                // no valid HOME directory
                continue;
            }
            char buf[256];
            size_t sz = sizeof(buf);
            home.ToUTF8(buf, sz);
            snprintf(f_filename, sizeof(f_filename), "%s/%s/sswf.rc", buf, *dir + 1);
        }
        else {
            snprintf(f_filename, sizeof(f_filename), "%s/sswf.rc", *dir);
        }
        if(f_filename[0] != '\0') {
#ifdef WIN32
            // Win98 can have problems with / in filenames
            char *s = f_filename;
            while(*s != '\0') {
                if(*s == '/') {
                    *s = '\\';
                }
                s++;
            }
#endif
            f_f = fopen(f_filename, "rb");
            if(f_f != 0) {
                break;
            }
        }
    }
    if(f_f == 0) {
        if(!accept_if_missing) {
            fprintf(stderr, "INSTALLATION ERROR: cannot find the sswf.rc file; it is usually put in "
#ifdef WIN32
                "include/sswf/scripts found under the folder of the sswf executable.\n"
#else
                "/etc/sswf/sswf.rc\n"
#endif
                );
            exit(1);
        }
        // if we want everything internal, we'll just use working defaults
        f_path = "include/sswf/scripts";
        f_db = "tmp/asc_packages.db";
        strcpy(f_filename, "internal.rc");
    }
}


void IntCompiler::rc_t::ReadRC(void)
{
    char buf[256], *s, *name, *param;
    int l, line, quote;

    // if f_f is null, we already have the defaults
    // and f_input_retriever is not NULL
    if(f_f == 0) {
        return;
    }

    line = 0;
    while(fgets(buf, sizeof(buf), f_f) != 0) {
        line++;
        s = buf;
        while(isspace(*s)) {
            s++;
        }
        if(*s == '#' || *s == '\n' || *s == '\0') {
            continue;
        }
        name = s;
        while(*s != '\0' && *s != '=' && !isspace(*s)) {
            s++;
        }
        l = s - name;
        while(isspace(*s)) {
            s++;
        }
        if(*s != '=') {
fprintf(stderr, "%s:%d: syntax error; expected an equal sign\n", f_filename, line);
            continue;
        }
        // skip the = and following spaces
        s++;
        while(isspace(*s)) {
            s++;
        }
        if(*s == '"' || *s == '\'') {
            quote = *s++;
            param = s;
            while(*s != '\0' && *s != quote && *s != '\n') {
                s++;
            }
        }
        else {
            param = s;
            while(*s != '\0' && *s != '\n') {
                s++;
            }
        }
        *s = '\0';
        if(l == 7 && strncmp(name, "version", 7) == 0) {
            // TODO: check that we understand this version
            continue;
        }
        if(l == 8 && strncmp(name, "asc_path", 8) == 0) {
//fprintf(stderr, "Got path! [%s]\n", param);
            f_path = param;
            continue;
        }
        if(l == 6 && strncmp(name, "asc_db", 6) == 0) {
            f_db = param;
            continue;
        }
    }
}




String IntCompiler::GetPackageFilename(const char *package_info)
{
    int        cnt;

    cnt = 0;
    while(package_info != '\0') {
        package_info++;
        if(package_info[-1] == ' ') {
            cnt++;
            if(cnt >= 3) {
                break;
            }
        }
    }
    if(*package_info != '"') {
        return "";
    }
    package_info++;
    const char *name = package_info;
    while(*package_info != '"' && *package_info != '\0') {
        package_info++;
    }

    String result;
    result.FromUTF8(name, package_info - name);

    return result;
}


/** \fn InputRetriever::Retrieve(const char *filename) = 0
 *
 * \brief Function you want to overload to provide your own input file to the loader.
 *
 * The IntCompiler::FindModule() function calls this function to retrieve an
 * input file. The input file is used to load the module specified by
 * \p filename.
 *
 * This is used by the libasc extension that enables you to put all the system
 * ASC files in your binary and thus avoid you having to deal with a directory
 * structure.
 *
 * \bug
 * The returned pointer MUST be a class derived from Input and MUST be allocated.
 * If the file cannot be opened by your retriever, then return NULL.
 *
 * \param[in] filename  The name of the module to be loaded
 *
 * \return NULL or an object derived from the Input class. You MUST allocate this object
 * since the IntCompiler::FindModule() will delete it once the module was loaded.
 */

bool IntCompiler::FindModule(const String& filename, NodePtr& result)
{
    int        i, j, p, r;
    module_t    *mod;

    p = 0;        // avoid warnings
    j = f_mod_count;
    if(j < 4) {
        // lack of precision forces us to do this with 0 to 3 elements
        p = 0;
        while(p < j) {
            r = filename.Compare(f_modules[p].f_filename);
            if(r == 0) {
                result = f_modules[p].f_node;
                return true;
            }
            // when filename < f_modules.f_filename,
            // we won't find anything anymore; also
            // p is at the right place to add the
            // file!
            if(r < 0) {
                break;
            }
            p++;
        }
    }
    else {
        // this is really the binary search
        // (see the / 2 below?)
        i = 0;
        while(i < j) {
            p = i + (j - i) / 2;
            r = filename.Compare(f_modules[p].f_filename);
            if(r == 0) {
                result = f_modules[p].f_node;
                return true;
            }
            if(r > 0) {
                p++;
                i = p;
            }
            else {
                j = p;
            }
        }
    }

// we couldn't find this entry, when n != 0 insert a new row
// otherwise try to load the module
    if(!result.HasNode()) {
        FileInput file_input;
        Input *in(NULL);
        char *fn = filename.GetUTF8();
        if(f_input_retriever != NULL) {
            in = f_input_retriever->Retrieve(fn);
        }
        if(in == NULL) {
            if(!file_input.Open(fn)) {
                fprintf(stderr, "FATAL ERROR: cannot open module file \"%s\".\n", fn);
                delete [] fn;
                exit(1);
            }
            in = &file_input;
        }

        Parser *parser = Parser::CreateParser();
        if(f_options != 0) {
            parser->SetOptions(*f_options);
        }
        parser->SetInput(*in);

        result = parser->Parse();

        delete parser;
        if(in != &file_input) {
            // This is ugly, we should have a shared pointer or
            // some other feature to handle that one!
            delete in;
        }

#if 0 && (defined(_DEBUG) || defined(DEBUG))
fprintf(stderr, "%s module:\n", fn);
result.Display(stderr);
#endif

        if(!result.HasNode()) {
            fprintf(stderr, "FATAL ERROR: cannot compile module file \"%s\".\n", fn);
            delete [] fn;
            exit(1);
        }

        delete [] fn;
    }

    // we need to enlarge the f_db_packages buffer
    if(f_mod_count >= f_mod_max) {
        // enlarge buffer
        f_mod_max += 250;
        mod = new module_t[f_mod_max];
        for(i = 0; (size_t) i < f_mod_count; ++i) {
            mod[i] = f_modules[i];
        }
        delete [] f_modules;
        f_modules = mod;
    }

    i = f_mod_count;
    while(i > p) {
        f_modules[i] = f_modules[i - 1];
        i--;
    }
    f_mod_count++;

    f_modules[p].f_filename = filename;
    f_modules[p].f_node = result;

    return true;
}



NodePtr IntCompiler::LoadModule(const char *module, const char *file)
{
    char        buf[256];
    char        path[256];
    size_t        sz;
    NodePtr        result;

    // create the name of the module
    sz = sizeof(path);
    g_rc.GetPath().ToUTF8(path, sz);
    sz = snprintf(buf, sizeof(buf), "%s/%s/%s", path, module, file);
    if(sz >= sizeof(buf)) {
        fprintf(stderr, "FATAL ERROR: filename too long; cannot load module.\n");
        exit(1);
    }

    // search for the module; if not already loaded, load it
    FindModule(buf, result);
    return result;
}


void IntCompiler::ReadDB(void)
{
    Input        *in(NULL);

    if(f_db != 0) {
        fclose(f_db);
        f_db = 0;
    }

// the input retriever defines the packages database?
    if(f_input_retriever != NULL) {
        // if we have a retriever query the retriever for the database
        // in this case the database is read only (i.e. f_db remains set
        // to zero)
        in = f_input_retriever->Retrieve("asc_packages.db");
    }
    if(in != NULL) {
        f_db_size = in->GetSize();
        delete [] f_db_data;
        f_db_data = new char[f_db_size + 2];
        // TODO: we do not currently support UTF-8 here, only ASCII...
        for(size_t i = 0; i < f_db_size; ++i) {
            f_db_data[i] = in->GetC();
        }
        // we're done with the input
        delete in;

        f_db_data[f_db_size] = '\0';
    }
    else {
// get resource filename if any
        String db = g_rc.GetDB();
        if(db.GetLength() == 0) {
            // internal default
            db = "~/.sswf/asc_packages.db";
        }

// check for user path
        const long *path = db.Get();
        long len = db.GetLength();
        if(len > 1
        && path[0] == '~'
        && (path[1] == '/' || path[1] == '\\')) {
            String s = f_home;
            s.AppendStr(path + 1, len - 1);
            db = s;
        }

// open the file
        char filename[256];
        size_t sz = sizeof(filename);
        db.ToUTF8(filename, sz);
        f_db = fopen(filename, "rb+");
        if(f_db == 0) {
            // if it fails, try to create possibly missing directories
            char *fn;
            fn = filename;
            while(*fn != '\0') {
                if(*fn == '/' || *fn == '\\') {
                    char c = *fn;
                    *fn = '\0';
#ifdef WIN32
                    mkdir(filename);
#else
                    mkdir(filename, 0700);
#endif
                    *fn = c;
                    do {
                        fn++;
                    } while(*fn == '/' || *fn == '\\');
                }
                else {
                    fn++;
                }
            }
            // now try again to open the file...
            f_db = fopen(filename, "wb+");
            if(f_db == 0) {
                // we can't create the database!
                fprintf(stderr, "FATAL ERROR: can't open or create database file \"%s\" for package information.\n", filename);
                exit(1);
            }
        }

// read the file at once [we need it this way to do binary searches]
// mmap would be a pain to manage since we would need to remap all the
// time whenever we insert new lines in the DB.
        fseek(f_db, 0, SEEK_END);
        f_db_size = ftell(f_db);
        fseek(f_db, 0, SEEK_SET);

        if(f_db_size == 0) {
            // when the file just got created, print out a comment
            fprintf(f_db, "# Database of SSWF ActionScript Compiler (asc)\n");
            fprintf(f_db, "# DO NOT EDIT UNLESS YOU KNOW WHAT YOU ARE DOING\n");
            fprintf(f_db, "# Copyright (c) 2005-2009 by Made to Order Software Corp.\n");
            fprintf(f_db, "# WARNING: package names below MUST be sorted\n");
            fprintf(f_db, "# This file is written in UTF-8\n");
            fprintf(f_db, "# You can safely modify it with an editor which supports UTF-8\n");
            fprintf(f_db, "# package name + element name + type + file name + line number\n");
            fflush(f_db);
        
            fseek(f_db, 0, SEEK_END);
            f_db_size = ftell(f_db);
            fseek(f_db, 0, SEEK_SET);
        }

        delete [] f_db_data;
        f_db_data = new char[f_db_size + 2];
        if(fread(f_db_data, 1, f_db_size, f_db) != f_db_size) {
            fprintf(stderr, "FATAL ERROR: can't read the database file: \"%s\".\n", filename);
            exit(1);
        }

        f_db_data[f_db_size] = '\0';
    }

// "parse" the file and create the sorted array
// [the file is assumed to already be sorted]
    // count the lines which aren't commented out
    // and we remove empty lines and change new lines to just '\n'
    f_db_count = 0;
    char c, *s, *d;
    s = d = f_db_data;
    c = *s;
    while(c != '\0') {
        // skip leading spaces and empty lines
        // (those are lost)
        while(isspace(*s) || *s == '\n' || *s == '\r') {
            s++;
        }
        // don't count comments (we don't need them in our array)
        if(*s != '#') {
            f_db_count++;
        }
        while(*s != '\n' && *s != '\r' && *s != '\0') {
            *d++ = *s++;
        }
        // We need to skip this here otherwise a comment on
        // the last line generates an error!
        while(*s == '\n' || *s == '\r') {
            s++;
        }
        c = *s;
        // we only want '\n', no '\r\n' nor just '\r'
        *d++ = '\n';
    }
    *d = '\0';
    // f_db_size can now be smaller or 1 byte bigger
    f_db_size = d - f_db_data;
//fprintf(stderr, "Count = %ld\n", (long) f_db_count);

    if(f_db_count < 1000) {
        f_db_max = 1000;
    }
    else {
        f_db_max = f_db_count + 100;
    }

    // save the start of each line which isn't commented
    f_db_packages = new char *[f_db_max];
    s = f_db_data;
    char **p = f_db_packages;
    while(*s != '\0') {
        // ignore comments
        if(*s != '#') {
            *p++ = s;
        }
        // TODO: we really need to check the validity of each
        //     entry and skip invalid lines as comments
        //     go to the next line
        while(*s != '\0') {
            s++;
            if(s[-1] == '\n') {
                break;
            }
        }
    }

// it's ready.
}




void IntCompiler::WriteDB(void)
{
    const char    *s, *start;
    size_t        idx;

// if we are in ReadOnly mode, we simply cannot save the database
    if(f_db == 0) {
        return;
    }

// clear the file
    fseek(f_db, 0, SEEK_SET);
#ifdef WIN32
    _chsize(fileno(f_db), 0);
#else
    if(ftruncate(fileno(f_db), 0) != 0) {
        fprintf(stderr, "ERROR: could not truncate database file.\n");
        return;
    }
#endif

// save all the comments at the start of the file
    s = f_db_data;
    while(*s != '\0') {
        start = s;
        // ignore lines that are not comments
        if(*s != '#') {
            continue;
        }
        while(*s != '\n' && *s != '\0') {
            s++;
        }
        fprintf(f_db, "%.*s\n", static_cast<int>(s - start), start);
        while(*s == '\n') {
            s++;
        }
    }

// now save the list of rows
    for(idx = 0; idx < f_db_count; ++idx) {
        start = s = f_db_packages[idx];
        while(*s != '\n' && *s != '\0') {
            s++;
        }
        fprintf(f_db, "%.*s\n", static_cast<int>(s - start), start);
    }

// make sure it's written on disk
    fflush(f_db);
}



// s2 ends with a space (name from database)
// s1 ends with a null terminator
int pckcmp(const char *s1, const char *s2)
{
    int    r, cnt;

    cnt = 0;
    while(*s1 != '\0' && *s2 != '\n' && *s2 != '\0') {
        if(*s2 == ' ') {
            cnt++;
            if(cnt == 2) {
                break;
            }
            if(*s1 != ' ') {
                r = *s1 - *s2;
                return r < 0 ? -1 : 1;
            }
            s1++;
            s2++;
            if(s1[0] == '*' && s1[1] == '\0') {
                return 0;
            }
            continue;
        }
        r = *s1 - *s2;
        if(r != 0) {
            return r < 0 ? -1 : 1;
        }
        s1++;
        s2++;
    }

    if(*s1 == '\0' && *s2 == ' ') {
        return 0;
    }

    return *s1 != '\0' ? 1 : -1;
}


// Search for a named element:
// <package name>{.<package name>}.<class, function, variable name>
// TODO: add support for '*' in <package name>
const char *IntCompiler::FindElement(const String& package_name, const String& element_name, NodePtr *element, const char *type)
{
    int        i, j, p, r, len;
    size_t        sz;
    char        buf[16], **pck;

    // transform the name in a UTF-8 string as supported by the database
    len = package_name.GetUTF8Length() + element_name.GetUTF8Length();
    if(len < 0) {
        fprintf(stderr, "INTERNAL ERROR: UTF8 convertion failed! (1)\n");
        exit(1);
    }
    len += 3;
#ifdef _MSVC
    // alloca() not available with cl
    class AutoDelete {
    public: AutoDelete(char *ptr) { f_ptr = ptr; }
        ~AutoDelete() { delete f_ptr; }
    private: char *f_ptr;
    };
    char *n = new char[len];
    AutoDelete ad_n(n);
#else
    char n[len];
#endif
    sz = len;
    package_name.ToUTF8(n, sz);
    n[len - sz] = ' ';
    sz--;
    element_name.ToUTF8(n + len - sz, sz);

    // the f_db_packages list of strings are sorted
    p = i = 0;
    j = f_db_count;
    if(j < 4) {
        // lack of precision forces us to do this with 0 to 3 elements
        p = 0;
        while(p < j) {
            r = pckcmp(n, f_db_packages[p]);
            if(r == 0) {
                goto found;
            }
            // when n < f_db_packages, we won't find anything
            if(r < 0) {
                break;
            }
            p++;
        }
    }
    else {
        // this is really the binary search
        // (see the / 2 below?)
        while(i < j) {
            p = i + (j - i) / 2;
            r = pckcmp(n, f_db_packages[p]);
            if(r == 0) {
                goto found;
            }
            if(r > 0) {
                p++;
                i = p;
            }
            else {
                j = p;
            }
        }
    }

// we couldn't find this entry, when type != 0 insert a new row
    if(type == 0) {
        return 0;
    }

    // we need to enlarge the f_db_packages buffer
    if(f_db_count >= f_db_max) {
        // enlarge buffer
        f_db_max += 250;
        pck = new char *[f_db_max];
        memcpy(pck, f_db_packages, sizeof(char *) * f_db_count);
        delete [] f_db_packages;
        f_db_packages = pck;
    }

    if((int) f_db_count - p > 0) {
        memmove(f_db_packages + p + 1, f_db_packages + p, sizeof(char *) * (f_db_count - p));
    }
    f_db_count++;

    // build the string which is:
    // <package name> <element name> <type> "<filename>" <line>
    {
    String entry(package_name);
    entry += " ";
    entry += element_name;
    entry += " ";
    entry += type;
    entry += " \"";
    entry += element->GetFilename();
    entry += "\" ";
    snprintf(buf, sizeof(buf), "%ld", element->GetLine());
    entry += buf;
    entry += "\n";

    len = entry.GetUTF8Length();
    if(len < 0) {
        fprintf(stderr, "INTERNAL ERROR: UTF8 convertion failed! (2)\n");
        exit(1);
    }
    len += 2;
    f_db_packages[p] = new char[len];
    sz = len;
    if(entry.ToUTF8(f_db_packages[p], sz) < 0) {
        fprintf(stderr, "INTERNAL ERROR: UTF8 convertion failed! (3)\n");
        exit(1);
    }
//fflush(stdout);
//fprintf(stderr, "Add [%s]\n", f_db_packages[p]);
    }

    return f_db_packages[p];

found:
    // TODO: if type != 0 we want to test that the filename referenced
    //     is the same as the one in the database
    return f_db_packages[p];
}


void IntCompiler::FindPackages_AddDatabaseEntry(const String& package_name, NodePtr& element, const char *type)
{
    // here, we totally ignore internal, private
    // and false entries right away
    unsigned long attr = GetAttributes(element);
    if((attr & (NODE_ATTR_PRIVATE | NODE_ATTR_FALSE | NODE_ATTR_INTERNAL)) != 0) {
        return;
    }

    Data& data = element.GetData();
    FindElement(package_name, data.f_str, &element, type);
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


void IntCompiler::LoadInternalPackages(const char *module)
{
#ifndef PATH_MAX
#define    PATH_MAX 259
#endif
    char        buf[PATH_MAX], path[PATH_MAX];
    size_t        sz;
    DIR        *dir;

    sz = sizeof(path);
    g_rc.GetPath().ToUTF8(path, sz);
    snprintf(buf, sizeof(buf), "%s/%s", path, module);
    dir = opendir(buf);
    if(dir == 0) {
#ifdef WIN32
        if(buf[0] != '/' && buf[0] != '\\'
        && (((buf[0] < 'a' || buf[0] > 'z')
          && (buf[0] < 'A' || buf[0] > 'Z'))
                || buf[1] != ':')) {
            char fullpath[MAX_PATH * 5];

            fullpath[0] = '\0';
            fullpath[sizeof(fullpath) - 1] = '\0';
            DWORD sz = GetModuleFileNameA(NULL, fullpath, sizeof(fullpath));
            if(sz < MAX_PATH * 5) {
                char *s = fullpath + strlen(fullpath);
                while(s > fullpath) {
                    if(s[-1] == '\\') {
                        s--;
                        while(s > fullpath) {
                            if(s[-1] == '\\') {
                                strncpy(s, buf, (sizeof(fullpath) - 1) - (s - fullpath));
                                dir = opendir(fullpath);
                                break;
                            }
                            s--;
                        }
                        break;
                    }
                    s--;
                }
            }
        }
#endif
        if(dir == 0) {
            fprintf(stderr, "INSTALLATION ERROR: cannot read directory \"%s\".\n", buf);
            exit(1);
        }
    }

    struct dirent *ent;
    while((ent = readdir(dir)) != 0) {
        const char *s = ent->d_name;
        const char *e = 0;
        while(*s != '\0') {
            if(*s == '.') {
                e = s;
            }
            s++;
        }
        // TODO: under MS-Windows and other such file systems,
        //     a filename is not case sensitive
        if(e == 0 || strcmp(e, ".asc") != 0
        || strcmp(ent->d_name, "as_init.asc") == 0) {
            continue;
        }
        // we've got a file of interest
        // TODO: we want to keep this package in RAM since
        //     we already parsed it!
        NodePtr p = LoadModule(module, ent->d_name);
        // now we can search the package in the actual code
        FindPackages(p);
    }

    // avoid leaks
    closedir(dir);
}


void IntCompiler::InternalImports(void)
{
    if(!g_global_import.HasNode())
    {
#if defined(_DEBUG) || defined(DEBUG)
fflush(stdout);
#endif
        // Read the resource file
        g_rc.FindRC(f_home, f_input_retriever != NULL);
        g_rc.ReadRC();
        g_rc.Close();

        g_global_import = LoadModule("global", "as_init.asc");
        g_system_import = LoadModule("system", "as_init.asc");
        g_native_import = LoadModule("native", "as_init.asc");
    }

    ReadDB();

    if(f_db_count == 0) {
        LoadInternalPackages("global");
        LoadInternalPackages("system");
        LoadInternalPackages("native");

        // this saves the internal packages info
        WriteDB();
    }
}



}
// namespace as2js

// vim: ts=4 sw=4 et
