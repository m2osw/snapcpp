/* string.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/string.h"
#include    "as2js/exceptions.h"

#include    <limits>


/** \file
 * \brief String implementation.
 *
 * We use the std::basic_string to create our own string using an int32_t
 * for each character. This allows us to have full UTF-32 Unicode characters.
 *
 * This function redefines a few functions that the base string library
 * does not offer because of the special character type we use.
 */


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


/** \brief Create a string from the specified input string.
 *
 * This function creates a string and initializes it with the specified
 * input string.
 *
 * The input is considered to be ISO-8859-1 and thus it gets copied in
 * the string as is (see the from_char() function.) If you have UTF-8
 * data, make sure to use the from_utf8() function instead.
 *
 * Note that we cannot include '\0' characters in our strings. This function
 * stops at the first null terminator no matter what.
 *
 * \note
 * The \p str pointer can be set to nullptr in which case the string is
 * considered empty.
 *
 * \param[in] str  A string, if not null terminated, make sure to define
 *                 the \p len parameter.
 * \param[in] len  The length of the string, if -1, expect a '\0'.
 *
 * \sa from_utf8()
 * \sa from_char()
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
 * The input is considered to be UTF-32 or UTF-16 depending on the width of
 * the wchar_t type.
 *
 * Note that we cannot include '\0' characters in our strings. This function
 * stops at the first null terminator no matter what.
 *
 * \note
 * The \p str pointer can be set to nullptr in which case the string is
 * considered empty.
 *
 * \param[in] str  A string, if not null terminated, make sure to define
 *                 the \p len parameter.
 * \param[in] len  The length of the string, if -1, expect a '\0'.
 *
 * \sa from_wchar()
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
 * The input is considered to be UTF-32 and thus it gets copied as is.
 *
 * Note that we cannot include '\0' characters in our strings. This function
 * stops at the first null terminator no matter what.
 *
 * \note
 * The \p str pointer can be set to nullptr in which case the string is
 * considered empty.
 *
 * \param[in] str  A string, if not null terminated, make sure to define
 *                 the \p len parameter.
 * \param[in] len  The length of the string, if -1, expect a '\0'.
 *
 * \sa from_as_char()
 */
