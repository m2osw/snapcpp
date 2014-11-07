// Snap Websites Servers -- Fuzzy String Comparisons
// Copyright (C) 2011-2014  Made to Order Software Corp.
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

#include "fuzzy_string_compare.h"

#include <vector>

namespace snap
{

/** \brief Computes the Levenshtein distance between two strings.
 *
 * This function calculates the Levenshtein distance between two strings
 * using the fastest algorithm, assuming that allocating memory is fast.
 *
 * The strings are expected to be UTF-32, although under a system like
 * MS-Windows a wstring uses UTF-16 instead...
 *
 * \note
 * This algorithm comes from Wikipedia:
 * https://en.wikipedia.org/wiki/Levenshtein_distance
 *
 * \param[in] s  The left hand side string.
 * \param[in] t  The right hand side string.
 *
 * \return The Levenshtein Distance between \p s and \p t.
 */
int levenshtein_distance(std::wstring s, std::wstring t)
{
    // degenerate cases
    if(s == t)
    {
        return 0; // exactly equal distance is zero
    }
    if(s.empty())
    {
        return t.length();
    }
    if(t.empty())
    {
        return s.length();
    }
 
    // create two work vectors of integer distances
    std::vector<int> v0(t.length() + 1);
    std::vector<int> v1(v0.size());
 
    // initialize v0 (the previous row of distances)
    // this row is A[0][i]: edit distance for an empty s
    // the distance is just the number of characters to delete from t
    for(size_t i(0); i < v0.size(); ++i)
    {
        v0[i] = i;
    }
 
    for(size_t i(0); i < s.length(); ++i)
    {
        // calculate v1 (current row distances) from the previous row v0
 
        // first element of v1 is A[i+1][0]
        //   edit distance is delete (i+1) chars from s to match empty t
        v1[0] = i + 1;
 
        // use formula to fill in the rest of the row
        for(size_t j(0); j < t.length(); j++)
        {
            int const cost(s[i] == t[j] ? 0 : 1);
            v1[j + 1] = std::min(v1[j] + 1, std::min(v0[j + 1] + 1, v0[j] + cost));
        }
 
        // copy v1 (current row) to v0 (previous row) for next iteration
        //v0 = v1; -- swap is a lot faster!
        v0.swap(v1);
    }
 
    return v0[t.length()];
}

} // namespace snap
// vim: ts=4 sw=4 et
