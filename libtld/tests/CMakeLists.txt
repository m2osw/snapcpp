#
# File:         CMakeLists.txt
# Object:       Definitions to create the build environment with cmake
#
# Copyright:    Copyright (c) 2011-2013 Made to Order Software Corp.
#               All Rights Reserved.
#
# http://snapwebsites.org/
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

##
## TLD tests
##
# Test the library directly
project(tld_internal_test)

# The reference to the src folder is required by the internal test
include_directories(
    "${PROJECT_BINARY_DIR}/../include"
    "${PROJECT_BINARY_DIR}/../src"
    "${PROJECT_SOURCE_DIR}/../include"
    "${PROJECT_SOURCE_DIR}/../src"
)

add_executable(${PROJECT_NAME}
    tld_internal_test.c
)
# Test the library as an end user
project(tld_test)
add_executable(${PROJECT_NAME}
    tld_test.c
)
target_link_libraries(${PROJECT_NAME}
    tld
)
# Test the tld_valid_uri() function
project(tld_test_full_uri)
add_executable(${PROJECT_NAME}
    tld_test_full_uri.c
)
target_link_libraries(${PROJECT_NAME}
    tld
)
# Test the C++ tld_object
project(tld_test_object)
add_executable(${PROJECT_NAME}
    tld_test_object.cpp
)
target_link_libraries(${PROJECT_NAME}
    tld
)
# Test TLD names as defined by Mozilla
project(tld_test_tld_names)
add_executable(${PROJECT_NAME}
    tld_test_tld_names.cpp
)
target_link_libraries(${PROJECT_NAME}
    tld
    ${QT_LIBRARIES}
)

##
## Run all the tests
##
project(run_libtld_tests)
add_custom_target(run_libtld_tests
	COMMAND echo "*** tld_internal_test"
	COMMAND tld_internal_test
	COMMAND echo "*** tld_test"
	COMMAND tld_test
	COMMAND echo "*** tld_test_full_uri"
	COMMAND tld_test_full_uri
	COMMAND echo "*** tld_test_object"
	COMMAND tld_test_object
	COMMAND echo "*** tld_test_tld_names"
	COMMAND tld_test_tld_names
	COMMAND echo "*** tld_test_xml.sh"
	COMMAND ./tld_test_xml.sh
	COMMAND echo "*** TLD tests all ran"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS
		tld_internal_test
		tld_test
		tld_test_full_uri
		tld_test_object
		tld_test_tld_names
)

# vim: ts=4 sw=4 et
