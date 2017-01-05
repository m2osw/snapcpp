#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
#
# Copyright:    Copyright (c) 2005-2017 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/project/as2js
# contact@m2osw.com
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

find_package( CppUnit    )
find_package( QxCppUnit  )
find_package( QxRunner   )
find_package( Qt5Core    REQUIRED )
find_package( Qt5Widgets REQUIRED )
find_package( ICU        )

# All the tests require cppunit all the packages we just checked
#
# The "tools" defined here are for the tests so ignore them too if the
# tests do not get generated
if(CPPUNIT_FOUND AND ICU_FOUND)

##
## Unicode character determination (helper executable)
##
project(unicode_characters)

add_definitions( ${QT_DEFINITIONS} )
add_executable(${PROJECT_NAME}
    unicode_characters.cpp
)
include_directories(
    SYSTEM
        ${QT_INCLUDES}
        ${ICU_INCLUDE_DIRS}
)
target_link_libraries( ${PROJECT_NAME}
    Qt5::Core
    Qt5::Widgets
    ${ICU_LIBRARIES}
    ${ICU_I18N_LIBRARIES}
)


##
## JSON to C++ string
##
project(json_to_string)

include_directories(
    ${as2js_compiler_BINARY_DIR}/include
    ${as2js_compiler_SOURCE_DIR}/include
    ${as2js_compiler_SOURCE_DIR}/lib
    ${as2js_compiler_SOURCE_DIR}/src
    ${ADVGETOPT_INCLUDE_DIRS}
)
add_executable(${PROJECT_NAME}
    json_to_string.cpp

    # Stick a copy of the tools license in the test utilities
    ../src/license.cpp
)
target_link_libraries( ${PROJECT_NAME}
    as2js
    ${ADVGETOPT_LIBRARIES}
)

function(json_to_cpp TARGET_NAME)
    add_custom_command(OUTPUT ${as2js_compiler_BINARY_DIR}/tests/test_as2js_${TARGET_NAME}.ci
        COMMAND json_to_string -o ${as2js_compiler_BINARY_DIR}/tests/test_as2js_${TARGET_NAME}.ci ${as2js_compiler_SOURCE_DIR}/tests/test_as2js_${TARGET_NAME}.json
        DEPENDS json_to_string ${as2js_compiler_SOURCE_DIR}/tests/test_as2js_${TARGET_NAME}.json
        COMMENT "Generating test_as2js_${TARGET_NAME}.ci")
endfunction(json_to_cpp)


json_to_cpp(parser_array)
json_to_cpp(parser_basics)
json_to_cpp(parser_class)
json_to_cpp(parser_enum)
json_to_cpp(parser_for)
json_to_cpp(parser_function)
json_to_cpp(parser_if)
json_to_cpp(parser_pragma)
json_to_cpp(parser_switch)
json_to_cpp(parser_synchronized)
json_to_cpp(parser_trycatch)
json_to_cpp(parser_type)
json_to_cpp(parser_variable)
json_to_cpp(parser_while)
json_to_cpp(parser_yield)
json_to_cpp(parser)


json_to_cpp(optimizer_additive)
json_to_cpp(optimizer_assignments)
json_to_cpp(optimizer_bitwise)
json_to_cpp(optimizer_compare)
json_to_cpp(optimizer_conditional)
json_to_cpp(optimizer_equality)
json_to_cpp(optimizer_logical)
json_to_cpp(optimizer_match)
json_to_cpp(optimizer_multiplicative)
json_to_cpp(optimizer_relational)
json_to_cpp(optimizer_statements)

json_to_cpp(compiler_class)
json_to_cpp(compiler_enum)
json_to_cpp(compiler_expression)



##
## as2js tests
##



