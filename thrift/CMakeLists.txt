cmake_minimum_required(VERSION 2.8)

project( thrift )

set(  THRIFT_VERS       0.9.0                                                        )
set(  DISTFILE_PATH     ${CMAKE_SOURCE_DIR}/../3rdParty/thrift-${THRIFT_VERS}.tar.gz )
set(  ROOT_DIR          ${PROJECT_BINARY_DIR}                                        )
set(  BUILD_DIR         ${ROOT_DIR}/thrift-${THRIFT_VERS}                            )
set(  DIST_DIR          ${ROOT_DIR}/dist                                             )
set(  CONFIGURE_TARGETS ${BUILD_DIR}/config.log                                      )
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

if( NOT EXISTS ${BUILD_DIR} )
    message( STATUS "Unpacking thrift v${THRIFT_VERS} source distribution file into local build area..." )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${DISTFILE_PATH}
        WORKING_DIRECTORY ${ROOT_DIR}
    )
    execute_process(
        COMMAND chmod a+x ${BUILD_DIR}/configure
        WORKING_DIRECTORY ${BUILD_DIR}
    )
endif()

if( NOT EXISTS ${BUILD_DIR}/config.log )
    message( STATUS "Configuring thrift v${THRIFT_VERS}..." )
    execute_process(
        COMMAND ${BUILD_DIR}/configure --prefix=${DIST_DIR} ${CONFIG_ARGS}
        OUTPUT_FILE ${BUILD_DIR}/configure.log
        ERROR_FILE ${BUILD_DIR}/configure.err
        WORKING_DIRECTORY ${BUILD_DIR}
    )
endif()

add_custom_target( thrift-make ALL
    COMMAND ${CMAKE_BUILD_TOOL} > ${BUILD_DIR}/make.log 2> ${BUILD_DIR}/make.err
    WORKING_DIRECTORY ${BUILD_DIR}
    COMMENT "Building thrift v${THRIFT_VERS}..."
)
add_custom_target( thrift-install ALL
    COMMAND ${CMAKE_BUILD_TOOL} install > ${BUILD_DIR}/install.log 2> ${BUILD_DIR}/install.err
    WORKING_DIRECTORY ${BUILD_DIR}
    DEPENDS thrift-make
    COMMENT "Installing thrift v${THRIFT_VERS}..."
)

install( DIRECTORY ${DIST_DIR}/include DESTINATION include )
install( DIRECTORY ${DIST_DIR}/bin     DESTINATION bin     )
install( DIRECTORY ${DIST_DIR}/lib     DESTINATION lib     )

# vim: ts=4 sw=4 et nocindent
