/* stream.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/stream.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"

#include    <unistd.h>


namespace as2js
{

/**********************************************************************/
/**********************************************************************/
/***  FILTERS  ********************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Push one byte in the decoder.
 *
 * This function pushes exactly one byte in the decoder.
 *
 * In most cases decoders expects their getc() function to be called
 * right after each putc(), although it is not mandatory.
 *
 * \param[in] c  The character to append to this decoding filter.
 */
void DecodingFilter::putc(unsigned char c)
{
    f_buffer.push_back(c);
}


/** \brief Retrieve the next character.
 *
 * This function retrieves the next input character.
 *
 * If there is data, but not enough of it, it returns Input::INPUT_NAC.
 * Processing can safely continue.
 *
 * If possible, the function avoids returning with the Input::INPUT_NAC
 * result (i.e. if a filter returns that value when there is still data
 * available in the buffer, their get_char() function gets called again.)
 *
 * If there is data, but it cannot properly be converted to a valid
 * character, it returns Input::INPUT_ERR.
 *
 * If there is no data, then Input::INPUT_EOF is returned.
 *
 * \return The next character available or one of the Input::INPUT_...
 *         result (EOF, NAC, ERR).
 */
Input::char_t DecodingFilter::getc()
{
    if(f_buffer.empty())
    {
        return Input::INPUT_EOF;
    }

    return get_char();
}


/** \brief Get the next ISO-8859-1 character.
 *
 * This function returns the next unsigned char from the input buffer.
 *
 * \note
 * We know that buffer has at least one byte because get_char() is
 * called only after getc() checked whether the input buffer was
 * empty.
 *
 * \return The next character.
 */
Input::char_t DecodingFilterISO88591::get_char()
{
    // no conversion for ISO-8859-1 to UTF-32
    Input::char_t c(f_buffer[0]);
    f_buffer.erase(f_buffer.begin());
    return c;
}


/** \brief Get the next UTF-8 character.
 *
 * This function reads the next UTF-8 character. Since UTF-8 makes use of
 * a variable number of bytes, the function may return Input::INPUT_NAC
 * meaning that not enough data is available in the input buffer.
 *
 * If an invalid UTF-8 sequence is discovered, then Input::INPUT_ERR is
 * returned. The function can still be called with additional data to
 * read whatever comes next. Multiple errors may be returned while skipping
 * encoded bytes.
 *
 * \return The next character, Input::INPUT_NAC, or Input::INPUT_ERR.
 */
Input::char_t DecodingFilterUTF8::get_char()
{
    // Note: we know that the buffer is at least 1 byte
    unsigned char b(f_buffer[0]);

    if(b < 0x80)
    {
        f_buffer.erase(f_buffer.begin());
        return b;
    }

    size_t l(0);
    as_char_t c(0);
    if(b >= 0xC0 && b <= 0xDF)
    {
        l = 2;
        c = b & 0x1F;
    }
    else if(b >= 0xE0 && b <= 0xEF)
    {
        l = 3;
        c = b & 0x0F;
    }
    else if(b >= 0xF0 && b <= 0xF7)
    {
        l = 4;
        c = b & 0x07;
    }
    else
    {
        // invalid UTF-8 sequence, erase one input byte
        f_buffer.erase(f_buffer.begin());
        return Input::INPUT_ERR;
    }
    if(f_buffer.size() < l)
    {
        // not enough bytes for this character
        return Input::INPUT_NAC;
    }
    for(size_t i(1); i < l; ++i)
    {
        b = f_buffer[i];
        if(b < 0x80 || b > 0xBF)
        {
            // found an invalid byte, remove bytes before that
            f_buffer.erase(f_buffer.begin(), f_buffer.begin() + i);
            return Input::INPUT_ERR;
        }
        c = (c << 6) | (b & 0x3F);
    }

    // get rid of those bytes
    f_buffer.erase(f_buffer.begin(), f_buffer.begin() + l);

    // is it a UTF-16 surrogate or too large a character?
    if(!String::valid_character(c))
    {
        return Input::INPUT_ERR;
    }

    // return that character
    return c;
}


