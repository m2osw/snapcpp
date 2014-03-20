cmake_minimum_required(VERSION 2.8)

project( zipios )

set( ZIPIOSCPP_VERS    0.1.5.9+cvs.2007.04.28                                         )
set( DISTFILE_PATH     ${CMAKE_SOURCE_DIR}/3rdParty/zipios++-${ZIPIOSCPP_VERS}.tar.gz )
set( ROOT_DIR          ${PROJECT_BINARY_DIR}                                          )
set( BUILD_DIR         ${ROOT_DIR}/zipios++-${ZIPIOSCPP_VERS}                         )
set( DIST_DIR          ${CMAKE_BINARY_DIR}/dist                                       )
set( CONFIGURE_TARGETS ${BUILD_DIR}/config.log                                        )
unset( CONFIG_ARGS )

if( EXISTS ${DIST_DIR}/include/zipios++/zipfile.h )
    message( STATUS "ZipIOS++ v${ZIPIOSCPP_VERS} already installed in dist folder." )
else()
    message( STATUS "Unpacking zipios++ v${ZIPIOSCPP_VERS} source distribution file into local build area..." )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${DISTFILE_PATH}
        WORKING_DIRECTORY ${ROOT_DIR}
    )

    message( STATUS "Configuring zipios++ v${ZIPIOSCPP_VERS}..." )
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
    message( STATUS "Building zipios++ v${ZIPIOSCPP_VERS}..." )
    execute_process(
        COMMAND ${CMAKE_BUILD_TOOL}
        OUTPUT_FILE ${BUILD_DIR}/make.log
        ERROR_FILE ${BUILD_DIR}/make.err
        WORKING_DIRECTORY ${BUILD_DIR}
    )
    message( STATUS "Installing zipios++ v${ZIPIOSCPP_VERS}..." )
    execute_process(
        COMMAND ${CMAKE_BUILD_TOOL} install
        OUTPUT_FILE ${BUILD_DIR}/install.log
        ERROR_FILE ${BUILD_DIR}/install.err
        WORKING_DIRECTORY ${BUILD_DIR}
    )
endif()

# vim: ts=4 sw=4 et nocindent