String::String(as_char_t const *str, int len)
    : basic_string()
{
    if(from_as_char(str, len) != conversion_result_t::STRING_GOOD)
    {
        throw exception_internal_error("String::String() called with an invalid input string");
    }
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String.
 *
 * The input is considered to be ISO-8859-1 and thus it gets copied in
 * the string as is (see the from_char() function.) If you have UTF-8
 * data, make sure to use the from_utf8() function instead.
 *
 * \param[in] str  The input string to copy in this String.
 *
 * \sa from_char()
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
 * The input string is taken as UTF-16 if wchar_t is 2 bytes and as
 * such converts the surrogates (0xD800 to 0xDFFF) to UTF-32 characters
 * as expected. If wchar_t is 4 bytes, the string is copied as is.
 *
 * \param[in] str  The input string to copy in this String.
 *
 * \sa from_wchar()
 */
String::String(std::wstring const& str)
    : basic_string()
{
    from_wchar(str.c_str(), static_cast<int>(str.length()));
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String.
 *
 * The input string is taken as UTF-32 and copied as is in its entirety.
 *
 * \param[in] str  The input string to copy in this String.
 */
String::String(std::basic_string<as_char_t> const& str)
    : basic_string(str)
{
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
 *
 * \sa from_char()
 */
String& String::operator = (char const *str)
{
    from_char(str);
    return *this;
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String. The string is viewed as
 * UTF-16 if wchar_t is 2 bytes, and UTF-32 if wchar_t is 4 bytes.
 * If another format is expected, make sure to use the proper function.
 *
 * \param[in] str  The string to copy in this String.
 *
 * \return A reference to this string.
 *
 * \sa from_wchar()
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
 *
 * \sa from_char()
 */
String& String::operator = (std::string const& str)
{
    from_char(str.c_str(), static_cast<int>(str.length()));
    return *this;
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String. The string is viewed as
 * UTF-16 if wchar_t is 2 bytes, and UTF-32 if wchar_t is 4 bytes.
 * If another format is expected, make sure to use the proper function.
 *
 * \param[in] str  The string to copy in this String.
 *
 * \return A reference to this string.
 *
 * \sa from_wchar()
 */
String& String::operator = (std::wstring const& str)
{
    from_wchar(str.c_str(), static_cast<int>(str.length()));
    return *this;
}


/** \brief Copy str in this String.
 *
 * This function copies str in this String. The string is viewed as
 * UTF-32. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] str  The string to copy in this String.
 *
 * \return A reference to this string.
 */
String& String::operator = (std::basic_string<as_char_t> const& str)
{
    basic_string<as_char_t>::operator = (str);
    return *this;
}


/** \brief Append str to this String.
 *
 * This function appends str to this String. The string is viewed as
 * ISO-8859-1. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] str  The string to append to this String.
 *
 * \return A reference to this string.
 */
String& String::operator += (char const *str)
{
    String s(str);
    basic_string<as_char_t>::operator += (s);
    return *this;
}


/** \brief Append str to this String.
 *
 * This function appends str to this String. The string is viewed as
 * UTF-16 if wchar_t is 2 bytes, and UTF-32 if wchar_t is 4 bytes.
 * If another format is expected, make sure to use the proper function.
 *
 * \param[in] str  The string to append to this String.
 *
 * \return A reference to this string.
 *
 * \sa from_wchar()
 */
String& String::operator += (wchar_t const *str)
{
    String s(str);
    basic_string<as_char_t>::operator += (s);
    return *this;
}


/** \brief Append str to this String.
 *
 * This function appends str to this String. The string is viewed as
 * UTF-32. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] str  The string to append to this String.
 *
 * \return A reference to this string.
 */
String& String::operator += (as_char_t const *str)
{
    basic_string<as_char_t>::operator += (str);
    return *this;
}


/** \brief Append str to this String.
 *
 * This function appends str to this String. The string is viewed as
 * ISO-8859-1. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] str  The string to append to this String.
 *
 * \return A reference to this string.
 */
String& String::operator += (std::string const& str)
{
    String s(str);
    basic_string<as_char_t>::operator += (s);
    return *this;
}


/** \brief Append str to this String.
 *
 * This function appends str to this String. The string is viewed as
 * UTF-16 if wchar_t is 2 bytes, and UTF-32 if wchar_t is 4 bytes.
 * If another format is expected, make sure to use the proper function.
 *
 * \param[in] str  The string to append to this String.
 *
 * \return A reference to this string.
 */
String& String::operator += (std::wstring const& str)
{
    String s(str);
    basic_string<as_char_t>::operator += (s);
    return *this;
}


/** \brief Append str to this String.
 *
 * This function append str to this String. The string is viewed as
 * UTF-32. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] str  The string to append to this String.
 *
 * \return A reference to this string.
 */
String& String::operator += (std::basic_string<as_char_t> const& str)
{
    basic_string<as_char_t>::operator += (str);
    return *this;
}


/** \brief Append c to this String.
 *
 * This function append c to this String. The character is viewed as
 * UTF-32. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] c  The character to append to this String.
 *
 * \return A reference to this string.
 */
String& String::operator += (as_char_t const c)
{
    basic_string<as_char_t>::operator += (c);
    return *this;
}


/** \brief Append c to this String.
 *
 * This function append c to this String. The character is viewed as
 * ISO-8859-1. If another format is expected, make sure to use the
 * proper function.
 *
 * \param[in] c  The character to append to this String.
 *
 * \return A reference to this string.
 */
String& String::operator += (char const c)
{
    basic_string<as_char_t>::operator += (static_cast<as_char_t>(static_cast<unsigned char>(c)));
    return *this;
}


/** \brief Append c to this String.
 *
 * This function append c to this String. The character is viewed as
 * UTF-32. If another format is expected, make sure to use the
 * proper function.
 *
 * \todo
 * Under MS-Windows the character is viewed as UTF-16, only we do
 * not properly manage surrogates in this case (i.e. if you just
 * added another surrogate, concatenate both surrogates in one
 * UTF-32 character.)
 *
 * \param[in] c  The character to append to this String.
 *
 * \return A reference to this string.
 */
String& String::operator += (wchar_t const c)
{
    // TODO: cannot add surrogate in this way?
    //       (under MS-Windows, where wchar_t is 16 bits, this would be
    //       the only way to add large characters with wchar_t... we could
    //       save leads and when a tail arrives convert the character, but
    //       that's rather unsafe...)
    if(valid_character(c))
    {
        basic_string<as_char_t>::operator += (static_cast<as_char_t>(c));
    }
    return *this;
}


// /** \brief Concatenate a String and a C-string.
//  *
//  * This function concatenate this String and a standard C-string.
//  *
//  * \note
//  * This function creates a copy of the string. If you can, try to
//  * use the += operator instead.
//  *
//  * \param[in] str  The string to concatenate at the end of this String.
//  *
//  * \return A new string with the concatenated result.
//  */
// String String::operator + (char const * str)
// {
//     String result(*this);
//     return result += str;
// }
// 
// 
// /** \brief Concatenate a String and a wide C-string.
//  *
//  * This function concatenate this String and a standard wide C-string.
//  *
//  * \note
//  * This function creates a copy of the string. If you can, try to
//  * use the += operator instead.
//  *
//  * \param[in] str  The string to concatenate at the end of this String.
//  *
//  * \return A new string with the concatenated result.
//  */
// String String::operator + (wchar_t const * str)
// {
//     String result(*this);
//     return result += str;
// }
// 
// 
// /** \brief Concatenate a String and a C-like string made of as_char_t characters.
//  *
//  * This function concatenate this String and a C-link string made of
//  * as_char_t characters. The array must be null terminated (\0).
//  *
//  * \note
//  * This function creates a copy of the string. If you can, try to
//  * use the += operator instead.
//  *
//  * \param[in] str  The string to concatenate at the end of this String.
//  *
//  * \return A new string with the concatenated result.
//  */
// String String::operator + (as_char_t const * str)
// {
//     String result(*this);
//     return result += str;
// }
// 
// 
// /** \brief Concatenate a String and a C++ string.
//  *
//  * This function concatenate this String and a C++ string.
//  *
//  * \note
//  * This function creates a copy of the string. If you can, try to
//  * use the += operator instead.
//  *
//  * \param[in] str  The string to concatenate at the end of this String.
//  *
//  * \return A new string with the concatenated result.
//  */
// String String::operator + (std::string const & str)
// {
//     String result(*this);
//     return result += str;
// }
// 
// 
// /** \brief Concatenate a String and a C++ wide string.
//  *
//  * This function concatenate this String and a C++ wide string.
//  *
//  * \note
//  * This function creates a copy of the string. If you can, try to
//  * use the += operator instead.
//  *
//  * \param[in] str  The string to concatenate at the end of this String.
//  *
//  * \return A new string with the concatenated result.
//  */
// String String::operator + (std::wstring const & str)
// {
//     String result(*this);
//     return result += str;
// }


/** \brief Copy a string in this String object.
 *
 * This function copies the ISO-8859-1 string pointer by str to
 * this string. The previous string is lost.
 *
 * If a null character is found, the copy stops.
 *
 * The \p len parameter can be used to limit the length of the copy.
 *
 * \note
 * This function can be called with a nullptr in \p str, in which
 * case the string is considered empty.
 *
 * \param[in] str  The input string to copy in this string.
 * \param[in] len  The maximum number of characters to copy, if -1, copy
 *                 up to the next null ('\0') character.
 *
 * \return Always STRING_GOOD since all bytes in ISO-8859-1 are all
 *         valid Unicode characters.
 */
String::conversion_result_t String::from_char(char const *str, int len)
{
    clear();
    if(str != nullptr)
    {
        if(len == -1)
        {
            for(; *str != '\0'; ++str)
            {
                append(1, static_cast<unsigned char>(*str));
            }
        }
        else
        {
            for(; len > 0 && *str != '\0'; --len, ++str)
            {
                append(1, static_cast<unsigned char>(*str));
            }
        }
    }

    return conversion_result_t::STRING_GOOD;
}


/** \brief Copy a wchar_t string to this String.
 *
 * This function copies a wchar_t string to this String. Internally we
 * only deal with UTF-32 characters. However, this function expects the
 * input to possibly be UTF-16 and converts surrogate characters to
 * UTF-32 as expected in UTF-16. (In other words, this functions works
 * under Linux and MS-Windows.)
 *
 * \note
 * This string is not modified if the input is not valid.
 *
 * \note
 * This function can be called with a nullptr in \p str, in which
 * case the string is considered empty.
 *
 * \param[in] str  The input string to copy in this string.
 * \param[in] len  The maximum number of characters to copy, if -1, copy
 *                 up to the next null ('\0') character.
 *
 * \return STRING_INVALID: if a character is not a valid UTF-32 character,
 *         STRING_BAD: if the input is invalid,
 *         STRING_END: could not be converted (not enough data for last
 *                     surrogate character),
 *         STRING_GOOD: the new string is valid.
 */
String::conversion_result_t String::from_wchar(wchar_t const *str, int len)
{
    struct out
    {
        String::conversion_result_t add(as_char_t c)
        {
            if(c >= 0xD800 && c < 0xDC00)
            {
                f_lead_surrogate = c;
                return conversion_result_t::STRING_END; // not an error unless it was the last character
            }
            else if(c >= 0xDC00 && c <= 0xDFFF)
            {
                if(f_lead_surrogate == 0)
                {
                    // invalid encoding
                    return conversion_result_t::STRING_BAD;
                }
                c = (((static_cast<as_char_t>(f_lead_surrogate) & 0x03FF) << 10) | (static_cast<as_char_t>(c) & 0x03FF)) + 0x10000;
                // Note: UTF-16 characters cannot be invalid here
                //       (unless we add code points such as 0xFFFE and 0xFFFF
                //       among invalid characters)
                if(!f_string.valid_character(c))
                {
                    return conversion_result_t::STRING_INVALID; // LCOV_EXCL_LINE
                }
                f_lead_surrogate = 0;
            }
            f_string.append(1, c);
            return conversion_result_t::STRING_GOOD;
        }

        String          f_string;
        as_char_t       f_lead_surrogate = 0;
    };

    out o;
    String::conversion_result_t result(conversion_result_t::STRING_GOOD);
    if(str != nullptr)
    {
        if(len == -1)
        {
            for(; *str != '\0'; ++str)
            {
                result = o.add(*str);
                if(result != conversion_result_t::STRING_GOOD && result != conversion_result_t::STRING_END)
                {
                    break;
                }
            }
        }
        else
        {
            for(; len > 0 && *str != '\0'; --len, ++str)
            {
                result = o.add(*str);
                if(result != conversion_result_t::STRING_GOOD && result != conversion_result_t::STRING_END)
                {
                    break;
                }
            }
        }
    }

    if(result == conversion_result_t::STRING_GOOD)
    {
        *this = o.f_string;
    }

    return result;
}


/** \brief Copy an as_char_t string to this String.
 *
 * This function copies an as_char_t string to this String. Since an
 * as_char_t string has the same character type as a String, this copy
 * is straight forward.
 *
 * The copy stops as soon as a null ('\0') character is found.
 *
 * \note
 * If an error occurs, this String object is not modified.
 *
 * \note
 * This function can be called with a nullptr in \p str, in which
 * case the string is considered empty.
 *
 * \param[in] str  The input string to copy in this string.
 * \param[in] len  The maximum number of characters to copy, if -1, copy
 *                 up to the next null ('\0') character.
 *
 * \return STRING_INVALID: if the resulting character is not a valid UTF-32 character,
 *         STRING_GOOD: the new string is valid.
 */
String::conversion_result_t String::from_as_char(as_char_t const *str, int len)
{
    String s;
    if(str != nullptr)
    {
        if(len == -1)
        {
            for(; *str != '\0'; ++str)
            {
                if(!valid_character(*str))
                {
                    return conversion_result_t::STRING_INVALID;
                }
                s.append(1, *str);
            }
        }
        else
        {
            for(; len > 0 && *str != '\0'; --len, ++str)
            {
                if(!valid_character(*str))
                {
                    return conversion_result_t::STRING_INVALID;
                }
                s.append(1, *str);
            }
        }
    }

    *this = s;

    return conversion_result_t::STRING_GOOD;
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
 * If an error occurs, this String object is not modified.
 *
 * \param[in] str  The input string to copy in this string.
 * \param[in] len  The maximum number of characters to copy, if -1, copy
 *                 up to the next null ('\0') character.
 *
 * \return STRING_INVALID: if the resulting character is not a valid
 *                         UTF-32 character,
 *         STRING_BAD: if the input is invalid,
 *         STRING_END: could not be converted (not enough data for last
 *                     character),
 *         STRING_GOOD: the new string is valid.
 */
String::conversion_result_t String::from_utf8(char const *str, int len)
{
    String          result;
    unsigned char   c;
    as_char_t       w;
    int             l;

    if(str != nullptr)
    {
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
                // The following are not valid UTF-8 characters, these are
                // refused below as we verify the validity of the character
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
                    return conversion_result_t::STRING_BAD;
                }
                if(len < l)
                {
                    // not enough character
                    return conversion_result_t::STRING_END;
                }
                len -= l;
                while(l > 0)
                {
                    c = static_cast<unsigned char>(*str++);
                    if(c < 0x80 || c > 0xBF)
                    {
                        return conversion_result_t::STRING_BAD;
                    }
                    l--;
                    w = (w << 6) | (c & 0x3F);
                }
            }
            if(!valid_character(w))
            {
                return conversion_result_t::STRING_INVALID;
            }
            result.append(1, w);
        }
    }

    // it worked, we can smash this String
    *this = result;

    return conversion_result_t::STRING_GOOD;
}


/** \brief Compare this String against a char const * string.
 *
 * This function compares an ISO-8859-1 string against this String.
 * If you have a UTF-8 string, make sure to use from_utf8() first
 * and then compare the two String's against each other.
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
 * and then compare the two String's against each other.
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


/** \brief Compare this String against a char const * string.
 *
 * This function compares an ISO-8859-1 string against this String.
 * If you have a UTF-8 string, make sure to use from_utf8() first
 * and then compare the two String's against each other.
 *
 * \param[in] str  The string to compare as ISO-8859-1.
 *
 * \return true if both strings are not equal.
 */
bool String::operator != (char const *str) const
{
    String s(str);
    return *this != s;
}


/** \brief Compare a String against a char const * string.
 *
 * This function compares an ISO-8859-1 string against a String.
 * If you have a UTF-8 string, make sure to use from_utf8() first
 * and then compare the two String's against each other.
 *
 * \param[in] str  The string to compare as ISO-8859-1.
 * \param[in] string  The String to compare with.
 *
 * \return true if both strings are not equal.
 */
bool operator != (char const *str, String const& string)
{
    String s(str);
    return s != string;
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
 * \todo
 * We are actually transforming the String object to properly check
 * all of its characters as added to the buffer so this function
 * should become obsolete at some point.
 *
 * \return true if the string is considered valid.
 *
 * \sa valid_character()
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
 * The UTF-32 type is limited in the code points that can be used. This
 * function returns true if the code point of \p c is considered valid.
 *
 * Characters in UTF-32 must be defined between 0 and 0x10FFFF inclusive,
 * except for code points 0xD800 to 0xDFFF which are used as surrogate
 * for UTF-16 encoding.
 *
 * \param[in] c  The character to be checked.
 *
 * \return true if c is considered valid.
 *
 * \sa valid()
 */
bool String::valid_character(as_char_t c)
{
    // Note: as_char_t is an int32_t (i.e. a signed value)
    return (c < 0xD800 || c > 0xDFFF)   // UTF-16 surrogates
        && c < 0x110000                 // too large?
        && c >= 0;                      // too small?
}


/** \brief Check whether this string represents a valid integer.
 *
 * This function checks the strings to see whether it represents a
 * valid integer. The function supports decimal and hexadecimal
 * numbers. Octals are not supported because JavaScript does not
 * convert numbers that start with a 0 as if these were octal
 * numbers.
 *
 * \li Decimal number: [-+]?[0-9]+
 * \li Hexadecimal number: [-+]?0[xX][0-9a-fA-F]+
 *
 * \return true if the string represents an integer.
 */
bool String::is_int64() const
{
    struct hex_test
    {
        static bool is_hex(as_char_t c)
        {
            return (c >= '0' && c <= '9')
                || (c >= 'a' && c <= 'f')
                || (c >= 'A' && c <= 'F');
        }
    };

    as_char_t const *s(c_str());

    // sign
    // TODO: in strict mode hexadecimal numbers cannot be signed
    if(*s == '-' || *s == '+')
    {
        ++s;
    }

    // handle special case of hexadecimal
    if(*s == '0')
    {
        ++s;
        if(*s == 'x' || *s == 'X')
        {
            if(s[1] == '\0')
            {
                // just "0x" or "0X" is not a valid number
                return false;
            }
            for(++s; hex_test::is_hex(*s); ++s);
            return *s == '\0';
        }
        // no octal support in strings
    }

    // number
    for(; *s >= '0' && *s <= '9'; ++s);

    return *s == '\0';
}


/** \brief Check whether the string represents a valid floating pointer number.
 *
 * This function parses the string to see whether it represents a valid
 * floating pointer number: an integral part, an optional decimal part,
 * and an optional signed exponent.
 *
 * The sign of the exponent is also itself optional.
 *
 * Note that this function returns true if the number is an integer in
 * decimal number representation, however, it will return false for
 * hexadecimal numbers. You may also call the is_number() function to
 * know whether a string represents either a decimal number or a floating
 * point number.
 *
 * \li A floating point number: [-+]?[0-9]+(\.[0-9]+)?([eE]?[0-9]+)?
 *
 * \todo
 * Ameliorate the test so if no digits are present where required then
 * an error is emitted (i.e. you may have '0.', '.0' but not just '.';
 * same problem with exponent).
 *
 * \return true if the string represents a floating point number.
 */
bool String::is_float64() const
{
    as_char_t const *s(c_str());

    // sign
    if(*s == '-' || *s == '+')
    {
        ++s;
    }

    // integral part
    for(; *s >= '0' && *s <= '9'; ++s);

    // if '.' check for a decimal part
    if(*s == '.')
    {
        for(++s; *s >= '0' && *s <= '9'; ++s);
    }

    // if 'e' check for an exponent
    if(*s == 'e' || *s == 'E')
    {
        ++s;
        if(*s == '+' || *s == '-')
        {
            // skip the sign
            ++s;
        }
        for(; *s >= '0' && *s <= '9'; ++s);
    }

    return *s == '\0';
}


/** \brief Check whether this string represents a number.
 *
 * This function checks whether this string represents a number.
 * This means it returns true in the following cases:
 *
 * \li The string represents a decimal number ([-+]?[0-9]+)
 * \li The string represents an hexadecimal number ([-+]?0[xX][0-9a-fA-F]+)
 * \li The string represents a floating point number ([-+]?[0-9]+(\.[0-9]+)?([eE]?[0-9]+)?)
 *
 * Unfortunately, JavaScript does not understand "true", "false",
 * and "null" as numbers (even though isNaN(true), isNaN(false),
 * and isNaN(null) all return true.)
 *
 * \return true if this string represents a valid number
 */
bool String::is_number() const
{
    // floats support integers so this is true if this string is an int64
    return is_int64() || is_float64();
}


/** \brief Convert a string to an integer number.
 *
 * This function verifies that the string represents a valid integer
 * number, if so, it converts it to such and returns the result.
 *
 * If the string does not represent a valid integer, then the function
 * should return NaN. Unfortunately, there is not NaN integer. Instead
 * it will return zero (0) or it will raise an exception.
 *
 * \note
 * When used by the lexer, it should always work since the lexer reads
 * integers with the same expected syntax.
 *
 * \exception exception_internal_error
 * The string is not empty and it does not represent what is considered
 * a valid JavaScript integer.
 *
 * \return The string converted to an integer.
 */
Int64::int64_type String::to_int64() const
{
    if(empty())
    {
        return 0;
    }

    if(is_int64())
    {
        // Check whether it is an hexadecimal number, because if so
        // we use base 16. We want to force the base because we do
        // not support base 8 which std::stoll() could otherwise
        // switch to when we have a number that starts with zero.
        as_char_t const *s(c_str());
        if(*s == '+' || *s == '-')
        {
            ++s;
        }
        if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        {
            // the strtoll() function supports the sign
            return std::stoll(to_utf8(), nullptr, 16);
        }
        return std::stoll(to_utf8(), nullptr, 10);
    }

    // this is invalid
    throw exception_internal_error("String::to_int64() called with an invalid integer");
}


/** \brief Convert a string to a floating point number.
 *
 * This function verifies that the string represents a valid floating
 * point number, if so, it converts it to such and returns the result.
 *
 * If the string does not represent a valid floating point, then the
 * function returns NaN.
 *
 * \warning
 * On an empty string, this function returns 0.0 and not NaN as expected
 * in JavaScript.
 *
 * \note
 * When used by the lexer, it should always work since the lexer reads
 * floating points with the same expected syntax.
 *
 * \return The string as a floating point.
 */
Float64::float64_type String::to_float64() const
{
    if(empty())
    {
        return 0.0;
    }

    if(is_float64())
    {
        return std::stod(to_utf8(), 0);
    }

    return std::numeric_limits<double>::quiet_NaN();
}


/** \brief Check whether the string is considered true.
 *
 * A string that is empty is considered false. Any other string is
 * considered true.
 *
 * \return true if the string is not empty.
 */
bool String::is_true() const
{
    if(empty())
    {
        return false;
    }
// Not too sure where I picked that up, but the documentation clearly says
// that an empty string is false, anything else is true...
//    if(is_int64())
//    {
//        return to_int64() != 0;
//    }
//    if(is_float64())
//    {
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wfloat-equal"
//        return strtod(to_utf8().c_str(), 0) != 0.0;
//#pragma GCC diagnostic pop
//    }
    return true;
}


/** \brief Calculate the length if converted to UTF-8.
 *
 * This function calculates the length necessary to convert the string
 * to UTF-8.
 *
 * \return The length if converted to UTF-8.
 */
ssize_t String::utf8_length() const
{
    ssize_t     r(0);
    as_char_t   c;

    for(as_char_t const *wc(c_str()); *wc != '\0'; ++wc)
    {
        // get one wide character
        c = *wc;
        if(!valid_character(c))
        {
            // character is not valid UTF-32
            return -1;
        }

        // simulate encoding
        if(c < 0x80)
        {
            r += 1;
        }
        else if(c < 0x800)
        {
            r += 2;
        }
        else if(c < 0x10000)
        {
            r += 3;
        }
        else //if(c < 0x200000)
        {
            r += 4;
        }
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
 * of this String, then first call the valid() function on the source.
 *
 * \todo
 * This String object is expected to not have any invalid characters
 * so this function always returns the conversion even if it finds
 * invalid characters.
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
            else
            {
                result.append(1, (c >> 18) | 0xF0);
                result.append(1, ((c >> 12) & 0x3F) | 0x80);
                result.append(1, ((c >> 6) & 0x3F) | 0x80);
                result.append(1, (c & 0x3F) | 0x80);
            }
        }
    }

    return result;
}


/** \brief Make a simplified copy of this string.
 *
 * This function makes a copy of this string while removing spaces
 * from the start, the end, and within the string keep a single
 * space.
 *
 * If the string starts with a number, then only the number is kept.
 *
 * \note
 * This function is primarily used to compare a string using the
 * smart match operator.
 *
 * \return The simplified string.
 */
String String::simplified() const
{
    String result;

    // TBD: should we limit the space check to spaces recognized by EMCAScript?
    as_char_t const *wc = c_str();
    while(*wc != '\0' && iswspace(*wc))
    {
        ++wc;
    }

    // accept a signed number
    if(*wc == '-' || *wc == '+')
    {
        result += *wc;
        ++wc;
    }
    if(*wc >= '0' && *wc <= '9')
    {
        // read the number, ignore the rest
        result += *wc;
        ++wc;
        while(*wc >= '0' && *wc <= '9')
        {
            result += *wc;
            ++wc;
        }
        if(*wc == '.')
        {
            result += *wc;
            ++wc;
            while(*wc >= '0' && *wc <= '9')
            {
                result += *wc;
                ++wc;
            }
            if(*wc == 'e' || *wc == 'E')
            {
                result += *wc;
                ++wc;
                if(*wc == '+' || *wc == '-')
                {
                    result += *wc;
                    ++wc;
                }
                while(*wc >= '0' && *wc <= '9')
                {
                    result += *wc;
                    ++wc;
                }
            }
        }
        // ignore anything else
    }
    else
    {
        // read the string, but simplify the spaces
        bool found_space(false);
        for(; *wc != '\0'; ++wc)
        {
            if(iswspace(*wc))
            {
                found_space = true;
            }
            else
            {
                if(found_space)
                {
                    result += ' ';
                    found_space = false;
                }
                result += *wc;
            }
        }
    }

    if(result.empty())
    {
        // make an empty string similar to zero
        result = "0";
    }

    return result;
}


/** \brief Send string to output stream.
 *
 * This function sends this String to the specified output buffer. It is
 * to ease the output of a string to stream such as std::cout and std::cerr.
 *
 * \param[in,out] out  Stream where the string is printed.
 * \param[in] str  The string to be printed out.
 *
 * \return A reference to the \p out stream.
 */
std::ostream& operator << (std::ostream& out, String const& str)
{
    // Note: under MS-Windows we'd need to use str.to_wchar() instead
    out << str.to_utf8();
    return out;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
