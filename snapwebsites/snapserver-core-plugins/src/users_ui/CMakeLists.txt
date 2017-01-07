#
# File:
#      plugins/users_ui/CMakeLists.txt
#
# Description:
#      Plugin used to handle the various forms to handle users on
#      Snap! C++. This includes the login, registration, forgotten
#      password, etc. as well as the user profile.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      Copyright (c) 2012-2016  Made to Order Software Corp.
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

project(plugin_users_ui)

find_package( Qt5Core REQUIRED )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

qt5_add_resources(SNAP_MANAGER_RESOURCES_RCC
    users_ui.qrc
)


snap_validate_xml( content.xml                           content/content.dtd )

# TODO: snap-form, which is deprecated
snap_validate_xml( users_ui/forgot-password-form.xml     form/snap-form.dtd )
snap_validate_xml( users_ui/login-box-form.xml           form/snap-form.dtd )
snap_validate_xml( users_ui/login-form.xml               form/snap-form.dtd )
snap_validate_xml( users_ui/new-password-form.xml        form/snap-form.dtd )
snap_validate_xml( users_ui/password-form.xml            form/snap-form.dtd )
snap_validate_xml( users_ui/register-box-form.xml        form/snap-form.dtd )
snap_validate_xml( users_ui/register-form.xml            form/snap-form.dtd )
snap_validate_xml( users_ui/replace-password-form.xml    form/snap-form.dtd )
snap_validate_xml( users_ui/resend-email-form.xml        form/snap-form.dtd )
snap_validate_xml( users_ui/verify-credentials-form.xml  form/snap-form.dtd )
snap_validate_xml( users_ui/verify-form.xml              form/snap-form.dtd )

# editor-form
snap_validate_xml( layout/users-settings-page.xml        editor/editor-form.dtd )
snap_validate_xml( layout/users-settings-parser.xsl      simplified-xslt20.dtd )

add_library(users_ui SHARED
    users_ui.cpp
    ${SNAP_MANAGER_MOC_CXX}
    ${SNAP_MANAGER_RESOURCES_RCC}
    content.xml
    users_ui/forgot-password-form.xml
    users_ui/login-box-form.xml
    users_ui/login-form.xml
    users_ui/new-password-form.xml
    users_ui/password-form.xml
    users_ui/register-box-form.xml
    users_ui/register-form.xml
    users_ui/replace-password-form.xml
    users_ui/resend-email-form.xml
    users_ui/verify-credentials-form.xml
    users_ui/verify-form.xml
    layout/users-settings-page.xml
    layout/users-settings-parser.xsl
)

target_link_libraries( users_ui Qt5::Core )

install(
    TARGETS users_ui LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION include/snapwebsites/plugins/users_ui
    FILES_MATCHING PATTERN "*.h"
)

# vim: ts=4 sw=4 et
