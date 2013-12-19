cmake_minimum_required(VERSION 2.8)

project( recipes )

set( PROJECT_LIST
	advgetopt
	controlled_vars
	googlerank
	libQtCassandra
	libQtSerialization
	libtld
	snapCMakeModules
	snapwebsites
	thrift
)

find_program( BZR bzr /usr/bin )

#unset( TARGETS )

set( TARBALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/tarballs )
file( MAKE_DIRECTORY ${TARBALL_DIR} )

foreach( project ${PROJECT_LIST} )
	#
	set( RECIPE_FILE ${CMAKE_CURRENT_BINARY_DIR}/${project}.recipe )
	file( WRITE ${RECIPE_FILE} "# bzr-builder format 0.3 deb-version {debupstream}\n" )
	file( APPEND ${RECIPE_FILE} "lp:~snapcpp/snapcpp/${project}\n" )
	if( NOT ${project} STREQUAL "thrift" )
		file( APPEND ${RECIPE_FILE} "nest-part src lp:snapcpp ${project} src\n" )
	endif()
	#
	set( BUILD_DIR  ${CMAKE_CURRENT_BINARY_DIR}/${project} )
	set( SOURCE_DIR ${BUILD_DIR}/${project}-{debversion}   )
	file( MAKE_DIRECTORY ${BUILD_DIR} )
	message( STATUS "Running dailydeb for ${project}" )
	execute_process(
			COMMAND
				rm -rf ${BUILD_DIR}
			COMMAND
				${BZR}
				dailydeb
				--allow-fallback-to-native
				--manifest ${CMAKE_CURRENT_BINARY_DIR}/${project}_manifest.txt
				${RECIPE_FILE}
				${BUILD_DIR}
				#OUTPUT_FILE ${BUILD_DIR}/out.log
				#ERROR_FILE ${BUILD_DIR}/err.log
			#DEPENDS ${RECIPE_FILE}
	)
	#
	#add_custom_target( ${project}_source_build DEPENDS ${SOURCE_DIR} )
	#list( APPEND TARGETS ${project}_source_build )
	#
	file( GLOB ${project}_tarball ${BUILD_DIR}/*.tar.gz )
	file( INSTALL ${${project}_tarball} DESTINATION ${TARBALL_DIR} )
endforeach()

#add_custom_target( build-recipes DEPENDS ${TARGETS} )

