project( zipios )

add_library( ${PROJECT_NAME} SHARED
	backbuffer.h
	basicentry.cpp
	collcoll.cpp
	dircoll.cpp
	directory.cpp
	directory.h
	fcoll.cpp
	fcollexceptions.cpp
	fileentry.cpp
	filepath.cpp
	filterinputstreambuf.cpp
	inflateinputstreambuf.cpp
	outputstringstream.h
	gzipoutputstream.cpp
	gzipoutputstreambuf.cpp
	zipfile.cpp
	ziphead.cpp
	zipheadio.cpp
	zipinputstream.cpp
	zipinputstreambuf.cpp
	zipios_common.h
	filteroutputstreambuf.cpp
	deflateoutputstreambuf.cpp
	zipoutputstreambuf.cpp
	zipoutputstream.cpp
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION ${VERSION}
    SOVERSION ${SOVERSION}
)

install( TARGETS ${PROJECT_NAME} DESTINATION lib )

project( appendzip )

add_executable( ${PROJECT_NAME} appendzip.cpp )

install( TARGETS ${PROJECT_NAME} DESTINATION bin )

