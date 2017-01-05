/* lexer.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/lexer.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"

#include    <iomanip>


namespace as2js
{


/** \brief The Lexer private functions to handle character types.
 *
 * This unnamed namespace is used by the lexer to define a set of
 * private functions and tables used to handle the characters
 * and tokens.
 */
namespace
{

/** \brief Define one valid range of characters.
 *
 * This structure defines the range of characters that represent
 * letters viewed as being valid in EMCAScript version 5.
 *
 * The range is defined as min/max pairs. The two values are inclusive.
 */
struct identifier_characters_t
{
    bool operator < (identifier_characters_t const & rhs)
    {
        return f_min < rhs.f_min;
    }

    as_char_t   f_min;
    as_char_t   f_max;
};


/** \brief List of characters that are considered to be letters.
 *
 * The ECMAScript version 5 document defines the letters supported in
 * its identifiers in terms of Unicode characters. This includes many
 * characters that represent either letters or punctuation.
 *
 * The following table includes ranges (min/max) that include characters
 * that are considered letters in JavaScript code.
 *
 * The table was generated using the code in:
 *
 * tests/unicode_characters.cpp
 *
 * The number of items in the table is defined as
 * g_identifier_characters_size (see below).
 */
identifier_characters_t g_identifier_characters[] =
{
    // The ASCII characters are already handled by the time we reach the
    // code using this table
    //{ 0x00030, 0x00039 },
    //{ 0x00041, 0x0005a },
    //{ 0x0005f, 0x0005f },
    //{ 0x00061, 0x0007a },
    { 0x000aa, 0x000aa },
    { 0x000b5, 0x000b5 },
    { 0x000ba, 0x000ba },
    { 0x000c0, 0x000d6 },
    { 0x000d8, 0x000f6 },
    { 0x000f8, 0x002c1 },
    { 0x002c6, 0x002d1 },
    { 0x002e0, 0x002e4 },
    { 0x002ec, 0x002ec },
    { 0x002ee, 0x002ee },
    { 0x00300, 0x00374 },
    { 0x00376, 0x00377 },
    { 0x0037a, 0x0037d },
    { 0x00386, 0x00386 },
    { 0x00388, 0x0038a },
    { 0x0038c, 0x0038c },
    { 0x0038e, 0x003a1 },
    { 0x003a3, 0x003f5 },
    { 0x003f7, 0x00481 },
    { 0x00483, 0x00487 },
    { 0x0048a, 0x00527 },
    { 0x00531, 0x00556 },
    { 0x00559, 0x00559 },
    { 0x00561, 0x00587 },
    { 0x00591, 0x005bd },
    { 0x005bf, 0x005bf },
    { 0x005c1, 0x005c2 },
    { 0x005c4, 0x005c5 },
    { 0x005c7, 0x005c7 },
    { 0x005d0, 0x005ea },
    { 0x005f0, 0x005f2 },
    { 0x00610, 0x0061a },
    { 0x00620, 0x00669 },
    { 0x0066e, 0x006d3 },
    { 0x006d5, 0x006dc },
    { 0x006df, 0x006e8 },
    { 0x006ea, 0x006fc },
    { 0x006ff, 0x006ff },
    { 0x00710, 0x0074a },
    { 0x0074d, 0x007b1 },
    { 0x007c0, 0x007f5 },
    { 0x007fa, 0x007fa },
    { 0x00800, 0x0082d },
    { 0x00840, 0x0085b },
    { 0x008a0, 0x008a0 },
    { 0x008a2, 0x008b2 },
    { 0x008e4, 0x008ff },
    { 0x00900, 0x00963 },
    { 0x00966, 0x0096f },
    { 0x00971, 0x00977 },
    { 0x00979, 0x0097f },
    { 0x00981, 0x00983 },
    { 0x00985, 0x0098c },
    { 0x0098f, 0x00990 },
    { 0x00993, 0x009a8 },
    { 0x009aa, 0x009b0 },
    { 0x009b2, 0x009b2 },
    { 0x009b6, 0x009b9 },
    { 0x009bc, 0x009c4 },
    { 0x009c7, 0x009c8 },
    { 0x009cb, 0x009ce },
    { 0x009d7, 0x009d7 },
    { 0x009dc, 0x009dd },
    { 0x009df, 0x009e3 },
    { 0x009e6, 0x009f1 },
    { 0x00a01, 0x00a03 },
    { 0x00a05, 0x00a0a },
    { 0x00a0f, 0x00a10 },
    { 0x00a13, 0x00a28 },
    { 0x00a2a, 0x00a30 },
    { 0x00a32, 0x00a33 },
    { 0x00a35, 0x00a36 },
    { 0x00a38, 0x00a39 },
    { 0x00a3c, 0x00a3c },
    { 0x00a3e, 0x00a42 },
    { 0x00a47, 0x00a48 },
    { 0x00a4b, 0x00a4d },
    { 0x00a51, 0x00a51 },
    { 0x00a59, 0x00a5c },
    { 0x00a5e, 0x00a5e },
    { 0x00a66, 0x00a75 },
    { 0x00a81, 0x00a83 },
    { 0x00a85, 0x00a8d },
    { 0x00a8f, 0x00a91 },
    { 0x00a93, 0x00aa8 },
    { 0x00aaa, 0x00ab0 },
    { 0x00ab2, 0x00ab3 },
    { 0x00ab5, 0x00ab9 },
    { 0x00abc, 0x00ac5 },
    { 0x00ac7, 0x00ac9 },
    { 0x00acb, 0x00acd },
    { 0x00ad0, 0x00ad0 },
    { 0x00ae0, 0x00ae3 },
    { 0x00ae6, 0x00aef },
    { 0x00b01, 0x00b03 },
    { 0x00b05, 0x00b0c },
    { 0x00b0f, 0x00b10 },
    { 0x00b13, 0x00b28 },
    { 0x00b2a, 0x00b30 },
    { 0x00b32, 0x00b33 },
    { 0x00b35, 0x00b39 },
    { 0x00b3c, 0x00b44 },
    { 0x00b47, 0x00b48 },
    { 0x00b4b, 0x00b4d },
    { 0x00b56, 0x00b57 },
    { 0x00b5c, 0x00b5d },
    { 0x00b5f, 0x00b63 },
    { 0x00b66, 0x00b6f },
    { 0x00b71, 0x00b71 },
    { 0x00b82, 0x00b83 },
    { 0x00b85, 0x00b8a },
    { 0x00b8e, 0x00b90 },
    { 0x00b92, 0x00b95 },
    { 0x00b99, 0x00b9a },
    { 0x00b9c, 0x00b9c },
    { 0x00b9e, 0x00b9f },
    { 0x00ba3, 0x00ba4 },
    { 0x00ba8, 0x00baa },
    { 0x00bae, 0x00bb9 },
    { 0x00bbe, 0x00bc2 },
    { 0x00bc6, 0x00bc8 },
    { 0x00bca, 0x00bcd },
    { 0x00bd0, 0x00bd0 },
    { 0x00bd7, 0x00bd7 },
    { 0x00be6, 0x00bef },
    { 0x00c01, 0x00c03 },
    { 0x00c05, 0x00c0c },
    { 0x00c0e, 0x00c10 },
    { 0x00c12, 0x00c28 },
    { 0x00c2a, 0x00c33 },
    { 0x00c35, 0x00c39 },
    { 0x00c3d, 0x00c44 },
    { 0x00c46, 0x00c48 },
    { 0x00c4a, 0x00c4d },
    { 0x00c55, 0x00c56 },
    { 0x00c58, 0x00c59 },
    { 0x00c60, 0x00c63 },
    { 0x00c66, 0x00c6f },
    { 0x00c82, 0x00c83 },
    { 0x00c85, 0x00c8c },
    { 0x00c8e, 0x00c90 },
    { 0x00c92, 0x00ca8 },
    { 0x00caa, 0x00cb3 },
    { 0x00cb5, 0x00cb9 },
    { 0x00cbc, 0x00cc4 },
    { 0x00cc6, 0x00cc8 },
    { 0x00cca, 0x00ccd },
    { 0x00cd5, 0x00cd6 },
    { 0x00cde, 0x00cde },
    { 0x00ce0, 0x00ce3 },
    { 0x00ce6, 0x00cef },
    { 0x00cf1, 0x00cf2 },
    { 0x00d02, 0x00d03 },
    { 0x00d05, 0x00d0c },
    { 0x00d0e, 0x00d10 },
    { 0x00d12, 0x00d3a },
    { 0x00d3d, 0x00d44 },
    { 0x00d46, 0x00d48 },
    { 0x00d4a, 0x00d4e },
    { 0x00d57, 0x00d57 },
    { 0x00d60, 0x00d63 },
    { 0x00d66, 0x00d6f },
    { 0x00d7a, 0x00d7f },
    { 0x00d82, 0x00d83 },
    { 0x00d85, 0x00d96 },
    { 0x00d9a, 0x00db1 },
    { 0x00db3, 0x00dbb },
    { 0x00dbd, 0x00dbd },
    { 0x00dc0, 0x00dc6 },
    { 0x00dca, 0x00dca },
    { 0x00dcf, 0x00dd4 },
    { 0x00dd6, 0x00dd6 },
    { 0x00dd8, 0x00ddf },
    { 0x00df2, 0x00df3 },
    { 0x00e01, 0x00e3a },
    { 0x00e40, 0x00e4e },
    { 0x00e50, 0x00e59 },
    { 0x00e81, 0x00e82 },
    { 0x00e84, 0x00e84 },
    { 0x00e87, 0x00e88 },
    { 0x00e8a, 0x00e8a },
    { 0x00e8d, 0x00e8d },
    { 0x00e94, 0x00e97 },
    { 0x00e99, 0x00e9f },
    { 0x00ea1, 0x00ea3 },
    { 0x00ea5, 0x00ea5 },
    { 0x00ea7, 0x00ea7 },
    { 0x00eaa, 0x00eab },
    { 0x00ead, 0x00eb9 },
    { 0x00ebb, 0x00ebd },
    { 0x00ec0, 0x00ec4 },
    { 0x00ec6, 0x00ec6 },
    { 0x00ec8, 0x00ecd },
    { 0x00ed0, 0x00ed9 },
    { 0x00edc, 0x00edf },
    { 0x00f00, 0x00f00 },
    { 0x00f18, 0x00f19 },
    { 0x00f20, 0x00f29 },
    { 0x00f35, 0x00f35 },
    { 0x00f37, 0x00f37 },
    { 0x00f39, 0x00f39 },
    { 0x00f3e, 0x00f47 },
    { 0x00f49, 0x00f6c },
    { 0x00f71, 0x00f84 },
    { 0x00f86, 0x00f97 },
    { 0x00f99, 0x00fbc },
    { 0x00fc6, 0x00fc6 },
    { 0x01000, 0x01049 },
    { 0x01050, 0x0109d },
    { 0x010a0, 0x010c5 },
    { 0x010c7, 0x010c7 },
    { 0x010cd, 0x010cd },
    { 0x010d0, 0x010fa },
    { 0x010fc, 0x01248 },
    { 0x0124a, 0x0124d },
    { 0x01250, 0x01256 },
    { 0x01258, 0x01258 },
    { 0x0125a, 0x0125d },
    { 0x01260, 0x01288 },
    { 0x0128a, 0x0128d },
    { 0x01290, 0x012b0 },
    { 0x012b2, 0x012b5 },
    { 0x012b8, 0x012be },
    { 0x012c0, 0x012c0 },
    { 0x012c2, 0x012c5 },
    { 0x012c8, 0x012d6 },
    { 0x012d8, 0x01310 },
    { 0x01312, 0x01315 },
    { 0x01318, 0x0135a },
    { 0x0135d, 0x0135f },
    { 0x01380, 0x0138f },
    { 0x013a0, 0x013f4 },
    { 0x01401, 0x0166c },
    { 0x0166f, 0x0167f },
    { 0x01681, 0x0169a },
    { 0x016a0, 0x016ea },
    { 0x016ee, 0x016f0 },
    { 0x01700, 0x0170c },
    { 0x0170e, 0x01714 },
    { 0x01720, 0x01734 },
    { 0x01740, 0x01753 },
    { 0x01760, 0x0176c },
    { 0x0176e, 0x01770 },
    { 0x01772, 0x01773 },
    { 0x01780, 0x017d3 },
    { 0x017d7, 0x017d7 },
    { 0x017dc, 0x017dd },
    { 0x017e0, 0x017e9 },
    { 0x0180b, 0x0180d },
    { 0x01810, 0x01819 },
    { 0x01820, 0x01877 },
    { 0x01880, 0x018aa },
    { 0x018b0, 0x018f5 },
    { 0x01900, 0x0191c },
    { 0x01920, 0x0192b },
    { 0x01930, 0x0193b },
    { 0x01946, 0x0196d },
    { 0x01970, 0x01974 },
    { 0x01980, 0x019ab },
    { 0x019b0, 0x019c9 },
    { 0x019d0, 0x019d9 },
    { 0x01a00, 0x01a1b },
    { 0x01a20, 0x01a5e },
    { 0x01a60, 0x01a7c },
    { 0x01a7f, 0x01a89 },
    { 0x01a90, 0x01a99 },
    { 0x01aa7, 0x01aa7 },
    { 0x01b00, 0x01b4b },
    { 0x01b50, 0x01b59 },
    { 0x01b6b, 0x01b73 },
    { 0x01b80, 0x01bf3 },
    { 0x01c00, 0x01c37 },
    { 0x01c40, 0x01c49 },
    { 0x01c4d, 0x01c7d },
    { 0x01cd0, 0x01cd2 },
    { 0x01cd4, 0x01cf6 },
    { 0x01d00, 0x01de6 },
    { 0x01dfc, 0x01f15 },
    { 0x01f18, 0x01f1d },
    { 0x01f20, 0x01f45 },
    { 0x01f48, 0x01f4d },
    { 0x01f50, 0x01f57 },
    { 0x01f59, 0x01f59 },
    { 0x01f5b, 0x01f5b },
    { 0x01f5d, 0x01f5d },
    { 0x01f5f, 0x01f7d },
    { 0x01f80, 0x01fb4 },
    { 0x01fb6, 0x01fbc },
    { 0x01fbe, 0x01fbe },
    { 0x01fc2, 0x01fc4 },
    { 0x01fc6, 0x01fcc },
    { 0x01fd0, 0x01fd3 },
    { 0x01fd6, 0x01fdb },
    { 0x01fe0, 0x01fec },
    { 0x01ff2, 0x01ff4 },
    { 0x01ff6, 0x01ffc },
    { 0x0200c, 0x0200d },
    { 0x0203f, 0x02040 },
    { 0x02054, 0x02054 },
    { 0x02071, 0x02071 },
    { 0x0207f, 0x0207f },
    { 0x02090, 0x0209c },
    { 0x020d0, 0x020dc },
    { 0x020e1, 0x020e1 },
    { 0x020e5, 0x020f0 },
    { 0x02102, 0x02102 },
    { 0x02107, 0x02107 },
    { 0x0210a, 0x02113 },
    { 0x02115, 0x02115 },
    { 0x02119, 0x0211d },
    { 0x02124, 0x02124 },
    { 0x02126, 0x02126 },
    { 0x02128, 0x02128 },
    { 0x0212a, 0x0212d },
    { 0x0212f, 0x02139 },
    { 0x0213c, 0x0213f },
    { 0x02145, 0x02149 },
    { 0x0214e, 0x0214e },
    { 0x02160, 0x02188 },
    { 0x02c00, 0x02c2e },
    { 0x02c30, 0x02c5e },
    { 0x02c60, 0x02ce4 },
    { 0x02ceb, 0x02cf3 },
    { 0x02d00, 0x02d25 },
    { 0x02d27, 0x02d27 },
    { 0x02d2d, 0x02d2d },
    { 0x02d30, 0x02d67 },
    { 0x02d6f, 0x02d6f },
    { 0x02d7f, 0x02d96 },
    { 0x02da0, 0x02da6 },
    { 0x02da8, 0x02dae },
    { 0x02db0, 0x02db6 },
    { 0x02db8, 0x02dbe },
    { 0x02dc0, 0x02dc6 },
    { 0x02dc8, 0x02dce },
    { 0x02dd0, 0x02dd6 },
    { 0x02dd8, 0x02dde },
    { 0x02de0, 0x02dff },
    { 0x02e2f, 0x02e2f },
    { 0x03005, 0x03007 },
    { 0x03021, 0x0302f },
    { 0x03031, 0x03035 },
    { 0x03038, 0x0303c },
    { 0x03041, 0x03096 },
    { 0x03099, 0x0309a },
    { 0x0309d, 0x0309f },
    { 0x030a1, 0x030fa },
    { 0x030fc, 0x030ff },
    { 0x03105, 0x0312d },
    { 0x03131, 0x0318e },
    { 0x031a0, 0x031ba },
    { 0x031f0, 0x031ff },
    { 0x03400, 0x04db5 },
    { 0x04e00, 0x09fcc },
    { 0x0a000, 0x0a48c },
    { 0x0a4d0, 0x0a4fd },
    { 0x0a500, 0x0a60c },
    { 0x0a610, 0x0a62b },
    { 0x0a640, 0x0a66f },
    { 0x0a674, 0x0a67d },
    { 0x0a67f, 0x0a697 },
    { 0x0a69f, 0x0a6f1 },
    { 0x0a717, 0x0a71f },
    { 0x0a722, 0x0a788 },
    { 0x0a78b, 0x0a78e },
    { 0x0a790, 0x0a79f },
    { 0x0a7a0, 0x0a7b1 },
    { 0x0a7f8, 0x0a827 },
    { 0x0a840, 0x0a873 },
    { 0x0a880, 0x0a8c4 },
    { 0x0a8d0, 0x0a8d9 },
    { 0x0a8e0, 0x0a8f7 },
    { 0x0a8fb, 0x0a8fb },
    { 0x0a900, 0x0a92d },
    { 0x0a930, 0x0a953 },
    { 0x0a960, 0x0a97c },
    { 0x0a980, 0x0a9c0 },
    { 0x0a9cf, 0x0a9d9 },
    { 0x0aa00, 0x0aa36 },
    { 0x0aa40, 0x0aa4d },
    { 0x0aa50, 0x0aa59 },
    { 0x0aa60, 0x0aa76 },
    { 0x0aa7a, 0x0aa7b },
    { 0x0aa80, 0x0aac2 },
    { 0x0aadb, 0x0aadd },
    { 0x0aae0, 0x0aaef },
    { 0x0aaf2, 0x0aaf6 },
    { 0x0ab01, 0x0ab06 },
    { 0x0ab09, 0x0ab0e },
    { 0x0ab11, 0x0ab16 },
    { 0x0ab20, 0x0ab26 },
    { 0x0ab28, 0x0ab2e },
    { 0x0abc0, 0x0abea },
    { 0x0abec, 0x0abed },
    { 0x0abf0, 0x0abf9 },
    { 0x0ac00, 0x0d7a3 },
    { 0x0d7b0, 0x0d7c6 },
    { 0x0d7cb, 0x0d7fb },
    { 0x0f900, 0x0fa6d },
    { 0x0fa70, 0x0fad9 },
    { 0x0fb00, 0x0fb06 },
    { 0x0fb13, 0x0fb17 },
    { 0x0fb1d, 0x0fb28 },
    { 0x0fb2a, 0x0fb36 },
    { 0x0fb38, 0x0fb3c },
    { 0x0fb3e, 0x0fb3e },
    { 0x0fb40, 0x0fb41 },
    { 0x0fb43, 0x0fb44 },
    { 0x0fb46, 0x0fbb1 },
    { 0x0fbd3, 0x0fd3d },
    { 0x0fd50, 0x0fd8f },
    { 0x0fd92, 0x0fdc7 },
    { 0x0fdf0, 0x0fdfb },
    { 0x0fe00, 0x0fe0f },
    { 0x0fe20, 0x0fe26 },
    { 0x0fe33, 0x0fe34 },
    { 0x0fe4d, 0x0fe4f },
    { 0x0fe70, 0x0fe74 },
    { 0x0fe76, 0x0fefc },
    { 0x0ff10, 0x0ff19 },
    { 0x0ff21, 0x0ff3a },
    { 0x0ff3f, 0x0ff3f },
    { 0x0ff41, 0x0ff5a },
    { 0x0ff66, 0x0ffbe },
    { 0x0ffc2, 0x0ffc7 },
    { 0x0ffca, 0x0ffcf },
    { 0x0ffd2, 0x0ffd7 },
    { 0x0ffda, 0x0ffdc },
    { 0x10000, 0x1000b },
    { 0x1000d, 0x10026 },
    { 0x10028, 0x1003a },
    { 0x1003c, 0x1003d },
    { 0x1003f, 0x1004d },
    { 0x10050, 0x1005d },
    { 0x10080, 0x100fa },
    { 0x10140, 0x10174 },
    { 0x101fd, 0x101fd },
    { 0x10280, 0x1029c },
    { 0x102a0, 0x102d0 },
    { 0x10300, 0x1031e },
    { 0x10330, 0x1034a },
    { 0x10380, 0x1039d },
    { 0x103a0, 0x103c3 },
    { 0x103c8, 0x103cf },
    { 0x103d1, 0x103d5 },
    { 0x10400, 0x1049d },
    { 0x104a0, 0x104a9 },
    { 0x10800, 0x10805 },
    { 0x10808, 0x10808 },
    { 0x1080a, 0x10835 },
    { 0x10837, 0x10838 },
    { 0x1083c, 0x1083c },
    { 0x1083f, 0x10855 },
    { 0x10900, 0x10915 },
    { 0x10920, 0x10939 },
    { 0x10980, 0x109b7 },
    { 0x109be, 0x109bf },
    { 0x10a00, 0x10a03 },
    { 0x10a05, 0x10a06 },
    { 0x10a0c, 0x10a13 },
    { 0x10a15, 0x10a17 },
    { 0x10a19, 0x10a33 },
    { 0x10a38, 0x10a3a },
    { 0x10a3f, 0x10a3f },
    { 0x10a60, 0x10a7c },
    { 0x10b00, 0x10b35 },
    { 0x10b40, 0x10b55 },
    { 0x10b60, 0x10b72 },
    { 0x10c00, 0x10c48 },
    { 0x11000, 0x11046 },
    { 0x11066, 0x1106f },
    { 0x11080, 0x110ba },
    { 0x110d0, 0x110e8 },
    { 0x110f0, 0x110f9 },
    { 0x11100, 0x11134 },
    { 0x11136, 0x1113f },
    { 0x11180, 0x111c8 },
    { 0x111d0, 0x111da },
    { 0x11680, 0x116b7 },
    { 0x116c0, 0x116c9 },
    { 0x12000, 0x1236e },
    { 0x12400, 0x12462 },
    { 0x13000, 0x1342e },
    { 0x16800, 0x16a38 },
    { 0x11f00, 0x16f44 },
    { 0x11f50, 0x16f7e },
    { 0x11f8f, 0x16f9f },
    { 0x1b000, 0x1b001 },
    { 0x1d165, 0x1d169 },
    { 0x1d16d, 0x1d172 },
    { 0x1d17b, 0x1d182 },
    { 0x1d185, 0x1d18b },
    { 0x1d1aa, 0x1d1ad },
    { 0x1d242, 0x1d244 },
    { 0x1d400, 0x1d454 },
    { 0x1d456, 0x1d49c },
    { 0x1d49e, 0x1d49f },
    { 0x1d4a2, 0x1d4a2 },
    { 0x1d4a5, 0x1d4a6 },
    { 0x1d4a9, 0x1d4ac },
    { 0x1d4ae, 0x1d4b9 },
    { 0x1d4bb, 0x1d4bb },
    { 0x1d4bd, 0x1d4c3 },
    { 0x1d4c5, 0x1d505 },
    { 0x1d507, 0x1d50a },
    { 0x1d50d, 0x1d514 },
    { 0x1d516, 0x1d51c },
    { 0x1d51e, 0x1d539 },
    { 0x1d53b, 0x1d53e },
    { 0x1d540, 0x1d544 },
    { 0x1d546, 0x1d546 },
    { 0x1d54a, 0x1d550 },
    { 0x1d552, 0x1d6a5 },
    { 0x1d6a8, 0x1d6c0 },
    { 0x1d6c2, 0x1d6da },
    { 0x1d6dc, 0x1d6fa },
    { 0x1d6fc, 0x1d714 },
    { 0x1d716, 0x1d734 },
    { 0x1d736, 0x1d74e },
    { 0x1d750, 0x1d76e },
    { 0x1d770, 0x1d788 },
    { 0x1d78a, 0x1d7a8 },
    { 0x1d7aa, 0x1d7c2 },
    { 0x1d7c4, 0x1d7cb },
    { 0x1d7ce, 0x1d7ff },
    { 0x1ee00, 0x1ee03 },
    { 0x1ee05, 0x1ee1f },
    { 0x1ee21, 0x1ee22 },
    { 0x1ee24, 0x1ee24 },
    { 0x1ee27, 0x1ee27 },
    { 0x1ee29, 0x1ee32 },
    { 0x1ee34, 0x1ee37 },
    { 0x1ee39, 0x1ee39 },
    { 0x1ee3b, 0x1ee3b },
    { 0x1ee42, 0x1ee42 },
    { 0x1ee47, 0x1ee47 },
    { 0x1ee49, 0x1ee49 },
    { 0x1ee4b, 0x1ee4b },
    { 0x1ee4d, 0x1ee4f },
    { 0x1ee51, 0x1ee52 },
    { 0x1ee54, 0x1ee54 },
    { 0x1ee57, 0x1ee57 },
    { 0x1ee59, 0x1ee59 },
    { 0x1ee5b, 0x1ee5b },
    { 0x1ee5d, 0x1ee5d },
    { 0x1ee5f, 0x1ee5f },
    { 0x1ee61, 0x1ee62 },
    { 0x1ee64, 0x1ee64 },
    { 0x1ee67, 0x1ee6a },
    { 0x1ee6c, 0x1ee72 },
    { 0x1ee74, 0x1ee77 },
    { 0x1ee79, 0x1ee7c },
    { 0x1ee7e, 0x1ee7e },
    { 0x1ee80, 0x1ee89 },
    { 0x1ee8b, 0x1ee9b },
    { 0x1eea1, 0x1eea3 },
    { 0x1eea5, 0x1eea9 },
    { 0x1eeab, 0x1eebb },
    { 0x1eef0, 0x1eef1 },
    { 0x20000, 0x2a6d6 },
    { 0x2a700, 0x2b734 },
    { 0x2b740, 0x2b81d },
    { 0x2f800, 0x2fa1d },
    { 0xe0100, 0xe01ef }
};

/** \brief The size of the character table.
 *
 * When defining the type of a character, the Lexer uses the
 * character table. This parameter defines the number of
 * entries defined in the table.
 */
size_t const g_identifier_characters_size = sizeof(g_identifier_characters) / sizeof(g_identifier_characters[0]);


}
// no name namespace

