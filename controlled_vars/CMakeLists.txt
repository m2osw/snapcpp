#
# File:		CMakeLists.txt
# Object:	Definitions to create the build environment
#
# Copyright:	Copyright (c) 2011-2013 Made to Order Software Corp.
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
## Controlled Variables Version
##
set(CONTROLLED_VARS_MAJOR 1)
set(CONTROLLED_VARS_MINOR 4)
set(CONTROLLED_VARS_PATCH 2)

##
## Controlled Variables Generator
##
project(controlled_vars)

# Basic static analysis done by the compiler, you may remove that line if
# you have problems compiling (specifically the -Werror, -pedantic, and
# -std=c++11 flags)
#set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -pedantic -std=c++11 -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Wno-unused -Wno-variadic-macros -Wno-parentheses -fdiagnostics-show-option")

## Include support modules
##
if( NOT CMAKE_MODULE_PATH )
    find_path( CMAKE_DIRECTORY_PATH snapCMakeModules
        PATHS ${CMAKE_SOURCE_DIR}
        NO_DEFAULT_PATH
    )
    set( CMAKE_MODULE_PATH ${CMAKE_DIRECTORY_PATH}/snapCMakeModules/Modules )
    message( "Found snapCMakeModules in ${CMAKE_MODULE_PATH}" )
endif()
find_package( SnapCMakeModules )
find_package( SnapDoxygen      )

include(CheckIncludeFiles)
check_include_files(unistd.h HAVE_UNISTD_H)
if(${CMAKE_SYSTEM_NAME}="Darwin")
  set(OS_DARWIN 1)
