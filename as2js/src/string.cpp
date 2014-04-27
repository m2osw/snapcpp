/* string.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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

#include "as2js/string.h"


namespace as2js
{



/** \brief Initialize an empty string.
 *
 * This function initializes an empty string.
 */
String::String()
    : basic_string()
{
}


/** \brief Copy a string in this string.
 *
 * This function initializes a string as a copy of another string.
 */
String::String(String const& str)
    : basic_string(str)
{
}


/** \brief Create a string from the specified input string.
 *
 * This function creates a string and initializes it with the specified
 * input string.
 *
 * The input is considered to be ISO-8859-1 and thus it gets copied in
 * the string as is. If you have UTF-8 data, make sure to use the fromUtf8()
 * function instead.
 *
 * Note that we cannot include '\0' characters in our strings. This function
 * stops at the first null terminator no matter what.
 *
 * \param[in] str  A string, if not null terminated, make sure to define the size.
 * \param[in] len  The length of the string, if -1, expect a '\0'.
 */
String::String(char const *str, int len)
    : basic_string()
{
    from_char(str, len);
}


/** \brief Create a string from the specified input string.
 *
 * This function creates a string and initializes it with the specified
 * input string.
 *
 * The input is considered to be UCS-4 or UTF-16 depending on the width of
 * the wchar_t type.
 *
 * Note that we cannot include '\0' characters in our strings. This function
 * stops at the first null terminator no matter what.
 *
 * \param[in] str  A string, if not null terminated, make sure to define the size.
 * \param[in] len  The length of the string, if -1, expect a '\0'.
 */
String::String(wchar_t const *str, int len)
    : basic_string()
{
    from_wchar(str, len);
}


/** \brief Create a string from the specified input string.
 *
 * This function creates a string and initializes it with the specified
 * input string.
 *
 * The input is considered to be UCS-4 and thus it gets copied as is.
 *
 * Note that we cannot include '\0' characters in our strings. This function
 * stops at the first null terminator no matter what.
 *
 * \param[in] str  A string, if not null terminated, make sure to define the size.
 * \param[in] len  The length of the string, if -1, expect a '\0'.
 */
