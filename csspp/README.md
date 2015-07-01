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
handle the command line argument.

The library requires at least C++11.

To generate the documentation, you will need to have Doxygen. That should
not be required (if you have problems with that, let me know.) If you
get Doxygen, having the dot tool is a good idea to get all the graphs
generated.

