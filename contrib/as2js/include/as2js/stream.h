#ifndef AS2JS_STREAM_H
#define AS2JS_STREAM_H
/* as2js/stream.h -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "string.h"
#include    "position.h"

#include    <memory>
#include    <vector>
#include    <fstream>


namespace as2js
{


class DecodingFilter
{
public:
    typedef std::shared_ptr<DecodingFilter>     pointer_t;
    typedef unsigned char                       byte_t;

    void                    putc(byte_t c);
    as_char_t               getc();

protected:
    virtual as_char_t       get_char() = 0;

    std::vector<byte_t>     f_buffer;
};


class DecodingFilterISO88591 : public DecodingFilter
{
protected:
    virtual as_char_t       get_char();
};


class DecodingFilterUTF8 : public DecodingFilter
{
protected:
    virtual as_char_t       get_char();
};


class DecodingFilterUTF16 : public DecodingFilter
{
protected:
    as_char_t               next_char(as_char_t c);

private:
    as_char_t               f_lead_surrogate = 0;
};


class DecodingFilterUTF16LE : public DecodingFilterUTF16
{
protected:
    virtual as_char_t       get_char();
};


class DecodingFilterUTF16BE : public DecodingFilterUTF16
{
protected:
    virtual as_char_t       get_char();
};


class DecodingFilterUTF32LE : public DecodingFilter
{
protected:
    virtual as_char_t       get_char();
};


class DecodingFilterUTF32BE : public DecodingFilter
{
protected:
    virtual as_char_t       get_char();
};


class DecodingFilterDetect : public DecodingFilter
{
protected:
    virtual as_char_t       get_char();

private:
    DecodingFilter::pointer_t   f_filter;
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
    typedef std::shared_ptr<Input>                  pointer_t;
    typedef as_char_t                               char_t;

    static char_t const     INPUT_EOF = -1;  // end of file
    static char_t const     INPUT_NAC = -2;  // not a character (filter requires more input)
    static char_t const     INPUT_ERR = -3;  // stream error

    Position&               get_position();
    Position const&         get_position() const;

    char_t                  getc();
    void                    ungetc(char_t c);

protected:
                            Input(DecodingFilter::pointer_t filter = DecodingFilter::pointer_t(new DecodingFilterDetect));
    virtual                 ~Input() {}

    virtual char_t          filter_getc();
    virtual char_t          get_byte(); // get_byte() is not abstract because the deriving class may instead redefine filter_getc()

private:
    DecodingFilter::pointer_t   f_filter;
    Position                    f_position;
    std::vector<char_t>         f_unget;
};





class StandardInput : public Input
{
public:
    typedef std::shared_ptr<StandardInput>          pointer_t;

                            StandardInput();

protected:
    virtual char_t          get_byte();
};





class FileInput : public Input
{
public:
    typedef std::shared_ptr<FileInput>              pointer_t;

    bool                    open(String const& filename);

protected:
    virtual char_t          get_byte();

    std::ifstream           f_file;
};





class StringInput : public Input
{
public:
    typedef std::shared_ptr<StringInput>            pointer_t;

                            StringInput(String const& str, Position::counter_t line = 1);

protected:
    virtual char_t          filter_getc();

private:
    String                  f_str;
    String::size_type       f_pos = 0;
};



// In order to support different types of file systems, the
// compiler supports a file retriever. Any time a file is
// opened, it calls the retriever (if defined) and uses
// that file. If no retriever was defined, the default is
// used: attempt to open the file with FileInput().
// In particular, this is used to handle the external definitions.
class InputRetriever
{
public:
    typedef std::shared_ptr<InputRetriever>         pointer_t;

    virtual                         ~InputRetriever() {}

    virtual Input::pointer_t        retrieve(String const& filename) = 0;
};




class Output
{
public:
    typedef std::shared_ptr<Output>             pointer_t;

    virtual                 ~Output() {}

    Position&               get_position();
    Position const&         get_position() const;

    void                    write(String const& data);

protected:
    virtual void            internal_write(String const& data) = 0;

    Position                f_position;
};


class StandardOutput : public Output
{
public:
                            StandardOutput();

protected:
    typedef std::shared_ptr<StandardOutput>     pointer_t;

    virtual void            internal_write(String const& data);
};


class FileOutput : public Output
{
public:
    typedef std::shared_ptr<FileOutput>         pointer_t;

    bool                    open(String const& filename);

protected:
    virtual void            internal_write(String const& data);

    std::ofstream           f_file;
};


class StringOutput : public Output
{
public:
    typedef std::shared_ptr<StringOutput>       pointer_t;

    String const&           get_string() const;

private:
    virtual void            internal_write(String const& data);

    String                  f_string;
};


}
// namespace as2js
#endif
// #ifndef AS2JS_STREAM_H

// vim: ts=4 sw=4 et
