/* stream.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2014 */

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

#include    "as2js/stream.h"

#include    <unistd.h>


namespace as2js
{




/**********************************************************************/
/**********************************************************************/
/***  INPUT  **********************************************************/
/**********************************************************************/
/**********************************************************************/


Position& Input::get_position()
{
    return f_position;
}


Position const& Input::get_position() const
{
    return f_position;
}


Input::char_t Input::getc()
{
    if(!f_unget.empty())
    {
        char_t result(f_unget.back());
        f_unget.pop_back();
        return result;
    }
    return internal_getc();
}


void Input::ungetc(char_t c)
{
    f_unget.push_back(c);
}


Input::input_size_t Input::get_size() const
{
    return -1;
}


/**********************************************************************/
/**********************************************************************/
/***  FILE INPUT  *****************************************************/
/**********************************************************************/
/**********************************************************************/

FileInput::~FileInput()
{
    close();
}


void FileInput::close()
{
    if(f_file && f_file != stdin)
    {
        fclose(f_file);
    }

    f_file.reset();
    f_size = -1;

    get_position().reset_counters();
}


bool FileInput::standard_input()
{
    close();

    f_file = stdin;

    if(f_file)
    {
        get_position().set_filename("-");
    }

    return f_file;
}


bool FileInput::open(String const& filename)
{
    close();

#ifdef WIN32
    // TODO implement WIN32 using wopen()?
    // (wopen() is limited in the filename length so the standard Win32
    // API is better for that... but then we don't get a FILE *...)
#else
    std::string utf8(filename.to_utf8());
    f_file = fopen(utf8.c_str(), "rb");
    if(f_file)
    {
        get_position().set_filename(filename);

        if(!isatty(fileno(f_file)))
        {
            // get the file size
            fseek(f_file, 0, SEEK_END);
            f_size = ftell(f_file);
            fseek(f_file, 0, SEEK_SET);
        }
    }
#endif

    return f_file;
}


FileInput::input_size_t FileInput::get_size() const
{
    return f_size;
}


Input::char_t FileInput::internal_getc()
{
    if(!f_file)
    {
        return AS_EOF;
    }

    char q[1];
    if(fread(q, sizeof(q), 1, f_file) != 1)
    {
        return AS_EOF;
    }

    // we assume ISO-8859-1
    return q[0];
}


Input::char_t FileUCS32Input::internal_getc()
{
    if(!f_file)
    {
        return AS_EOF;
    }

    char q[4];
    if(fread(q, sizeof(q), 1, f_file) != 1)
    {
        return AS_EOF;
    }

    char_t c((q[0] << 24) | (q[1] << 16) | (q[2] << 8) | q[3]);

    // all negative values are invalid Unicode
    // at this time... So is 0xFFFF.
    if(c < 0 || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF))
    {
        // invalid character
        return 0x0000FFFF;
    }

    return c;
}


/**********************************************************************/
/**********************************************************************/
/***  STRING INPUT  ***************************************************/
/**********************************************************************/
/**********************************************************************/

void StringInput::set(String const& str, Position::counter_t line)
{
    get_position().reset_counters(line);

    f_pos = 0;
    f_str = str;
}


Input::char_t StringInput::internal_getc()
{
    char_t  c;

    if(f_pos < f_str.length())
    {
        c = f_str[f_pos];
        ++f_pos;
    }
    else
    {
        c = AS_EOF;
    }

    return c;
}


// returns the number of characters (UCS-4)
Input::input_size_t StringInput::get_size() const
{
    return f_str.length();
}



}
// namespace as2js

// vim: ts=4 sw=4 et
