/* unicode_characters.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

/** \file
 * \brief Find different types of Unicode characters.
 *
 * This function determine what's what as per the ECMAScript definitions
 * used by the lexer.
 *
 * For example, <USP> means all Unicode defined spaces. Here we check
 * all the Unicode characters and determine which are spaces (as one
 * of the functions.) This ensures that our lexer implementation is
 * correct.
 *
 * Note that ECMA expects Unicode 3.0 as a base so if we do not support
 * newer characters we are fine (i.e. that means we do not have to check
 * the unicode characters in our lexer, but we have to make sure that at
 * least all Unicode 3.0 characters are supported.)
 */

#include <iostream>

#include <QString>
#include <QChar>

// See http://icu-project.org/apiref/icu4c/index.html
#include <unicode/uchar.h>
//#include <unicode/cuchar> // once available in Linux...

#include <iomanip>


void usp()
{
    std::cout << "Qt <USP>";
    for(uint c(0); c < 0x110000; ++c)
    {
        if(c >= 0xD800 && c <= 0xDFFF)
        {
            continue;
        }
        // from Qt
        QChar::Category cat(QChar::category(c));
        if(cat == QChar::Separator_Space)
        {
            std::cout << std::hex << " 0x" << c;
        }
    }
    std::cout << std::endl << "Lx <USP>";
    for(UChar32 c(0); c < 0x110000; ++c)
    {
        if(c >= 0xD800 && c <= 0xDFFF)
        {
            continue;
        }
        // from Linux
        //if(u_isspace(c)) // this one includes many controls
        //if(u_isJavaSpaceChar(c)) // this one includes 0x2028 and 0x2029
        if(u_charType(c) == U_SPACE_SEPARATOR) // this is what ECMAScript defines as legal
        {
            std::cout << std::hex << " 0x" << c;
        }
    }
    std::cout << std::endl;
}

void identifier()
{
    //          Uppercase letter (Lu)
    //          Lowercase letter (Ll)
    //          Titlecase letter (Lt)
    //          Modifier letter (Lm)
    //          Other letter (Lo)
    //          Letter number (Nl)
    //          Non-spacing mark (Mn)
    //          Combining spacing mark (Mc)
    //          Decimal number (Nd)
    //          Connector punctuation (Pc)
    //          ZWNJ
    //          ZWJ
    //          $
    //          _
    std::cout << std::endl << "id characters:\n";
    int32_t first(-1);
    int32_t count(0);
    for(UChar32 c(0); c < 0x110000; ++c)
    {
        if(c >= 0xD800 && c <= 0xDFFF)
        {
            if(first != -1)
            {
                std::cout << std::hex << "  " << first << ", " << c - 1 << ",\n";
                ++count;
                first = -1;
            }
            continue;
        }
        // from Linux
        switch(u_charType(c))
        {
        case U_UPPERCASE_LETTER:
        case U_LOWERCASE_LETTER:
        case U_TITLECASE_LETTER:
        case U_MODIFIER_LETTER:
        case U_OTHER_LETTER:
        case U_LETTER_NUMBER:
        case U_NON_SPACING_MARK:
        case U_COMBINING_SPACING_MARK:
        case U_DECIMAL_DIGIT_NUMBER:
        case U_CONNECTOR_PUNCTUATION:
            if(first == -1)
            {
                first = c;
            }
            break;

        default:
            if(first != -1)
            {
                std::cout << std::hex << std::setfill('0') << "    { 0x" << std::setw(5) << first << ", 0x" << std::setw(5) << c - 1 << " },\n";
                ++count;
                first = -1;
            }
            break;

        }
    }
    if(first != -1)
    {
        std::cout << std::hex << "  " << first << ", " << 0x10FFFF << std::dec << ",\n";
        ++count;
    }
    std::cout << "got " << count << " groups\n\n";
}

int main()
{
    usp();
    identifier();
    return 0;
}

// vim: ts=4 sw=4 et
