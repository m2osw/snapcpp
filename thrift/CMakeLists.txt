cmake_minimum_required(VERSION 2.8)

project( thrift )

set( THRIFT_VERS       0.9.0                                                     )
set( DISTFILE_PATH     ${CMAKE_SOURCE_DIR}/3rdParty/thrift-${THRIFT_VERS}.tar.gz )
set( ROOT_DIR          ${PROJECT_BINARY_DIR}                                     )
set( BUILD_DIR         ${ROOT_DIR}/thrift-${THRIFT_VERS}                         )
set( DIST_DIR          ${CMAKE_BINARY_DIR}/dist                                  )
set( CONFIGURE_TARGETS ${BUILD_DIR}/config.log                                   )
set( CONFIG_ARGS
    --without-csharp
    --without-java
    --without-erlang
    --without-python
    --without-perl
    --without-php
    --without-php_extension
    --without-ruby
    --without-haskell
    --without-go
)

if( EXISTS ${DIST_DIR}/include/thrift/Thrift.h )
    message( STATUS "Thrift v${THRIFT_VERS} already installed in dist folder." )
else()
    message( STATUS "Unpacking thrift v${THRIFT_VERS} source distribution file into local build area..." )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${DISTFILE_PATH}
        WORKING_DIRECTORY ${ROOT_DIR}
    )

    message( STATUS "Configuring thrift v${THRIFT_VERS}..." )
    execute_process(
        COMMAND chmod a+x ${BUILD_DIR}/configure
        WORKING_DIRECTORY ${BUILD_DIR}
    )
    execute_process(
        COMMAND ${BUILD_DIR}/configure --prefix=${DIST_DIR} ${CONFIG_ARGS}
        OUTPUT_FILE ${BUILD_DIR}/configure.log
        ERROR_FILE ${BUILD_DIR}/configure.err
        WORKING_DIRECTORY ${BUILD_DIR}
    )
    message( STATUS "Building thrift v${THRIFT_VERS}..." )
    execute_process(
        COMMAND ${CMAKE_BUILD_TOOL}
        OUTPUT_FILE ${BUILD_DIR}/make.log
        ERROR_FILE ${BUILD_DIR}/make.err
        WORKING_DIRECTORY ${BUILD_DIR}
    )
    message( STATUS "Installing thrift v${THRIFT_VERS}..." )
    execute_process(
        COMMAND ${CMAKE_BUILD_TOOL} install
        OUTPUT_FILE ${BUILD_DIR}/install.log
        ERROR_FILE ${BUILD_DIR}/install.err
        WORKING_DIRECTORY ${BUILD_DIR}
    )
endif()

# vim: ts=4 sw=4 et nocindent