/** \brief Decode a UTF-16 character.
 *
 * This function is called with a 2 byte value which either represents
 * a Unicode character as is, or a UTF-16 surrogate. When a surrogate
 * is detected, it is transformed in a full Unicode character by this
 * function. The function needs to be called twice to decode one full
 * Unicode character described using a surrogate.
 *
 * If an invalid surrogate sequence is found, then the function
 * returns Input::INPUT_ERR.
 *
 * When the lead surrogate is found, the function returns Input::INPUT_NAC
 * meaning that more data is necessary and the function needs to be called
 * again to proceed.
 *
 * \param[in] c  Two byte value representing a Unicode character or a UTF-16
 *               surrogate.
 *
 * \return The following character, Input::INPUT_NAC, or Input::INPUT_ERR.
 */
Input::char_t DecodingFilterUTF16::next_char(Input::char_t c)
{
    if(c >= 0xD800 && c < 0xDC00)
    {
        f_lead_surrogate = c;
        return Input::INPUT_NAC; // not an error unless it was the last 2 bytes
    }
    else if(c >= 0xDC00 && c <= 0xDFFF)
    {
        if(f_lead_surrogate == 0)
        {
            // lead surrogate missing, skip trail
            f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 2);
            return Input::INPUT_ERR;
        }
        c = (((static_cast<as_char_t>(f_lead_surrogate) & 0x03FF) << 10) | (static_cast<as_char_t>(c) & 0x03FF)) + 0x10000;
        f_lead_surrogate = 0;
    }
    else if(f_lead_surrogate != 0)
    {
        // trail surrogate missing
        f_lead_surrogate = 0;
        return Input::INPUT_ERR;
    }

    return c;
}


/** \brief Decode UTF-16 in Little Endian format.
 *
 * This function reads data in UTF-16 Little Endian. The function may
 * return Input::INPUT_NAC if called without enough data forming a
 * unicode character or when only the lead surrogate is read.
 *
 * The function returns Input::INPUT_ERR if the function finds a lead
 * without a trail surrogate, or a trail without a lead.
 *
 * \return The next character, Input::INPUT_ERR, or Input::INPUT_NAC.
 */
Input::char_t DecodingFilterUTF16LE::get_char()
{
    Input::char_t c;
    do
    {
        if(f_buffer.size() < 2)
        {
            return Input::INPUT_NAC;
        }

        c = next_char(f_buffer[0] + f_buffer[1] * 256);
        if(c == Input::INPUT_ERR)
        {
            return Input::INPUT_ERR;
        }
        f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 2);
    }
    while(c == Input::INPUT_NAC);

    return c;
}


/** \brief Decode UTF-16 in Big Endian format.
 *
 * This function reads data in UTF-16 Big Endian. The function may
 * return Input::INPUT_NAC if called without enough data forming a
 * unicode character or when only the lead surrogate is read.
 *
 * The function returns Input::INPUT_ERR if the function finds a lead
 * without a trail surrogate, or a trail without a lead.
 *
 * \return The next character or Input::INPUT_NAC.
 */
Input::char_t DecodingFilterUTF16BE::get_char()
{
    Input::char_t c;
    do
    {
        if(f_buffer.size() < 2)
        {
            return Input::INPUT_NAC;
        }

        c = next_char(f_buffer[0] * 256 + f_buffer[1]);
        if(c == Input::INPUT_ERR)
        {
            return Input::INPUT_ERR;
        }
        f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 2);
    }
    while(c == Input::INPUT_NAC);

    return c;
}


/** \brief Decode UTF-32 in Little Endian format.
 *
 * This function reads data in UTF-32 Little Endian. The function may
 * return Input::INPUT_ERR if the input represents an invalid character
 * (i.e. a character larger than 0x10FFFF or representing a UTF-16
 * surrogate encoding.)
 *
 * If the buffer does not at least include 4 bytes, then the function
 * returns Input::INPUT_NAC.
 *
 * \return The next character, Input::INPUT_NAC, or Input::INPUT_ERR.
 */