/**********************************************************************/
/**********************************************************************/
/***  PARSER CREATOR  *************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Initialize the lexer object.
 *
 * The constructor of the Lexer expect a valid pointer of an Input
 * stream.
 *
 * It optionally accepts an Options pointer. If the pointer is null,
 * then all the options are assumed to be set to zero (0). So all
 * extensions are turned off.
 *
 * \param[in] input  The input stream.
 * \param[in] options  A set of options, may be null.
 */
Lexer::Lexer(Input::pointer_t input, Options::pointer_t options)
    : f_input(input)
    , f_options(options)
    //, f_char_type(CHAR_NO_FLAGS) -- auto-init
    //, f_position() -- auto-init
    //, f_result_type(NODE_UNKNOWN) -- auto-init
    //, f_result_string("") -- auto-init
    //, f_result_int64(0) -- auto-init
    //, f_result_float64(0.0) -- auto-init
{
    if(!f_input)
    {
        throw exception_invalid_data("The 'input' pointer cannot be null in the Lexer() constructor.");
    }
    if(!f_options)
    {
        throw exception_invalid_data("The 'options' pointer cannot be null in the Lexer() constructor.");
    }
}



/** \brief Retrieve the input stream pointer.
 *
 * This function returns the input stream pointer of the Lexer object.
 *
 * \return The input pointer as specified when creating the Lexer object.
 */
