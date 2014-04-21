/* input.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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

#include  "as2js/as2js.h"




namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  ERROR STREAM  ***************************************************/
/**********************************************************************/
/**********************************************************************/

ErrorStream::ErrorStream(void)
{
    // The error counter is never reset.
    // If you need a reset error counter, you probably need a
    // new error stream object.
    f_errcnt = 0;
    //f_node -- auto-init
}


ErrorStream::~ErrorStream()
{
    // necessary for the virtual-ness
}


const String& ErrorStream::IntGetFilename(void) const
{
    if(f_node.HasNode()) {
        return f_node.GetFilename();
    }
    // we need to keep a copy... well! that's just in case
    // you have an error, after all...
    f_filename = GetFilename();
    return f_filename;
}


long ErrorStream::IntLine(void) const
{
    if(f_node.HasNode()) {
        return f_node.GetLine();
    }
    return Line();
}


const char *ErrorStream::GetFilename(void) const
{
    // some default unlikely filename...
    return "asc";
}


long ErrorStream::Line(void) const
{
    // no line number by default
    return 1;
}


void ErrorStream::Error(err_code_t err_code, const char *message)
{
    long line = IntLine();
    if(line <= 0) {
        line = 1;
    }

    const String& filename = IntGetFilename();
#ifdef _MSVC
    // this allocates memory... too bad!
    char *fn = filename.GetUTF8();
#else
    size_t len = filename.GetUTF8Length() + 2;
    char fn[len];
    if(len == 2) {
        fn[0] = '?';
        fn[1] = '\0';
    }
    else {
        filename.ToUTF8(fn, len);
    }
#endif
    // at this time we don't print out the error code...
    fprintf(stderr, "%s:%ld: error: %s\n", fn, line, message);
#ifdef _MSVC
    delete [] fn;
#endif
}


void ErrorStream::ErrMsg(err_code_t err_code, const NodePtr& node, const char *format, ...)
{
    va_list        ap;

    f_node = node;
    va_start(ap, format);
    ErrMsg(err_code, format, ap);
    va_end(ap);
    f_node.ClearNode();
}


void ErrorStream::ErrMsg(err_code_t err_code, const char *format, ...)
{
    va_list        ap;

    va_start(ap, format);
    ErrMsg(err_code, format, ap);
    va_end(ap);
}


void ErrorStream::ErrMsg(err_code_t err_code, const char *format, va_list ap)
{
    int        len;

    f_errcnt++;

#if defined(_MSVC) || defined(__MINGW32__)
    // the *snprintf() functions are broken under Windows
    // and only return the size when your buffer is large
    // enough! (unless this is a BFD behavior?)
    char msg[1024];
    len = vsnprintf(msg, sizeof(msg), format, ap);
    msg[sizeof(msg) - 1] = '\0';
    Error(err_code, msg);
#else
    // first we get the size
    char unused[4];
    len = vsnprintf(unused, 0, format, ap);
    if(len > 0) {
        // size is valid, generate the message
        ++len;
        char msg[len];
        vsnprintf(msg, len, format, ap);
        Error(err_code, msg);
    }
    else {
        // no message... but an error none the less!
        Error(err_code, "?");
    }
#endif
}


void ErrorStream::ErrStrMsg(err_code_t err_code, const NodePtr& node, const char *format, ...)
{
    va_list        ap;

    f_node = node;
    va_start(ap, format);
    ErrStrMsg(err_code, format, ap);
    va_end(ap);
    f_node.ClearNode();
}


void ErrorStream::ErrStrMsg(err_code_t err_code, const char *format, ...)
{
    va_list        ap;

    va_start(ap, format);
    ErrStrMsg(err_code, format, ap);
    va_end(ap);
}


void ErrorStream::ErrStrMsg(err_code_t err_code, const char *format, va_list ap)
{
    long        value;
    double        fvalue;
    void        *ptr;
    bool        flag_long;
    char        buf[256], *msg;
    String        message;

    f_errcnt++;

    while(*format != '\0') {
        if(*format == '%') {
            format++;
            if(*format == '%') {
                message.AppendChar('%');
                format++;
                continue;
            }
            // TODO: add flags (' ', #, *, ., 0, -, +, ', I)
            // TODO: positioning ([1-9]+$)
            // TODO: all sizes (h, hh)
            flag_long = *format == 'l';
            if(flag_long) {
                format++;
            }
            switch(*format) {
            case 'S':
                message += *va_arg(ap, String *);
                break;

            case 'c':
                // WARNING: we assume a valid Unicode char here!
                message.AppendChar(va_arg(ap, int));
                break;

            case 's':
                message += va_arg(ap, char *);
                break;

            case 'p':
                ptr = va_arg(ap, void *);
                snprintf(buf, sizeof(buf), "%p", ptr);
                message += buf;
                break;

            case 'd':
            case 'i':
                if(flag_long) {
                    // this is not like int on 64 bits
                    value = va_arg(ap, long);
                }
                else {
                    value = va_arg(ap, int);
                }
                snprintf(buf, sizeof(buf), "%ld", value);
                message += buf;
                break;

            case 'e':
                fvalue = va_arg(ap, double);
                snprintf(buf, sizeof(buf), "%e", fvalue);
                message += buf;
                break;

            case 'f':
                fvalue = va_arg(ap, double);
                snprintf(buf, sizeof(buf), "%f", fvalue);
                message += buf;
                break;

            case 'g':
                fvalue = va_arg(ap, double);
                snprintf(buf, sizeof(buf), "%g", fvalue);
                message += buf;
                break;

            default:
fprintf(stderr, "INTERNAL ERROR: unsupported format '%c' for ErrStrMsg()\n", *format);
                AS_ASSERT(0);
                break;

            }
        }
        else {
            message.AppendChar(*format);
        }
        format++;
    }

    msg = message.GetUTF8();
    Error(err_code, msg);
    delete [] msg;
}




