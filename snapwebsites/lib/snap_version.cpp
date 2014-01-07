// Snap Websites Server -- verify and manage version and names in filenames
// Copyright (C) 2014  Made to Order Software Corp.
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

#include "versioned_filename.h"
#include "qstring_stream.h"
#include <iostream>

namespace snap
{

/** \brief Initialize a versioned filename object.
 *
 * The versioned filename class initializes the versioned filename object
 * with an extension which is mandatory and unique.
 *
 * \note
 * The period in the extension is optional. However, the extension cannot
 * be the empty string.
 *
 * \param[in] extension  The expected extension (i.e. ".js" for JavaScript files).
 */
versioned_filename::versioned_filename(QString const& extension)
    //: f_valid(false) -- auto-init
    //, f_error("") -- auto-init
    : f_extension(extension)
    //, f_name("") -- auto-init
    //, f_version_string("") -- auto-init
    //, f_version() -- auto-init
{
    if(f_extension.isEmpty())
    {
        throw versioned_filename_exception_invalid_extension("the extension of a versioned filename cannot be the empty string");
    }

    // make sure the extension includes the period
    if(f_extension.at(0).unicode() != '.')
    {
        f_extension = "." + f_extension;
    }
}


/** \brief Set the name of a file through the parser.
 *
 * This function is used to setup a versioned filename from a full filename.
 * The input filename can include a path. It must end with the valid
 * extension (as defined when creating the versioned_filename object.)
 * Assuming the function returns true, the get_filename() function
 * returns the basename (i.e. the filename without the path nor the
 * extension, although you can get the extension if you ask for it.)
 *
 * The filename is then broken up in a name, a version, and browser, all of
 * which are checked for validity. If invalid, the function returns false.
 *
 * \code
 * .../some/path/name_version_browser.ext
 * \endcode
 *
 * Note that the browser part is optional. In general, if not indicated
 * it means the file is compatible with all browsers.
 *
 * \note
 * This function respects the contract: if the function returns false,
 * then the name, version, and browser information are not changed.
 *
 * However, on entry the value of f_error is set to the empty string and
 * the value of f_valid is set to false. So most of the functions will
 * continue to return the old value of the versioned filename, except
 * the compare() and relational operators.
 *
 * \return true if the filename was a valid versioned filename.
 */
bool versioned_filename::set_filename(QString const& filename)
{
    f_error.clear();
    f_valid = false;

    // the extension must be exactly "extension"
    if(!filename.endsWith(f_extension))
    {
        f_error = "this filename must end with \"" + f_extension + "\" in lowercase. \"" + filename + "\" is not valid.";
        return false;
    }

    int const max(filename.length() - f_extension.length());

    int start(filename.lastIndexOf('/'));
    if(start == -1)
    {
        start = 0;
    }
    else
    {
        ++start;
    }

    // now break the name in two parts: <name> and <version> [<browser>]
    int p1(filename.indexOf('_', start));
    if(p1 == -1 || p1 > max)
    {
        f_error = "a versioned filename is expected to include an underscore (_) as the name and version separator. \"" + filename + "\" is not valid.";
        return false;
    }
    // and check whether the <browser> part is specified
    int p2(filename.indexOf('_', p1 + 1));
    if(p2 > max || p2 == -1)
    {
        p2 = max;
    }
    else
    {
        if(p2 + 1 >= max)
        {
            f_error = "a browser name must be specified in a versioned filename if you include two underscores (_). \"" + filename + "\" is not valid.";
            return false;
        }
    }

    // name
    QString name(filename.mid(start, p1 - start));
    if(!validate_name(name))
    {
        return false;
    }

    // version
    ++p1;
    QString version_string(filename.mid(p1, p2 - p1));
    version_t version;
    if(!validate_version(version_string, version))
    {
        return false;
    }

    // browser
    QString browser;
    if(p2 < max)
    {
        // validate only if not empty (since it is optional empty is okay)
        browser = filename.mid(p2 + 1, max - p2 - 1);
        if(!validate_name(browser))
        {
            return false;
        }
    }

    // save the result
    f_name = name;
    f_version_string = version_string;
    f_version.swap(version);
    f_browser = browser;
    f_valid = true;

    return true;
}


/** \brief Set the name of the versioned filename object.
 *
 * A versioned filename is composed of a name, a version, and an optional
 * browser reference. This function is used to replace the name.
 *
 * The name is checked using the \p validate_name() function.
 *
 * \param[in] name  The new file name.
 *
 * \return true if the name is valid.
 */
bool versioned_filename::set_name(QString const& name)
{
    bool r(validate_name(name));
    if(r)
    {
        f_name = name;
    }

    return r;
}


/** \brief Verify that the name or browser strings are valid.
 *
 * The \p name parameter is checked for validity. It must match the
 * following:
 *
 * \li Start with a letter [a-z].
 * \li Include only letters [a-z], digits [0-9], and dahses (-).
 * \li The name does not end with a dash (-).
 * \li The name does not include two dashes one after another (--).
 * \li The name is 2 characters or more.
 *
 * Which looks like this as a regex:
 *
 * \code
 * /[a-z][-a-z0-9]+/
 * \endcode
 *
 * The names must exclusively be composed of lowercase letters. This will
 * allow, one day, to run Snap! on computers that do not distinguish
 * between case (i.e. Mac OS/X.)
 *
 * \note
 * This function is used to verify the name and the browser strings.
 *
 * \param[in] name  The name to be checked against the name pattern.
 *
 * \return true if the name matches, false otherwise.
 */
bool versioned_filename::validate_name(QString const& name)
{
    // length constraint
    int const max(name.length());
    if(max < 2)
    {
        f_error = "the name or browser in a versioned filename must be at least two characters. \"" + name + "\" is not valid.";
        return false;
    }

    // make sure that the name starts with a letter ([a-z])
    ushort c(name.at(0).unicode());
    if(c < 'a' || c > 'z')
    {
        // name cannot start with dash (-) or a digit ([0-9])
        f_error = "the name or browser of a versioned filename must start with a letter [a-z]. \"" + name + "\" is not valid.";
        return false;
    }
    if(name.at(max - 1).unicode() == '-')
    {
        // name cannot end with a dash (-)
        f_error = "A versioned name or browser cannot end with a dash (-). \"" + name + "\" is not valid.";
        return false;
    }

    // start with 1 because we just checked character 0 and it's valid
    for(int i(1); i < max; ++i)
    {
        c = name.at(i).unicode();

        if(c == '-')
        {
            // prevent two dashes in a row
            // the -1 is safe because we start the loop at 1
            if(name.at(i - 1).unicode() == '-')
            {
                f_error = "A name or browser versioned filename cannot include two dashes (--) one after another. \"" + name + "\" is not valid.";
                return false;
            }
        }
        else if((c < '0' || c > '9')
             && (c < 'a' || c > 'z'))
        {
            // name can only include [a-z0-9] and dashes (-)
            f_error = "A name or browser versioned filename can only include letters (a-z), digits (0-9), or dashes (-). \"" + name + "\" is not valid.";
            return false;
        }
    }

    return true;
}


/** \brief Set the version of the versioned filename.
 *
 * This function sets the version of the versioned filename. Usually, you
 * will call the set_filename() function which sets the name, the version,
 * and the optional browser all at once and especially let the parsing
 * work to the versioned_filename class.
 *
 * \param[in] version_string  The version in the form of a string.
 *
 * \return true if the version was considered valid.
 */
bool versioned_filename::set_version(QString const& version_string)
{
    QString vs(version_string);
    version_t version;
    bool r(validate_version(vs, version));
    if(r)
    {
        f_version_string = vs;
        f_version.swap(version);
    }

    return r;
}


/** \brief Validate a version.
 *
 * This function validates a version string and returns the result.
 *
 * The validation includes three steps:
 *
 * \li Parse the input version_string parameter in separate numbers.
 * \li Save those numbers in the version vector.
 * \li Generate a canonicalized version_string.
 *
 * The function only supports sets of numbers in the version. Something
 * similar to 1.2.3. The regex of the version_string looks like this:
 *
 * \code
 * [0-9]+(\.[0-9]+)*
 * \endcode
 *
 * The versions are viewed as:
 *
 * \li Major Release Version (public)
 * \li Minor Release Version (public)
 * \li Patch Version (still public)
 * \li Development or Build Version (not public)
 *
 * While in development, each change should be reflected by incrementing
 * the development (or build) version number by 1. That way your browser
 * will automatically reload the new file.
 *
 * Once the development is over and a new version is to be released,
 * remove the development version or reset it to zero and increase the
 * Patch Version, or one of the Release Versions as appropriate.
 *
 * If you are trying to install a 3rd party JavaScript library which uses
 * a different scheme for their version, learn of their scheme and adapt
 * it to our versions. For example, a version defined as:
 *
 * \code
 * <major-version>.<minor-version>[<patch>]
 * \endcode
 *
 * where <patch> is a letter, can easily be converted to a 1.2.3 type of
 * version where the letters are numbered starting at 1 (if no patch letter,
 * use zero.)
 *
 * In the end the function returns an array of integer in the \p version
 * parameter. This parameter is used by subsequent compare() calls.
 *
 * \note
 * The version "0" is considered valid although maybe not useful (We
 * suggest that you do not use it, use at least 0.0.0.1)
 *
 * \param[in,out] version_string  The version to be parsed.
 * \param[out] version  The array of numbers.
 *
 * \return true if the version is considered valid.
 */
bool versioned_filename::validate_version(QString& version_string, version_t& version)
{
    version.clear();

    int const max(version_string.length());
    if(max < 1)
    {
        f_error = "The version in a versioned filename is required after the name. \"" + version_string + "\" is not valid.";
        return false;
    }
    if(version_string.at(max - 1).unicode() == '.')
    {
        f_error = "The version in a versioned filename cannot end with a period. \"" + version_string + "\" is not valid.";
        return false;
    }

    for(int i(0); i < max;)
    {
        // force the version to have a digit at the start
        // and after each period
        ushort c(version_string.at(i).unicode());
        if(c < '0' || c > '9')
        {
            f_error = "The version of a versioned filename is expected to have a number at the start and after each period. \"" + version_string + "\" is not valid.";
            return false;
        }
        int value(c - '0');
        // start with ++i because we just checked character 'i'
        for(++i; i < max;)
        {
            c = version_string.at(i).unicode();
            ++i;
            if(c < '0' || c > '9')
            {
                if(c != '.')
                {
                    f_error = "The version of a versioned filename is expected to be composed of numbers and periods (.) only. \"" + version_string + "\" is not valid.";
                    return false;
                }
                if(i == max)
                {
                    throw snap_logic_exception("The version_string was already checked for an ending '.' and yet we reached that case later in the function.");
                }
                break;
            }
            value = value * 10 + c - '0';
        }
        version.push_back(value);
    }

    // canonicalize the array
    while(version.size() > 1 && 0 == version[version.size() - 1])
    {
        version.pop_back();
    }

    // canonicalize the version string now
    version_string.clear();
    int const jmax(version.size());
    for(int j(0); j < jmax; ++j)
    {
        if(j != 0)
        {
            version_string += ".";
        }
        version_string += QString("%1").arg(version[j]);
    }

    return true;
}


/** \brief Return the canonicalized filename.
 *
 * This function returns the canonicalized filename. This means all version
 * numbers have leading 0's removed, ending .0 are all removed, and the
 * path is removed.
 *
 * The \p extension flag can be used to get the extension appended or not.
 *
 * \param[in] extension  Set to true to get the extension.
 *
 * \return The canonicalized filename.
 */
QString versioned_filename::get_filename(bool extension) const
{
    if(!f_valid)
    {
        return "";
    }
    return f_name
         + "_" + f_version_string
         + (f_browser.isEmpty() ? "" : "_" + f_browser)
         + (extension ? f_extension : "");
}


/** \brief Compare two versioned_filename's against each others.
 *
 * This function first makes sure that both filenames are considered
 * valid, if not, the function returns COMPARE_INVALID (-2).
 *
 * Assuming the two filenames are valid, the function returns:
 *
 * \li COMPARE_SMALLER (-1) if this filename is considered to appear before rhs
 * \li COMPARE_EQUAL (0) if both filenames are considered equal
 * \li COMPARE_LARGER (1) if this filename is considered to appear after rhs
 *
 * The function first compares the name (get_name()) of each object.
 * If not equal, return COMPARE_SMALLER or COMPARE_LARGER.
 *
 * When the name are equal, the function compares the browser (get_browser())
 * of each object. If not equal, rethrn COMPARE_SMALLER or COMPARE_LARGER.
 *
 * When the name and the browser are equal, then the function compares the
 * versions starting with the major release number. If a version array is
 * longer than the other, the missing values in the smaller array are
 * considered to be zero. That way "1.2.3" > "1.2" because "1.2" is the
 * same as "1.2.0" and 3 > 0.
 *
 * \param[in] rhs  The right hand side to compare against this versioned
 *                 filename.
 *
 * \return -2, -1, 0, or 1 depending on the order (or unordered status)
 */
versioned_filename::compare_t versioned_filename::compare(versioned_filename const& rhs) const
{
    if(!f_valid || !rhs.f_valid)
    {
        return COMPARE_INVALID;
    }

    if(f_name < rhs.f_name)
    {
        return COMPARE_SMALLER;
    }
    if(f_name > rhs.f_name)
    {
        return COMPARE_LARGER;
    }

    if(f_browser < rhs.f_browser)
    {
        return COMPARE_SMALLER;
    }
    if(f_browser > rhs.f_browser)
    {
        return COMPARE_LARGER;
    }

    int const max(std::max(f_version.size(), rhs.f_version.size()));
    for(int i(0); i < max; ++i)
    {
        int l(i >=     f_version.size() ? 0 : static_cast<int>(    f_version[i]));
        int r(i >= rhs.f_version.size() ? 0 : static_cast<int>(rhs.f_version[i]));
        if(l < r)
        {
            return COMPARE_SMALLER;
        }
        if(l > r)
        {
            return COMPARE_LARGER;
        }
    }

    return COMPARE_EQUAL;
}


/** \brief Compare two filenames for equality.
 *
 * This function returns true if both filenames are considered equal
 * (i.e. if the compare() function returns 0.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered equal.
 */
bool versioned_filename::operator == (versioned_filename const& rhs) const
{
    int r(compare(rhs));
    return r == COMPARE_EQUAL;
}


/** \brief Compare two filenames for differences.
 *
 * This function returns true if both filenames are not considered equal
 * (i.e. if the compare() function returns -1 or 1.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered different.
 */
bool versioned_filename::operator != (versioned_filename const& rhs) const
{
    int r(compare(rhs));
    return r == COMPARE_SMALLER || r == COMPARE_LARGER;
}


/** \brief Compare two filenames for inequality.
 *
 * This function returns true if this filename is considered to appear before
 * \p rhs filename (i.e. if the compare() function returns -1.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered inequal.
 */
bool versioned_filename::operator <  (versioned_filename const& rhs) const
{
    int r(compare(rhs));
    return r == COMPARE_SMALLER;
}


/** \brief Compare two filenames for inequality.
 *
 * This function returns true if this filename is considered to appear before
 * \p rhs filename or both are equal (i.e. if the compare() function
 * returns -1 or 0.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered inequal.
 */
bool versioned_filename::operator <= (versioned_filename const& rhs) const
{
    int r(compare(rhs));
    return r == COMPARE_SMALLER || r == COMPARE_EQUAL;
}


/** \brief Compare two filenames for inequality.
 *
 * This function returns true if this filename is considered to appear after
 * \p rhs filename (i.e. if the compare() function returns 1.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered inequal.
 */
bool versioned_filename::operator >  (versioned_filename const& rhs) const
{
    int r(compare(rhs));
    return r > COMPARE_EQUAL;
}


/** \brief Compare two filenames for inequality.
 *
 * This function returns true if this filename is considered to appear before
 * \p rhs filename or both are equal (i.e. if the compare() function
 * returns 0 or 1.)
 *
 * Note that if one or both filenames are considered unordered, the
 * function always returns false.
 *
 * \param[in] rhs  The other filename to compare against.
 *
 * \return true if both filenames are considered inequal.
 */
bool versioned_filename::operator >= (versioned_filename const& rhs) const
{
    int r(compare(rhs));
    return r >= COMPARE_EQUAL;
}


} // namespace snap
// vim: ts=4 sw=4 et