Input::pointer_t Lexer::get_input() const
{
    return f_input;
}


/** \brief Retrieve the next character of input.
 *
 * This function reads one character of input and returns it.
 *
 * If the character is a newline, linefeed, etc. it affects the current
 * line number, page number, etc. as required. The following characters
 * have such an effect:
 *
 * \li '\\n' -- the newline character adds a new line
 * \li '\\r' -- the carriage return character adds a new line; if followed
 *              by a '\n', remove it too; always return '\\n' and not '\\r'
 * \li '\\f' -- the formfeed adds a new page
 * \li LINE SEPARATOR (0x2028) -- add a new line
 * \li PARAGRAPH SEPARATOR (0x2029) -- add a new paragraph
 *
 * If the ungetc() function was called before a call to getc(), then
 * that last character is returned instead of a new character from the
 * input stream. In that case, the character has no effect on the line
 * number, page number, etc.
 *
 * \internal
 *
 * \return The next Unicode character.
 */
Input::char_t Lexer::getc()
{
    Input::char_t c;

    // if some characters were ungotten earlier, re-read those first
    // and avoid any side effects on the position... (which means
    // we could be a bit off, but the worst case is for regular expressions
    // and assuming the regular expression is valid, it will not be a
    // problem either...)
    if(!f_unget.empty())
    {
        c = f_unget.back();
        f_unget.pop_back();
        f_char_type = char_type(c);
    }
    else
    {
        c = f_input->getc();

        f_char_type = char_type(c);
        if((f_char_type & (CHAR_LINE_TERMINATOR | CHAR_WHITE_SPACE)) != 0)
        {
            // Unix (Linux, Mac OS/X, HP-UX, SunOS, etc.) uses '\n'
            // Microsoft (MS-DOS, MS-Windows) uses '\r\n'
            // Macintosh (OS 1 to OS 9, and Apple 1,2,3) uses '\r'
            switch(c)
            {
            case '\n':   // LINE FEED (LF)
                // '\n' represents a newline
                f_input->get_position().new_line();
                break;

            case '\r':   // CARRIAGE RETURN (CR)
                // skip '\r\n' as one newline
                // also in case we are on Mac, skip each '\r' as one newline
                f_input->get_position().new_line();
                c = f_input->getc();
                if(c != '\n') // if '\n' follows, skip it silently
                {
                    ungetc(c);
                }
                c = '\n';
                break;

            case '\f':   // FORM FEED (FF)
                // view the form feed as a new page for now...
                f_input->get_position().new_page();
                break;

            //case 0x0085: // NEXT LINE (NEL) -- not in ECMAScript 5
            //    // 
            //    f_input->get_position().new_line();
            //    break;

            case 0x2028: // LINE SEPARATOR (LSEP)
                f_input->get_position().new_line();
                break;

            case 0x2029: // PARAGRAPH SEPARATOR (PSEP)
                f_input->get_position().new_paragraph();
                break;

            }
        }
    }

    return c;
}


