# File:         FindThrift.cmake
# Object:       Look for the thrift module.
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
# If you have included Snap's Third Party package, then this module will
# point to that instead of trying to locate it on the system.
#
# THRIFT_FOUND        - System has Thrift
# THRIFT_INCLUDE_DIRS - The Thrift include directories
# THRIFT_LIBRARIES    - The libraries needed to use Thrift (none)
# THRIFT_DEFINITIONS  - Compiler switches required for using Thrift (none)
#
find_path( THRIFT_INCLUDE_DIR thrift/thrift.h
		   PATHS $ENV{THRIFT_INCLUDE_DIR}
		   PATH_SUFFIXES thrift
		 )
find_library( THRIFT_LIBRARY thrift
			PATHS $ENV{THRIFT_LIBRARY}
		)
mark_as_advanced( THRIFT_INCLUDE_DIR THRIFT_LIBRARY )

set( THRIFT_INCLUDE_DIRS ${THRIFT_INCLUDE_DIR} ${THRIFT_INCLUDE_DIR}/thrift )
set( THRIFT_LIBRARIES    ${THRIFT_LIBRARY}     )

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set THRIFT_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(
	Thrift
	DEFAULT_MSG
	THRIFT_INCLUDE_DIRS
	THRIFT_LIBRARIES
)

