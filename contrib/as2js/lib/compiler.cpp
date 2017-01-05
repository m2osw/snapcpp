/* compiler.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/compiler.h"


namespace as2js
{







/** \brief Initialize the compiler object.
 *
 * The compiler includes many sub-classes that it initializes here.
 * Especially, it calls the internal_imports() function to load all
 * the internal modules, database, resource file.
 *
 * The options parameter represents the command line options setup
 * by a user and within the code with the 'use' keyword (i.e. pragmas).
 *
 * \param[in] options  The options object to use while compiling.
 */
Compiler::Compiler(Options::pointer_t options)
    : f_time(time(nullptr))
    //, f_optimizer(nullptr) -- auto-init
    , f_options(options)
    //, f_program(nullptr) -- auto-init
    //, f_input_retriever(nullptr) -- auto-init
    //, f_err_flags(0) -- auto-init
    //, f_scope(nullptr) -- auto-init
    //, f_modules() -- auto-init
{
    internal_imports();
}


Compiler::~Compiler()
{
}


InputRetriever::pointer_t Compiler::set_input_retriever(InputRetriever::pointer_t retriever)
{
    InputRetriever::pointer_t old(f_input_retriever);

    f_input_retriever = retriever;

    return old;
}










}
// namespace as2js

// vim: ts=4 sw=4 et