/** \brief Unget a character.
 *
 * Whenever reading a token, it is most often that the end of the token
 * is discovered by reading one too many character. This function is
 * used to push that character back in the input stream.
 *
 * Also the stream implementation also includes an unget, we do not use
 * that unget. The reason is that the getc() function needs to know
 * whether the character is a brand new character from that input stream
 * or the last ungotten character. The difference is important to know
 * whether the character has to have an effect on the line number,
 * page number, etc.
 *
 * The getc() function first returns the last character sent via
 * ungetc() (i.e. LIFO).
 *
 * \internal
 *
 * \param[in] c  The input character to "push back in the stream".
 */
void Lexer::ungetc(Input::char_t c)
{
    // WARNING: we do not use the f_input ungetc() because otherwise
    //          it would count lines, paragraphs, or pages twice,
    //          which would be a problem...
    if(c > 0 && c < 0x110000)
    {
        // unget only if not an invalid characters (especially not EOF)
        f_unget.push_back(c);
    }
}


/** \brief Determine the type of a character.
 *
 * This function determines the type of a character.
 *
 * The function first uses a switch for most of the characters used in
 * JavaScript are ASCII characters and thus are well defined and can
 * have their type defined in a snap.
 *
 * Unicode characters make use of a table to convert the character in
 * a type. Unicode character are either viewed as letters (CHAR_LETTER)
 * or as punctuation (CHAR_PUNCTUATION).
 *
 * The exceptions are the characters viewed as either line terminators
 * or white space characters. Those are captured by the switch.
 *
 * \important
 * Each character type is is a flag that can be used to check whether
 * the character is of a certain category, or a set of categories all
 * at once (i.e. (CHAR_LETTER | CHAR_DIGIT) means any character which
 * represents a letter or a digit.)
 *
 * \internal
 *
 * \param[in] c  The character of which the type is to be determined.
 *
 * \return The character type (one of the CHAR_...)
 */
Lexer::char_type_t Lexer::char_type(Input::char_t c)
{
    switch(c) {
    case '\0':   // NULL (NUL)
    case String::STRING_CONTINUATION: // ( '\' + line terminator )
        return CHAR_INVALID;

    case '\n':   // LINE FEED (LF)
    case '\r':   // CARRIAGE RETURN (CR)
    //case 0x0085: // NEXT LINE (NEL) -- not in ECMAScript 5
    case 0x2028: // LINE SEPARATOR (LSEP)
    case 0x2029: // PARAGRAPH SEPARATOR (PSEP)
        return CHAR_LINE_TERMINATOR;

    case '\t':   // CHARACTER TABULATION (HT)
    case '\v':   // LINE TABULATION (VT)
    case '\f':   // FORM FEED (FF)
    case ' ':    // SPACE (SP)
    case 0x00A0: // NO-BREAK SPACE
    case 0x1680: // OGHAM SPACE MARK
    case 0x180E: // MOGOLIAN VOWEL SEPARATOR (MVS)
    case 0x2000: // EN QUAD (NQSP)
    case 0x2001: // EM QUAD (MQSP)
    case 0x2002: // EN SPACE (EMSP)
    case 0x2003: // EM SPACE (ENSP)
    case 0x2004: // THREE-PER-EM SPACE (3/MSP)
    case 0x2005: // FOUR-PER-EM SPACE (4/MSP)
    case 0x2006: // SIX-PER-EM SPACE (6/MSP)
    case 0x2007: // FIGURE SPACE (FSP)
    case 0x2008: // PUNCTUATION SPACE (PSP)
    case 0x2009: // THIN SPACE (THSP)
    case 0x200A: // HAIR SPACE HSP)
    //case 0x200B: // ZERO WIDTH SPACE (ZWSP) -- this was accepted before, but it is not marked as a Zs category
    case 0x202F: // NARROW NO-BREAK SPACE (NNBSP)
    case 0x205F: // MEDIUM MATHEMATICAL SPACE (MMSP)
    case 0x3000: // IDEOGRAPHIC SPACE (IDSP)
    case 0xFEFF: // BYTE ORDER MARK (BOM)
        return CHAR_WHITE_SPACE;

    case '0': // '0' ... '9'
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return CHAR_DIGIT | CHAR_HEXDIGIT;

    case 'a': // 'a' ... 'f'
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'A': // 'A' ... 'F'
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
        return CHAR_LETTER | CHAR_HEXDIGIT;

    case '_':
    case '$':
        return CHAR_LETTER;

    default:
        if((c >= 'g' && c <= 'z')
        || (c >= 'G' && c <= 'Z'))
        {
            return CHAR_LETTER;
        }
        if((c & 0x0FFFF) >= 0xFFFE
        || (c >= 0xD800 && c <= 0xDFFF))
        {
            // 0xFFFE and 0xFFFF are invalid in all planes
            // surrogates are not valid standalone characters
            return CHAR_INVALID;
        }
        if(c < 0x7F)
        {
            return CHAR_PUNCTUATION;
        }
        // TODO: this will be true in most cases, but not always!
        //       documentation says:
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
        //
        // TODO: test with std::lower_bound() instead...
        //
        //identifier_characters_t searched{c, 0};
        //auto const & it(std::lower_bound(g_identifier_characters, g_identifier_characters + g_identifier_characters_size, searched);
        //if(it != g_identifier_characters + g_identifier_characters_size
        //&& c <= it->f_max) // make sure upper bound also matches
        //{
        //    return CHAR_LETTER;
        //}

        {
            size_t i, j, p;
            int    r;

            i = 0;
            j = g_identifier_characters_size;
            while(i < j)
            {
                p = (j - i) / 2 + i;
                if(g_identifier_characters[p].f_min <= c && c <= g_identifier_characters[p].f_max)
                {
                    return CHAR_LETTER;
                }
                r = g_identifier_characters[p].f_min - c;
                if(r < 0)
                {
                    i = p + 1;
                }
                else
                {
                    j = p;
                }
            }
        }
        return CHAR_PUNCTUATION;

    }
    /*NOTREACHED*/
}




/** \brief Read an hexadecimal number.
 *
 * This function reads 0's and 1's up until another character is found
 * or \p max digits were read. That other character is ungotten so the
 * next call to getc() will return that non-binary character.
 *
 * Since the function is called without an introducing digit, the
 * number could end up being empty. If that happens, an error is
 * generated and the function returns -1 (although -1 is a valid
 * number assuming you accept all 64 bits.)
 *
 * \internal
 *
 * \param[in] max  The maximum number of digits to read.
 *
 * \return The number just read as an integer (64 bit).
 */
int64_t Lexer::read_hex(unsigned long max)
{
    int64_t result(0);
    Input::char_t c(getc());
    unsigned long p(0);
    for(; (f_char_type & CHAR_HEXDIGIT) != 0 && p < max; ++p)
    {
        if(c <= '9')
        {
            result = result * 16 + c - '0';
        }
        else if(c <= 'F')
        {
            result = result * 16 + c - ('A' - 10);
        }
        else if(c <= 'f')
        {
            result = result * 16 + c - ('a' - 10);
        }
        c = getc();
    }
    ungetc(c);

    if(p == 0)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
        msg << "invalid hexadecimal number, at least one digit is required";
        return -1;
    }

    // TODO: In strict mode, should we check whether we got p == max?
    // WARNING: this is also used by the ReadNumber() function

    return result;
}


/** \brief Read a binary number.
 *
 * This function reads 0's and 1's up until another character is found
 * or \p max digits were read. That other character is ungotten so the
 * next call to getc() will return that non-binary character.
 *
 * Since the function is called without an introducing digit, the
 * number could end up being empty. If that happens, an error is
 * generated and the function returns -1 (although -1 is a valid
 * number assuming you accept all 64 bits.)
 *
 * \internal
 *
 * \param[in] max  The maximum number of digits to read.
 *
 * \return The number just read as an integer (64 bit).
 */
int64_t Lexer::read_binary(unsigned long max)
{
    int64_t result(0);
    Input::char_t c(getc());
    unsigned long p(0);
    for(; (c == '0' || c == '1') && p < max; ++p)
    {
        result = result * 2 + c - '0';
        c = getc();
    }
    ungetc(c);

    if(p == 0)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
        msg << "invalid binary number, at least one digit is required";
        return -1;
    }

    return result;
}


/** \brief Read an octal number.
 *
 * This function reads octal digits up until a character other than a
 * valid octal digit or \p max digits were read. That character is
 * ungotten so the next call to getc() will return that non-octal
 * character.
 *
 * \internal
 *
 * \param[in] c  The character that triggered a call to read_octal().
 * \param[in] max  The maximum number of digits to read.
 *
 * \return The number just read as an integer (64 bit).
 */
int64_t Lexer::read_octal(Input::char_t c, unsigned long max)
{
    int64_t result(c - '0');
    c = getc();
    for(unsigned long p(1); c >= '0' && c <= '7' && p < max; ++p, c = getc())
    {
        result = result * 8 + c - '0';
    }
    ungetc(c);

    return result;
}


/** \brief Read characters representing an escape sequence.
 *
 * This function reads the next few characters transforming them in one
 * escape sequence character.
 *
 * Some characters are extensions and require the extended escape
 * sequences to be turned on in order to be accepted. These are marked
 * as an extension in the list below.
 *
 * The function supports:
 *
 * \li \\u#### -- the 4 digit Unicode character
 * \li \\U######## -- the 8 digit Unicode character, this is an extension
 * \li \\x## or \\X## -- the 2 digit ISO-8859-1 character
 * \li \\' -- escape the single quote (') character
 * \li \\" -- escape the double quote (") character
 * \li \\\\ -- escape the backslash (\) character
 * \li \\b -- the backspace character
 * \li \\e -- the escape character, this is an extension
 * \li \\f -- the formfeed character
 * \li \\n -- the newline character
 * \li \\r -- the carriage return character
 * \li \\t -- the tab character
 * \li \\v -- the vertical tab character
 * \li \\\<newline> or \\\<#x2028> or \\\<#x2029> -- continuation characters
 * \li \\### -- 1 to 3 octal digit ISO-8859-1 character, this is an extension
 * \li \\0 -- the NUL character
 *
 * Any other character generates an error message if appearing after a
 * backslash (\).
 *
 * \internal
 *
 * \param[in] accept_continuation  Whether the backslash + newline combination
 *                                 is acceptable in this token.
 *
 * \return The escape character if valid, '?' otherwise.
 */
