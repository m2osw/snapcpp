#
# File:
#      conf/CMakeLists.txt
#
# Description:
#      Install configuration files and samples.
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

project(snapmanagercgi_conf)

snap_validate_xml( snapmanagercgi-parser.xsl simplified-xslt20.dtd )

snap_validate_xml( bundle-cassandra.xml bundles.xsd )
snap_validate_xml( bundle-logserver.xml bundles.xsd )
snap_validate_xml( bundle-rkhunter.xml bundles.xsd )
snap_validate_xml( bundle-tripwire.xml bundles.xsd )
snap_validate_xml( bundle-vpn_client.xml bundles.xsd )
snap_validate_xml( bundle-vpn_server.xml bundles.xsd )

snap_validate_xml( service-snapmanagerdaemon.xml snapinit.xsd )

#################################################################################
## Config files
##
install(
    FILES 
        # log4cplus properties
        snapmanagercgi.properties
        snapmanagerdaemon.properties

        # snap configuration files
        snapmanager.conf

        # misc files -- TODO add a sample apache settings for snapmanager.cgi?
        snapmanagercgi-parser.xsl
        snap-bundles-apache2.conf
        snap-bundles-footer.html

    DESTINATION
        share/snapwebsites/conf
)

#################################################################################
## Save XSD files in their own directory
##
install(
    FILES
        bundles.xsd

    DESTINATION
        share/snapwebsites/xsd
)

#################################################################################
## Bundles are also saved under a bundle directory for reference
##
install(
    FILES
        bundle-cassandra.xml
        bundle-logserver.xml
        bundle-rkhunter.xml
        bundle-tripwire.xml
        bundle-vpn_client.xml
        bundle-vpn_server.xml

    DESTINATION
        share/snapwebsites/bundles
)


#################################################################################
## Service definitions
##
install(
    FILES 
        service-snapmanagerdaemon.xml

    DESTINATION
        share/snapwebsites/conf/snapinit.d
)


# vim: ts=4 sw=4 et
