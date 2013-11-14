#
# File:		CMakeLists.txt
# Object:	Definitions to create the build environment
#
# Copyright:	Copyright (c) 2012 Made to Order Software Corp.
#		All Rights Reserved.
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
cmake_minimum_required(VERSION 2.8)

##
## IP Lock Version
##
set(IPLOCK_MAJOR 2)
set(IPLOCK_MINOR 0)
set(IPLOCK_PATCH 0)

project(iplock)

##
## Include common Snap! cmake modules
##
if( NOT CMAKE_MODULE_PATH )
    find_path( CMAKE_DIRECTORY_PATH snapCMakeModules
        PATHS ${CMAKE_SOURCE_DIR}
        NO_DEFAULT_PATH
    )
    set( CMAKE_MODULE_PATH ${CMAKE_DIRECTORY_PATH}/snapCMakeModules )
    message( "Found snapCMakeModules in ${CMAKE_MODULE_PATH}" )
endif()
find_package( SnapCMakeModules )

##
## IP Lock Tool
##
add_executable(iplock
	iplock.cpp
)

##
## To pack the source
##
set(CPACK_PACKAGE_NAME "iplock")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "iplock -- Tool to block (or release) an IP address in your iptables firewall")
set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
set(CPACK_PACKAGE_CONTACT "alexis@m2osw.com")
set(CPACK_RESOURCE_FILE_LICENSE "${iplock_SOURCE_DIR}/LICENSE.txt")
set(CPACK_GENERATOR "TGZ" "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/work-files/;/.svn/;.swp$;.*~;cscope.*;/tmp/;BUILD;Build;build")
set(CPACK_PACKAGE_VERSION "${IPLOCK_MAJOR}.${IPLOCK_MINOR}.${IPLOCK_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${IPLOCK_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${IPLOCK_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${IPLOCK_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "iplock-${IPLOCK_MAJOR}.${IPLOCK_MINOR}.${IPLOCK_PATCH}")
#set(CPACK_PACKAGE_EXECUTABLES "")
set(CPACK_DEBIAN_PACKAGE_SECTION "admin")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://snapwebsites.org/project/iplock")
include(CPack)

