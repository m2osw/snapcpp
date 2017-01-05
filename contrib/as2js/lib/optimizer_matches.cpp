/* optimizer_compare.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

/*

Copyright (c) 2005-2017 Made to Order Software Corp.

http://snapwebsites.org/project/as2js

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and
associated documentation files (the "Software"), to
deal in the Software without restriction, including
without limitation the rights to use, copy, modify,
merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice
shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include    "optimizer_tables.h"

#include    "as2js/exceptions.h"


namespace as2js
{
namespace optimizer_details
{


/** \brief Hide all optimizer compare implementation details.
 *
 * This unnamed namespace is used to further hide all the optimizer
 * details.
 */
namespace
{


/** \brief Compare a node against a specific match.
 *
 * This function checks the data of one node against the data defined
 * by the match parameter.
 *
 * The matching processes uses the parameters defined in the optimization
 * match structure. This includes:
 *
 * \li Node Type -- whether one of the node types defined in the match
 *                  structure is equal to the type of \p node.
 * \li Attributes -- whether one set of the attributes defined in the
 *                   match structure is equal to the attributes defined
 *                   in \p node
 * \li Flags -- whether one set of the flags defined in the match structure
 *              is equal to the attributes defined in \p node
 * \li Links -- whether each set of links has at least one tree that
 *              matches the links of \p node
 *
 * Any one of those match lists can be empty (its size is zero) in which
 * case it it ignored and the node can as well have any value there. It
 * is very likely that testing attributes, flags, or links on a node of
 * which the type was not tested will not be a good match.
 *
 * \internal
 *
 * \param[in] node  The node to compare against.
 * \param[in] match  The optimization match to use against this node.
 *
 * \return true if the node is a perfect match.
 */
bool match_node(node_pointer_vector_t& node_array, Node::pointer_t node, optimization_match_t const *match)
{
    // match node types
    if(match->f_node_types_count > 0)
    {
        Node::node_t const node_type(node->get_type());
        for(size_t idx(0);; ++idx)
        {
            if(idx >= match->f_node_types_count)
            {
                return false;
            }
            if(match->f_node_types[idx] == node_type)
            {
                break;
            }
        }
    }

    if(match->f_with_value != nullptr)
    {
        optimization_match_t::optimization_literal_t const *value(match->f_with_value);

        // note: we only need to check STRING, INT64, and FLOAT64 literals
        switch(value->f_operator)
        {
        case Node::node_t::NODE_ASSIGNMENT:
            if(node->has_side_effects())
            {
                return false;
            }
            break;

        case Node::node_t::NODE_IDENTIFIER:
            if(value->f_int64 != 0)
            {
                if(static_cast<size_t>(value->f_int64) >= node_array.size())
                {
                    throw exception_internal_error("INTERNAL ERROR: identifier check using an index larger than the existing nodes"); // LCOV_EXCL_LINE
                }
                if(node_array[value->f_int64]->get_string() != node->get_string())
                {
                    return false;
                }
            }
            else
            {
                if(node->get_string() != value->f_string)
                {
                    return false;
                }
            }
            break;

        case Node::node_t::NODE_BITWISE_AND:
            switch(node->get_type())
            {
            case Node::node_t::NODE_INT64:
                {
                    uint32_t mask(static_cast<uint32_t>(value->f_float64));
                    if((node->get_int64().get() & mask) != value->f_int64)
                    {
                        return false;
                    }
                }
                break;

            case Node::node_t::NODE_FLOAT64:
                {
                    uint32_t mask(static_cast<uint32_t>(value->f_float64));
                    if((static_cast<uint32_t>(node->get_float64().get()) & mask) != value->f_int64)
                    {
                        return false;
                    }
                }
                break;

            default:
                throw exception_internal_error("INTERNAL ERROR: optimizer optimization_literal_t table used against an unsupported node type."); // LCOV_EXCL_LINE

            }
            break;

        case Node::node_t::NODE_EQUAL:
        case Node::node_t::NODE_STRICTLY_EQUAL:
            switch(node->get_type())
            {
            // This is not yet accessible (as in, nothing makes use of it
            // and I'm not totally sure it will come up, re-add later if
            // useful.)
            //case Node::node_t::NODE_STRING:
            //    if(node->get_string() != value->f_string)
            //    {
            //        return false;
            //    }
            //    break;

            case Node::node_t::NODE_INT64:
                if(node->get_int64().get() != value->f_int64)
                {
                    return false;
                }
                break;

            case Node::node_t::NODE_FLOAT64:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                // if we expect a NaN make sure both are NaN
                // remember that == and != always return false
                // when checked with one or two NaN
                if(std::isnan(value->f_float64))
                {
                    if(!node->get_float64().is_NaN())
                    {
                        return false;
                    }
                }
                else if(node->get_float64().get() != value->f_float64)
                {
                    return false;
                }
#pragma GCC diagnostic pop
                break;

            default:
                throw exception_internal_error("INTERNAL ERROR: optimizer optimization_literal_t table used against an unsupported node type."); // LCOV_EXCL_LINE

            }
            break;

        case Node::node_t::NODE_TRUE:
            if(node->to_boolean_type_only() != Node::node_t::NODE_TRUE)
            {
                return false;
            }
            break;

        case Node::node_t::NODE_FALSE:
            if(node->to_boolean_type_only() != Node::node_t::NODE_FALSE)
            {
                return false;
            }
            break;

        default:
            throw exception_internal_error("INTERNAL ERROR: optimizer optimization_literal_t table using an unsupported comparison operator."); // LCOV_EXCL_LINE

        }
    }

    // match node attributes
    if(match->f_attributes_count > 0)
    {
        Node::attribute_set_t attrs;
        // note: if the list of attributes is just one entry and that
        //       one entry is NODE_ATTR_max, we compare the same thing
        //       twice (i.e. that all attributes are false)
        for(size_t idx(0); idx < match->f_attributes_count; ++idx)
        {
            if(match->f_attributes[idx] == Node::attribute_t::NODE_ATTR_max)
            {
                if(!node->compare_all_attributes(attrs))
                {
                    return false;
                }
                attrs.reset();
            }
            else
            {
                attrs[static_cast<size_t>(match->f_attributes[idx])] = true;
            }
        }
        if(!node->compare_all_attributes(attrs))
        {
            return false;
        }
    }

    // match node flags
    if(match->f_flags_count > 0)
    {
        Node::flag_set_t flags;
        // note: if the list of flags is just one entry and that
        //       one entry is NODE_FALG_max, we compare the same thing
        //       twice (i.e. that all flags are false)
        for(size_t idx(0); idx < match->f_flags_count; ++idx)
        {
            if(match->f_flags[idx] == Node::flag_t::NODE_FLAG_max)
            {
                if(!node->compare_all_flags(flags))
                {
                    return false;
                }
                flags.reset();
            }
            else
            {
                flags[static_cast<size_t>(match->f_flags[idx])] = true;
            }
        }
        if(!node->compare_all_flags(flags))
        {
            return false;
        }
    }

	// TODO: we may want to add tests for the instance, type node, goto exit, goto enter links

    // everything matched
    return true;
}


}
// noname namespace


