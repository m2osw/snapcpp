// CSS Preprocessor
// Copyright (C) 2015  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// Documentation only file

// WARNING:
//   We show C-like comments in this block so we use //! as the introducer
//   in order to keep everything looking like a comment even when we have
//   a */ in the comment.
//
//   If you use gvim, you may want to change your comment setup to:
//
//      set comments=s1:/*,mb:*,ex:*/,b://,b://!
//
//   in order to include the //! introducer (assuming you don't already
//   have it in there!) You could also include ///
//

//! \page compiler_reference CSS Preprocessor Reference
//! \tableofcontents
//!
//! CSS Preprocessor is an extension to the CSS language that adds features
//! not otherwise available in CSS to make it easier to quickly write
//! advanced CSS documents.
//!
//! \section features Features
//!
//! The main features of CSS Preprocessor are:
//!
//! * A Validator which verifies that the syntax of all the fields that
//!   you are using are all valid;
//! * Variables to make it more dynamic, our variables support being set
//!   to absolutely anything;
//! * Nesting of rules to avoid having to write complete selectors for
//!   each rule;
//! * Functions to apply to field values;
//! * Control directives, including conditional compiling;
//! * Beautified or compressed output.
//!
//! \section syntax Syntax
//!
//! The syntax supported by the CSS Preprocessor language follows the
//! standard CSS 3 syntax with just a few exceptions. Files are expected
//! to be named with extension .scss, although the compiler does not
//! enforce the extension when loading a file specified on the command
//! line, it does enforce it for the
//! <a href="at-keyword-page.html">\@import</a> rule.
//!
//! The one main exception to the CSS 3 syntax is the setting of a variable
//! at the top level (i.e. a global variable). Setting a variable looks like
//! declaring a field:
//!
//! \code
//!     $color: #123;
//!     $width: 50px;
//!
//!     $block: {
//!         color: $color;
//!         width: $width
//!     };    // <- notice this mandatory colon in this case
//! \endcode
//!
//! In CSS 3, this is not allowed at the top level, which expects lists of
//! selectors followed by a block or @-rules.
//!
//! Another exception is the support of nested fields. These look like
//! qualified rules by default, but selectors can have a ':' only if followed
//! by an identifier, so a colon followed by a '{' is clearly not a qualified
//! rule. Note that to further ensure the validity of the rule, we also
//! enforce a ';' at the end of the construct. With all of that we can safely
//! change the behavior and support the nested fields as SASS does:
//!
//! \code
//!     font: {
//!         family: helvetica;
//!         style: italic;
//!         size: 120%;
//!     };   // <- notice the mandatory ';' in this case
//!
//!     // which becomes
//!     font-family: helvetica;
//!     font-style: italic;
//!     font-size: 120%;
//! \endcode
//!
//! Other exceptions are mainly in the lexer which support additional tokens
//! such as the variable syntax ($\<name>), the reference character (&), and
//! the placeholder extension (%\<identifier>).
//!
//! However, anything that is not supported generates an error and no output
//! is generated. This allows you to write scripts and makefiles that make
//! sure that your output is always valid CSS.
//!
//! \section known-bugs Known Bugs
//!
//! * Case Sensitivity
//!
//! At this time, the CSS Preprocessor does not handle identifiers correctly.
//! It will force them all to lowercase, meaning that the case is not valid
//! for documents such as XML that are not case insensitive like HTML.
//!
//! \section using-csspp Using CSS Preprocessor
//!
//! You may use the CSS Preprocessor command line. It is very similar to
//! using a compiler:
//!
//! \code
//!      csspp input.scss -o output.css
//! \endcode
//!
//! The command line tool supports many options. By default the output is
//! written to standard output. The tool exits with 1 on errors and 0 on
//! warnings or no messages.
//!
//! If you are writing a C++ application, you may directly include the
//! library. With cmake, you may use the FindCSSPP macros as in:
//!
//! \code
//!      # cmake loads CSSPP_INCLUDE_DIRS and CSSPP_LIBRARIES
//!      find_package(CSSPP REQUIRED)
//! \endcode
//!
//! Then look at the API documentation for details on how to use the
//! csspp objects. In general, you want to open a file, give it to
//! a lexer object. Create a parser and parse the input. With the
//! resulting node tree, create a compiler and generate a tree that
//! can be output using an assembler object.
//!
//! \code
//!     #include <csspp/compiler.h>
//!     #include <csspp/parser.h>
//!
//!     std::ifstream in;
//!     if(!in.open("my-file.scss"))
//!     {
//!         std::cerr << "error: cannot open file.\n";
//!         exit(1);
//!     }
//!     csspp::position pos("my-file.scss");
//!     csspp::lexer l(in, pos);
//!     csspp::parser p(l);
//!     csspp::node::pointer_t root(p.stylesheet());
//!     csspp::compiler c;
//!     c.set_root(root);
//!     //c.set_...(); -- setup various flags
//!     //c.add_paths("."); -- add various path to use with @import
//!     c.compile();
//!     csspp::assembler a(std::cout);
//!     a.output(c.get_root());
//! \endcode
//!
//! \section comments Comments (C and C++)
//!
//! The CSS Preprocessor supports standard C comments and C++ comments:
//!
//! \code
//! /* a standard C-like comment
//!  * which can span on multiple lines */
//!
//! // A C++-like comment
//! \endcode
//!
//! C++ comments that span on multiple lines are viewed as one comment.
//!
//! \code
//!      // The following 3 lines comment is viewed as just one line
//!      // which makes it possible to use C++ comments for large blocks
//!      // as if you where using C-like comments
//! \endcode
//!
//! All comments are removed from the output except those that include
//! the special keyword "@preserve". This is useful to include comments
//! such as copyrights.
//!
//! \warning
//! We do not allow CSS tricks including weird use of comments in .scss
//! files. Although the output could include such, we assume that the final
//! output is specialized for a specific browser so such tricks are never
//! necessary. Actually, only comments marked with \@preserve are kept and
//! a preserved comment appearing in the wrong place will generally create
//! an error.
//!
//! Variable expension is provided for comments with the \@preserve keyword.
//! The variables have to be written between curly brackets as in:
//!
//! \code
//!      /* My Project (c) 2015  My Company
//!       * @preserve
//!       * Generated by csspp version {$_csspp_version}
//!       */
//! \endcode
//!
//! To be SASS compatible, we will also remove a preceeding '#' character:
//!
//! \code
//!      /* Version: #{$my_project_version} */
//! \endcode
//!
//! \section at-commands @-commands
//!
//! The CSS Preprocess compiler adds a pletoria of @-commands to support
//! various useful capabilities to the compiler.
//!
//! \section selectors Selectors
//!
//! The same selectors as CSS 3 are supported by the CSS Preprocessor.
//! All the lists of selectors get compiled to make sure they are valid
//! CSS code.
//!
//! Also like SASS, we support the %<name> selector. This allows for
//! creating rules that do not automatically get inserted in the output.
//! This allows for the definition of various CSS libraries with rules
//! that get used only when required in the final output.
//!
//! \ref selectors_rules
//!
//! \section expressions Expressions
//!
//! The CSS Preprocessor adds support for C-like expressions. The
//! syntax is described in the CSS Preprocess Expressions page.
//! The expressions are accepted between an @-keyword and a block:
//!
//! \code
//!     ... AT-KEYWORD <expressions> { ... }
//! \endcode
//!
//! Or the value of fields in a declaration.
//!
//! \code
//!     ... IDENTIFIER ':' ... <expressions> ... ';'
//! \endcode
//!
//! \ref expression_page
//!

// Local Variables:
// mode: cpp
// indent-tabs-mode: nil
// c-basic-offset: 4
// tab-width: 4
// End:

// vim: ts=4 sw=4 et