# Test library objects
# Using cppunit does not require a function
#function( as2jsLibraryTest )
#    set( options        NO_OPTIONS )
#    set( oneValueArgs   PROJECT_NAME )
#    set( multiValueArgs SOURCES )
#    cmake_parse_arguments( ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
#
#    if( NOT ARG_PROJECT_NAME )
#        message( FATAL_ERROR "You must specify PROJECT_NAME to as2jsLibraryTest!" )
#    endif()
#    if( NOT ARG_SOURCES )
#        message( FATAL_ERROR "You must specify SOURCES to as2jsLibraryTest!" )
#    endif()
#
#    message( "Setup test \"" ${ARG_PROJECT_NAME} "\"" )
#    project( ${ARG_PROJECT_NAME} )
#    include_directories(
#        ${as2js_compiler_BINARY_DIR}/include
#        ${as2js_compiler_BINARY_DIR}/src
#        ${as2js_compiler_SOURCE_DIR}/include
#        ${as2js_compiler_SOURCE_DIR}/src
#    )
#    add_executable( ${ARG_PROJECT_NAME}
#        ${ARG_SOURCES}
#    )
#    target_link_libraries( ${ARG_PROJECT_NAME}
#        as2js
#    )
#endfunction()


# Library: class String
#as2jsLibraryTest(PROJECT_NAME test_as2js_string SOURCES test_as2js_string)

project(test_as2js)

# TODO: add proper Qt stuff
if(QT_FOUND AND LIBQXCPPUNIT_FOUND AND LIBQXRUNNER_FOUND)
    set(HAVE_QT5 1)
    add_definitions(-DHAVE_QT5)
else(QT_FOUND AND LIBQXCPPUNIT_FOUND AND LIBQXRUNNER_FOUND)
    message("No GUI tests will be created because you do not seem to have libqt4-dev, libqxcppunit-dev, libqxrunner-dev available")
endif(QT_FOUND AND LIBQXCPPUNIT_FOUND AND LIBQXRUNNER_FOUND)

include_directories(
    SYSTEM
        ${QT_INCLUDES}
        ${ICU_INCLUDE_DIRS}
        ${CPPUNIT_INCLUDE_DIR}
)
include_directories(
    ${as2js_compiler_BINARY_DIR}/include
    ${as2js_compiler_BINARY_DIR}/tests
    ${as2js_compiler_SOURCE_DIR}/include
    ${as2js_compiler_SOURCE_DIR}/lib
    ${as2js_compiler_SOURCE_DIR}/src
    ${ADVGETOPT_INCLUDE_DIRS}
)
add_executable(${PROJECT_NAME}
    test_as2js_main.cpp

    test_as2js_compiler.cpp
    test_as2js_db.cpp
    test_as2js_float64.cpp
    test_as2js_int64.cpp
    test_as2js_json.cpp
    test_as2js_lexer.cpp
    test_as2js_message.cpp
    test_as2js_node.cpp
    test_as2js_node_display.cpp
    test_as2js_optimizer.cpp
    test_as2js_options.cpp
    test_as2js_parser.cpp
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_compiler_class.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_compiler_enum.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_compiler_expression.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_additive.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_assignments.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_bitwise.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_compare.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_conditional.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_equality.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_logical.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_match.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_multiplicative.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_relational.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_optimizer_statements.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_array.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_basics.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_class.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_enum.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_for.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_function.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_if.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_pragma.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_switch.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_synchronized.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_trycatch.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_type.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_yield.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_variable.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_while.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser_yield.ci
    ${as2js_compiler_BINARY_DIR}/tests/test_as2js_parser.ci
    test_as2js_position.cpp
    test_as2js_rc.cpp
    test_as2js_stream.cpp
    test_as2js_string.cpp
    test_as2js_version.cpp

    # Stick a copy of the tools license in the tests
    ../src/license.cpp
)
if(HAVE_QT5)
    set(CPPUNIT_QT5 ${LIBQXCPPUNIT_LIBRARIES} )
endif(HAVE_QT5)
target_link_libraries( ${PROJECT_NAME}
    as2js
    ${ADVGETOPT_LIBRARIES}
    Qt5::Core
    Qt5::Widgets
    ${ICU_LIBRARIES}
    ${ICU_I18N_LIBRARIES}
    ${CPPUNIT_LIBRARIES}
    ${CPPUNIT_QT5}
)


##
## Run all the tests
##
project(run_as2js_tests)
add_custom_target(run_as2js_tests
    COMMAND echo "*** test_as2js"
    COMMAND test_as2js
    DEPENDS
        test_as2js
)