String::String(as_char_t const *str, int len)
    : basic_string()
{
    from_as_char(str, len);
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String.
 *
 * The input string is taken as ISO-8859-1. If it uses another format,
 * please make sure to use the correct function.
 *
 * \param[in] str  The input string to copy in this String.
 */
String::String(std::string const& str)
    : basic_string()
{
    from_char(str.c_str(), static_cast<int>(str.length()));
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String.
 *
 * The input string is taken as UTF-16 and as such converts the surrogates
 * (0xD800 to 0xDFFF) to UCS-4 characters as expected.
 *
 * \param[in] str  The input string to copy in this String.
 */
String::String(std::wstring const& str)
    : basic_string()
{
    from_wchar(str.c_str(), static_cast<int>(str.length()));
}


/** \brief Copy str in this String.
 *
 * This function makes a copy of str in this string.
 *
 * \param[in] str  The input string to copy in this String.
 *
 * \return A reference to this string.
 */
String& String::operator = (String const& str)
{
    // use basic string implementation as is
    basic_string<as_char_t>::operator = (str);
    return *this;
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String. The string is viewed as
 * ISO-8859-1. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] str  The string to copy in this String.
 *
 * \return A reference to this string.
 */
String& String::operator = (char const *str)
{
    from_char(str);
    return *this;
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String. The string is viewed as
 * UTF-16. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] str  The string to copy in this String.
 *
 * \return A reference to this string.
 */
String& String::operator = (wchar_t const *str)
{
    from_wchar(str);
    return *this;
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String. The string is viewed as
 * ISO-8859-1. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] str  The string to copy in this String.
 *
 * \return A reference to this string.
 */
String& String::operator = (std::string const& str)
{
    from_char(str.c_str(), static_cast<int>(str.length()));
    return *this;
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String. The string is viewed as
 * UTF-16. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] str  The string to copy in this String.
 *
 * \return A reference to this string.
 */
String& String::operator = (std::wstring const& str)
{
    from_wchar(str.c_str(), static_cast<int>(str.length()));
    return *this;
}


/** \brief Copy a string in this String object.
 *
 * This function copies the ISO-8859-1 string pointer by str to
 * this string. The previous string is lost.
 *
 * If a null character is found, the copy stops.
 *
 * The len parameter can be used to limit the length of the copy.
 *
 * \param[in] str  The input string to copy in this string.
 * \param[in] len  The maximum number of characters to copy, if -1, copy
 *                 up to the next null ('\0') character.
 */
void String::from_char(char const *str, int len)
{
    clear();
    if(len == -1)
    {
        for(; *str != '\0'; ++str)
        {
            append(1, static_cast<as_char_t>(*str));
        }
    }
    else
    {
        for(; len > 0 && *str != '\0'; --len, ++str)
        {
            append(1, static_cast<as_char_t>(*str));
        }
    }
}


/** \brief Copy a wchar_t string to this String.
 *
 * This function copies a wchar_t string to this String. Internally we
 * only deal with UCS-4 characters. However, this function expects the
 * input to possibly be UTF-16 and converts surrogate characters to
 * UCS-4 as expected in UTF-16.
 *
 * \param[in] str  The input string to copy in this string.
 * \param[in] len  The maximum number of characters to copy, if -1, copy
 *                 up to the next null ('\0') character.
 */
void String::from_wchar(wchar_t const *str, int len)
{
    struct out
    {
        out(String& str)
            : f_string(str)
        {
        }

        void add(as_char_t c)
        {
            if(c >= 0xD800 && c < 0xDC00)
            {
                f_lead_surrogate = c;
                return;
            }
            else if(c >= 0xDC00 && c <= 0xDFFF)
            {
                if(f_lead_surrogate == 0)
                {
                    // ignore invalid character
                    return;
                }
                c = (((static_cast<as_char_t>(f_lead_surrogate) & 0x03FF) << 10) | (static_cast<as_char_t>(c) & 0x03FF)) + 0x10000;
                f_lead_surrogate = 0;
            }
            f_string.append(1, c);
        }

        String&         f_string;
        zas_char_t      f_lead_surrogate;
    };

    clear();

    out o(*this);
    if(len == -1)
    {
        for(; *str != '\0'; ++str)
        {
            o.add(*str);
        }
    }
    else
    {
        for(; len > 0 && *str != '\0'; --len, ++str)
        {
            o.add(*str);
        }
    }
}


/** \brief Copy a as_char_t string to this String.
 *
 * This function copies an as_char_t string to this String. Since an
 * as_char_t string has the same character type as a String, this copy
 * is straight forward.
 *
 * The copy stops as soon as a null ('\0') character is found.
 *
 * \param[in] str  The input string to copy in this string.
 * \param[in] len  The maximum number of characters to copy, if -1, copy
 *                 up to the next null ('\0') character.
 */
void String::from_as_char(as_char_t const *str, int len)
{
    if(len == -1)
    {
        for(; *str != '\0'; ++str)
        {
            append(1, *str);
        }
    }
    else
    {
        for(; len > 0 && *str != '\0'; --len, ++str)
        {
            append(1, *str);
        }
    }
}


/** \brief Copy a UTF-8 string to this String.
 *
 * This function copies a string to this String. In this case
 * the input string is considered to be UTF-8.
 *
 * If you have an std::string, use the c_str() operation to call this
 * function.
 *
 * The copy stops as soon as a null ('\0') character is found.
 *
 * \note
 * If an error occurs, the String object is not modified.
 *
 * \param[in] str  The input string to copy in this string.
 * \param[in] len  The maximum number of characters to copy, if -1, copy
 *                 up to the next null ('\0') character.
 *
 * \return -1 if the input could not be converted, the string length otherwise.
 */
int String::from_utf8(char const *str, int len)
{
    String          result;
    unsigned char   c;
    as_char_t       w;
    int             l;

    if(len == -1)
    {
        // it's a bit of a waste, but makes it a lot easier
        len = std::char_traits<char>::length(str);
    }

    while(len > 0)
    {
        --len;
        c = static_cast<unsigned char>(*str++);

        if(c < 0x80)
        {
            w = c;
        }
        else
        {
            if(c >= 0xC0 && c <= 0xDF)
            {
                l = 1;
                w = c & 0x1F;
            }
            else if(c >= 0xE0 && c <= 0xEF)
            {
                l = 2;
                w = c & 0x0F;
            }
            else if(c >= 0xF0 && c <= 0xF7)
            {
                l = 3;
                w = c & 0x07;
            }
            else if(c >= 0xF8 && c <= 0xFB)
            {
                l = 4;
                w = c & 0x03;
            }
            else if(c >= 0xFC && c <= 0xFD)
            {
                l = 5;
                w = c & 0x01;
            }
            else
            {
                // invalid UTF-8 sequence
                return -1;
            }
            if(len < l)
            {
                // not enough character
                return -1;
            }
            len -= l;
            while(l > 0)
            {
                c = static_cast<unsigned char>(*str++);
                if(c < 0x80 || c > 0xBF)
                {
                    return -1;
                }
                l--;
                w = (w << 6) | (c & 0x3F);
            }
        }
        if(!valid_character(w))
        {
            return -1;
        }
        result.append(1, w);
    }

    // it worked, we can smash this String
    *this = result;
    return length();
}


/** \brief Compare this String against a char const * string.
 *
 * This function compares an ISO-8859-1 string against this String.
 * If you have a UTF-8 string, make sure to use from_utf8() first
 * and then compare two String's against each other.
 *
 * \param[in] str  The string to compare as ISO-8859-1.
 *
 * \return true if both strings are equal.
 */
bool String::operator == (char const *str) const
{
    String s(str);
    return *this == s;
}


/** \brief Compare a String against a char const * string.
 *
 * This function compares an ISO-8859-1 string against a String.
 * If you have a UTF-8 string, make sure to use from_utf8() first
 * and then compare two String's against each other.
 *
 * \param[in] str  The string to compare as ISO-8859-1.
 * \param[in] string  The String to compare with.
 *
 * \return true if both strings are equal.
 */
bool operator == (char const *str, String const& string)
{
    String s(str);
    return s == string;
}


/** \brief Check validity of the string.
 *
 * This function checks all the characters for validity. This is based
 * on a Unicode piece of code that clearly specifies that a certain
 * number of characters just cannot be used (i.e. this includes UTF-16
 * surrogates, and any value larger than 0x10FFFF or negative numbers.)
 *
 * Note that the null character '\0' is considered valid and part of
 * the string, however, anything after that character is ignored.
 *
 * \return true if the string is considered valid.
 */
bool String::valid() const
{
    for(as_char_t  const *s(c_str()); *s != '\0'; ++s)
    {
        if(!valid_character(*s))
        {
            return false;
        }
    }

    return true;
}


/** \brief Check whether a character is considered valid.
 *
 * The UCS-4 type is limited in the code points that can be used. This
 * function returns true if the code point of \p c is considered valid.
 *
 * Characters in UCS-4 must be defined between 0 and 0x10FFFF inclusive,
 * except for code points 0xD800 to 0xDFFF which are used as surrogate
 * for UTF-16 encoding.
 *
 * \param[in] c  The character to be checked.
 *
 * \return true if c is considered valid.
 */
bool String::valid_character(as_char_t c)
{
    return (c < 0xD800 || c > 0xDFFF)   // UTF-16 surrogates
        && c < 0x110000                 // too large?
        && c >= 0;                      // too small?
}


/** \brief Calculate the length if converted to UTF-8.
 *
 * This function calculates the length necessary to convert the string
 * to UTF-8.
 *
 * \return The length if converted to UTF-8.
 */
size_t String::utf8_length() const
{
    size_t      r, l;
    as_char_t   c;

    // result
    r = 0;

    for(as_char_t  const *wc = c_str(); *wc != '\0'; ++wc)
    {
        // get one wide character
        c = *wc;

        // simluate encoding
        if(c < 0x80)
        {
            l = 1;
        }
        else if(c < 0x800)
        {
            l = 2;
        }
        else if(c < 0x10000)
        {
            l = 3;
        }
        else if(c < 0x200000)
        {
            l = 4;
        }
        else if(c < 0x4000000)
        {
            l = 5;
        }
        else if(c > 0)
        {
            l = 6;
        }
        else
        {
            // an invalid wide character (negative!)
            return -1;
        }
        r += l;
    }

    return r;
}


/** \brief Convert a string to UTF-8 and return the result.
 *
 * This function converts this String in UTF-8 using an std::string
 * and then returns the result.
 *
 * \warning
 * Remember that you cannot use a UTF-8 as direct input of a constructor
 * or assignment operator of the String class. Instead, make sure to use
 * the from_utf8() function.
 *
 * \note
 * The function skips any character considered invalid. If you want to
 * know whether the resulting UTF-8 string is an exact representation
 * of this String, then call the valid() function.
 *
 * \return The String converted to UTF-8 and saved in an std::string.
 */
std::string String::to_utf8() const
{
    std::string     result;
    as_char_t       c;

    // make sure we always have a null at the end...
    for(as_char_t const *wc = c_str(); *wc != '\0'; ++wc)
    {
        // get one wide character
        c = *wc;
        if(valid_character(c))
        {
            // only encode characters considered valid
            if(c < 0x80)
            {
                /* this will also encode '\0'... */
                result.append(1, c);
            }
            else if(c < 0x800)
            {
                result.append(1, (c >> 6) | 0xC0);
                result.append(1, (c & 0x3F) | 0x80);
            }
            else if(c < 0x10000)
            {
                result.append(1, (c >> 12) | 0xE0);
                result.append(1, ((c >> 6) & 0x3F) | 0x80);
                result.append(1, (c & 0x3F) | 0x80);
            }
            else if(c < 0x200000)
            {
                result.append(1, (c >> 18) | 0xF0);
                result.append(1, ((c >> 12) & 0x3F) | 0x80);
                result.append(1, ((c >> 6) & 0x3F) | 0x80);
                result.append(1, (c & 0x3F) | 0x80);
            }
            else
            {
                result.append(1, (c >> 24) | 0xF8);
                result.append(1, ((c >> 18) & 0x3F) | 0x80);
                result.append(1, ((c >> 12) & 0x3F) | 0x80);
                result.append(1, ((c >> 6) & 0x3F) | 0x80);
                result.append(1, (c & 0x3F) | 0x80);
            }
        }
    }

    return result;
}


std::ostream& operator << (std::ostream& out, String const& str)
{
    out << str.to_utf8();
    return out;
}


}
// namespace as

// vim: ts=4 sw=4 et
