################################################################################
# File:
#      plugins/CMakeLists.txt
#
# Description:
#      Set of extensions used by the Snap! Websites server.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2011-2016 Made to Order Software Corp.
#
#      http://snapwebsites.org/
#      contact@m2osw.com
#
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#     
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#     
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software
#      Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#
################################################################################

project( snapwebsites_plugins )

################################################################################
# We need a common place to install the plugins
#
set( PLUGIN_INSTALL_DIR lib/snapwebsites/plugins )

################################################################################
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

################################################################################
# Automatically run the moc compiler on qt files
set( CMAKE_AUTOMOC TRUE )

################################################################################
# Override the DTD_SOURCE_PATH that is provided by the libsnapwebsites library
set( DTD_SOURCE_PATH "${snapwebsites_project_SOURCE_DIR}/plugins" CACHE PATH "Location of the default DTD files" )

## Include support modules
##
find_package( AdvGetOpt          REQUIRED )
find_package( Boost              REQUIRED )
find_package( CSSPP              REQUIRED )
find_package( ControlledVars     REQUIRED )
find_package( LibTLD             REQUIRED )
find_package( LibXml2            REQUIRED )
find_package( log4cplus          REQUIRED )
find_package( OpenSSL            REQUIRED )
find_package( ProcPS             REQUIRED )
find_package( Qt5Core            REQUIRED )
find_package( Qt5Network         REQUIRED )
find_package( Qt5Script          REQUIRED )
find_package( Qt5Sql             REQUIRED )
find_package( Qt5Widgets         REQUIRED )
find_package( Qt5Xml             REQUIRED )
find_package( Qt5XmlPatterns     REQUIRED )
find_package( QtCassandra        REQUIRED )
find_package( QtSerialization    REQUIRED )
find_package( SnapCMakeModules   REQUIRED )
find_package( SnapDoxygen        REQUIRED )
find_package( SnapWebsites       REQUIRED )
find_package( ZLIB               REQUIRED )
find_package( ZipIos             REQUIRED 2.1      )

include_directories( SYSTEM
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Script_INCLUDE_DIRS}
    ${Qt5Sql_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${Qt5XmlPatterns_INCLUDE_DIRS}
)
include_directories(
    ${SNAPWEBSITES_INCLUDE_DIRS}
)
set( QT_LIBRARIES
    Qt5::Core
    Qt5::Network
    Qt5::Script
    Qt5::Sql
    Qt5::Xml
    Qt5::XmlPatterns
)

SnapGetVersion( COREPLUGINS )

find_library( UUID uuid )
if( ${UUID} STREQUAL "UUID-NOTFOUND" )
    message( FATAL_ERROR "Cannot find uuid library!" )
endif()

find_program( ZIP zip               REQUIRED )
find_program( CLOSURELINTER gjslint REQUIRED DOC "verify that Google JavaScript linter is installed: sudo apt-get install closure-linter" )

if( "${XMLLINT_EXECUTABLE}" STREQUAL "XMLLINT_EXECUTABLE-NOTFOUND" )
    message( FATAL_ERROR "xmllint was not found, you need to install libxml2-utils" )
endif()

if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
    message("Debug is in effect for snapserver-core-plugins!")
    add_definitions( -DDEBUG -D_DEBUG -DCONTROLLED_VARS_DEBUG )
else()
    message("Debug is turned OFF for snapserver-core-plugins")
    add_definitions( -DNDEBUG )
endif()
# A few extra warnings specifically for snapwebsites libraries and tools
# You can also play with -Weffc++ although we are definitively not compliant
# (especially we do not define all the auto-initialized variables!)
# -Wconversion -- would be nice, a few things are still not cooperating with that one
# -DUNISTR_FROM_STRING_EXPLICIT=explicit is for the Unicode library, to make sure it does not copy strings arounds automatically
# -DMAGICKCORE_QUANTUM_DEPTH=16 is the default depth, newer versions of ImageMagick require that definition from you (no internal default)
# -DMAGICKCORE_HDRI_ENABLE=0 is also the default for ImageMagick; I'm not too sure what HDRI stands for though...
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wnoexcept -DUNISTR_FROM_STRING_EXPLICIT=explicit -DMAGICKCORE_QUANTUM_DEPTH=16 -DMAGICKCORE_HDRI_ENABLE=0" )
set( CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fwrapv" )

include_directories(
    ${ADVGETOPT_INCLUDE_DIRS}
    ${QTCASSANDRA_INCLUDE_DIRS}
    ${QTSERIALIZATION_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}
    ${CONTROLLEDVARS_INCLUDE_DIRS}
    ${LIBTLD_INCLUDE_DIRS}
)

##
## Include xmllint utilties
##
include( SnapCssLint )
include( SnapJsLint  )
include( SnapXmlLint )

################################################################################
# Keep the plugins in alphabetical order for ease of management
#
add_subdirectory(antihammering)
add_subdirectory(antivirus)
add_subdirectory(attachment)
add_subdirectory(avatar)
add_subdirectory(bookkeeping)
add_subdirectory(char_chart)
add_subdirectory(content)
add_subdirectory(cookie_consent_silktide)
add_subdirectory(date_widgets)
add_subdirectory(ecommerce)
add_subdirectory(editor)
add_subdirectory(epayment)
add_subdirectory(epayment_creditcard)
add_subdirectory(epayment_paypal)
add_subdirectory(epayment_stripe)
add_subdirectory(favicon)
add_subdirectory(feed)
add_subdirectory(filter)
add_subdirectory(flash)
add_subdirectory(form)
add_subdirectory(hashtag)
add_subdirectory(header)
add_subdirectory(images)
add_subdirectory(info)
add_subdirectory(javascript)
add_subdirectory(layout)
add_subdirectory(links)
add_subdirectory(list)
add_subdirectory(listener)
add_subdirectory(locale)
add_subdirectory(locale_settings)
add_subdirectory(locale_widgets)
add_subdirectory(mailinglist)
add_subdirectory(menu)
add_subdirectory(messages)
add_subdirectory(mimetype)
add_subdirectory(oauth2)
add_subdirectory(output)
add_subdirectory(password)
add_subdirectory(path)
add_subdirectory(permissions)
add_subdirectory(qrcode)
add_subdirectory(robotstxt)
add_subdirectory(search)
add_subdirectory(sessions)
add_subdirectory(sendmail)
add_subdirectory(server_access)
add_subdirectory(shorturl)
add_subdirectory(sitemapxml)
add_subdirectory(snap_software_description)
add_subdirectory(sortable)
add_subdirectory(taxonomy)
add_subdirectory(test_plugin_suite)
add_subdirectory(test_plugin)
add_subdirectory(timetracker)
add_subdirectory(tracker)
add_subdirectory(users)
add_subdirectory(users_ui)
add_subdirectory(versions)
add_subdirectory(webform)

##
## Analyse xml and javascript
##
snap_build_css_targets()
snap_build_js_targets()
snap_build_xml_targets()


# vim: ts=4 sw=4 nocindent
