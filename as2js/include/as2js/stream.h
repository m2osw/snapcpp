#ifndef AS2JS_STREAM_H
#define AS2JS_STREAM_H
/* stream.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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



//#include    <string>
//#include    <vector>


namespace as2js
{


class Position
{
public:
    typedef int32_t     counter_t;
    typedef controlled_vars::auto_init<counter_t, 0>    zcounter_t;

    void                reset_counters(counter_t line = 1)
                        {
                            f_page = 1;
                            f_page_line = 1;
                            f_paragraph = 1;
                            f_line = line;
                        }

    void                next_line();

    void                new_page()
                        {
                            ++f_page;
                            f_page_line = 1;
                            f_paragraph = 1;
                        }

    void                new_paragraph()
                        {
                            ++f_paragraph;
                        }

    void                new_line()
                        {
                            ++f_page_line;
                            ++f_line;
                        }

    counter_t           page() const
                        {
                            return f_page;
                        }

    counter_t           page_line() const
                        {
                            return f_page_line;
                        }

    counter_t           paragraph() const
                        {
                            return f_paragraph;
                        }

    virtual counter_t   line() const
                        {
                            return f_line;
                        }

private:
    zcounter_t          f_page;
    zcounter_t          f_page_line;
    zcounter_t          f_paragraph;
    zcounter_t          f_line;
};



// I/O interface that YOU have to derive from so the
// parser can read the input data from somewhere
// You need to implement the GetC() function. You can
// also overload the Error() function so it prints
// the errors in a console of your choice.
// The GetFilename() is used by the default Error()
// function. It is used to generate an error like gcc.
// That function returns "asc" by default.
//
// Two examples are available below. One reads a USC-4
// formatted file and the other reads a string.
class Input
{
public:
    typedef long        char_t;

    static char_t const AS_EOF = -1;

                        Input();
    virtual             ~Input();

    virtual char_t      GetC() = 0;

    // return the size if known, -1 if unknown
    virtual long        GetSize() const;

};







class FileInput : public Input
{
public:
                FileInput(void);
    virtual            ~FileInput();
    virtual const char *    GetFilename(void) const;
    bool            StandardInput(void);
    bool            Open(const char *filename);
    void            Close(void);
    virtual long        GetC(void);
    virtual long        GetSize(void) const;
    void            SetOriginalFilename(const char *original_filename);

protected:
    char *            f_filename;
    char *            f_original_filename;
    FILE *            f_file;
    long            f_size;
};


class FileUCS32Input : public FileInput
{
public:
    virtual long        GetC(void);
};



class StringInput : public Input
{
public:
                StringInput(const char *filename = 0);
    virtual            ~StringInput();

    void            Set(const long *str, long size, unsigned long line);
    long            GetSize(void) const;
    virtual long        GetC(void);

    virtual const char *    GetFilename(void) const;

private:
    int            f_pos;
    String            f_str;
    const char *        f_filename;
};



// In order to support different types of file systems, the
// compiler supports a file retriever. Any time a file is
// opened, it calls the retriever (if defined) and uses
// that file. If no retriever was defined, the default is
// used: attempt to open the file with fopen() or an equivalent.
// In particular, this is used to handle the internal ASC files.
class InputRetriever
{
public:
    virtual            ~InputRetriever() {}

    virtual Input *        Retrieve(const char *filename) = 0;
};




}
// namespace as2js
#endif
// #ifndef AS2JS_AS_H

// vim: ts=4 sw=4 et
