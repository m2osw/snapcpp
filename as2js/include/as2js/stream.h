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

#include    "string.h"
#include    "position.h"


namespace as2js
{



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
    typedef as_char_t       char_t;
    typedef ssize_t         input_size_t;

    static char_t const     AS_EOF = -1;

                            Input();
    virtual                 ~Input();

    void                    set_filename(String const& filename);
    Position const&         get_position() const;

    virtual char_t          getc() = 0;

    // return the size if known, -1 if unknown
    virtual input_size_t    get_size() const;

private:
    Position                f_position;
};







class FileInput : public Input
{
public:
                            FileInput();
    virtual                 ~FileInput();

    bool                    standard_input();
    bool                    open(std::string const& filename);
    void                    close();

    virtual char_t          getc();
    virtual input_size_t    get_size() const;

    void                    set_original_filename(String const& original_filename);

protected:
    std::string             f_original_filename;
    FILE *                  f_file;
    ssize_t                 f_size;
};


class FileUCS32Input : public FileInput
{
public:
    virtual char_t          getc();
};



class StringInput : public Input
{
public:
                            StringInput();
    virtual                 ~StringInput();

    void                    set(QString const& str, Position::counter_t line);

    virtual char_t          getc();
    virtual input_size_t    get_size() const;

private:
    String::size_type       f_pos;
    String                  f_str;
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
    virtual                         ~InputRetriever() {}

    virtual std::shared_ptr<Input>  Retrieve(String const& filename) = 0;
};




}
// namespace as2js
#endif
// #ifndef AS2JS_STREAM_H

// vim: ts=4 sw=4 et