Input::char_t DecodingFilterUTF32LE::get_char()
{
    if(f_buffer.size() < 4)
    {
        return Input::INPUT_NAC;
    }

    // little endian has byte 0 as the least significant
    Input::char_t c(
              (f_buffer[0] <<  0)
            | (f_buffer[1] <<  8)
            | (f_buffer[2] << 16)
            | (f_buffer[3] << 24)
        );
    f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 4);
    if(!String::valid_character(c))
    {
        return Input::INPUT_ERR;
    }
    return c;
}


/** \brief Decode UTF-32 in Big Endian format.
 *
 * This function reads data in UTF-32 Big Endian. The function may
 * return Input::INPUT_ERR if the input represents an invalid character
 * (i.e. a character larger than 0x10FFFF or representing a UTF-16
 * surrogate encoding.)
 *
 * If the buffer does not at least include 4 bytes, then the function
 * returns Input::INPUT_NAC.
 *
 * \return The next character, Input::INPUT_NAC, or Input::INPUT_ERR.
 */
Input::char_t DecodingFilterUTF32BE::get_char()
{
    if(f_buffer.size() < 4)
    {
        return Input::INPUT_NAC;
    }

    // big endian has byte 0 as the most significant
    Input::char_t c(
              (f_buffer[0] << 24)
            | (f_buffer[1] << 16)
            | (f_buffer[2] <<  8)
            | (f_buffer[3] <<  0)
        );
    f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 4);
    if(!String::valid_character(c))
    {
        return Input::INPUT_ERR;
    }
    return c;
}


/** \brief Read the next character in any format.
 *
 * This function reads one character from the input stream. At first
 * the stream is considered to be undefined (no specific filter defined).
 *
 * Once we have an least 4 bytes of data, we try to detect a BOM. If no
 * BOM is detected, make sure that the characters are valid UTF-8, and
 * if so, use the UTF-8 filter, otherwise fallback on the ISO-8859-1
 * filter unless we notice many zeroes in which case we use one of
 * the UTF-16 or UTF-32 decoders.
 *
 * \bug
 * Known bug: if the input file is less than 4 bytes it cannot be
 * used because this filter will always return a NAC. So even a valid
 * source of 1, 2, or 3 characters fails. However, the likelihood of
 * such a script to be useful are probably negative so we do not care
 * too much.
 *
 * \return The following characters, Input::INPUT_NAC, or Input::INPUT_ERR.
 */
Input::char_t DecodingFilterDetect::get_char()
{
    if(!f_filter)
    {
        if(f_buffer.size() < 4)
        {
            return Input::INPUT_NAC;
        }

        // read the BOM in big endian
        uint32_t bom(
                  (f_buffer[0] << 24)
                | (f_buffer[1] << 16)
                | (f_buffer[2] <<  8)
                | (f_buffer[3] <<  0)
            );

        if(bom == 0x0000FEFF)
        {
            // UTF-32 Big Endian
            f_filter.reset(new DecodingFilterUTF32BE);
            f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 4);
        }
        else if(bom == 0xFFFE0000)
        {
            // UTF-32 Little Endian
            f_filter.reset(new DecodingFilterUTF32LE);
            f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 4);
        }
        else if((bom >> 16) == 0xFEFF)
        {
            // UTF-16 Big Endian
            f_filter.reset(new DecodingFilterUTF16BE);
            f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 2);
        }
        else if((bom >> 16) == 0xFFFE)
        {
            // UTF-16 Little Endian
            f_filter.reset(new DecodingFilterUTF16LE);
            f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 2);
        }
        else if((bom & 0xFFFFFF00) == 0xEFBBBF00)
        {
            // UTF-8
            f_filter.reset(new DecodingFilterUTF8);
            f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 3);
        }
        else
        {
            // if each character is valid UTF-8, the use UTF-8
            String s;
            String::conversion_result_t r(s.from_utf8(reinterpret_cast<char const *>(&f_buffer[0]), f_buffer.size()));
            if(r == String::conversion_result_t::STRING_GOOD || r == String::conversion_result_t::STRING_END)
            {
                f_filter.reset(new DecodingFilterUTF8);
            }
            else
            {
                // fallback to ISO-8859-1 (should very rarely happen!)
                f_filter.reset(new DecodingFilterISO88591);
            }
        }
    }

    // we do not get BOMs returned, yet we could check for the BOM
    // character and adjust the filter if we detect it being
    // swapped (it does not look like Unicode promotes that scheme
    // anymore though, therefore at this point we won't do that...)

    Input::char_t c(f_filter->getc());
    while((c == Input::INPUT_EOF || c == Input::INPUT_NAC)
       && !f_buffer.empty())
    {
        // transmit the data added to "this" filter
        // down to f_filter, but only as required
        // because otherwise we'd generate an EOF
        f_filter->putc(f_buffer[0]);
        f_buffer.erase(f_buffer.begin(), f_buffer.begin() + 1);
        c = f_filter->getc();
    }

    return c;
}