Input::char_t Lexer::escape_sequence(bool accept_continuation)
{
    Input::char_t c(getc());
    switch(c)
    {
    case 'u':
        // 4 hex digits
        return read_hex(4);

    case 'U':
        // We support full Unicode without the need for the programmer to
        // encode his characters in UTF-16 by hand! The compiler spits out
        // the characters using two '\uXXXX' characters.
        if(has_option_set(Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES))
        {
            // 8 hex digits
            return read_hex(8);
        }
        break;

    case 'x':
    case 'X':
        // 2 hex digits
        return read_hex(2);

    case '\'':
    case '\"':
    case '\\':
        return c;

    case 'b':
        return '\b';

    case 'e':
        if(has_option_set(Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES))
        {
            return '\033';
        }
        break;

    case 'f':
        return '\f';

    case 'n':
        return '\n';

    case 'r':
        return '\r';

    case 't':
        return '\t';

    case 'v':
        return '\v';

    case '\n':
    case 0x2028:
    case 0x2029:
        if(accept_continuation)
        {
            return String::STRING_CONTINUATION;
        }
        // make sure line terminators do not get skipped
        ungetc(c);
        break;

    default:
        if(has_option_set(Options::option_t::OPTION_EXTENDED_ESCAPE_SEQUENCES))
        {
            if(c >= '0' && c <= '7')
            {
                return read_octal(c, 3);
            }
        }
        else
        {
            if(c == '0')
            {
                return '\0';
            }
        }
        break;

    }

    if(c > ' ' && c < 0x7F)
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNKNOWN_ESCAPE_SEQUENCE, f_input->get_position());
        msg << "unknown escape letter '" << static_cast<char>(c) << "'";
    }
    else
    {
        Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNKNOWN_ESCAPE_SEQUENCE, f_input->get_position());
        msg << "unknown escape letter '\\U" << std::hex << std::setfill('0') << std::setw(8) << static_cast<int32_t>(c) << "'";
    }

    return '?';
}


/** \brief Read a set of characters as defined by \p flags.
 *
 * This function reads all the characters as long as their type match
 * the specified flags. The result is saved in the \p str parameter.
 *
 * At the time the function is called, \p c is expected to be the first
 * character to be added to \p str.
 *
 * The first character that does not satisfy the flags is pushed back
 * in the input stream so one can call getc() again to retrieve it.
 *
 * \param[in] c  The character that prompted this call and which ends up
 *               first in \p str.
 * \param[in] flags  The flags that must match each character, including
 *                   \p c character type.
 * \param[in,out] str  The resulting string. It is expected to be empty on
 *                     call but does not need to (it does not get cleared.)
 *
 * \internal
 *
 * \return The next character, although it was also ungotten.
 */
Input::char_t Lexer::read(Input::char_t c, char_type_t flags, String& str)
{
    do
    {
        if((f_char_type & CHAR_INVALID) == 0)
        {
            str += c;
        }
        c = getc();
    }
    while((f_char_type & flags) != 0 && c >= 0);

    ungetc(c);

    return c;
}



/** \brief Read an identifier.
 *
 * This function reads an identifier and checks whether that identifier
 * is a keyword.
 *
 * The list of reserved keywords has defined in ECMAScript is defined
 * below. Note that includes all versions (1 through 5) and we mark
 * all of these identifiers as keywords and we are NOT flexible at
 * all with those. (i.e. JavaScript allows for keywords to be used
 * as object field names as in 'myObj.break = 123;' and we do not.)
 *
 * \li abstract
 * \li boolean
 * \li break
 * \li byte
 * \li case
 * \li catch
 * \li char
 * \li class
 * \li const
 * \li continue
 * \li debugger
 * \li default
 * \li delete
 * \li do
 * \li double
 * \li else
 * \li enum
 * \li export
 * \li extends
 * \li false
 * \li final
 * \li finally
 * \li float
 * \li for
 * \li function
 * \li goto
 * \li if
 * \li implements
 * \li import
 * \li in
 * \li int
 * \li instanceof
 * \li interface
 * \li let
 * \li long
 * \li native
 * \li new
 * \li null
 * \li package
 * \li private
 * \li protected
 * \li public
 * \li return
 * \li short
 * \li static
 * \li super
 * \li switch
 * \li synchronized
 * \li this
 * \li throw
 * \li throws
 * \li transient
 * \li true
 * \li try
 * \li typeof
 * \li var
 * \li void
 * \li volatile
 * \li while
 * \li with
 * \li yield
 *
 * The function sets the f_result_type and f_result_string as required.
 *
 * We also understand additional keywords as defined here:
 *
 * \li as -- from ActionScript, to do a cast
 * \li is -- from ActionScript, to check a value type
 * \li namespace -- to encompass many declarations in a namespace
 * \li use -- to avoid having to declare certain namespaces, declare number
 *            types, change pragma (options) value
 *
 * We also support the special names:
 *
 * \li Infinity, which is supposed to be a global variable
 * \li NaN, which is supposed to be a global variable
 * \li undefined, which is supposed to never be defined
 * \li __FILE__, which gets transformed to the filename of the input stream
 * \li __LINE__, which gets transformed to the current line number
 *
 * \internal
 *
 * \param[in] c  The current character representing the first identifier character.
 */