endif(${CMAKE_SYSTEM_NAME}="Darwin")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/controlled_vars_version.h.in ${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_version.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(${PROJECT_NAME}
	controlled_vars.cpp
)

##
## Controlled Variables Header
##
project(controlled_vars.h)

add_custom_command(
	OUTPUT controlled_vars.h
	COMMAND controlled_vars
	DEPENDS controlled_vars
)

add_custom_target( make_controlled_vars_headers ALL DEPENDS controlled_vars.h )

install(FILES
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_auto_enum_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_auto_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_exceptions.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_fauto_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_limited_auto_enum_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_limited_auto_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_limited_fauto_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_limited_need_enum_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_limited_need_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_limited_no_enum_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_limited_no_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_need_enum_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_need_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_no_enum_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_no_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_ptr_auto_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_ptr_need_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_ptr_no_init.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_static_assert.h
		${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_version.h
	DESTINATION include/controlled_vars
)

install(FILES
		INSTALL.txt
		LICENSE.txt
		README.txt
	DESTINATION share/doc/controlledvars
)

# Pack the Headers the way I want them for SourceForge.net
# (the default packer adds -Linux, even though this is not specific to an OS)
add_custom_target( controlled_vars_headers
	COMMAND rm -rf include
	COMMAND mkdir -p include/controlled_vars
	COMMAND cp -p controlled_vars*.h include/controlled_vars/.
	COMMAND tar czf controlled_vars_headers-${CONTROLLED_VARS_MAJOR}.${CONTROLLED_VARS_MINOR}.${CONTROLLED_VARS_PATCH}.tar.gz include
	COMMAND echo "Created ${CMAKE_CURRENT_BINARY_DIR}/controlled_vars_headers-${CONTROLLED_VARS_MAJOR}.${CONTROLLED_VARS_MINOR}.${CONTROLLED_VARS_PATCH}.tar.gz"
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS
		controlled_vars.h
		# Not defined as an output, but generated at the same time controlled_vars.h is
		#controlled_vars_version.h
		#controlled_vars_exceptions.h
		#controlled_vars_static_assert.h
		#controlled_vars_auto_init.h
		#controlled_vars_limited_auto_init.h
		#controlled_vars_need_init.h
		#controlled_vars_limited_need_init.h
		#controlled_vars_no_init.h
		#controlled_vars_limited_no_init.h
		# ... (and many others added since then)
)

##
## Controlled Variables Test
##
project(controlled_vars_test)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(controlled_vars_test
	tests/controlled_vars_test.cpp
	controlled_vars.h
)

##
## Auto-init Test
##
project(controlled_vars_auto_init)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(controlled_vars_auto_init
	tests/controlled_vars_auto_init.cpp
	controlled_vars.h
)

##
## Limited Auto-init Test
##
project(controlled_vars_limited_auto_init)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(controlled_vars_limited_auto_init
	tests/controlled_vars_limited_auto_init.cpp
	controlled_vars.h
)

##
## Need-init Test
##
project(controlled_vars_need_init)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(controlled_vars_need_init
	tests/controlled_vars_need_init.cpp
	controlled_vars.h
)

##
## Need-init Test
##
project(controlled_vars_limited_need_init)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(controlled_vars_limited_need_init
	tests/controlled_vars_limited_need_init.cpp
	controlled_vars.h
)

##
## No-init Test
##
project(controlled_vars_no_init)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(controlled_vars_no_init
	tests/controlled_vars_no_init.cpp
	controlled_vars.h
)

##
## Limited no-init Test
##
project(controlled_vars_limited_no_init)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(controlled_vars_limited_no_init
	tests/controlled_vars_limited_no_init.cpp
	controlled_vars.h
)

##
## Auto Pointer Test
##
project(controlled_vars_auto_ptr_init)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(controlled_vars_auto_ptr_init
	tests/controlled_vars_auto_ptr_init.cpp
	controlled_vars.h
)

##
##
##
project(run_tests)

add_custom_target(run_tests
	COMMAND echo "controlled_vars_test"
	COMMAND controlled_vars_test
	COMMAND echo "controlled_vars_auto_init"
	COMMAND controlled_vars_auto_init
	COMMAND echo "controlled_vars_limited_auto_init"
	COMMAND controlled_vars_limited_auto_init
	COMMAND echo "controlled_vars_need_init"
	COMMAND controlled_vars_need_init
	COMMAND echo "controlled_vars_limited_need_init"
	COMMAND controlled_vars_limited_need_init
	COMMAND echo "controlled_vars_no_init"
	COMMAND controlled_vars_no_init
	COMMAND echo "controlled_vars_limited_no_init"
	COMMAND controlled_vars_limited_no_init
	COMMAND echo "controlled_vars_auto_ptr_init"
	COMMAND controlled_vars_auto_ptr_init
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	DEPENDS
		controlled_vars_test
		controlled_vars_auto_init
		controlled_vars_limited_auto_init
		controlled_vars_need_init
		controlled_vars_limited_need_init
		controlled_vars_no_init
		controlled_vars_limited_no_init
)

##
## To pack the source
##
set(CPACK_PACKAGE_NAME "controlled_vars")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "controlled_vars -- C++ templates to constrain basic types")
set(CPACK_PACKAGE_VENDOR "Made to Order Software Corporation")
set(CPACK_PACKAGE_CONTACT "alexis@m2osw.com")
set(CPACK_RESOURCE_FILE_LICENSE "${controlled_vars_SOURCE_DIR}/LICENSE.txt")
set(CPACK_GENERATOR "TGZ" "DEB")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES "/CVS/;/work-files/;/.svn/;.swp$;.*~;cscope.*;/tmp/;BUILD;Build;build")
set(CPACK_PACKAGE_VERSION "${CONTROLLED_VARS_MAJOR}.${CONTROLLED_VARS_MINOR}.${CONTROLLED_VARS_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${CONTROLLED_VARS_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${CONTROLLED_VARS_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${CONTROLLED_VARS_PATCH}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "controlled_vars-${CONTROLLED_VARS_MAJOR}.${CONTROLLED_VARS_MINOR}.${CONTROLLED_VARS_PATCH}")
#set(CPACK_PACKAGE_EXECUTABLES "")
set(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "http://snapwebsites.org/project/controlled-vars")
# I'm not too sure how we can add this one... components?
# See http://www.vtk.org/Wiki/CMake:Component_Install_With_CPack
#set(CPACK_DEBIAN_PACKAGE_DEV_DEPENDENCIES "cmake")
include(CPack)