/**********************************************************************/
/**********************************************************************/
/***  INPUT  **********************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Initialize an input object.
 *
 * This function initializes the input object making it ready to be
 * used to read data from a file, a string, or a TTY.
 *
 * The \p filter should generally not be specified, although if you
 * know the format of an input file, it can be useful to force the
 * filter to the exact format. We only support Unicode formats,
 * though.
 *
 * \param[in] filter  The filter to use while reading the input data.
 */
Input::Input(DecodingFilter::pointer_t filter)
    : f_filter(filter)
    //, f_position() -- auto-init
    //, f_unget() -- auto-init
{
}


/** \brief Get the position object of the input object.
 *
 * The stream manages a position object. The call can use this function
 * to retrieve a read/write version of the current position.
 *
 * \return A modifiable version of the position object.
 */
Position& Input::get_position()
{
    return f_position;
}


/** \brief Get the position object of the input object.
 *
 * The stream manages a position object. The call can use this function
 * to retrieve a read-only version of the current position.
 *
 * \return A constant version of the position object.
 */
Position const& Input::get_position() const
{
    return f_position;
}


/** \brief Get one character.
 *
 * This function retrieves the next character from the input object.
 *
 * If the caller used the ungetc() function, then the characters that
 * were ungotten are returned first in the opposite order (FILO).
 *
 * \return The next character available in the stream.
 */
Input::char_t Input::getc()
{
    if(!f_unget.empty())
    {
        char_t result(f_unget.back());
        f_unget.pop_back();
        return result;
    }
    return filter_getc();
}


/** \brief Unget one character.
 *
 * This function saves the specified character \p c in a buffer of the
 * Input object. The next getc() call will first return that last character
 * the caller unget.
 *
 * \param[in] c  The character to unget.
 */
void Input::ungetc(char_t c)
{
    // silently avoid ungetting special values such as INPUT_EOF
    // (TBD: maybe we should check surrogates?)
    if(c > 0 && c < 0x110000)
    {
        f_unget.push_back(c);
    }
}


/** \brief Get the next character.
 *
 * This function reads the next character from the input. In most cases
 * this reads one or more bytes from the input file, and then it
 * converts those bytes in a character using a filter.
 *
 * This function does not return Input::INPUT_NAC. Instead it reads as
 * much data as it can and returns the next character, no matter what.
 * However, it may return EOF if the end of the file is reached, or
 * ERR if a character in the stream is not valid. There are two types
 * of invalid characters: (1) numbers that are outside of the Unicode
 * range (0 .. 0x010FFFF) or a UTF-16 surrogate in a format that does
 * not support such surrogate (UTF-8, UTF-32), and (2) byte sequences
 * that end before a valid character can be formed (missing surrogate,
 * invalid UTF-8).
 *
 * \return The next character, Input::INPUT_EOF, or Input::INPUT_ERR.
 */
Input::char_t Input::filter_getc()
{
    // if the input class used does not overload this function,
    // then we get the next byte and try to convert it to a
    // character, if that works, return that character

    char_t w;
    do
    {
        char_t c(get_byte());
        if(c == Input::INPUT_EOF)
        {
            // determine the final result
            w = f_filter->getc();
            return w == Input::INPUT_NAC ? Input::INPUT_ERR : w;
        }
        f_filter->putc(c);
        w = f_filter->getc();
    }
    while(w == Input::INPUT_NAC || w == Input::INPUT_EOF);
    // EOF can happen if we bump in a BOM in the middle of nowhere
    // so we have to loop on EOF as well

    return w;
}


