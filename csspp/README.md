# CSS Preprocessor

The CSS language is actually pretty flat and rather cubersum to maintain.
Many preprocessors have been created, but none in modern C++ supporting
CSS 3 and the preliminary CSS 4 languages. CSS Preprocessor offers that
functionality as a command line tool and a C++ library.

The main features of CSS Preprocessor are:

  * Nested rules
  * Nested fields
  * Variables
  * User defined functions
  * @mixin to create advanced / complex variables and functions
  * Conditional compilation
  * Strong validation of your CSS code (rules, field names, field data)
  * Minify or beautify CSS code

# Dependencies

The CMakeLists.txt file depends on the snapCMakeModules.

The csspp command line tool makes use of the advgetopt C++ library to
handle the command line argument. The advgetopt C++ library makes use
of the controlled\_vars headers.

The library requires at least C++11.

To generate the documentation, you will need to have Doxygen. That should
not be required (if you have problems with that, let me know.) If you
get Doxygen, having the dot tool is a good idea to get all the graphs
generated.

# Compile the library and csspp command line tool

The INSTALL in the root directory tells you how to generate the
distribution directory (or dev/INSTALL in the csspp project itself.)

We will be looking at making this simpler with time... for now, the
environment is a bit convoluted.

# Documentation

The [documentation](http://csspp.org/documentation/csspp-doc-1.0/ "CSS Preprocessor Documentation")
is available online. A copy can be downloaded from SourceForge.net.

# Contact

If you have any problems, questions, suggestions, feel free to post
them here:

[https://sourceforge.net/p/csspp/tickets/](https://sourceforge.net/p/csspp/tickets/)

