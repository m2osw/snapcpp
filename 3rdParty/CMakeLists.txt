project( 3rdParty )

find_package( SnapBuild REQUIRED )

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

ConfigureMakeProject(
		PROJECT_NAME  thrift
		VERSION       ${THRIFT_VERSION}
		DISTFILE_PATH ${TARFILE}
		CONFIG_ARGS   ${CONFIG_ARGS}
		)

# vim: ts=4 sw=4 noet