/** \brief Function used to get the following byte of data.
 *
 * This virtual function is used by the filter_getc() function to
 * retrieve the next character of data from the input stream. The
 * default implementation of the function throws because it
 * should never get called.
 *
 * Note that it is possible to bypass this function by implementing
 * instead the filter_getc() in your own class.
 *
 * \exception exception_internal_error
 * This function always raises this exception because it should
 * not be called.
 *
 * \return The next byte from the input stream.
 */
Input::char_t Input::get_byte()
{
    // this function should never be called
    throw exception_internal_error("internal error: the get_byte() of the Input class was called");
}


/**********************************************************************/
/**********************************************************************/
/***  STANDARD INPUT  *************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Use standard input as the input stream.
 *
 * This function sets up the input file to the standard input of the
 * process. In that case the filename is set to "-". However, there
 * is not size available.
 *
 * The function first calls close() to make sure that any previous
 * call to standard_input() or open() get cleaned up.
 *
 * \return true if the file could be opened.
 */
StandardInput::StandardInput()
{
    get_position().set_filename("-");
}


/** \brief Read one by from the standard input.
 *
 * This function returns the next byte found in the standard input
 * stream.
 *
 * If the input stream can end and the end was reached, then
 * INPUT_EOF is returned.
 *
 * \return The next byte from the input stream.
 */
Input::char_t StandardInput::get_byte()
{
    char c;
    if(std::cin.get(c))
    {
        return static_cast<char_t>(c) & 255;
    }
    return INPUT_EOF;
}


/**********************************************************************/
/**********************************************************************/
/***  FILE INPUT  *****************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Use the named file as the input stream.
 *
 * This function sets up the named file as the input stream of this
 * FileInput object.
 *
 * The function first calls close() to make sure that any previous
 * call to standard_input() or open() get cleaned up.
 *
 * \note
 * This function is not placed in the constructor because we want
 * to return false if the file cannot be opened.
 *
 * \param[in] filename  The name of the file to open.
 *
 * \return true if the file could be opened.
 */
bool FileInput::open(String const& filename)
{
    if(f_file.is_open())
    {
        throw exception_file_already_open("file object for \"" + get_position().get_filename().to_utf8() + "\" cannot be reused for \"" + filename.to_utf8() + "\"");
    }

    std::string utf8(filename.to_utf8());
    f_file.open(utf8.c_str());
    if(!f_file.is_open())
    {
        return false;
    }
    get_position().set_filename(filename);

    return true;
}


/** \brief Get the next byte from the file.
 *
 * This function reads one byte from the input file and returns it.
 *
 * \return The read byte.
 */
Input::char_t FileInput::get_byte()
{
    char c;
    if(f_file.get(c))
    {
        return static_cast<char_t>(c) & 255;
    }
    return INPUT_EOF;
}




/**********************************************************************/
/**********************************************************************/
/***  STRING INPUT  ***************************************************/
/**********************************************************************/
/**********************************************************************/

/** \brief Initliaze the string input.
 *
 * This function initialize a StringInput object with the specified
 * string and line number. By default, line is set to 1 since strings
 * represent code from the start of a file.
 *
 * \param[in] str  The string parameter which we will read characters from.
 * \param[in] line  The start line for the Position object.
 */
StringInput::StringInput(String const& str, Position::counter_t line)
    : f_str(str)
    //, f_pos(0) -- auto-init
{
    // in case line is not set to 1
    get_position().reset_counters(line);
}


/** \brief Get the next character.
 *
 * This function bypasses the Input filter since we already have
 * UTF-32 characters in the input string.
 *
 * \return The next character from the string or Input::INPUT_EOF.
 */
Input::char_t StringInput::filter_getc() // bypass the filters
{
    char_t  c(INPUT_EOF);

    if(f_pos < f_str.length())
    {
        c = f_str[f_pos];
        ++f_pos;
    }
    return c;
}




