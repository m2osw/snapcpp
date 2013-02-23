#
# File:
#      CMakeLists.txt
#
# Description:
#      Create the documentation of the entire Snap! Websites environment.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      Permission is hereby granted, free of charge, to any person obtaining a
#      copy of this software and associated documentation files (the
#      "Software"), to deal in the Software without restriction, including
#      without limitation the rights to use, copy, modify, merge, publish,
#      distribute, sublicense, and/or sell copies of the Software, and to
#      permit persons to whom the Software is furnished to do so, subject to
#      the following conditions:
#
#      The above copyright notice and this permission notice shall be included
#      in all copies or substantial portions of the Software.
#
#      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
#      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
#      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
#      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
#      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#


##
## Documentation
##
AddDoxygenTarget( snapwebsites ${SNAPWEBSITES_VERSION_MAJOR} ${SNAPWEBSITES_VERSION_MINOR} ${SNAPWEBSITES_VERSION_PATCH} )
#project(snapwebsites_documentation)
#find_package(Doxygen)
#if(DOXYGEN_FOUND)
#	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/snapwebsites.doxy.in ${CMAKE_CURRENT_BINARY_DIR}/snapwebsites.doxy @ONLY)
#	set(DOCUMENTATION_OUTPUT snapwebsites-doc-${SNAPWEBSITES_VERSION_MAJOR}.${SNAPWEBSITES_VERSION_MINOR})
#	add_custom_target(${PROJECT_NAME}
#		rm -rf ${DOCUMENTATION_OUTPUT}
#		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/snapwebsites.doxy
#		COMMAND echo Compacting as ${DOCUMENTATION_OUTPUT}.tar.gz
#		COMMAND tar czf ${DOCUMENTATION_OUTPUT}.tar.gz ${DOCUMENTATION_OUTPUT}
#		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#		COMMENT "Generating API documentation with Doxygen" VERBATIM
#	)
#endif(DOXYGEN_FOUND)
