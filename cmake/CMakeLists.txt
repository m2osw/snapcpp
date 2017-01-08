#
# File:
#      CMakeLists.txt
#
# Description:
#      Definitions to create the build environment with cmake
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2017 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

cmake_minimum_required(VERSION 3.5)

project( snapCMakeModules )

install( FILES Modules/CheckCXXAbstractClass.cmake DESTINATION share/cmake-3.5/Modules )

set( MODULES_LIST
	CassandraCppDriver
	Catch
	CppUnit
	ICU
	LibEvent
	log4cplus
	Magic
	ProcPS
	QREncode
	QxCppUnit
	QxRunner
	SnapBuild
	SnapCMakeModules
	SnapDoxygen
)

foreach( _module ${MODULES_LIST} )
	install( FILES Modules/${_module}Config.cmake DESTINATION share/cmake/${_module} )
endforeach()

file( GLOB SCRIPTS Modules/*.sh Modules/*.pl )
install( FILES ${SCRIPTS} DESTINATION bin )