# Source:
# http://stackoverflow.com/questions/23977313/with-cmake-how-could-i-write-a-test-to-verify-that-a-class-is-abstract
#
# Here I test all the abstract classes that are abstract now and are
# expected to remain abstract forever
include(CheckCXXAbstractClass)


#
# class DecodingFilter
#
unset(BUILT_ABSTRACT_DECODING_FILTER CACHE)
set(
    CMAKE_REQUIRED_INCLUDES 
        ${ADVGETOPT_INCLUDE_DIRS}
        ${as2js_compiler_BINARY_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/src
        ${CPPUNIT_INCLUDE_DIR}
        ${QT_INCLUDES}
)
check_cxx_abstract_class(
"#include \"as2js/stream.h\"
int main() { new as2js::DecodingFilter; }"
    BUILT_ABSTRACT_DECODING_FILTER
)
if (BUILT_ABSTRACT_DECODING_FILTER)
    message(FATAL_ERROR "class as2js::DecodingFilter can be instantiated, but should be abstract.")
endif()


#
# class DecodingFilterUTF16
#
unset(BUILT_ABSTRACT_DECODING_FILTER_UTF16 CACHE)
set(
    CMAKE_REQUIRED_INCLUDES 
        ${ADVGETOPT_INCLUDE_DIRS}
        ${as2js_compiler_BINARY_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/src
        ${CPPUNIT_INCLUDE_DIR}
        ${QT_INCLUDES}
)
check_cxx_abstract_class(
"#include \"as2js/stream.h\"
int main() { new as2js::DecodingFilterUTF16; }"
    BUILT_ABSTRACT_DECODING_FILTER_UTF16
)
if(BUILT_ABSTRACT_DECODING_FILTER_UTF16)
    message(FATAL_ERROR "class as2js::DecodingFilterUTF16 can be instantiated, but should be abstract.")
endif()


#
# class Input
#
unset(BUILT_ABSTRACT_INPUT CACHE)
set(
    CMAKE_REQUIRED_INCLUDES 
        ${ADVGETOPT_INCLUDE_DIRS}
        ${as2js_compiler_BINARY_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/src
        ${CPPUNIT_INCLUDE_DIR}
        ${QT_INCLUDES}
)
check_cxx_abstract_class(
"#include \"as2js/stream.h\"
int main() { new as2js::Input; }"
    BUILT_ABSTRACT_INPUT
)
if(BUILT_ABSTRACT_INPUT)
    message(FATAL_ERROR "class as2js::Input can be instantiated, but should be abstract.")
endif()


#
# class InputRetriever
#
unset(BUILT_ABSTRACT_INPUT_RETRIEVER CACHE)
set(
    CMAKE_REQUIRED_INCLUDES 
        ${ADVGETOPT_INCLUDE_DIRS}
        ${as2js_compiler_BINARY_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/src
        ${CPPUNIT_INCLUDE_DIR}
        ${QT_INCLUDES}
)
check_cxx_abstract_class(
"#include \"as2js/stream.h\"
int main() { new as2js::InputRetriever; }"
    BUILT_ABSTRACT_INPUT_RETRIEVER
)
if(BUILT_ABSTRACT_INPUT_RETRIEVER)
    message(FATAL_ERROR "class as2js::InputRetriever can be instantiated, but should be abstract.")
endif()


#
# class Output
#
unset(BUILT_ABSTRACT_OUTPUT CACHE)
set(
    CMAKE_REQUIRED_INCLUDES 
        ${ADVGETOPT_INCLUDE_DIRS}
        ${as2js_compiler_BINARY_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/include
        ${as2js_compiler_SOURCE_DIR}/src
        ${CPPUNIT_INCLUDE_DIR}
        ${QT_INCLUDES}
)
check_cxx_abstract_class(
"#include \"as2js/stream.h\"
int main() { new as2js::Output; }"
    BUILT_ABSTRACT_OUTPUT
)
if(BUILT_ABSTRACT_OUTPUT)
    message(FATAL_ERROR "class as2js::Output can be instantiated, but should be abstract.")
endif()




else(CPPUNIT_FOUND AND ICU_FOUND)
    message("No test will be created because you do not seem to have libcppunit-dev and libicu-dev available")
endif(CPPUNIT_FOUND AND ICU_FOUND)

# vim: ts=4 sw=4 et