/**********************************************************************/
/**********************************************************************/
/***  INPUT  **********************************************************/
/**********************************************************************/
/**********************************************************************/

Input::Input(void)
{
    ResetCounters();
}

Input::~Input()
{
    // necessary for the virtual-ness
}


long Input::GetSize(void) const
{
    return -1;
}


/**********************************************************************/
/**********************************************************************/
/***  FILE INPUT  *****************************************************/
/**********************************************************************/
/**********************************************************************/

FileInput::FileInput(void)
{
    f_filename = 0;
    f_original_filename = 0;
    f_file = 0;
    f_size = -1;
}


FileInput::~FileInput()
{
    Close();
}


const char *FileInput::GetFilename(void) const
{
    // in case the file being read wasn't the original
    // (i.e. it was converted with iconv for instance)
    return f_original_filename ? f_original_filename : f_filename;
}


void FileInput::Close(void)
{
    delete [] f_filename;
    delete [] f_original_filename;
    if(f_file != 0 && f_file != stdin) {
        fclose(f_file);
    }

    f_filename = 0;
    f_original_filename = 0;
    f_file = 0;
    f_size = -1;

    ResetCounters();
}


bool FileInput::StandardInput(void)
{
    Close();

    f_file = stdin;

    if(f_file != 0) {
        f_filename = new char[2];
        f_filename[0] = '-';
        f_filename[1] = '\0';
    }

    return true;
}


bool FileInput::Open(const char *filename)
{
    Close();

    f_file = fopen(filename, "rb");
#ifdef WIN32
    if(f_file == 0) {
        // Using 'new' just because cl doesn't know how to dynamically
        // allocate a buffer on the stack...
        char *fullpath = new char[MAX_PATH * 5 + strlen(filename) + 1];

        fullpath[0] = '\0';
        DWORD sz = GetModuleFileNameA(NULL, fullpath, MAX_PATH * 5);
        if(sz < MAX_PATH * 5) {
            char *s = fullpath + strlen(fullpath);
            while(s > fullpath) {
                if(s[-1] == '\\') {
                    s--;
                    while(s > fullpath) {
                        if(s[-1] == '\\') {
                            strcpy(s, filename);
                            f_file = fopen(fullpath, "rb");
                            break;
                        }
                        s--;
                    }
                    break;
                }
                s--;
            }
        }
        delete [] fullpath;
    }
#endif
    if(f_file != 0) {
        int len = strlen(filename) + 1;
        f_filename = new char[len];
        memcpy(f_filename, filename, len);

#ifndef WIN32
        if(!isatty(fileno(f_file))) {
#endif
            fseek(f_file, 0, SEEK_END);
            f_size = ftell(f_file);
            fseek(f_file, 0, SEEK_SET);
#ifndef WIN32
        }
#endif
    }

    return f_file != 0;
}


long FileInput::GetSize(void) const
{
    return f_size;
}


long FileInput::GetC(void)
{
    if(f_file == 0) {
        return AS_EOF;
    }

    char q[1];
    if(fread(q, sizeof(q), 1, f_file) != 1) {
        return AS_EOF;
    }

    // we assume ISO-8859-1
    return q[0];
}


void FileInput::SetOriginalFilename(const char *original_filename)
{
    if(f_file == 0) {
        return;
    }
    delete [] f_original_filename;
    if(original_filename != 0) {
        int len = strlen(original_filename) + 1;
        f_original_filename = new char[len];
        memcpy(f_original_filename, original_filename, len);
    }
}


long FileUCS32Input::GetC(void)
{
    if(f_file == 0) {
        return AS_EOF;
    }

    char q[4];
    if(fread(q, sizeof(q), 1, f_file) != 1) {
        return AS_EOF;
    }

    long c = (q[0] << 24) | (q[1] << 16) | (q[2] << 8) | q[3];

    // all negative values are invalid Unicode
    // at this time... So is 0xFFFF.
    if(c < 0) {
        return 0x0000FFFF;
    }

    return c;
}


/**********************************************************************/
/**********************************************************************/
/***  STRING INPUT  ***************************************************/
/**********************************************************************/
/**********************************************************************/

StringInput::StringInput(const char *filename)
{
    f_pos = 0;
    //f_str -- auto-init
    f_filename = filename;
}


StringInput::~StringInput()
{
    // required because of the virtual-ness
}


void StringInput::Set(const long *str, long size, unsigned long line)
{
    ResetCounters(line);

    f_pos = 0;
    f_str.Set(str, size);
}


long StringInput::GetC(void)
{
    long        c;

    if(f_pos < f_str.GetLength()) {
        c = f_str.Get()[f_pos];
        ++f_pos;
    }
    else {
        c = AS_EOF;
    }

    return c;
}


const char *StringInput::GetFilename(void) const
{
    if(f_filename) {
        return f_filename;
    }

    return Input::GetFilename();
}


// returns the number of characters (UCS-4)
long StringInput::GetSize(void) const
{
    return f_str.GetLength();
}



}
// namespace as2js

// vim: ts=4 sw=4 et
