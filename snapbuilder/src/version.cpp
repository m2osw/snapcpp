// Copyright (c) 2021-2023  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/snapbuilder
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief The version of the snap builder.
 *
 * This file records the snap builder tool version.
 */

// self
//
#include    "version.h"


// last include
//
#include    <snapdev/poison.h>



namespace builder
{




/** \brief Get the major version.
 *
 * This function returns the major version.
 *
 * \return The major version.
 */
int get_major_version()
{
    return SNAPBUILDER_VERSION_MAJOR;
}


/** \brief Get the minor version.
 *
 * This function returns the minor version.
 *
 * \return The release version.
 */
int get_release_version()
{
    return SNAPBUILDER_VERSION_MINOR;
}


/** \brief Get the patch version.
 *
 * This function returns the patch version.
 *
 * \return The patch version.
 */
int get_patch_version()
{
    return SNAPBUILDER_VERSION_PATCH;
}


/** \brief Get the full version as a string.
 *
 * This function returns the major, minor, and patch versions in the
 * form of a string.
 *
 * \return The tool version.
 */
char const * get_version_string()
{
    return SNAPBUILDER_VERSION_STRING;
}


} // builder namespace
// vim: ts=4 sw=4 et