void Lexer::read_identifier(Input::char_t c)
{
    // identifiers support character escaping like strings
    // so we have a special identifier read instead of
    // calling the read() function
    String str;
    for(;;)
    {
        // here escaping is not used to insert invalid characters
        // in a literal, but instead to add characters that
        // could otherwise be difficult to type (or possibly
        // difficult to share between users).
        //
        // so we immediately manage the backslash and use the
        // character type of the escape character!
        if(c == '\\')
        {
            c = escape_sequence(false);
            f_char_type = char_type(c);
            if((f_char_type & (CHAR_LETTER | CHAR_DIGIT)) == 0 || c < 0)
            {
                // do not unget() this character...
                break;
            }
        }
        else if((f_char_type & (CHAR_LETTER | CHAR_DIGIT)) == 0 || c < 0)
        {
            // unget this character
            ungetc(c);
            break;
        }
        if((f_char_type & CHAR_INVALID) == 0)
        {
            str += c;
        }
        c = getc();
    }

    // An identifier can be a keyword, we check that right here!
    size_t l(str.length());
    if(l > 1)
    {
        as_char_t const *s(str.c_str());
        switch(s[0])
        {
        case 'a':
            if(l == 8 && str == "abstract")
            {
                f_result_type = Node::node_t::NODE_ABSTRACT;
                return;
            }
            if(l == 2 && s[1] == 's')
            {
                f_result_type = Node::node_t::NODE_AS;
                return;
            }
            break;

        case 'b':
            if(l == 7 && str == "boolean")
            {
                f_result_type = Node::node_t::NODE_BOOLEAN;
                return;
            }
            if(l == 5 && str == "break")
            {
                f_result_type = Node::node_t::NODE_BREAK;
                return;
            }
            if(l == 4 && str == "byte")
            {
                f_result_type = Node::node_t::NODE_BYTE;
                return;
            }
            break;

        case 'c':
            if(l == 4 && str == "case")
            {
                f_result_type = Node::node_t::NODE_CASE;
                return;
            }
            if(l == 5 && str == "catch")
            {
                f_result_type = Node::node_t::NODE_CATCH;
                return;
            }
            if(l == 4 && str == "char")
            {
                f_result_type = Node::node_t::NODE_CHAR;
                return;
            }
            if(l == 5 && str == "class")
            {
                f_result_type = Node::node_t::NODE_CLASS;
                return;
            }
            if(l == 5 && str == "const")
            {
                f_result_type = Node::node_t::NODE_CONST;
                return;
            }
            if(l == 8 && str == "continue")
            {
                f_result_type = Node::node_t::NODE_CONTINUE;
                return;
            }
            break;

        case 'd':
            if(l == 8 && str == "debugger")
            {
                f_result_type = Node::node_t::NODE_DEBUGGER;
                return;
            }
            if(l == 7 && str == "default")
            {
                f_result_type = Node::node_t::NODE_DEFAULT;
                return;
            }
            if(l == 6 && str == "delete")
            {
                f_result_type = Node::node_t::NODE_DELETE;
                return;
            }
            if(l == 2 && s[1] == 'o')
            {
                f_result_type = Node::node_t::NODE_DO;
                return;
            }
            if(l == 6 && str == "double")
            {
                f_result_type = Node::node_t::NODE_DOUBLE;
                return;
            }
            break;

        case 'e':
            if(l == 4 && str == "else")
            {
                f_result_type = Node::node_t::NODE_ELSE;
                return;
            }
            if(l == 4 && str == "enum")
            {
                f_result_type = Node::node_t::NODE_ENUM;
                return;
            }
            if(l == 6 && str == "ensure")
            {
                f_result_type = Node::node_t::NODE_ENSURE;
                return;
            }
            if(l == 6 && str == "export")
            {
                f_result_type = Node::node_t::NODE_EXPORT;
                return;
            }
            if(l == 7 && str == "extends")
            {
                f_result_type = Node::node_t::NODE_EXTENDS;
                return;
            }
            break;

        case 'f':
            if(l == 5 && str == "false")
            {
                f_result_type = Node::node_t::NODE_FALSE;
                return;
            }
            if(l == 5 && str == "final")
            {
                f_result_type = Node::node_t::NODE_FINAL;
                return;
            }
            if(l == 7 && str == "finally")
            {
                f_result_type = Node::node_t::NODE_FINALLY;
                return;
            }
            if(l == 5 && str == "float")
            {
                f_result_type = Node::node_t::NODE_FLOAT;
                return;
            }
            if(l == 3 && s[1] == 'o' && s[2] == 'r')
            {
                f_result_type = Node::node_t::NODE_FOR;
                return;
            }
            if(l == 8 && str == "function")
            {
                f_result_type = Node::node_t::NODE_FUNCTION;
                return;
            }
            break;

        case 'g':
            if(l == 4 && str == "goto")
            {
                f_result_type = Node::node_t::NODE_GOTO;
                return;
            }
            break;

        case 'i':
            if(l == 2 && s[1] == 'f')
            {
                f_result_type = Node::node_t::NODE_IF;
                return;
            }
            if(l == 10 && str == "implements")
            {
                f_result_type = Node::node_t::NODE_IMPLEMENTS;
                return;
            }
            if(l == 6 && str == "import")
            {
                f_result_type = Node::node_t::NODE_IMPORT;
                return;
            }
            if(l == 2 && s[1] == 'n')
            {
                f_result_type = Node::node_t::NODE_IN;
                return;
            }
            if(l == 6 && str == "inline")
            {
                f_result_type = Node::node_t::NODE_INLINE;
                return;
            }
            if(l == 10 && str == "instanceof")
            {
                f_result_type = Node::node_t::NODE_INSTANCEOF;
                return;
            }
            if(l == 9 && str == "interface")
            {
                f_result_type = Node::node_t::NODE_INTERFACE;
                return;
            }
            if(l == 9 && str == "invariant")
            {
                f_result_type = Node::node_t::NODE_INVARIANT;
                return;
            }
            if(l == 2 && s[1] == 's')
            {
                f_result_type = Node::node_t::NODE_IS;
                return;
            }
            break;

        case 'I':
            if(l == 8 && str == "Infinity")
            {
                // Note:
                //
                // JavaScript does NOT automaticlly see this identifier as
                // a number, so you can write statements such as:
                //
                //     var Infinity = 123;
                //
                // On our end, by immediately transforming that identifier
                // into a number, we at least prevent such strange syntax
                // and we do not have to "specially" handle "Infinity" when
                // encountering an identifier.
                //
                // However, JavaScript considers Infinity as a read-only
                // object defined in the global scope. It can also be
                // retrieved from Number as in:
                //
                //     Number.POSITIVE_INFINITY
                //     Number.NEGATIVE_INFINITY
                //
                // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Infinity
                //
                f_result_type = Node::node_t::NODE_FLOAT64;
                f_result_float64.set_infinity();
                return;
            }
            break;

        case 'l':
            if(l == 4 && str == "long")
            {
                f_result_type = Node::node_t::NODE_LONG;
                return;
            }
            break;

        case 'n':
            if(l == 9 && str == "namespace")
            {
                f_result_type = Node::node_t::NODE_NAMESPACE;
                return;
            }
            if(l == 6 && str == "native")
            {
                f_result_type = Node::node_t::NODE_NATIVE;
                return;
            }
            if(l == 3 && s[1] == 'e' && s[2] == 'w')
            {
                f_result_type = Node::node_t::NODE_NEW;
                return;
            }
            if(l == 4 && str == "null")
            {
                f_result_type = Node::node_t::NODE_NULL;
                return;
            }
            break;

        case 'N':
            if(l == 3 && s[1] == 'a' && s[2] == 'N')
            {
                // Note:
                //
                // JavaScript does NOT automatically see this identifier as
                // a number, so you can write statements such as:
                //
                //     var NaN = 123;
                //
                // On our end, by immediately transforming that identifier
                // into a number, we at least prevent such strange syntax
                // and we do not have to "specially" handle "NaN" when
                // encountering an identifier.
                //
                // However, JavaScript considers NaN as a read-only
                // object defined in the global scope. It can also be
                // retrieved from Number as in:
                //
                //     Number.NaN
                //
                // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/NaN
                //
                f_result_type = Node::node_t::NODE_FLOAT64;
                f_result_float64.set_NaN();
                return;
            }
            break;

        case 'p':
            if(l == 7 && str == "package")
            {
                f_result_type = Node::node_t::NODE_PACKAGE;
                return;
            }
            if(l == 7 && str == "private")
            {
                f_result_type = Node::node_t::NODE_PRIVATE;
                return;
            }
            if(l == 9 && str == "protected")
            {
                f_result_type = Node::node_t::NODE_PROTECTED;
                return;
            }
            if(l == 6 && str == "public")
            {
                f_result_type = Node::node_t::NODE_PUBLIC;
                return;
            }
            break;

        case 'r':
            if(l == 7 && str == "require")
            {
                f_result_type = Node::node_t::NODE_REQUIRE;
                return;
            }
            if(l == 6 && str == "return")
            {
                f_result_type = Node::node_t::NODE_RETURN;
                return;
            }
            break;

        case 's':
            if(l == 5 && str == "short")
            {
                f_result_type = Node::node_t::NODE_SHORT;
                return;
            }
            if(l == 6 && str == "static")
            {
                f_result_type = Node::node_t::NODE_STATIC;
                return;
            }
            if(l == 5 && str == "super")
            {
                f_result_type = Node::node_t::NODE_SUPER;
                return;
            }
            if(l == 6 && str == "switch")
            {
                f_result_type = Node::node_t::NODE_SWITCH;
                return;
            }
            if(l == 12 && str == "synchronized")
            {
                f_result_type = Node::node_t::NODE_SYNCHRONIZED;
                return;
            }
            break;

        case 't':
            if(l == 4 && str == "then")
            {
                f_result_type = Node::node_t::NODE_THEN;
                return;
            }
            if(l == 4 && str == "this")
            {
                f_result_type = Node::node_t::NODE_THIS;
                return;
            }
            if(l == 5 && str == "throw")
            {
                f_result_type = Node::node_t::NODE_THROW;
                return;
            }
            if(l == 6 && str == "throws")
            {
                f_result_type = Node::node_t::NODE_THROWS;
                return;
            }
            if(l == 9 && str == "transient")
            {
                f_result_type = Node::node_t::NODE_TRANSIENT;
                return;
            }
            if(l == 4 && str == "true")
            {
                f_result_type = Node::node_t::NODE_TRUE;
                return;
            }
            if(l == 3 && s[1] == 'r' && s[2] == 'y')
            {
                f_result_type = Node::node_t::NODE_TRY;
                return;
            }
            if(l == 6 && str == "typeof")
            {
                f_result_type = Node::node_t::NODE_TYPEOF;
                return;
            }
            break;

        case 'u':
            if(l == 9 && str == "undefined")
            {
                // Note: undefined is actually not a reserved keyword, but
                //       by reserving it, we avoid stupid mistakes like:
                //
                //       var undefined = 5;
                //
                // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/undefined
                //
                f_result_type = Node::node_t::NODE_UNDEFINED;
                return;
            }
            if(l == 3 && s[1] == 's' && s[2] == 'e')
            {
                f_result_type = Node::node_t::NODE_USE;
                return;
            }
            break;

        case 'v':
            if(l == 3 && s[1] == 'a' && s[2] == 'r')
            {
                f_result_type = Node::node_t::NODE_VAR;
                return;
            }
            if(l == 4 && str == "void")
            {
                f_result_type = Node::node_t::NODE_VOID;
                return;
            }
            if(l == 8 && str == "volatile")
            {
                f_result_type = Node::node_t::NODE_VOLATILE;
                return;
            }
            break;

        case 'w':
            if(l == 5 && str == "while")
            {
                f_result_type = Node::node_t::NODE_WHILE;
                return;
            }
            if(l == 4 && str == "with")
            {
                f_result_type = Node::node_t::NODE_WITH;
                return;
            }
            break;

        case 'y':
            if(l == 5 && str == "yield")
            {
                f_result_type = Node::node_t::NODE_YIELD;
                return;
            }
            break;

        case '_':
            if(l == 8 && str == "__FILE__")
            {
                f_result_type = Node::node_t::NODE_STRING;
                f_result_string = f_input->get_position().get_filename();
                return;
            }
            if(l == 8 && str == "__LINE__")
            {
                f_result_type = Node::node_t::NODE_INT64;
                f_result_int64 = f_input->get_position().get_line();
                return;
            }
            break;

        }
    }

    if(l == 0)
    {
        f_result_type = Node::node_t::NODE_UNKNOWN;
    }
    else
    {
        f_result_type = Node::node_t::NODE_IDENTIFIER;
        f_result_string = str;
    }
}


/** \brief Read one number from the input stream.
 *
 * This function is called whenever a digit is found in the input
 * stream. It may also be called if a period was read (the rules
 * are a little more complicated for the period.)
 *
 * The function checks the following character, if it is:
 *
 * \li 'x' or 'X' -- it reads an hexadecimal number, see read_hex()
 * \li 'b' or 'B' -- it reads a binary number, see read_binary()
 * \li '0' -- if the number starts with a zero, it reads an octal,
 *            see read_octal()
 * \li '.' -- it reads a floating point number
 * \li otherwise it reads an integer, although if the integer is
 *     followed by '.', 'e', or 'E', it ends up reading the number
 *     as a floating point
 *
 * The result is directly saved in the necessary f_result_...
 * variables.
 *
 * \internal
 *
 * \param[in] c  The digit or period that triggered this call.
 */
