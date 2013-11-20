# File:         CMakeLists.txt
# Object:       Build the AdvGetopt library
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
## AdvGetopt library
##
project(advgetopt)
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
)
add_library( ${PROJECT_NAME} STATIC
    advgetopt.h
    advgetopt.cpp
)
install(
    TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    COMPONENT development
)
install(
    FILES advgetopt.h
    DESTINATION include/advgetopt
    COMPONENT development
)

# vim: ts=4 sw=4 et nocindent

