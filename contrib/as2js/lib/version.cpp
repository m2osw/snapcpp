/* version.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "as2js/as2js.h"

#include    <iostream>

/** \file
 * \brief Define the version of the as2js library.
 *
 * This file implements the function used to retrieve the library
 * version at runtime. This can be used to compare with the version
 * used to compile the library. If the first or second numbers have
 * changed, then the library may not be compatible. The third number
 * can change and it should not be a problem as only internals would
 * have changed in that case.
 *
 * It should always be safe to use this version function, even when
 * the library changes drastically.
 */


/** \mainpage
 *
 * The idea of creating the as2js project was born from the time I worked
 * on my ActionScript compiler for my sswf project, a library to create
 * Flash animations.
 *
 * While working with ActionScript, I learned that it would be "easy" to
 * write a JavaScript compiler that would support classes and other
 * advance declarations that JavaScript does not support.
 *
 * Today, I am bringing this to life by working on the as2js project:
 *
 * AlexScript to JavaScript.
 *
 * So... how does it work? I have better documentation online on how to
 * use the compiler itself (the as2js command line tool.) Here I
 * mainly document the as2js library. This library can directly be used
 * by your project instead of the as2js command line tool. Hence, allowing
 * you do everything in memory!
 *
 * At time of writing, I do not have a complete compiler so I cannot
 * give a full example on how to use the library, but the as2js command
 * line tool will show you how it gets done. Also, the numerous tests
 * can be reviewed to see how things work and make use of some of that
 * code in your own project.
 *
 * The following should be close to what you'd want to do:
 *
 * \code
 *      class message_callback : public as2js::MessageCallback
 *      {
 *          virtual void output(as2js::message_level_t message_level, as2js::err_code_t error_code, as2js::Position const& pos, std::string const& message)
 *          {
 *              ... output message ...
 *          }
 *      } message_handler;
 *      as2js::Message::set_message_callback(&message_handler);
 *      as2js::FileInput in;
 *      if(in.open("script.js"))
 *      {
 *          as2js::Options::pointer_t opt(new as2js::Options);
 *          opt->set_option(as2js::Options::OPTION_STRICT, 1);
 *          as2js::Parser::pointer_t p(new as2js::Parser(in, opt));
 *          as2js::Node::pointer_t root(p->parse());
 *          if(as2js::Message::error_count() == 0)
 *          {
 *              as2js::Compiler::pointer_t c(new as2js::Compiler);
 *              c->set_options(opt);
 *              if(c->compile(root) == 0)
 *              {
 *                  ... assemble (not implemented yet) ...
 *              }
 *          }
 *      }
 * \endcode
 *
 * The result is JavaScript code that any browser is capable of running,
 * assuming your own code does not use features not available in a browser,
 * of course...
 */




/** \brief The AlexScript to JavaScript namespace.
 *
 * All the definitions from the as2js compiler are found inside this
 * namespace. There are other sub-namespaces used here and there, but
 * this is the major one.
 *
 * Of course, we use a few \#define and those are not protected with
 * a namespace. All of the \#define should start with AS2JS_ though,
 * to avoid potential conflicts.
 */
namespace as2js
{

/** \brief Return the library version.
 *
 * This function can be used to get the current version of the as2js
 * library. It returns a string of the form:
 *
 * \code
 * <major>.<minor>.<release>
 * \endcode
 *
 * where each entry is a number (only numerics are used.)
 *
 * Note that this is different from using the AS2JS_VERSION macro
 * (defined in as2js.h) in that the macro defines the version you
 * are compiling against and not automatically the version that
 * your code will run against.
 *
 * \todo
 * Add another function that checks whether your code is compatible
 * with this library.
 *
 * \return The version of the loaded library.
 */
char const *as2js_library_version()
{
	return AS2JS_VERSION;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