void Lexer::read_number(Input::char_t c)
{
    String      number;

    // TODO: accept '_' within the number (between digits) like Java 7
    if(c == '.')
    {
        // in case the std::stod() does not support a missing 0
        // at the start of a floating point
        number = "0";
    }
    else if(c == '0')
    {
        c = getc();
        if(c == 'x' || c == 'X')
        {
            // hexadecimal number
            f_result_type = Node::node_t::NODE_INT64;
            f_result_int64 = read_hex(16);
            return;
        }
        if(has_option_set(Options::option_t::OPTION_BINARY)
        && (c == 'b' || c == 'B'))
        {
            // binary number
            f_result_type = Node::node_t::NODE_INT64;
            f_result_int64 = read_binary(64);
            return;
        }
        // octal is not permitted in ECMAScript version 3+
        // (especially in strict  mode)
        if(has_option_set(Options::option_t::OPTION_OCTAL)
        && c >= '0' && c <= '7')
        {
            // octal
            f_result_type = Node::node_t::NODE_INT64;
            f_result_int64 = read_octal(c, 22);
            return;
        }
        number = "0";
        ungetc(c);
    }
    else
    {
        c = read(c, CHAR_DIGIT, number);
    }

    // TODO: we may want to support 32 bits floats as well
    //       JavaScript really only supports 64 bit floats
    //       and nothing else...
    f_result_type = Node::node_t::NODE_FLOAT64;
    if(c == '.')
    {
        getc(); // re-read the '.' character

        Input::char_t f(getc()); // check the following character
        if(f != '.' && (f_char_type & CHAR_DIGIT) != 0)
        {
            ungetc(f);

            Input::char_t q(read(c, CHAR_DIGIT, number));
            if(q == 'e' || q == 'E')
            {
                getc();        // skip the 'e'
                c = getc();    // get the character after!
                if(c == '-' || c == '+' || (c >= '0' && c <= '9'))
                {
                    number += 'e';
                    c = read(c, CHAR_DIGIT, number);
                }
                else
                {
                    ungetc(c);
                    ungetc(q);
                    f_char_type = char_type(q); // restore this character type, we'll most certainly get an error
                }
            }
            // TODO: detect whether an error was detected in the conversion
            f_result_float64 = number.to_float64();
            return;
        }
        if(f == 'e' || f == 'E')
        {
            Input::char_t s(getc());
            if(s == '+' || s == '-')
            {
                Input::char_t e(getc());
                if((f_char_type & CHAR_DIGIT) != 0)
                {
                    // considered floating point
                    number += 'e';
                    number += s;
                    c = read(e, CHAR_DIGIT, number);
                    f_result_float64 = number.to_float64();
                    return;
                }
                ungetc(e);
            }
            // TODO:
            // Here we could check to know whether this really
            // represents a decimal number or whether the decimal
            // point is a member operator. This can be very tricky.
            //
            // This is partially done now, we still fail in cases
            // were someone was to use a member name such as e4z
            // because we would detect 'e' as exponent and multiply
            // the value by 10000... then fail on the 'z'
            if((f_char_type & CHAR_DIGIT) != 0)
            {
                // considered floating point
                number += 'e';
                c = read(s, CHAR_DIGIT, number);
                f_result_float64 = number.to_float64();
                return;
            }
            ungetc(s);
        }
        // restore the '.' and following character (another '.' or a letter)
        // this means we allow for 33.length and 3..5
        ungetc(f);
        ungetc('.');
        f_char_type = char_type('.');
    }
    else if(c == 'e' || c == 'E')
    {
        getc(); // re-read the 'e'

        Input::char_t s(getc());
        if(s == '+' || s == '-')
        {
            Input::char_t e(getc());
            if((f_char_type & CHAR_DIGIT) != 0)
            {
                // considered floating point
                number += 'e';
                number += s;
                c = read(e, CHAR_DIGIT, number);
                f_result_float64 = number.to_float64();
                return;
            }
            ungetc(e);
        }
        // TODO:
        // Here we could check to know whether this really
        // represents a decimal number or whether the decimal
        // point is a member operator. This can be very tricky.
        //
        // This is partially done now, we still fail in cases
        // were someone was to use a member name such as e4z
        // because we would detect 'e' as exponent and multiply
        // the value by 10000... then fail on the 'z'
        if((f_char_type & CHAR_DIGIT) != 0)
        {
            // considered floating point
            number += 'e';
            c = read(s, CHAR_DIGIT, number);
            f_result_float64 = number.to_float64();
            return;
        }
        ungetc(s);
    }


    // TODO: Support 8, 16, 32 bits, unsigned thereof?
    //       (we have NODE_BYTE and NODE_SHORT, but not really a 32bit
    //       definition yet; NODE_LONG should be 64 bits I think,
    //       although really all of those are types, not literals.)
    f_result_type = Node::node_t::NODE_INT64;

    // TODO: detect whether an error was detected in the conversion
    //       (this would mainly be overflows)
    f_result_int64 = std::stoull(number.to_utf8(), nullptr, 10);
}


/** \brief Read one string.
 *
 * This function reads one string from the input stream.
 *
 * The function expects \p quote as an input parameter representing the
 * opening quote. It will read the input stream up to the next line
 * terminator (unless escaped) or the closing quote.
 *
 * Note that we support backslash quoted "strings" which actually
 * represent regular expressions. These cannot be continuated on
 * the following line.
 *
 * This function sets the result type to NODE_STRING. It is changed
 * by the caller when a regular expression was found instead.
 *
 * \internal
 *
 * \param[in] quote  The opening quote, which will match the closing quote.
 */
void Lexer::read_string(Input::char_t quote)
{
    f_result_type = Node::node_t::NODE_STRING;
    f_result_string.clear();

    for(Input::char_t c(getc()); c != quote; c = getc())
    {
        if(c < 0)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNTERMINATED_STRING, f_input->get_position());
            msg << "the last string was not closed before the end of the input was reached";
            return;
        }
        if((f_char_type & CHAR_LINE_TERMINATOR) != 0)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNTERMINATED_STRING, f_input->get_position());
            msg << "a string cannot include a line terminator";
            return;
        }
        if(c == '\\')
        {
            c = escape_sequence(quote != '`');

            // here c can be equal to quote (c == quote)
        }
        if(c != String::STRING_CONTINUATION)
        {
            f_result_string += c;
        }
    }
}



/** \brief Create a new node of the specified type.
 *
 * This helper function creates a new node at the current position. This
 * is useful internally and in the parser when creating nodes to build
 * the input tree and in order for the new node to get the correct
 * position according to the current lexer position.
 *
 * \param[in] type  The type of the new node.
 *
 * \return A pointer to the new node.
 */
Node::pointer_t Lexer::get_new_node(Node::node_t type)
{
    Node::pointer_t node(new Node(type));
    node->set_position(f_position);
    // no data by default in this case
    return node;
}


/** \brief Get the next token from the input stream.
 *
 * This function reads one token from the input stream and transform
 * it in a Node. The Node is automatically assigned the position after
 * the token was read.
 *
 * \return The node representing the next token, or a NODE_EOF if the
 *         end of the stream was found.
 */
Node::pointer_t Lexer::get_next_token()
{
    // get the info
    get_token();

    // create a node for the result
    Node::pointer_t node(new Node(f_result_type));
    node->set_position(f_position);
    switch(f_result_type)
    {
    case Node::node_t::NODE_IDENTIFIER:
    case Node::node_t::NODE_REGULAR_EXPRESSION:
    case Node::node_t::NODE_STRING:
        node->set_string(f_result_string);
        break;

    case Node::node_t::NODE_INT64:
        if((f_char_type & CHAR_LETTER) != 0)
        {
            // numbers cannot be followed by a letter
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
            msg << "unexpected letter after an integer";
            f_result_int64 = -1;
        }
        node->set_int64(f_result_int64);
        break;

    case Node::node_t::NODE_FLOAT64:
        if((f_char_type & CHAR_LETTER) != 0)
        {
            // numbers cannot be followed by a letter
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_INVALID_NUMBER, f_input->get_position());
            msg << "unexpected letter after a floating point number";
            f_result_float64 = -1.0;
        }
        node->set_float64(f_result_float64);
        break;

    default:
        // no data attached
        break;

    }
    return node;
}


/** \brief Read one token in the f_result_... variables.
 *
 * This function reads one token from the input stream. It reads one
 * character and determine the type of token (identifier, string,
 * number, etc.) and then reads the whole token.
 *
 * The main purpose of the function is to read characters from the
 * stream and determine what token it represents. It uses many
 * sub-functions to read more complex tokens such as identifiers
 * and numbers.
 *
 * If the end of the input stream is reached, the function returns
 * with a NODE_EOF. The function can be called any number of times
 * after the end of the input is reached.
 *
 * Only useful tokens are returned. Comments and white spaces (space,
 * tab, new line, line feed, etc.) are all skipped silently.
 *
 * The function detects invalid characters which are ignored although
 * the function will first emit an error.
 *
 * This is the function that handles the case of a regular expression
 * written between slashes (/.../). One can also use the backward
 * quotes (`...`) for regular expression to avoid potential confusions
 * with the divide character.
 *
 * \note
 * Most extended operators, such as the power operator (**) are
 * silently returned by this function. If the extended operators are
 * not allowed, the parser will emit an error as required. However,
 * a few operators (<> and :=) are returned jus like the standard
 * operator (NODE_NOT_EQUAL and NODE_ASSIGNMENT) and thus the error
 * has to be emitted here, and it is.
 *
 * \internal
 */
