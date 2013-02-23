/* TLD library -- TLD, domain name, and sub-domain extraction
 * Copyright (C) 2011-2013  Made to Order Software Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef LIB_TLD_DATA_H
#define LIB_TLD_DATA_H
/** \file
 * \brief Declaration of the tld_description structure.
 *
 * The generated tld_data.c source file makes use of the structure
 * and global variables defined in this header file. These are private
 * to the library but required across different files.
 */

/** \brief Description of a TLD.
 *
 * This structure defines one TLD. It includes the category, status, whether
 * it is an exception, the TLD name, and if the TLD is linked to a country
 * the name of the country.
 */
struct tld_description
{
    /** \brief One of the enum tld_category values.
     *
     * The category of this TLD such as TLD_CATEGORY_INTERNATIONAL for ".com".
     */
    unsigned char               f_category;

    /** \brief One of the enum tld_status values.
     *
     * The status of this specific TLD such as TLD_STATUS_VALID for a TLD that
     * is currently in use. We define many TLDs that are not in use because
     * they can match other createria which makes them otherwise unsafe. Also
     * we define some country TLDs that are not currently used such as ".ss"
     * for South Soudan.
     */
    unsigned char               f_status;

    /** \brief The offset to the start of the array of next TLD levels.
     *
     * The value represents the start of an array within the TLD table that
     * defines a set of sub-TLD names. The end of the array is defined in
     * the f_end_offset field.
     *
     * For example, the .uk TLD supports .ac and .co. Therefore, the .uk TLD
     * definition includes a start and end offset that point to those .ac
     * and .co sub-names.
     *
     * This value may be USHRT_MAX when no next TLD level is defined.
     * In that case, the f_end_offset is also USHRT_MAX.
     */
    unsigned short              f_start_offset;
    
    /** \brief The offset to the end of the array of next TLD levels.
     *
     * This value is the complement of the f_start_offset index and it
     * indicates the end of the array. This index is excluded (i.e. the
     * entry at that offset is not part of the start/end list of sub-TLDs.)
     *
     * This value may be USHRT_MAX when no next TLD level is defined.
     * In that case, the f_start_offset is also USHRT_MAX.
     */
    unsigned short              f_end_offset;

    /* \brief The TLD this exception applies to.
     *
     * Exceptions refer to the actual TLD that needs to be returned when
     * found. For example, the UK library uses library.uk instead of the
     * expected library.org.uk. The exception is the sub-TLD "british-library"
     * and the apply-to TLD is "uk". By default "uk" is considered
     * invalid and would return an error. When found through an exception
     * though, it is returned as a valid TLD.
     *
     * Note that means the british-library.uk website can set a cookie
     * using ".uk" as the domain name.
     *
     * \note
     * This field is only defined if f_status is TLD_STATUS_EXCEPTION.
     */
    unsigned short              f_exception_apply_to;

    /** \brief The TLD level to be returned with this exception.
     *
     * The level when jumping to the "apply to" exception. This is generally
     * the same as the TLD defined at the f_exception_apply_to offset.
     *
     * \note
     * This field may not be required anymore.
     */
    unsigned char               f_exception_level;

    /** \brief The concerned TLD part without periods.
     *
     * This is the actual TLD such as "com" or "uk". This string does
     * not include the period since there is no need to waste several
     * Kb of data (remember that you'd have to read it to compare with
     * the input data too!)
     *
     * The string is read-only and cannot ever be modified.
     * However, we do not return this string to the caller. Instead
     * we return a pointer inside the caller's string.
     */
    const char *                f_tld;

    /** \brief The name of the country owning this extension.
     *
     * Many of the TLDs are owned by one of the 270+ countries of
     * the world. All such TLDs have this pointer defined with the
     * country name in English, cased as expected in English.
     *
     * Note that although we use the English name, some country
     * names still make use of accentuated characters.
     */
    const char *                f_country;
};

/** \brief Array of all the TLDs defined in the tld_data.c file.
 *
 * This array is defined in the tld_data.c which is automatically
 * generated by the tld_parser tool.
 */
extern const struct tld_description     tld_descriptions[];

/** \brief The start offset of the top-most top-level-domain names.
 *
 * This variable holds the start of the top-most domain names
 * in the tld_descriptions table. It is always defined.
 */
extern unsigned short                   tld_start_offset;

/** \brief The end offset of the top-most top-level domain names.
 *
 * This variable holds the end of the top-most domain names
 * in the tld_descriptions table. It is always defined. It
 * actually represents the last offset + 1 (i.e. it is
 * exclusive like the f_end_offset of the tld_description
 * structure.)
 *
 * Note that this value also represents the size of of the
 * tld_descriptions table since the list of top-most domain
 * names are saved last.
 */
extern unsigned short                   tld_end_offset;

/** \brief The maximum number of levels defined in the TLD table.
 *
 * This value represents the maximum number of levels defined in
 * the tld_descriptions. This can be used by different functions
 * to limit the number of levels searched in the input string.
 *
 * As an example, the .co.uk TLD uses 2 levels: level 1 is ".uk"
 * and level 2 is ".co". The longest TLD currently uses 4 levels.
 */
extern int                              tld_max_level;

#endif
//#ifndef LIB_TLD_DATA_H
// vim: ts=4 sw=4 et
