project( thrift-gencpp-cassandra )

set_property( GLOBAL APPEND PROPERTY ${PROJECT_NAME}_HEADER_FILES
	${PROJECT_SOURCE_DIR}/Cassandra.h
	${PROJECT_SOURCE_DIR}/cassandra_constants.h
	${PROJECT_SOURCE_DIR}/cassandra_types.h
	)

set_property( GLOBAL APPEND PROPERTY ${PROJECT_NAME}_SOURCE_FILES
	${PROJECT_SOURCE_DIR}/Cassandra.cpp
	${PROJECT_SOURCE_DIR}/cassandra_constants.cpp
	${PROJECT_SOURCE_DIR}/cassandra_types.cpp
	)

