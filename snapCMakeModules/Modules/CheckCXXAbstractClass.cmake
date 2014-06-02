# - Check whether given C++ source fails compiling because of an abstract
#   type
#
# CHECK_CXX_ABSTRACT_CLASS(<code> <var>)
#  <code>       - source code to try to compile, must define 'main'
#  <var>        - variable to store whether the source code compiled
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
#

#=============================================================================
# Based on the Kitware, Inc. CheckCXXSourceCompiles.cmake
#=============================================================================



macro(CHECK_CXX_ABSTRACT_CLASS SOURCE VAR)
  if("${VAR}" MATCHES "^${VAR}$")
    set(MACRO_CHECK_FUNCTION_DEFINITIONS
      "-D${VAR} ${CMAKE_REQUIRED_FLAGS}")
    if(CMAKE_REQUIRED_INCLUDES)
      set(CHECK_CXX_SOURCE_COMPILES_ADD_INCLUDES
        "-DINCLUDE_DIRECTORIES:STRING=${CMAKE_REQUIRED_INCLUDES}")
    else()
      set(CHECK_CXX_SOURCE_COMPILES_ADD_INCLUDES)
    endif()
    file(WRITE "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.cxx"
      "${SOURCE}\n")

    message(STATUS "Performing Test ${VAR}")
    try_compile(${VAR}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/src.cxx
      COMPILE_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS}
      CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
      "${CHECK_CXX_SOURCE_COMPILES_ADD_INCLUDES}"
      OUTPUT_VARIABLE OUTPUT)

    # If the compile succeeds, then it is a failure (althgough the link should
    # pretty much always fail?!)
    if(NOT ${VAR})
      if("${OUTPUT}" MATCHES "cannot allocate an object of abstract type")
      elseif("${OUTPUT}" MATCHES "because the following virtual functions are pure within")
      elseif("${OUTPUT}" MATCHES "error: .* is protected")
      elseif("${OUTPUT}" MATCHES "error: .* is private")
      else()
        set(${VAR} 1)
      endif()
    endif()

    if(${VAR})
      set(${VAR} "" CACHE INTERNAL "Test ${VAR}")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Performing C++ SOURCE FILE Test ${VAR} succeeded compiling the instantiation of an abstract class or did not get a valid error to prove that it is abstract:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n")
      message(FATAL_ERROR "Performing Test ${VAR} - Failed, compilation succeeded or no error meant that the class is abstract")
    else()
      # So... the compilation failed, make sure it says one of:
      # "cannot allocate an object of abstract type"
      # "because the following virtual functions are pure within"
      message(STATUS "Performing Test ${VAR} - Success")
      set(${VAR} "" CACHE INTERNAL "Test ${VAR}")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Performing C++ SOURCE FILE Test ${VAR} failed as expected with the following output:\n"
        "${OUTPUT}\n"
        "Source file was:\n${SOURCE}\n")
    endif()
  endif()
endmacro()

# vim: ts=2 sw=2 et
