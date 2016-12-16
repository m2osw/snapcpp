# File:         CMakeLists.txt
# Object:       Build the AdvGetopt library
#
# Copyright:    Copyright (c) 2011-2016 Made to Order Software Corp.
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
## AdvGetopt library
##
project(advgetopt)

# Put the version in the header file
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/advgetopt.h.in ${CMAKE_CURRENT_BINARY_DIR}/advgetopt.h )

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_library( ${PROJECT_NAME} SHARED
    advgetopt.h
    advgetopt.cpp
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${LIBADVGETOPT_VERSION_MAJOR}.${LIBADVGETOPT_VERSION_MINOR}
    SOVERSION ${LIBADVGETOPT_VERSION_MAJOR}
)

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/advgetopt.h
    DESTINATION include/advgetopt
)

##
## TLD parser
##
project(hide-warnings)
add_executable(${PROJECT_NAME}
    hide-warnings.c
)
install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    COMPONENT runtime
)

# vim: ts=4 sw=4 et nocindent
