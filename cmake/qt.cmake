# Find Qt4 libraries on the local system
#
macro( SnapIncludeQt4 )
    find_package( Qt4 4.8.1 REQUIRED ${ARGN} )
    include( ${QT_USE_FILE} )
    add_definitions( ${QT_DEFINITIONS} )
endmacro()

# vim: ts=4 sw=4 noet