void Lexer::get_token()
{
    for(Input::char_t c(getc());; c = getc())
    {
        f_position = f_input->get_position();
        if(c < 0)
        {
            // we're done
            f_result_type = Node::node_t::NODE_EOF;
            return;
        }

        if((f_char_type & (CHAR_WHITE_SPACE | CHAR_LINE_TERMINATOR)) != 0)
        {
            continue;
        }

        if((f_char_type & CHAR_INVALID) != 0)
        {
            Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION, f_input->get_position());
            msg << "invalid character '\\U" << std::hex << std::setfill('0') << std::setw(8) << c << "' found as is in the input stream";
            continue;
        }

        if((f_char_type & CHAR_LETTER) != 0)
        {
            read_identifier(c);
            if(f_result_type == Node::node_t::NODE_UNKNOWN)
            {
                // skip empty identifiers, in most cases
                // this was invalid data in the input
                // and we will have had a message output
                // already so we do not have more to do
                // here
                continue; // LCOV_EXCL_LINE
            }
            return;
        }

        if((f_char_type & CHAR_DIGIT) != 0)
        {
            read_number(c);
            return;
        }

        switch(c)
        {
        case '\\':
            // identifiers can start with a character being escaped
            // (it still needs to be a valid character for an identifier though)
            read_identifier(c);
            if(f_result_type != Node::node_t::NODE_UNKNOWN)
            {
                // this is a valid token, return it
                return;
            }
            // not a valid identifier, ignore here
            // (the read_identifier() emits errors as required)
            break;

        case '"':
        case '\'':
        case '`':    // TODO: do we want to support the correct regex syntax?
            read_string(c);
            if(c == '`')
            {
                f_result_type = Node::node_t::NODE_REGULAR_EXPRESSION;
            }
            return;

        case '<':
            c = getc();
            if(c == '<')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_SHIFT_LEFT;
                return;
            }
            if(c == '=')
            {
                c = getc();
                if(c == '>')
                {
                    f_result_type = Node::node_t::NODE_COMPARE;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_LESS_EQUAL;
                return;
            }
            if(c == '%')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_ROTATE_LEFT;
                return;
            }
            if(c == '>')
            {
                // unfortunately we cannot know whether '<>' or '!=' was used
                // once this function returns so in this very specific case
                // the extended operator has to be checked here
                if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_input->get_position());
                    msg << "the '<>' operator is only available when extended operators are authorized (use extended_operators;).";
                }
                f_result_type = Node::node_t::NODE_NOT_EQUAL;
                return;
            }
            if(c == '?')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_MINIMUM;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_MINIMUM;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_LESS;
            return;

        case '>':
            c = getc();
            if(c == '>')
            {
                c = getc();
                if(c == '>')
                {
                    c = getc();
                    if(c == '=')
                    {
                        f_result_type = Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED;
                        return;
                    }
                    ungetc(c);
                    f_result_type = Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED;
                    return;
                }
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_SHIFT_RIGHT;
                return;
            }
            if(c == '=')
            {
                f_result_type = Node::node_t::NODE_GREATER_EQUAL;
                return;
            }
            if(c == '%')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_ROTATE_RIGHT;
                return;
            }
            if(c == '?')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_MAXIMUM;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_MAXIMUM;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_GREATER;
            return;

        case '!':
            c = getc();
            if(c == '~')
            {
                // http://perldoc.perl.org/perlop.html#Binding-Operators
                f_result_type = Node::node_t::NODE_NOT_MATCH;
                return;
            }
            if(c == '=')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_STRICTLY_NOT_EQUAL;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_NOT_EQUAL;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_LOGICAL_NOT;
            return;

        case '=':
            c = getc();
            if(c == '=')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_STRICTLY_EQUAL;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_EQUAL;
                return;
            }
            if((f_options->get_option(Options::option_t::OPTION_EXTENDED_OPERATORS) & 2) != 0)
            {
                // This one most people will not understand it...
                // The '=' operator by itself is often missused and thus a
                // big source of bugs. By forbiding it, we only allow :=
                // and == (and ===) which makes it safer to use the language.
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_input->get_position());
                msg << "the '=' operator is not available when extended operators value bit 1 is set (use extended_operators(2);).";
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_ASSIGNMENT;
            return;

        case ':':
            c = getc();
            if(c == '=')
            {
                // unfortunately we cannot know whether ':=' or '=' was used
                // once this function returns so in this very specific case
                // the extended operator has to be checked here
                if(!has_option_set(Options::option_t::OPTION_EXTENDED_OPERATORS))
                {
                    Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_NOT_ALLOWED, f_input->get_position());
                    msg << "the ':=' operator is only available when extended operators are authorized (use extended_operators;).";
                }
                f_result_type = Node::node_t::NODE_ASSIGNMENT;
                return;
            }
            if(c == ':')
            {
                f_result_type = Node::node_t::NODE_SCOPE;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_COLON;
            return;

        case '~':
            c = getc();
            if(c == '=')
            {
                // http://perldoc.perl.org/perlop.html#Binding-Operators
                // Note that we inverse it (perl uses =~) because otherwise
                // we may interfer with a valid expression:
                //    a = ~b;  <=>  a=~b;
                f_result_type = Node::node_t::NODE_MATCH;
                return;
            }
            if(c == '~')
            {
                // http://perldoc.perl.org/perlop.html#Smartmatch-Operator
                // WARNING: if ~~ is used as a unary, then it may get
                //          converted back to two BITWISE NOT by the
                //          parser (so 'a = ~~b;' works as expected).
                f_result_type = Node::node_t::NODE_SMART_MATCH;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_BITWISE_NOT;
            return;

        case '+':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::node_t::NODE_ASSIGNMENT_ADD;
                return;
            }
            if(c == '+')
            {
                f_result_type = Node::node_t::NODE_INCREMENT;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_ADD;
            return;

        case '-':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::node_t::NODE_ASSIGNMENT_SUBTRACT;
                return;
            }
            if(c == '-')
            {
                f_result_type = Node::node_t::NODE_DECREMENT;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_SUBTRACT;
            return;

        case '*':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::node_t::NODE_ASSIGNMENT_MULTIPLY;
                return;
            }
            if(c == '*')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_POWER;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_POWER;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_MULTIPLY;
            return;

        case '/':
            c = getc();
            if(c == '/')
            {
                // skip comments (to end of line)
                do
                {
                    c = getc();
                }
                while((f_char_type & CHAR_LINE_TERMINATOR) == 0 && c >= 0);
                break;
            }
            if(c == '*')
            {
                // skip comments (multiline)
                do
                {
                    c = getc();
                    while(c == '*')
                    {
                        c = getc();
                        if(c == '/')
                        {
                            c = -1;
                            break;
                        }
                    }
                }
                while(c > 0);
                break;
            }
            // before we can determine whether we have
            //    a literal RegExp
            //    a /=
            //    a /
            // we have to read more data to match a RegExp (so at least
            // up to another / with valid RegExp characters in between
            // or no such thing and we have to back off)
            {
                String regexp;
                Input::char_t r(c);
                for(;;)
                {
                    if(r < 0 || (f_char_type & CHAR_LINE_TERMINATOR) != 0 || r == '/')
                    {
                        break;
                    }
                    if((f_char_type & CHAR_INVALID) == 0)
                    {
                        regexp += r;
                    }
                    r = getc();
                }
                if(r == '/')
                {
                    // TBD -- shall we further verify that this looks like a
                    //        regular expression before accepting it as such?
                    //
                    // this is a valid regular expression written between /.../
                    // read the flags that follow if any
                    read(r, CHAR_LETTER | CHAR_DIGIT, regexp);
                    f_result_type = Node::node_t::NODE_REGULAR_EXPRESSION;
                    f_result_string = "/";
                    f_result_string += regexp;
                    return;
                }
                // not a regular expression, so unget all of that stuff
                size_t p(regexp.length());
                while(p > 0)
                {
                    --p;
                    ungetc(regexp[p]);
                }
                // 'c' is still the character gotten at the start of this case
            }
            if(c == '=')
            {
                // the '=' was ungotten, so skip it again
                getc();
                f_result_type = Node::node_t::NODE_ASSIGNMENT_DIVIDE;
                return;
            }
            f_result_type = Node::node_t::NODE_DIVIDE;
            return;

        case '%':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::node_t::NODE_ASSIGNMENT_MODULO;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_MODULO;
            return;

        case '?':
            f_result_type = Node::node_t::NODE_CONDITIONAL;
            return;

        case '&':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::node_t::NODE_ASSIGNMENT_BITWISE_AND;
                return;
            }
            if(c == '&')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_LOGICAL_AND;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_BITWISE_AND;
            return;

        case '^':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR;
                return;
            }
            if(c == '^')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_LOGICAL_XOR;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_BITWISE_XOR;
            return;

        case '|':
            c = getc();
            if(c == '=')
            {
                f_result_type = Node::node_t::NODE_ASSIGNMENT_BITWISE_OR;
                return;
            }
            if(c == '|')
            {
                c = getc();
                if(c == '=')
                {
                    f_result_type = Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR;
                    return;
                }
                ungetc(c);
                f_result_type = Node::node_t::NODE_LOGICAL_OR;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_BITWISE_OR;
            return;

        case '.':
            c = getc();
            if(c >= '0' && c <= '9')
            {
                // this is probably a valid float
                ungetc(c);
                ungetc('.');
                read_number('.');
                return;
            }
            if(c == '.')
            {
                c = getc();
                if(c == '.')
                {
                    // Elipsis!
                    f_result_type = Node::node_t::NODE_REST;
                    return;
                }
                ungetc(c);

                // Range (not too sure if this is really used yet
                // and whether it will be called RANGE)
                f_result_type = Node::node_t::NODE_RANGE;
                return;
            }
            ungetc(c);
            f_result_type = Node::node_t::NODE_MEMBER;
            return;

        case '[':
            f_result_type = Node::node_t::NODE_OPEN_SQUARE_BRACKET;
            return;

        case ']':
            f_result_type = Node::node_t::NODE_CLOSE_SQUARE_BRACKET;
            return;

        case '{':
            f_result_type = Node::node_t::NODE_OPEN_CURVLY_BRACKET;
            return;

        case '}':
            f_result_type = Node::node_t::NODE_CLOSE_CURVLY_BRACKET;
            return;

        case '(':
            f_result_type = Node::node_t::NODE_OPEN_PARENTHESIS;
            return;

        case ')':
            f_result_type = Node::node_t::NODE_CLOSE_PARENTHESIS;
            return;

        case ';':
            f_result_type = Node::node_t::NODE_SEMICOLON;
            return;

        case ',':
            f_result_type = Node::node_t::NODE_COMMA;
            return;

        case 0x221E: // INFINITY
            // unicode infinity character which is viewed as a punctuation
            // otherwise so we can reinterpret it safely (it could not be
            // part of an identifier)
            f_result_type = Node::node_t::NODE_FLOAT64;
            f_result_float64.set_infinity();
            return;

        case 0xFFFD: // REPACEMENT CHARACTER
            // Java has defined character FFFD as representing NaN so if
            // found in the input we take it as such...
            //
            // see Unicode pri74:
            // http://www.unicode.org/review/resolved-pri.html
            f_result_type = Node::node_t::NODE_FLOAT64;
            f_result_float64.set_NaN();
            return;

        default:
            if(c > ' ' && c < 0x7F)
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION, f_input->get_position());
                msg << "unexpected punctuation '" << static_cast<char>(c) << "'";
            }
            else
            {
                Message msg(message_level_t::MESSAGE_LEVEL_ERROR, err_code_t::AS_ERR_UNEXPECTED_PUNCTUATION, f_input->get_position());
                msg << "unexpected punctuation '\\U" << std::hex << std::setfill('0') << std::setw(8) << c << "'";
            }
            break;

        }
    }
    /*NOTREACHED*/
}


/** \brief Check whether a given option is set.
 *
 * Because the lexer checks options in many places, it makes use of this
 * helper function to simplify the many tests in the rest of the code.
 *
 * This function checks whether the specified option is set. If so,
 * then it returns true, otherwise it returns false.
 *
 * \note
 * Some options may be set to values other than 0 and 1. In that case
 * this function cannot be used. Right now, this function returns true
 * if the option is \em set, meaning that the option value is not zero.
 * For example, the OPTION_EXTENDED_OPERATORS option may be set to
 * 0, 1, 2, or 3.
 *
 * \param[in] option  The option to check.
 *
 * \return true if the option was set, false otherwise.
 */
bool Lexer::has_option_set(Options::option_t option) const
{
    return f_options->get_option(option) != 0;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
