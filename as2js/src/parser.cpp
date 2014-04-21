/* parser.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2009 */

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

#include "as2js/parser.h"


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  PARSER CREATOR  *************************************************/
/**********************************************************************/
/**********************************************************************/

Parser *Parser::CreateParser(void)
{
    return new IntParser();
}


const char *Parser::Version(void)
{
    return TO_STR(SSWF_VERSION);
}



/**********************************************************************/
/**********************************************************************/
/***  INTERNAL PARSER  ************************************************/
/**********************************************************************/
/**********************************************************************/

IntParser::IntParser(void)
{
    //f_lexer -- auto-init
    f_options = 0;
    //f_root -- auto-init [we keep it unknown at the start]
    //f_data -- auto-init
    f_unget_pos = 0;
    //f_unget[...] -- ignored with f_unget_pos = 0
}


IntParser::~IntParser()
{
    // required for the virtual-ness
}


void IntParser::SetInput(Input& input)
{
    f_lexer.SetInput(input);
}


void IntParser::SetOptions(Options& options)
{
    f_options = &options;
    f_lexer.SetOptions(options);
}


NodePtr& IntParser::Parse(void)
{
    // This parses everything and creates ONE tree
    // with the result. The tree obviously needs to
    // fit in RAM...

    // We lose the previous tree if any and create a new
    // root node. This is our program node.
    GetToken();
    Program(f_root);

    return f_root;
}


void IntParser::GetToken(void)
{
    bool reget = f_unget_pos > 0;

    if(f_unget_pos > 0) {
        --f_unget_pos;
        f_data = f_unget[f_unget_pos];
    }
    else {
        f_data = f_lexer.GetNextToken();
    }

    if(f_options != 0
    && f_options->GetOption(AS_OPTION_DEBUG_LEXER) != 0) {
        fprintf(stderr, "%s: ", reget ? "RE-TOKEN" : "TOKEN");
        f_data.Display(stderr);
        fprintf(stderr, "\n");
    }
}


void IntParser::UngetToken(const Data& data)
{
    AS_ASSERT(f_unget_pos < MAX_UNGET);

    f_unget[f_unget_pos] = data;
    ++f_unget_pos;
}




}
// namespace as

// vim: ts=4 sw=4 et