/** \brief Compare a node against an optimization tree.
 *
 * This function goes through a node tree and and optimization tree.
 * If they both match, then the function returns true.
 *
 * The function is generally called using the node to be checked and
 * the match / match_count parameters as found in an optimization
 * structure.
 *
 * The depth is expected to start at zero.
 *
 * The function is recursive in order to handle the whole tree (i.e.
 * when the function determines that the node is a match with the
 * current match level, it then checks all the children of the current
 * node if required.)
 *
 * \internal
 *
 * \param[in] node_array  The final array of nodes matched
 * \param[in] node  The node to be checked against this match.
 * \param[in] match  An array of match definitions.
 * \param[in] match_count  The size of the match array.
 * \param[in] depth  The current depth (match->f_depth == depth).
 *
 * \return true if the node matches that optimization match tree.
 */
bool match_tree(node_pointer_vector_t& node_array, Node::pointer_t node, optimization_match_t const *match, size_t match_count, uint8_t depth)
{
    // attempt a match only if proper depth
    if(match->f_depth == depth
    && match_node(node_array, node, match))
    {
        node_array.push_back(node);
//std::cerr << "Matched " << node->get_type_name()
//                        << ", depth=" << static_cast<int>(depth)
//                        << ", count=" << match_count
//                        << ", children? " << ((match->f_match_flags & OPTIMIZATION_MATCH_FLAG_CHILDREN) != 0 ? "YES" : "no")
//                        << ", size=" << node_array.size()
//                        << "\n";

        size_t const max_child(node->get_children_size());
        size_t c(max_child);

        // it matched, do we have more to check in the tree?
        --match_count;
        if(match_count != 0 && (match->f_match_flags & OPTIMIZATION_MATCH_FLAG_CHILDREN) != 0)
        {

#if defined(_DEBUG) || defined(DEBUG)
            if(depth >= 255)
            {
                throw exception_internal_error("INTERNAL ERROR: optimizer is using a depth of more than 255."); // LCOV_EXCL_LINE
            }
#endif

            // check that the children are a match
            uint8_t const next_level(depth + 1);

            c = 0;
            for(; match_count > 0; --match_count)
            {
                ++match;
                if(match->f_depth == next_level)
                {
                    if(c >= max_child)
                    {
                        // another match is required, but no more children are
                        // available in this node...
                        return false;
                    }
                    if(!match_tree(node_array, node->get_child(c), match, match_count, next_level))
                    {
                        // not a match
                        return false;
                    }
                    ++c;
                }
                else if(match->f_depth < next_level)
                {
                    // we arrived at the end of this list of children
                    break;
                }
            }
        }

        // return true if all children were taken in account
        return c >= max_child;
    }

    // no matches
    return false;
}


}
// namespace optimizer_details
}
// namespace as2js

// vim: ts=4 sw=4 et