/**********************************************************************/
/**********************************************************************/
/***  OUTPUT  *********************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Get the position object of the input object.
 *
 * The stream manages a position object. The call can use this function
 * to retrieve a read/write version of the current position.
 *
 * \return A modifiable version of the position object.
 */
Position& Output::get_position()
{
    return f_position;
}


/** \brief Get the position object of the input object.
 *
 * The stream manages a position object. The call can use this function
 * to retrieve a read-only version of the current position.
 *
 * \return A constant version of the position object.
 */
Position const& Output::get_position() const
{
    return f_position;
}


/** \brief Write data to this output stream.
 *
 * This function writes the specified string to the output stream.
 * Since we pretty much only support text based files, we just
 * use this format.
 *
 * All outputs are done in UTF-8.
 *
 * If the function cannot write to the destination, then it throws
 * an error.
 *
 * \param[in] data  The string to be written to the output stream.
 */
void Output::write(String const& data)
{
    internal_write(data);
}


/**********************************************************************/
/**********************************************************************/
/***  STANDARD OUTPUT  ************************************************/
/**********************************************************************/
/**********************************************************************/

/** \brief Initializes the standard output object.
 *
 * This function initializes the standard output object, more
 * specifically it defines its filename as "-".
 */
StandardOutput::StandardOutput()
{
    get_position().set_filename("-");
}


/** \brief Write a string to standard output.
 *
 * This function writes the specified string of data to the output
 * in UTF-8 format.
 *
 * Note that the streams do not save a BOM at the start of files.
 *
 * \param[in] data  The string to write in the standard output.
 */
void StandardOutput::internal_write(String const& data)
{
    std::cout << data.to_utf8();
    if(!std::cout)
    {
        // should we do something here?
        Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_IO_ERROR, get_position());
        msg << "I/O error: could not write to output.";
        throw exception_exit(1, "I/O error: could not write to output.");
    }
}


/**********************************************************************/
/**********************************************************************/
/***  OUTPUT FILE  ****************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Open the output file.
 *
 * This function is used to open the output file.
 *
 * One FileOutput object can only be used to output to one file. Trying
 * to reuse the same object with a different filename will generate
 * an exception.
 *
 * \todo
 * Generate an error message for why the file could not be opened.
 *
 * \param[in] filename  The name of the file to open for output.
 *
 * \return true if the file was successfully opened, false if an error
 *         occured.
 */
bool FileOutput::open(String const& filename)
{
    if(f_file.is_open())
    {
        throw exception_file_already_open("file object for \"" + get_position().get_filename().to_utf8() + "\" cannot be reused for \"" + filename.to_utf8() + "\"");
    }

    std::string utf8(filename.to_utf8());
    f_file.open(utf8.c_str());
    if(!f_file.is_open())
    {
        return false;
    }
    get_position().set_filename(filename);

    return true;
}


/** \brief Write to the output file.
 *
 * This function writes the specified \p data string to this output file.
 *
 * If an error occurs, the process writes a fatal error in stderr and
 * exists.
 *
 * \param[in] data  The string to write to the output file.
 */
void FileOutput::internal_write(String const& data)
{
    f_file << data.to_utf8();
    if(!f_file)
    {
        // should we do something here?
        Message msg(message_level_t::MESSAGE_LEVEL_FATAL, err_code_t::AS_ERR_IO_ERROR, get_position());
        msg << "I/O error: could not write to output.";
        throw exception_exit(1, "I/O error: could not write to output.");
    }
}


/**********************************************************************/
/**********************************************************************/
/***  OUTPUT STRING  **************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Retrieve a copy of the output string.
 *
 * This function is used to retrieve the output string used as a buffer
 * each time the write() function is called.
 *
 * \return A reference to the internal string.
 */
String const& StringOutput::get_string() const
{
    return f_string;
}


/** \brief Write to the output string.
 *
 * This function writes the specified \p data string to this output string.
 *
 * \param[in] data  The string to write to the output file.
 */
void StringOutput::internal_write(String const& data)
{
    f_string += data;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
