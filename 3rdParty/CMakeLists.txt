project( 3rdParty )

set_property( GLOBAL PROPERTY 3RDPARTY_INCLUDED TRUE )

function( ConfigureMakeProject PROJECT_NAME VERSION DISTFILE_PATH )
	set( SRC_DIR           ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VERSION}                            )
	set( BUILD_DIR         ${CMAKE_BINARY_DIR}/${PROJECT_NAME}-${VERSION}/${PROJECT_NAME}-${VERSION} )
	set( DIST_DIR          ${CMAKE_BINARY_DIR}/dist                                                  )
	set( CONFIGURE_TARGETS ${BUILD_DIR}/config.log                                                   )

	if( NOT EXISTS ${SRC_DIR} )
		message( STATUS "Unpacking ${PROJECT_NAME} source distribution file into local build area." )
		file( MAKE_DIRECTORY ${SRC_DIR} )
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E tar xzf ${DISTFILE_PATH}
			WORKING_DIRECTORY ${SRC_DIR}
		)
		execute_process(
			COMMAND chmod a+x ${BUILD_DIR}/configure
			WORKING_DIRECTORY ${BUILD_DIR}
		)
	endif()

	if( NOT EXISTS ${BUILD_DIR} )
		file( MAKE_DIRECTORY ${BUILD_DIR} )
	endif()

	add_custom_command(
		OUTPUT ${CONFIGURE_TARGETS}
		COMMAND ${BUILD_DIR}/configure --prefix=${DIST_DIR} ${ARGN}
			1> ${BUILD_DIR}/${PROJECT_NAME}_configure.log
			2> ${BUILD_DIR}/${PROJECT_NAME}_configure.err
		WORKING_DIRECTORY ${BUILD_DIR}
		COMMENT "Running ${PROJECT_NAME} configuration"
		)

	set( BUILD_OUTPUT ${BUILD_DIR}/stamp-h1 )
	add_custom_command(
		OUTPUT ${BUILD_OUTPUT}
		COMMAND make
			1> ${BUILD_DIR}/${PROJECT_NAME}_make.log
			2> ${BUILD_DIR}/${PROJECT_NAME}_make.err
		DEPENDS ${CONFIGURE_TARGETS}
		WORKING_DIRECTORY ${BUILD_DIR}
		COMMENT "Building ${PROJECT_NAME}"
		)

	set( INSTALL_OUTPUT ${DIST_DIR}/include/${PROJECT_NAME}/config.h )
	add_custom_command(
		OUTPUT ${INSTALL_OUTPUT}
		COMMAND make install
			1>> ${BUILD_DIR}/${PROJECT_NAME}_make.log
			2>> ${BUILD_DIR}/${PROJECT_NAME}_make.err
		DEPENDS ${BUILD_OUTPUT}
		WORKING_DIRECTORY ${BUILD_DIR}
		COMMENT "Installing ${PROJECT_NAME}"
		)

	add_custom_target(
		${PROJECT_NAME}-build ALL
		DEPENDS ${INSTALL_OUTPUT}
		)
endfunction()

set( THRIFT_VERSION 0.9.0 )
set( TARFILE ${3rdParty_SOURCE_DIR}/thrift-${THRIFT_VERSION}.tar.gz )
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
ConfigureMakeProject( thrift ${THRIFT_VERSION} ${TARFILE} ${CONFIG_ARGS} )

# vim: ts=4 sw=4 noet
