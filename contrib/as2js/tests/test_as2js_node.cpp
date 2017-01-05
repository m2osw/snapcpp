/* test_as2js_node.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2017 */

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

#include    "test_as2js_node.h"
#include    "test_as2js_main.h"

#include    "as2js/exceptions.h"
#include    "as2js/message.h"
#include    "as2js/node.h"
#include    "as2js/os_raii.h"

#include    <cstring>
#include    <algorithm>
#include    <iomanip>

#include    <cppunit/config/SourcePrefix.h>
CPPUNIT_TEST_SUITE_REGISTRATION( As2JsNodeUnitTests );


#include "test_as2js_node_data.ci"


void As2JsNodeUnitTests::test_type()
{
    // test all the different types available
    std::bitset<static_cast<size_t>(as2js::Node::node_t::NODE_max)> valid_types;
    for(size_t i(0); i < g_node_types_size; ++i)
    {
        // define the type
        as2js::Node::node_t const node_type(g_node_types[i].f_type);

        if(static_cast<size_t>(node_type) > static_cast<size_t>(as2js::Node::node_t::NODE_max))
        {
            if(node_type != as2js::Node::node_t::NODE_EOF)
            {
                std::cerr << "Somehow a node type (" << static_cast<int>(node_type)
                          << ") is larger than the maximum allowed ("
                          << (static_cast<int>(as2js::Node::node_t::NODE_max) - 1) << ")" << std::endl;
            }
        }
        else
        {
            valid_types[static_cast<size_t>(node_type)] = true;
        }

        // get the next type of node
        as2js::Node::pointer_t node(new as2js::Node(node_type));

        // check the type
        CPPUNIT_ASSERT(node->get_type() == node_type);

        // get the name
        char const *name(node->get_type_name());
//std::cerr << "type = " << static_cast<int>(node_type) << " / " << name << "\n";
        CPPUNIT_ASSERT(strcmp(name, g_node_types[i].f_name) == 0);

        // test functions determining general types
        CPPUNIT_ASSERT(node->is_number() == false || node->is_number() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_number() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_NUMBER) == 0));

        // This NaN test is not sufficient for strings
        CPPUNIT_ASSERT(node->is_nan() == false || node->is_nan() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_nan() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_NAN) == 0));

        CPPUNIT_ASSERT(node->is_int64() == false || node->is_int64() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_int64() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_INT64) == 0));

        CPPUNIT_ASSERT(node->is_float64() == false || node->is_float64() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_float64() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_FLOAT64) == 0));

        CPPUNIT_ASSERT(node->is_boolean() == false || node->is_boolean() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_boolean() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_BOOLEAN) == 0));

        CPPUNIT_ASSERT(node->is_true() == false || node->is_true() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_true() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_TRUE) == 0));

        CPPUNIT_ASSERT(node->is_false() == false || node->is_false() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_false() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_FALSE) == 0));

        CPPUNIT_ASSERT(node->is_string() == false || node->is_string() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_string() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_STRING) == 0));

        CPPUNIT_ASSERT(node->is_undefined() == false || node->is_undefined() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_undefined() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_UNDEFINED) == 0));

        CPPUNIT_ASSERT(node->is_null() == false || node->is_null() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_null() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_NULL) == 0));

        CPPUNIT_ASSERT(node->is_identifier() == false || node->is_identifier() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_identifier() ^ ((g_node_types[i].f_flags & TEST_NODE_IS_IDENTIFIER) == 0));

        CPPUNIT_ASSERT(node->is_literal() == false || node->is_literal() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->is_literal() ^ ((g_node_types[i].f_flags & (
                                                                                          TEST_NODE_IS_INT64
                                                                                        | TEST_NODE_IS_FLOAT64
                                                                                        | TEST_NODE_IS_TRUE
                                                                                        | TEST_NODE_IS_FALSE
                                                                                        | TEST_NODE_IS_STRING
                                                                                        | TEST_NODE_IS_UNDEFINED
                                                                                        | TEST_NODE_IS_NULL)) == 0));

        if(!node->is_literal())
        {
            as2js::Node::pointer_t literal(new as2js::Node(as2js::Node::node_t::NODE_STRING));
            CPPUNIT_ASSERT(as2js::Node::compare(node, literal, as2js::Node::compare_mode_t::COMPARE_STRICT)  == as2js::compare_t::COMPARE_ERROR);
            CPPUNIT_ASSERT(as2js::Node::compare(node, literal, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_ERROR);
            CPPUNIT_ASSERT(as2js::Node::compare(node, literal, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_ERROR);
            CPPUNIT_ASSERT(as2js::Node::compare(literal, node, as2js::Node::compare_mode_t::COMPARE_STRICT)  == as2js::compare_t::COMPARE_ERROR);
            CPPUNIT_ASSERT(as2js::Node::compare(literal, node, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_ERROR);
            CPPUNIT_ASSERT(as2js::Node::compare(literal, node, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_ERROR);
            CPPUNIT_ASSERT(as2js::Node::compare(literal, node, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_ERROR);
        }

        CPPUNIT_ASSERT(node->has_side_effects() == false || node->has_side_effects() == true);
        CPPUNIT_ASSERT(static_cast<as2js::Node const *>(node.get())->has_side_effects() ^ ((g_node_types[i].f_flags & TEST_NODE_HAS_SIDE_EFFECTS) == 0));

        if(g_node_types[i].f_operator != nullptr)
        {
            char const *op(as2js::Node::operator_to_string(g_node_types[i].f_type));
            CPPUNIT_ASSERT(op != nullptr);
            CPPUNIT_ASSERT(strcmp(g_node_types[i].f_operator, op) == 0);
            //std::cerr << " testing " << node->get_type_name() << " from " << op << std::endl;
            CPPUNIT_ASSERT(as2js::Node::string_to_operator(op) == g_node_types[i].f_type);

            // check the special case for not equal
            if(strcmp(g_node_types[i].f_operator, "!=") == 0)
            {
                CPPUNIT_ASSERT(as2js::Node::string_to_operator("<>") == g_node_types[i].f_type);
            }

            // check the special case for assignment
            if(strcmp(g_node_types[i].f_operator, "=") == 0)
            {
                CPPUNIT_ASSERT(as2js::Node::string_to_operator(":=") == g_node_types[i].f_type);
            }
        }
        else
        {
            // static function can also be called from the node pointer
            //std::cerr << " testing " << node->get_type_name() << std::endl;
            CPPUNIT_ASSERT(node->operator_to_string(g_node_types[i].f_type) == nullptr);
            CPPUNIT_ASSERT(as2js::Node::string_to_operator(node->get_type_name()) == as2js::Node::node_t::NODE_UNKNOWN);
        }

        if((g_node_types[i].f_flags & TEST_NODE_IS_SWITCH_OPERATOR) == 0)
        {
            // only NODE_PARAM_MATCH accepts this call
            as2js::Node::pointer_t node_switch(new as2js::Node(as2js::Node::node_t::NODE_SWITCH));
            CPPUNIT_ASSERT_THROW(node_switch->set_switch_operator(node_type), as2js::exception_internal_error);
        }
        else
        {
            as2js::Node::pointer_t node_switch(new as2js::Node(as2js::Node::node_t::NODE_SWITCH));
            node_switch->set_switch_operator(node_type);
            CPPUNIT_ASSERT(node_switch->get_switch_operator() == node_type);
        }
        if(node_type != as2js::Node::node_t::NODE_SWITCH)
        {
            // a valid operator, but not a valid node to set
            CPPUNIT_ASSERT_THROW(node->set_switch_operator(as2js::Node::node_t::NODE_STRICTLY_EQUAL), as2js::exception_internal_error);
            // not a valid node to get
            CPPUNIT_ASSERT_THROW(node->get_switch_operator(), as2js::exception_internal_error);
        }

        if((g_node_types[i].f_flags & TEST_NODE_IS_PARAM_MATCH) == 0)
        {
            // only NODE_PARAM_MATCH accepts this call
            CPPUNIT_ASSERT_THROW(node->set_param_size(10), as2js::exception_internal_error);
        }
        else
        {
            // zero is not acceptable
            CPPUNIT_ASSERT_THROW(node->set_param_size(0), as2js::exception_internal_error);
            // this one is accepted
            node->set_param_size(10);
            // cannot change the size once set
            CPPUNIT_ASSERT_THROW(node->set_param_size(10), as2js::exception_internal_error);
        }

        if((g_node_types[i].f_flags & TEST_NODE_IS_BOOLEAN) == 0)
        {
            CPPUNIT_ASSERT_THROW(node->get_boolean(), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->set_boolean(rand() & 1), as2js::exception_internal_error);
        }
        else if((g_node_types[i].f_flags & TEST_NODE_IS_TRUE) != 0)
        {
            CPPUNIT_ASSERT(node->get_boolean());
        }
        else
        {
            CPPUNIT_ASSERT(!node->get_boolean());
        }

        if((g_node_types[i].f_flags & TEST_NODE_IS_INT64) == 0)
        {
            CPPUNIT_ASSERT_THROW(node->get_int64(), as2js::exception_internal_error);
            as2js::Int64 random(rand());
            CPPUNIT_ASSERT_THROW(node->set_int64(random), as2js::exception_internal_error);
        }

        if((g_node_types[i].f_flags & TEST_NODE_IS_FLOAT64) == 0)
        {
            CPPUNIT_ASSERT_THROW(node->get_float64(), as2js::exception_internal_error);
            as2js::Float64 random(rand());
            CPPUNIT_ASSERT_THROW(node->set_float64(random), as2js::exception_internal_error);
        }

        // here we have a special case as "many" different nodes accept
        // a string to represent one thing or another
        if((g_node_types[i].f_flags & TEST_NODE_ACCEPT_STRING) == 0)
        {
            CPPUNIT_ASSERT_THROW(node->get_string(), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->set_string("test"), as2js::exception_internal_error);
        }
        else
        {
            node->set_string("random test");
            CPPUNIT_ASSERT(node->get_string() == "random test");
        }

        // first test the flags that this type of node accepts
        std::bitset<static_cast<int>(as2js::Node::flag_t::NODE_FLAG_max)> valid_flags;
        for(flags_per_node_t const *node_flags(g_node_types[i].f_node_flags);
                                    node_flags->f_flag != as2js::Node::flag_t::NODE_FLAG_max;
                                    ++node_flags)
        {
            // mark this specific flag as valid
            valid_flags[static_cast<int>(node_flags->f_flag)] = true;

            as2js::Node::flag_set_t set;
            CPPUNIT_ASSERT(node->compare_all_flags(set));


            // before we set it, always false
            CPPUNIT_ASSERT(!node->get_flag(node_flags->f_flag));
            node->set_flag(node_flags->f_flag, true);
            CPPUNIT_ASSERT(node->get_flag(node_flags->f_flag));

            CPPUNIT_ASSERT(!node->compare_all_flags(set));
            set[static_cast<int>(node_flags->f_flag)] = true;
            CPPUNIT_ASSERT(node->compare_all_flags(set));

            node->set_flag(node_flags->f_flag, false);
            CPPUNIT_ASSERT(!node->get_flag(node_flags->f_flag));
        }

        // now test all the other flags
        for(int j(-5); j <= static_cast<int>(as2js::Node::flag_t::NODE_FLAG_max) + 5; ++j)
        {
            if(j < 0
            || j >= static_cast<int>(as2js::Node::flag_t::NODE_FLAG_max)
            || !valid_flags[j])
            {
                CPPUNIT_ASSERT_THROW(node->get_flag(static_cast<as2js::Node::flag_t>(j)), as2js::exception_internal_error);
                CPPUNIT_ASSERT_THROW(node->set_flag(static_cast<as2js::Node::flag_t>(j), true), as2js::exception_internal_error);
                CPPUNIT_ASSERT_THROW(node->set_flag(static_cast<as2js::Node::flag_t>(j), false), as2js::exception_internal_error);
            }
        }

        // test completely invalid attribute indices
        for(int j(-5); j < 0; ++j)
        {
            CPPUNIT_ASSERT_THROW(node->get_attribute(static_cast<as2js::Node::attribute_t>(j)), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->set_attribute(static_cast<as2js::Node::attribute_t>(j), true), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->set_attribute(static_cast<as2js::Node::attribute_t>(j), false), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->attribute_to_string(static_cast<as2js::Node::attribute_t>(j)), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(as2js::Node::attribute_to_string(static_cast<as2js::Node::attribute_t>(j)), as2js::exception_internal_error);
        }
        for(int j(static_cast<int>(as2js::Node::attribute_t::NODE_ATTR_max));
                j <= static_cast<int>(as2js::Node::attribute_t::NODE_ATTR_max) + 5;
                ++j)
        {
            CPPUNIT_ASSERT_THROW(node->get_attribute(static_cast<as2js::Node::attribute_t>(j)), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->set_attribute(static_cast<as2js::Node::attribute_t>(j), true), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->set_attribute(static_cast<as2js::Node::attribute_t>(j), false), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(node->attribute_to_string(static_cast<as2js::Node::attribute_t>(j)), as2js::exception_internal_error);
            CPPUNIT_ASSERT_THROW(as2js::Node::attribute_to_string(static_cast<as2js::Node::attribute_t>(j)), as2js::exception_internal_error);
        }

        // attributes can be assigned to all types except NODE_PROGRAM
        // which only accepts NODE_DEFINED
        for(int j(0); j < static_cast<int>(as2js::Node::attribute_t::NODE_ATTR_max); ++j)
        {
            bool valid(true);
            switch(node_type)
            {
            case as2js::Node::node_t::NODE_PROGRAM:
                valid = j == static_cast<int>(as2js::Node::attribute_t::NODE_ATTR_DEFINED);
                break;

            case as2js::Node::node_t::NODE_ADD:
            case as2js::Node::node_t::NODE_ARRAY:
            case as2js::Node::node_t::NODE_ARRAY_LITERAL:
            case as2js::Node::node_t::NODE_AS:
            case as2js::Node::node_t::NODE_ASSIGNMENT:
            case as2js::Node::node_t::NODE_ASSIGNMENT_ADD:
            case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
            case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
            case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
            case as2js::Node::node_t::NODE_ASSIGNMENT_DIVIDE:
            case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
            case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
            case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
            case as2js::Node::node_t::NODE_ASSIGNMENT_MAXIMUM:
            case as2js::Node::node_t::NODE_ASSIGNMENT_MINIMUM:
            case as2js::Node::node_t::NODE_ASSIGNMENT_MODULO:
            case as2js::Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
            case as2js::Node::node_t::NODE_ASSIGNMENT_POWER:
            case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
            case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
            case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
            case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
            case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
            case as2js::Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
            case as2js::Node::node_t::NODE_BITWISE_AND:
            case as2js::Node::node_t::NODE_BITWISE_NOT:
            case as2js::Node::node_t::NODE_BITWISE_OR:
            case as2js::Node::node_t::NODE_BITWISE_XOR:
            case as2js::Node::node_t::NODE_CALL:
            case as2js::Node::node_t::NODE_CONDITIONAL:
            case as2js::Node::node_t::NODE_DECREMENT:
            case as2js::Node::node_t::NODE_DELETE:
            case as2js::Node::node_t::NODE_DIVIDE:
            case as2js::Node::node_t::NODE_EQUAL:
            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_FLOAT64:
            case as2js::Node::node_t::NODE_FUNCTION:
            case as2js::Node::node_t::NODE_GREATER:
            case as2js::Node::node_t::NODE_GREATER_EQUAL:
            case as2js::Node::node_t::NODE_IDENTIFIER:
            case as2js::Node::node_t::NODE_IN:
            case as2js::Node::node_t::NODE_INCREMENT:
            case as2js::Node::node_t::NODE_INSTANCEOF:
            case as2js::Node::node_t::NODE_INT64:
            case as2js::Node::node_t::NODE_IS:
            case as2js::Node::node_t::NODE_LESS:
            case as2js::Node::node_t::NODE_LESS_EQUAL:
            case as2js::Node::node_t::NODE_LIST:
            case as2js::Node::node_t::NODE_LOGICAL_AND:
            case as2js::Node::node_t::NODE_LOGICAL_NOT:
            case as2js::Node::node_t::NODE_LOGICAL_OR:
            case as2js::Node::node_t::NODE_LOGICAL_XOR:
            case as2js::Node::node_t::NODE_MATCH:
            case as2js::Node::node_t::NODE_MAXIMUM:
            case as2js::Node::node_t::NODE_MEMBER:
            case as2js::Node::node_t::NODE_MINIMUM:
            case as2js::Node::node_t::NODE_MODULO:
            case as2js::Node::node_t::NODE_MULTIPLY:
            case as2js::Node::node_t::NODE_NAME:
            case as2js::Node::node_t::NODE_NEW:
            case as2js::Node::node_t::NODE_NOT_EQUAL:
            case as2js::Node::node_t::NODE_NULL:
            case as2js::Node::node_t::NODE_OBJECT_LITERAL:
            case as2js::Node::node_t::NODE_POST_DECREMENT:
            case as2js::Node::node_t::NODE_POST_INCREMENT:
            case as2js::Node::node_t::NODE_POWER:
            case as2js::Node::node_t::NODE_PRIVATE:
            case as2js::Node::node_t::NODE_PUBLIC:
            case as2js::Node::node_t::NODE_RANGE:
            case as2js::Node::node_t::NODE_ROTATE_LEFT:
            case as2js::Node::node_t::NODE_ROTATE_RIGHT:
            case as2js::Node::node_t::NODE_SCOPE:
            case as2js::Node::node_t::NODE_SHIFT_LEFT:
            case as2js::Node::node_t::NODE_SHIFT_RIGHT:
            case as2js::Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
            case as2js::Node::node_t::NODE_STRICTLY_EQUAL:
            case as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL:
            case as2js::Node::node_t::NODE_STRING:
            case as2js::Node::node_t::NODE_SUBTRACT:
            case as2js::Node::node_t::NODE_SUPER:
            case as2js::Node::node_t::NODE_THIS:
            case as2js::Node::node_t::NODE_TRUE:
            case as2js::Node::node_t::NODE_TYPEOF:
            case as2js::Node::node_t::NODE_UNDEFINED:
            case as2js::Node::node_t::NODE_VIDENTIFIER:
            case as2js::Node::node_t::NODE_VOID:
                break;

            default:
                // any other type and you get an exception
                valid = j != static_cast<int>(as2js::Node::attribute_t::NODE_ATTR_TYPE);
                break;

            }

            //if(node_type == as2js::Node::node_t::NODE_PROGRAM
            //&& j != static_cast<int>(as2js::Node::attribute_t::NODE_ATTR_DEFINED))
            if(!valid)
            {
                CPPUNIT_ASSERT_THROW(node->get_attribute(static_cast<as2js::Node::attribute_t>(j)), as2js::exception_internal_error);
                CPPUNIT_ASSERT_THROW(node->set_attribute(static_cast<as2js::Node::attribute_t>(j), true), as2js::exception_internal_error);
                CPPUNIT_ASSERT_THROW(node->set_attribute(static_cast<as2js::Node::attribute_t>(j), false), as2js::exception_internal_error);
            }
            else
            {
                // before we set it, always false
                CPPUNIT_ASSERT(!node->get_attribute(static_cast<as2js::Node::attribute_t>(j)));
                node->set_attribute(static_cast<as2js::Node::attribute_t>(j), true);
                CPPUNIT_ASSERT(node->get_attribute(static_cast<as2js::Node::attribute_t>(j)));
                // since we reset them all we won't have a problem with conflicts in this loop
                node->set_attribute(static_cast<as2js::Node::attribute_t>(j), false);
                CPPUNIT_ASSERT(!node->get_attribute(static_cast<as2js::Node::attribute_t>(j)));
            }
            char const *attr_name1(node->attribute_to_string(static_cast<as2js::Node::attribute_t>(j)));
            CPPUNIT_ASSERT(attr_name1 != nullptr);
            char const *attr_name2(as2js::Node::attribute_to_string(static_cast<as2js::Node::attribute_t>(j)));
            CPPUNIT_ASSERT(attr_name2 != nullptr);
            CPPUNIT_ASSERT(strcmp(attr_name1, attr_name2) == 0);

            switch(static_cast<as2js::Node::attribute_t>(j))
            {
            case as2js::Node::attribute_t::NODE_ATTR_PUBLIC:       CPPUNIT_ASSERT(strcmp(attr_name1, "PUBLIC")         == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_PRIVATE:      CPPUNIT_ASSERT(strcmp(attr_name1, "PRIVATE")        == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_PROTECTED:    CPPUNIT_ASSERT(strcmp(attr_name1, "PROTECTED")      == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_INTERNAL:     CPPUNIT_ASSERT(strcmp(attr_name1, "INTERNAL")       == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_TRANSIENT:    CPPUNIT_ASSERT(strcmp(attr_name1, "TRANSIENT")      == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_VOLATILE:     CPPUNIT_ASSERT(strcmp(attr_name1, "VOLATILE")       == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_STATIC:       CPPUNIT_ASSERT(strcmp(attr_name1, "STATIC")         == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_ABSTRACT:     CPPUNIT_ASSERT(strcmp(attr_name1, "ABSTRACT")       == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_VIRTUAL:      CPPUNIT_ASSERT(strcmp(attr_name1, "VIRTUAL")        == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_ARRAY:        CPPUNIT_ASSERT(strcmp(attr_name1, "ARRAY")          == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_INLINE:       CPPUNIT_ASSERT(strcmp(attr_name1, "INLINE")         == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_REQUIRE_ELSE: CPPUNIT_ASSERT(strcmp(attr_name1, "REQUIRE_ELSE")   == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_ENSURE_THEN:  CPPUNIT_ASSERT(strcmp(attr_name1, "ENSURE_THEN")    == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_NATIVE:       CPPUNIT_ASSERT(strcmp(attr_name1, "NATIVE")         == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_DEPRECATED:   CPPUNIT_ASSERT(strcmp(attr_name1, "DEPRECATED")     == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_UNSAFE:       CPPUNIT_ASSERT(strcmp(attr_name1, "UNSAFE")         == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_CONSTRUCTOR:  CPPUNIT_ASSERT(strcmp(attr_name1, "CONSTRUCTOR")    == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_FINAL:        CPPUNIT_ASSERT(strcmp(attr_name1, "FINAL")          == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_ENUMERABLE:   CPPUNIT_ASSERT(strcmp(attr_name1, "ENUMERABLE")     == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_TRUE:         CPPUNIT_ASSERT(strcmp(attr_name1, "TRUE")           == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_FALSE:        CPPUNIT_ASSERT(strcmp(attr_name1, "FALSE")          == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_UNUSED:       CPPUNIT_ASSERT(strcmp(attr_name1, "UNUSED")         == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_DYNAMIC:      CPPUNIT_ASSERT(strcmp(attr_name1, "DYNAMIC")        == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_FOREACH:      CPPUNIT_ASSERT(strcmp(attr_name1, "FOREACH")        == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_NOBREAK:      CPPUNIT_ASSERT(strcmp(attr_name1, "NOBREAK")        == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_AUTOBREAK:    CPPUNIT_ASSERT(strcmp(attr_name1, "AUTOBREAK")      == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_TYPE:         CPPUNIT_ASSERT(strcmp(attr_name1, "TYPE")           == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_DEFINED:      CPPUNIT_ASSERT(strcmp(attr_name1, "DEFINED")        == 0); break;
            case as2js::Node::attribute_t::NODE_ATTR_max:          CPPUNIT_ASSERT(!"attribute max should not be checked in this test"); break;
            }
        }
    }

    // make sure that special numbers are correctly caught
    for(size_t i(0); i < static_cast<size_t>(as2js::Node::node_t::NODE_max); ++i)
    {
        if(!valid_types[i])
        {
            as2js::Node::node_t node_type(static_cast<as2js::Node::node_t>(i));
//std::cerr << "assert node type " << i << "\n";
            CPPUNIT_ASSERT_THROW(new as2js::Node(node_type), as2js::exception_incompatible_node_type);
        }
    }

    // test with completely random numbers too (outside of the
    // standard range of node types.)
    for(size_t i(0); i < 100; ++i)
    {
        int32_t j((rand() << 16) ^ rand());
        if(j < -1 || j >= static_cast<ssize_t>(as2js::Node::node_t::NODE_max))
        {
            as2js::Node::node_t node_type(static_cast<as2js::Node::node_t>(j));
            CPPUNIT_ASSERT_THROW(new as2js::Node(node_type), as2js::exception_incompatible_node_type);
        }
    }
}


void As2JsNodeUnitTests::test_compare()
{
    as2js::Node::pointer_t node1_true(new as2js::Node(as2js::Node::node_t::NODE_TRUE));
    as2js::Node::pointer_t node2_false(new as2js::Node(as2js::Node::node_t::NODE_FALSE));
    as2js::Node::pointer_t node3_true(new as2js::Node(as2js::Node::node_t::NODE_TRUE));
    as2js::Node::pointer_t node4_false(new as2js::Node(as2js::Node::node_t::NODE_FALSE));

    as2js::Node::pointer_t node5_33(new as2js::Node(as2js::Node::node_t::NODE_INT64));
    as2js::Int64 i33;
    i33.set(33);
    node5_33->set_int64(i33);

    as2js::Node::pointer_t node6_101(new as2js::Node(as2js::Node::node_t::NODE_INT64));
    as2js::Int64 i101;
    i101.set(101);
    node6_101->set_int64(i101);

    as2js::Node::pointer_t node7_33(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
    as2js::Float64 f33;
    f33.set(3.3);
    node7_33->set_float64(f33);

    as2js::Node::pointer_t node7_nearly33(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
    as2js::Float64 fnearly33;
    fnearly33.set(3.300001);
    node7_nearly33->set_float64(fnearly33);

    as2js::Node::pointer_t node8_101(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
    as2js::Float64 f101;
    f101.set(1.01);
    node8_101->set_float64(f101);

    as2js::Node::pointer_t node9_null(new as2js::Node(as2js::Node::node_t::NODE_NULL));
    as2js::Node::pointer_t node10_null(new as2js::Node(as2js::Node::node_t::NODE_NULL));

    as2js::Node::pointer_t node11_undefined(new as2js::Node(as2js::Node::node_t::NODE_UNDEFINED));
    as2js::Node::pointer_t node12_undefined(new as2js::Node(as2js::Node::node_t::NODE_UNDEFINED));

    as2js::Node::pointer_t node13_empty_string(new as2js::Node(as2js::Node::node_t::NODE_STRING));
    as2js::Node::pointer_t node14_blah(new as2js::Node(as2js::Node::node_t::NODE_STRING));
    node14_blah->set_string("blah");
    as2js::Node::pointer_t node15_foo(new as2js::Node(as2js::Node::node_t::NODE_STRING));
    node15_foo->set_string("foo");
    as2js::Node::pointer_t node16_07(new as2js::Node(as2js::Node::node_t::NODE_STRING));
    node16_07->set_string("0.7");
    as2js::Node::pointer_t node17_nearly33(new as2js::Node(as2js::Node::node_t::NODE_STRING));
    node17_nearly33->set_string("3.300001");

    // BOOLEAN
    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node1_true, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node3_true, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node1_true, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node3_true, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node1_true, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node3_true, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node1_true, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node3_true, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node1_true, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node3_true, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node1_true, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node3_true, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node2_false, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node4_false, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node2_false, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node4_false, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node2_false, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node4_false, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node2_false, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node4_false, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node2_false, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node4_false, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node2_false, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node4_false, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node2_false, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node2_false, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node4_false, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node4_false, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);

    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node2_false, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node2_false, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node4_false, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node4_false, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);

    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node2_false, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node2_false, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node4_false, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node3_true, node4_false, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);

    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node1_true, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node3_true, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node1_true, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node3_true, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);

    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node1_true, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node3_true, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node1_true, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node3_true, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);

    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node1_true, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node3_true, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node1_true, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node4_false, node3_true, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);

    // FLOAT
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node7_nearly33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_nearly33, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node17_nearly33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node17_nearly33, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node8_101, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node8_101, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node7_nearly33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_nearly33, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node17_nearly33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node17_nearly33, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node8_101, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node8_101, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node7_nearly33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_nearly33, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node17_nearly33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node17_nearly33, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node7_33, node8_101, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node8_101, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

    // INTEGER
    CPPUNIT_ASSERT(as2js::Node::compare(node5_33, node5_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node5_33, node6_101, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node6_101, node5_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node6_101, node6_101, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node5_33, node5_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node5_33, node6_101, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node6_101, node5_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node6_101, node6_101, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node5_33, node5_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node5_33, node6_101, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node6_101, node5_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node6_101, node6_101, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

    // NULL
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node9_null, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node10_null, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node9_null, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node10_null, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node9_null, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node10_null, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node9_null, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node10_null, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node9_null, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node10_null, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node9_null, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node10_null, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

    // UNDEFINED
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node11_undefined, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node12_undefined, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node11_undefined, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node12_undefined, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node11_undefined, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node12_undefined, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node11_undefined, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node12_undefined, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node11_undefined, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node12_undefined, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node11_undefined, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node12_undefined, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

    // STRING
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node14_blah, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node15_foo, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node14_blah, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node15_foo, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node15_foo, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node15_foo, node14_blah, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node15_foo, node15_foo, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node14_blah, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node15_foo, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node14_blah, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node15_foo, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node15_foo, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node15_foo, node14_blah, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node15_foo, node15_foo, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node14_blah, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node15_foo, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node14_blah, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node15_foo, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node15_foo, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node15_foo, node14_blah, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node15_foo, node15_foo, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

    // NULL vs UNDEFINED
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node11_undefined, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node12_undefined, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node11_undefined, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node12_undefined, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node9_null, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node9_null, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node10_null, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node10_null, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);

    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node11_undefined, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node12_undefined, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node11_undefined, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node12_undefined, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node9_null, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node9_null, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node10_null, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node10_null, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_EQUAL);

    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node11_undefined, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node12_undefined, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node11_undefined, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node10_null, node12_undefined, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node9_null, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node9_null, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node10_null, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);
    CPPUNIT_ASSERT(as2js::Node::compare(node12_undefined, node10_null, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_EQUAL);

    // <any> against FLOAT64
    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node5_33, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node6_101, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node16_07, node7_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);

    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node5_33, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node6_101, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node16_07, node7_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);

    CPPUNIT_ASSERT(as2js::Node::compare(node1_true, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node2_false, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node5_33, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node6_101, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node9_null, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node11_undefined, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node13_empty_string, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node14_blah, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node16_07, node7_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);

    // FLOAT64 against <any>
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node1_true, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node2_false, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node5_33, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node6_101, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node9_null, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node11_undefined, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node14_blah, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node16_07, as2js::Node::compare_mode_t::COMPARE_STRICT) == as2js::compare_t::COMPARE_UNORDERED);

    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node1_true, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node2_false, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node5_33, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node6_101, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node9_null, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node11_undefined, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node14_blah, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node16_07, as2js::Node::compare_mode_t::COMPARE_LOOSE) == as2js::compare_t::COMPARE_GREATER);

    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node2_false, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node5_33, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node6_101, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_LESS);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node9_null, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node11_undefined, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node13_empty_string, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node14_blah, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_UNORDERED);
    CPPUNIT_ASSERT(as2js::Node::compare(node8_101, node16_07, as2js::Node::compare_mode_t::COMPARE_SMART) == as2js::compare_t::COMPARE_GREATER);
}


void As2JsNodeUnitTests::test_conversions()
{
    // first test simple conversions
    for(size_t i(0); i < g_node_types_size; ++i)
    {
        // original type
        as2js::Node::node_t original_type(g_node_types[i].f_type);

        // all nodes can be converted to UNKNOWN
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_unknown(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            node->to_unknown();
            CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_UNKNOWN);
        }

        // CALL can be convert to AS
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_as(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            if(original_type == as2js::Node::node_t::NODE_CALL)
            {
                // in this case it works
                CPPUNIT_ASSERT(node->to_as());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_AS);
            }
            else
            {
                // in this case it fails
                CPPUNIT_ASSERT(!node->to_as());
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
        }

        // test what would happen if we were to call to_boolean()
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                node->to_boolean_type_only();
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            as2js::Node::node_t new_type(node->to_boolean_type_only());
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(new_type == as2js::Node::node_t::NODE_TRUE);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
            case as2js::Node::node_t::NODE_UNDEFINED:
            case as2js::Node::node_t::NODE_INT64: // by default integers are set to zero
            case as2js::Node::node_t::NODE_FLOAT64: // by default floating points are set to zero
            case as2js::Node::node_t::NODE_STRING: // by default strings are empty
                CPPUNIT_ASSERT(new_type == as2js::Node::node_t::NODE_FALSE);
                break;

            default:
                CPPUNIT_ASSERT(new_type == as2js::Node::node_t::NODE_UNDEFINED);
                break;

            }
        }

        // a few nodes can be converted to a boolean value
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_boolean(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_boolean());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_TRUE);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
            case as2js::Node::node_t::NODE_UNDEFINED:
            case as2js::Node::node_t::NODE_INT64: // by default integers are set to zero
            case as2js::Node::node_t::NODE_FLOAT64: // by default floating points are set to zero
            case as2js::Node::node_t::NODE_STRING: // by default strings are empty
                CPPUNIT_ASSERT(node->to_boolean());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FALSE);
                break;

            default:
                CPPUNIT_ASSERT(!node->to_boolean());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a couple types of nodes can be converted to a CALL
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_call(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_ASSIGNMENT:
            case as2js::Node::node_t::NODE_MEMBER:
                CPPUNIT_ASSERT(node->to_call());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_CALL);
                break;

            default:
                CPPUNIT_ASSERT(!node->to_call());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a few types of nodes can be converted to an INT64
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_int64(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_INT64: // no change
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                break;

            case as2js::Node::node_t::NODE_FLOAT64:
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
            case as2js::Node::node_t::NODE_UNDEFINED:
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == 0);
                break;

            case as2js::Node::node_t::NODE_STRING: // empty string to start with...
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == 0);

                // if not empty...
                {
                    as2js::Node::pointer_t node_str(new as2js::Node(original_type));
                    node_str->set_string("34");
                    CPPUNIT_ASSERT(node_str->to_int64());
                    CPPUNIT_ASSERT(node_str->get_type() == as2js::Node::node_t::NODE_INT64);
                    CPPUNIT_ASSERT(node_str->get_int64().get() == 34);
                }
                {
                    as2js::Node::pointer_t node_str(new as2js::Node(original_type));
                    node_str->set_string("+84");
                    CPPUNIT_ASSERT(node_str->to_int64());
                    CPPUNIT_ASSERT(node_str->get_type() == as2js::Node::node_t::NODE_INT64);
                    CPPUNIT_ASSERT(node_str->get_int64().get() == 84);
                }
                {
                    as2js::Node::pointer_t node_str(new as2js::Node(original_type));
                    node_str->set_string("-37");
                    CPPUNIT_ASSERT(node_str->to_int64());
                    CPPUNIT_ASSERT(node_str->get_type() == as2js::Node::node_t::NODE_INT64);
                    CPPUNIT_ASSERT(node_str->get_int64().get() == -37);
                }
                {
                    as2js::Node::pointer_t node_str(new as2js::Node(original_type));
                    node_str->set_string("3.4");
                    CPPUNIT_ASSERT(node_str->to_int64());
                    CPPUNIT_ASSERT(node_str->get_type() == as2js::Node::node_t::NODE_INT64);
                    CPPUNIT_ASSERT(node_str->get_int64().get() == 3);
                }
                {
                    as2js::Node::pointer_t node_str(new as2js::Node(original_type));
                    node_str->set_string("34e+5");
                    CPPUNIT_ASSERT(node_str->to_int64());
                    CPPUNIT_ASSERT(node_str->get_type() == as2js::Node::node_t::NODE_INT64);
                    CPPUNIT_ASSERT(node_str->get_int64().get() == 3400000);
                }
                {
                    as2js::Node::pointer_t node_str(new as2js::Node(original_type));
                    node_str->set_string("some NaN");
                    CPPUNIT_ASSERT(node_str->to_int64());
                    CPPUNIT_ASSERT(node_str->get_type() == as2js::Node::node_t::NODE_INT64);
                    CPPUNIT_ASSERT(node_str->get_int64().get() == 0);
                }
                break;

            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == 1);
                break;

            default:
                CPPUNIT_ASSERT(!node->to_int64());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a few types of nodes can be converted to a FLOAT64
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_float64(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_INT64: // no change
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
                break;

            case as2js::Node::node_t::NODE_FLOAT64:
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
            case as2js::Node::node_t::NODE_STRING:
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == 0.0);
#pragma GCC diagnostic pop
                break;

            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == 1.0);
#pragma GCC diagnostic pop
                break;

            case as2js::Node::node_t::NODE_UNDEFINED:
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
                CPPUNIT_ASSERT(node->get_float64().is_NaN());
                break;

            default:
                CPPUNIT_ASSERT(!node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // IDENTIFIER can be converted to LABEL
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_label(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            if(original_type == as2js::Node::node_t::NODE_IDENTIFIER)
            {
                // in this case it works
                node->to_label();
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_LABEL);
            }
            else
            {
                // this one fails with a soft error (returns false)
                CPPUNIT_ASSERT(!node->to_label());
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
        }

        // a few types of nodes can be converted to a Number
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_number(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_INT64: // no change!
            case as2js::Node::node_t::NODE_FLOAT64: // no change!
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_NULL:
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == 0);
                break;

            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == 1);
                break;

            case as2js::Node::node_t::NODE_STRING: // empty strings represent 0 here
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == 0.0);
#pragma GCC diagnostic pop
                break;

            case as2js::Node::node_t::NODE_UNDEFINED:
//std::cerr << " . type = " << static_cast<int>(original_type) << " / " << node->get_type_name() << "\n";
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
                CPPUNIT_ASSERT(node->get_float64().is_NaN());
                break;

            default:
                CPPUNIT_ASSERT(!node->to_number());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a few types of nodes can be converted to a STRING
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_string(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_STRING:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                CPPUNIT_ASSERT(node->get_string() == "");
                break;

            case as2js::Node::node_t::NODE_FLOAT64:
            case as2js::Node::node_t::NODE_INT64:
                // by default numbers are zero; we have other tests
                // to verify the conversion
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "0");
                break;

            case as2js::Node::node_t::NODE_FALSE:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "false");
                break;

            case as2js::Node::node_t::NODE_TRUE:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "true");
                break;

            case as2js::Node::node_t::NODE_NULL:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "null");
                break;

            case as2js::Node::node_t::NODE_UNDEFINED:
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == "undefined");
                break;

            case as2js::Node::node_t::NODE_IDENTIFIER: // the string remains the same
            //case as2js::Node::node_t::NODE_VIDENTIFIER: // should the VIDENTIFIER be supported too?
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                break;

            default:
                CPPUNIT_ASSERT(!node->to_string());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // a few types of nodes can be converted to an IDENTIFIER
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_identifier(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            switch(original_type)
            {
            case as2js::Node::node_t::NODE_IDENTIFIER:
                CPPUNIT_ASSERT(node->to_identifier());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                CPPUNIT_ASSERT(node->get_string() == "");
                break;

            case as2js::Node::node_t::NODE_PRIVATE:
                CPPUNIT_ASSERT(node->to_identifier());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
                CPPUNIT_ASSERT(node->get_string() == "private");
                break;

            case as2js::Node::node_t::NODE_PROTECTED:
                CPPUNIT_ASSERT(node->to_identifier());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
                CPPUNIT_ASSERT(node->get_string() == "protected");
                break;

            case as2js::Node::node_t::NODE_PUBLIC:
                CPPUNIT_ASSERT(node->to_identifier());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_IDENTIFIER);
                CPPUNIT_ASSERT(node->get_string() == "public");
                break;

            default:
                CPPUNIT_ASSERT(!node->to_identifier());
                CPPUNIT_ASSERT(node->get_type() == original_type);
                break;

            }
        }

        // IDENTIFIER can be converted to VIDENTIFIER
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_videntifier(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            if(original_type == as2js::Node::node_t::NODE_IDENTIFIER)
            {
                // in this case it works
                node->to_videntifier();
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_VIDENTIFIER);
            }
            else
            {
                // this one fails dramatically
                CPPUNIT_ASSERT_THROW(node->to_videntifier(), as2js::exception_internal_error);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
        }

        // VARIABLE can be converted to VAR_ATTRIBUTES
        {
            as2js::Node::pointer_t node(new as2js::Node(original_type));
            {
                as2js::NodeLock lock(node);
                CPPUNIT_ASSERT_THROW(node->to_var_attributes(), as2js::exception_locked_node);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
            if(original_type == as2js::Node::node_t::NODE_VARIABLE)
            {
                // in this case it works
                node->to_var_attributes();
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_VAR_ATTRIBUTES);
            }
            else
            {
                // in this case it fails
                CPPUNIT_ASSERT_THROW(node->to_var_attributes(), as2js::exception_internal_error);
                CPPUNIT_ASSERT(node->get_type() == original_type);
            }
        }
    }

    bool got_dot(false);
    for(int i(0); i < 100; ++i)
    {
        // Integer to other types
        {
            as2js::Int64 j((static_cast<int64_t>(rand()) << 48)
                         ^ (static_cast<int64_t>(rand()) << 32)
                         ^ (static_cast<int64_t>(rand()) << 16)
                         ^ (static_cast<int64_t>(rand()) <<  0));

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                as2js::Float64 invalid;
                CPPUNIT_ASSERT_THROW(node->set_float64(invalid), as2js::exception_internal_error);
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->to_int64());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_int64().get() == j.get());
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                CPPUNIT_ASSERT(node->to_number());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
                CPPUNIT_ASSERT(node->get_int64().get() == j.get());
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                as2js::Node::node_t bool_type(node->to_boolean_type_only());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(bool_type == (j.get() ? as2js::Node::node_t::NODE_TRUE : as2js::Node::node_t::NODE_FALSE));
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                CPPUNIT_ASSERT(node->to_boolean());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_type() == (j.get() ? as2js::Node::node_t::NODE_TRUE : as2js::Node::node_t::NODE_FALSE));
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                CPPUNIT_ASSERT(node->to_float64());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
                as2js::Float64 flt(j.get());
                CPPUNIT_ASSERT(node->get_float64().nearly_equal(flt, 0.0001));
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_INT64));
                node->set_int64(j);
                CPPUNIT_ASSERT(node->to_string());
                // probably always true here; we had false in the loop prior
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                CPPUNIT_ASSERT(node->get_string() == as2js::String(std::to_string(j.get())));
            }
        }

        // Floating point to other values
        bool first(true);
        do
        {
            // generate a random 64 bit number
            double s1(rand() & 1 ? -1 : 1);
            double n1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                        ^ (static_cast<int64_t>(rand()) << 32)
                                        ^ (static_cast<int64_t>(rand()) << 16)
                                        ^ (static_cast<int64_t>(rand()) <<  0)));
            double d1(static_cast<double>((static_cast<int64_t>(rand()) << 48)
                                        ^ (static_cast<int64_t>(rand()) << 32)
                                        ^ (static_cast<int64_t>(rand()) << 16)
                                        ^ (static_cast<int64_t>(rand()) <<  0)));
            if(!first && n1 >= d1)
            {
                // the dot is easier to reach with very small numbers
                // so create a small number immediately
                std::swap(n1, d1);
                d1 *= 1e+4;
            }
            double r(n1 / d1 * s1);
            as2js::Float64 j(r);

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_int64());
                CPPUNIT_ASSERT(node->get_int64().get() == static_cast<as2js::Int64::int64_type>(j.get()));
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_number());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == j.get());
#pragma GCC diagnostic pop
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                as2js::Node::node_t bool_type(node->to_boolean_type_only());
                // probably always true here; we had false in the loop prior
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(bool_type == (j.get() ? as2js::Node::node_t::NODE_TRUE : as2js::Node::node_t::NODE_FALSE));
#pragma GCC diagnostic pop
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_boolean());
                // probably always true here; we had false in the loop prior
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_type() == (j.get() ? as2js::Node::node_t::NODE_TRUE : as2js::Node::node_t::NODE_FALSE));
#pragma GCC diagnostic pop

                // also test the set_boolean() with valid values
                node->set_boolean(true);
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_TRUE);
                node->set_boolean(false);
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FALSE);
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_float64());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_FLOAT64);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
                CPPUNIT_ASSERT(node->get_float64().get() == j.get());
#pragma GCC diagnostic pop
            }

            {
                as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
                node->set_float64(j);
                CPPUNIT_ASSERT(node->to_string());
                CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
                as2js::String str(as2js::String(std::to_string(j.get())));
                if(str.find('.') != as2js::String::npos)
                {
                    // remove all least significant zeroes if any
                    while(str.back() == '0')
                    {
                        str.pop_back();
                    }
                    // make sure the number does not end with a period
                    if(str.back() == '.')
                    {
                        str.pop_back();
                        got_dot = true;
                    }
                }
                CPPUNIT_ASSERT(node->get_string() == str);
            }
            first = false;
        }
        while(!got_dot);
    }

    // verify special floating point values
    { // NaN -> String
        as2js::Float64 j;
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
        j.set_NaN();
        node->set_float64(j);
        CPPUNIT_ASSERT(node->to_string());
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(node->get_string() == "NaN");
    }
    { // NaN -> Int64
        as2js::Float64 j;
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
        j.set_NaN();
        node->set_float64(j);
        CPPUNIT_ASSERT(node->to_int64());
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(node->get_int64().get() == 0);
    }
    { // +Infinity
        as2js::Float64 j;
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
        j.set_infinity();
        node->set_float64(j);
        CPPUNIT_ASSERT(node->to_string());
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(node->get_string() == "Infinity");
    }
    { // +Infinity
        as2js::Float64 j;
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
        j.set_infinity();
        node->set_float64(j);
        CPPUNIT_ASSERT(node->to_int64());
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(node->get_int64().get() == 0);
    }
    { // -Infinity
        as2js::Float64 j;
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
        j.set_infinity();
        j.set(-j.get());
        node->set_float64(j);
        CPPUNIT_ASSERT(node->to_string());
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_STRING);
        CPPUNIT_ASSERT(node->get_string() == "-Infinity");
    }
    { // +Infinity
        as2js::Float64 j;
        as2js::Node::pointer_t node(new as2js::Node(as2js::Node::node_t::NODE_FLOAT64));
        j.set_infinity();
        j.set(-j.get());
        node->set_float64(j);
        CPPUNIT_ASSERT(node->to_int64());
        CPPUNIT_ASSERT(node->get_type() == as2js::Node::node_t::NODE_INT64);
        CPPUNIT_ASSERT(node->get_int64().get() == 0);
    }
}


void As2JsNodeUnitTests::test_tree()
{
    class TrackedNode : public as2js::Node
    {
    public:
        TrackedNode(as2js::Node::node_t type, int& count)
            : Node(type)
            , f_count(count)
        {
            ++f_count;
        }

        virtual ~TrackedNode()
        {
            --f_count;
        }

    private:
        int&        f_count;
    };

    // counter to know how many nodes we currently have allocated
    int counter(0);

    // a few basic tests
    {
        as2js::Node::pointer_t parent(new TrackedNode(as2js::Node::node_t::NODE_DIRECTIVE_LIST, counter));

        CPPUNIT_ASSERT_THROW(parent->get_child(-1), std::out_of_range);
        CPPUNIT_ASSERT_THROW(parent->get_child(0), std::out_of_range);
        CPPUNIT_ASSERT_THROW(parent->get_child(1), std::out_of_range);

        // now we properly test whether the append_child(),
        // insert_child(), and set_child() functions are used
        // with a null pointer (which is considered illegal)
        as2js::Node::pointer_t null_pointer;
        CPPUNIT_ASSERT_THROW(parent->append_child(null_pointer), as2js::exception_invalid_data);
        CPPUNIT_ASSERT_THROW(parent->insert_child(123, null_pointer), as2js::exception_invalid_data);
        CPPUNIT_ASSERT_THROW(parent->set_child(9, null_pointer), as2js::exception_invalid_data);

        for(int i(0); i < 20; ++i)
        {
            as2js::Node::pointer_t child(new TrackedNode(as2js::Node::node_t::NODE_DIRECTIVE_LIST, counter));
            parent->append_child(child);

            CPPUNIT_ASSERT_THROW(parent->get_child(-1), std::out_of_range);
            for(int j(0); j <= i; ++j)
            {
                parent->get_child(j);
            }
            CPPUNIT_ASSERT_THROW(parent->get_child(i + 1), std::out_of_range);
            CPPUNIT_ASSERT_THROW(parent->get_child(i + 2), std::out_of_range);
        }
    }

    // first test: try with all types as the parent and children
    for(size_t i(0); i < g_node_types_size; ++i)
    {
        // type
        as2js::Node::node_t parent_type(g_node_types[i].f_type);

        as2js::Node::pointer_t parent(new TrackedNode(parent_type, counter));
        CPPUNIT_ASSERT(parent->get_children_size() == 0);

        size_t valid_children(0);
        for(size_t j(0); j < g_node_types_size; ++j)
        {
            as2js::Node::node_t child_type(g_node_types[j].f_type);

            as2js::Node::pointer_t child(new TrackedNode(child_type, counter));

//std::cerr << "parent " << parent->get_type_name() << " child " << child->get_type_name() << "\n";
            // some nodes cannot be parents...
            switch(parent_type)
            {
            case as2js::Node::node_t::NODE_ABSTRACT:
            case as2js::Node::node_t::NODE_AUTO:
            case as2js::Node::node_t::NODE_BOOLEAN:
            case as2js::Node::node_t::NODE_BREAK:
            case as2js::Node::node_t::NODE_BYTE:
            case as2js::Node::node_t::NODE_CLOSE_CURVLY_BRACKET:
            case as2js::Node::node_t::NODE_CLOSE_PARENTHESIS:
            case as2js::Node::node_t::NODE_CLOSE_SQUARE_BRACKET:
            case as2js::Node::node_t::NODE_CHAR:
            case as2js::Node::node_t::NODE_COLON:
            case as2js::Node::node_t::NODE_COMMA:
            case as2js::Node::node_t::NODE_CONST:
            case as2js::Node::node_t::NODE_CONTINUE:
            case as2js::Node::node_t::NODE_DEFAULT:
            case as2js::Node::node_t::NODE_DOUBLE:
            case as2js::Node::node_t::NODE_ELSE:
            case as2js::Node::node_t::NODE_THEN:
            case as2js::Node::node_t::NODE_EMPTY:
            case as2js::Node::node_t::NODE_EOF:
            case as2js::Node::node_t::NODE_IDENTIFIER:
            case as2js::Node::node_t::NODE_INLINE:
            case as2js::Node::node_t::NODE_INT64:
            case as2js::Node::node_t::NODE_FALSE:
            case as2js::Node::node_t::NODE_FINAL:
            case as2js::Node::node_t::NODE_FLOAT:
            case as2js::Node::node_t::NODE_FLOAT64:
            case as2js::Node::node_t::NODE_GOTO:
            case as2js::Node::node_t::NODE_LONG:
            case as2js::Node::node_t::NODE_NATIVE:
            case as2js::Node::node_t::NODE_NULL:
            case as2js::Node::node_t::NODE_OPEN_CURVLY_BRACKET:
            case as2js::Node::node_t::NODE_OPEN_PARENTHESIS:
            case as2js::Node::node_t::NODE_OPEN_SQUARE_BRACKET:
            case as2js::Node::node_t::NODE_PRIVATE:
            case as2js::Node::node_t::NODE_PROTECTED:
            case as2js::Node::node_t::NODE_PUBLIC:
            case as2js::Node::node_t::NODE_REGULAR_EXPRESSION:
            case as2js::Node::node_t::NODE_REST:
            case as2js::Node::node_t::NODE_SEMICOLON:
            case as2js::Node::node_t::NODE_SHORT:
            case as2js::Node::node_t::NODE_STRING:
            case as2js::Node::node_t::NODE_STATIC:
            case as2js::Node::node_t::NODE_THIS:
            case as2js::Node::node_t::NODE_TRANSIENT:
            case as2js::Node::node_t::NODE_TRUE:
            case as2js::Node::node_t::NODE_UNDEFINED:
            case as2js::Node::node_t::NODE_VIDENTIFIER:
            case as2js::Node::node_t::NODE_VOID:
            case as2js::Node::node_t::NODE_VOLATILE:
                // append child to parent must fail
                if(rand() & 1)
                {
                    CPPUNIT_ASSERT_THROW(parent->append_child(child), as2js::exception_incompatible_node_type);
                }
                else
                {
                    CPPUNIT_ASSERT_THROW(child->set_parent(parent), as2js::exception_incompatible_node_type);
                }
                break;

            default:
                switch(child_type)
                {
                case as2js::Node::node_t::NODE_CLOSE_CURVLY_BRACKET:
                case as2js::Node::node_t::NODE_CLOSE_PARENTHESIS:
                case as2js::Node::node_t::NODE_CLOSE_SQUARE_BRACKET:
                case as2js::Node::node_t::NODE_COLON:
                case as2js::Node::node_t::NODE_COMMA:
                case as2js::Node::node_t::NODE_ELSE:
                case as2js::Node::node_t::NODE_THEN:
                case as2js::Node::node_t::NODE_EOF:
                case as2js::Node::node_t::NODE_OPEN_CURVLY_BRACKET:
                case as2js::Node::node_t::NODE_OPEN_PARENTHESIS:
                case as2js::Node::node_t::NODE_OPEN_SQUARE_BRACKET:
                case as2js::Node::node_t::NODE_ROOT:
                case as2js::Node::node_t::NODE_SEMICOLON:
                    // append child to parent must fail
                    if(rand() & 1)
                    {
                        CPPUNIT_ASSERT_THROW(parent->append_child(child), as2js::exception_incompatible_node_type);
                    }
                    else
                    {
                        CPPUNIT_ASSERT_THROW(child->set_parent(parent), as2js::exception_incompatible_node_type);
                    }
                    break;

                default:
                    // append child to parent
                    if(rand() & 1)
                    {
                        parent->append_child(child);
                    }
                    else
                    {
                        child->set_parent(parent);
                    }

                    CPPUNIT_ASSERT(parent->get_children_size() == valid_children + 1);
                    CPPUNIT_ASSERT(child->get_parent() == parent);
                    CPPUNIT_ASSERT(child->get_offset() == valid_children);
                    CPPUNIT_ASSERT(parent->get_child(valid_children) == child);
                    CPPUNIT_ASSERT(parent->find_first_child(child_type) == child);
                    CPPUNIT_ASSERT(!parent->find_next_child(child, child_type));

                    ++valid_children;
                    break;

                }
                break;

            }
        }
    }

    // we deleted as many nodes as we created
    CPPUNIT_ASSERT(counter == 0);

    // Test a more realistic tree with a few nodes and make sure we
    // can apply certain function and that the tree exactly results
    // in what we expect
    {
        // 1. Create the following in directive a:
        //
        //  // first block (directive_a)
        //  {
        //      a = Math.e ** 1.424;
        //  }
        //  // second block (directive_b)
        //  {
        //  }
        //
        // 2. Move it to directive b
        //
        //  // first block (directive_a)
        //  {
        //  }
        //  // second block (directive_b)
        //  {
        //      a = Math.e ** 1.424;
        //  }
        //
        // 3. Verify that it worked
        //

        // create all the nodes as the lexer would do
        as2js::Node::pointer_t root(new TrackedNode(as2js::Node::node_t::NODE_ROOT, counter));
        as2js::Position pos;
        pos.reset_counters(22);
        pos.set_filename("test.js");
        root->set_position(pos);
        as2js::Node::pointer_t directive_list_a(new TrackedNode(as2js::Node::node_t::NODE_DIRECTIVE_LIST, counter));
        as2js::Node::pointer_t directive_list_b(new TrackedNode(as2js::Node::node_t::NODE_DIRECTIVE_LIST, counter));
        as2js::Node::pointer_t assignment(new TrackedNode(as2js::Node::node_t::NODE_ASSIGNMENT, counter));
        as2js::Node::pointer_t identifier_a(new TrackedNode(as2js::Node::node_t::NODE_IDENTIFIER, counter));
        identifier_a->set_string("a");
        as2js::Node::pointer_t power(new TrackedNode(as2js::Node::node_t::NODE_POWER, counter));
        as2js::Node::pointer_t member(new TrackedNode(as2js::Node::node_t::NODE_MEMBER, counter));
        as2js::Node::pointer_t identifier_math(new TrackedNode(as2js::Node::node_t::NODE_IDENTIFIER, counter));
        identifier_math->set_string("Math");
        as2js::Node::pointer_t identifier_e(new TrackedNode(as2js::Node::node_t::NODE_IDENTIFIER, counter));
        identifier_e->set_string("e");
        as2js::Node::pointer_t literal(new TrackedNode(as2js::Node::node_t::NODE_FLOAT64, counter));
        as2js::Float64 f;
        f.set(1.424);
        literal->set_float64(f);

        // build the tree as the parser would do
        root->append_child(directive_list_a);
        root->append_child(directive_list_b);
        directive_list_a->append_child(assignment);
        assignment->append_child(identifier_a);
        assignment->insert_child(-1, power);
        power->append_child(member);
        CPPUNIT_ASSERT_THROW(power->insert_child(10, literal), as2js::exception_index_out_of_range);
        power->insert_child(1, literal);
        member->append_child(identifier_e);
        member->insert_child(0, identifier_math);

        // verify we can unlock mid-way
        as2js::NodeLock temp_lock(member);
        CPPUNIT_ASSERT(member->is_locked());
        temp_lock.unlock();
        CPPUNIT_ASSERT(!member->is_locked());

        // as a complement to testing the lock, make sure that emptiness
        // (i.e. null pointer) is properly handled all the way
        {
            as2js::Node::pointer_t empty;
            as2js::NodeLock empty_lock(empty);
        }
        {
            as2js::Node::pointer_t empty;
            as2js::NodeLock empty_lock(empty);
            empty_lock.unlock();
        }

        // apply some tests
        CPPUNIT_ASSERT(root->get_children_size() == 2);
        CPPUNIT_ASSERT(directive_list_a->get_children_size() == 1);
        CPPUNIT_ASSERT(directive_list_a->get_child(0) == assignment);
        CPPUNIT_ASSERT(directive_list_b->get_children_size() == 0);
        CPPUNIT_ASSERT(assignment->get_children_size() == 2);
        CPPUNIT_ASSERT(assignment->get_child(0) == identifier_a);
        CPPUNIT_ASSERT(assignment->get_child(1) == power);
        CPPUNIT_ASSERT(identifier_a->get_children_size() == 0);
        CPPUNIT_ASSERT(power->get_children_size() == 2);
        CPPUNIT_ASSERT(power->get_child(0) == member);
        CPPUNIT_ASSERT(power->get_child(1) == literal);
        CPPUNIT_ASSERT(member->get_children_size() == 2);
        CPPUNIT_ASSERT(member->get_child(0) == identifier_math);
        CPPUNIT_ASSERT(member->get_child(1) == identifier_e);
        CPPUNIT_ASSERT(identifier_math->get_children_size() == 0);
        CPPUNIT_ASSERT(identifier_e->get_children_size() == 0);
        CPPUNIT_ASSERT(literal->get_children_size() == 0);

        CPPUNIT_ASSERT(root->has_side_effects());
        CPPUNIT_ASSERT(directive_list_a->has_side_effects());
        CPPUNIT_ASSERT(!directive_list_b->has_side_effects());
        CPPUNIT_ASSERT(!power->has_side_effects());

        // now move the assignment from a to b
        assignment->set_parent(directive_list_b);

        CPPUNIT_ASSERT(root->get_children_size() == 2);
        CPPUNIT_ASSERT(directive_list_a->get_children_size() == 0);
        CPPUNIT_ASSERT(directive_list_b->get_children_size() == 1);
        CPPUNIT_ASSERT(directive_list_b->get_child(0) == assignment);
        CPPUNIT_ASSERT(assignment->get_children_size() == 2);
        CPPUNIT_ASSERT(assignment->get_child(0) == identifier_a);
        CPPUNIT_ASSERT(assignment->get_child(1) == power);
        CPPUNIT_ASSERT(identifier_a->get_children_size() == 0);
        CPPUNIT_ASSERT(power->get_children_size() == 2);
        CPPUNIT_ASSERT(power->get_child(0) == member);
        CPPUNIT_ASSERT(power->get_child(1) == literal);
        CPPUNIT_ASSERT(member->get_children_size() == 2);
        CPPUNIT_ASSERT(member->get_child(0) == identifier_math);
        CPPUNIT_ASSERT(member->get_child(1) == identifier_e);
        CPPUNIT_ASSERT(identifier_math->get_children_size() == 0);
        CPPUNIT_ASSERT(identifier_e->get_children_size() == 0);
        CPPUNIT_ASSERT(literal->get_children_size() == 0);

        power->delete_child(0);
        CPPUNIT_ASSERT(power->get_children_size() == 1);
        CPPUNIT_ASSERT(power->get_child(0) == literal);

        power->insert_child(0, member);
        CPPUNIT_ASSERT(power->get_children_size() == 2);
        CPPUNIT_ASSERT(power->get_child(0) == member);
        CPPUNIT_ASSERT(power->get_child(1) == literal);

        CPPUNIT_ASSERT(root->has_side_effects());
        CPPUNIT_ASSERT(!directive_list_a->has_side_effects());
        CPPUNIT_ASSERT(directive_list_b->has_side_effects());
        CPPUNIT_ASSERT(!power->has_side_effects());

        // create a new literal
        as2js::Node::pointer_t literal_seven(new TrackedNode(as2js::Node::node_t::NODE_FLOAT64, counter));
        as2js::Float64 f7;
        f7.set(-7.33312);
        literal_seven->set_float64(f7);
        directive_list_a->append_child(literal_seven);
        CPPUNIT_ASSERT(directive_list_a->get_children_size() == 1);
        CPPUNIT_ASSERT(directive_list_a->get_child(0) == literal_seven);

        // now replace the old literal with the new one (i.e. a full move actually)
        power->set_child(1, literal_seven);
        CPPUNIT_ASSERT(power->get_children_size() == 2);
        CPPUNIT_ASSERT(power->get_child(0) == member);
        CPPUNIT_ASSERT(power->get_child(1) == literal_seven);

        // replace with itself should work just fine
        power->set_child(0, member);
        CPPUNIT_ASSERT(power->get_children_size() == 2);
        CPPUNIT_ASSERT(power->get_child(0) == member);
        CPPUNIT_ASSERT(power->get_child(1) == literal_seven);

        // verify that a replace fails if the node pointer is null
        as2js::Node::pointer_t null_pointer;
        CPPUNIT_ASSERT_THROW(literal_seven->replace_with(null_pointer), as2js::exception_invalid_data);

        // replace with the old literal
        literal_seven->replace_with(literal);
        CPPUNIT_ASSERT(power->get_children_size() == 2);
        CPPUNIT_ASSERT(power->get_child(0) == member);
        CPPUNIT_ASSERT(power->get_child(1) == literal);

        // verify that a node without a parent generates an exception
        CPPUNIT_ASSERT_THROW(root->replace_with(literal_seven), as2js::exception_no_parent);

        // verify that we cannot get an offset on a node without a parent
        CPPUNIT_ASSERT_THROW(root->get_offset(), as2js::exception_no_parent);

        // check out our tree textually
        //std::cout << std::endl << *root << std::endl;

        // finally mark a node as unknown and call clean_tree()
        CPPUNIT_ASSERT(!member->is_locked());
        {
            as2js::NodeLock lock(member);
            CPPUNIT_ASSERT(member->is_locked());
            CPPUNIT_ASSERT_THROW(member->to_unknown(), as2js::exception_locked_node);
            CPPUNIT_ASSERT(member->get_type() == as2js::Node::node_t::NODE_MEMBER);
        }
        CPPUNIT_ASSERT(!member->is_locked());
        // try too many unlock!
        CPPUNIT_ASSERT_THROW(member->unlock(), as2js::exception_internal_error);
        member->to_unknown();
        CPPUNIT_ASSERT(member->get_type() == as2js::Node::node_t::NODE_UNKNOWN);
        {
            as2js::NodeLock lock(member);
            CPPUNIT_ASSERT_THROW(root->clean_tree(), as2js::exception_locked_node);
            CPPUNIT_ASSERT(member->get_type() == as2js::Node::node_t::NODE_UNKNOWN);
            CPPUNIT_ASSERT(member->get_parent());
        }
        root->clean_tree();

        // manual lock, no unlock before deletion...
        {
            as2js::Node *bad_lock(new as2js::Node(as2js::Node::node_t::NODE_UNKNOWN));
            bad_lock->lock();
            bool success(false);
            try
            {
                // Somehow it looks like the message is not being checked
                //test_callback c;
                //c.f_expected_message_level = as2js::message_level_t::MESSAGE_LEVEL_FATAL;
                //c.f_expected_error_code = as2js::err_code_t::AS_ERR_NOT_ALLOWED;
                //c.f_expected_pos.set_filename("unknown-file");
                //c.f_expected_pos.set_function("unknown-func");
                //c.f_expected_message = "a node got deleted while still locked.";

                delete bad_lock;
            }
            catch(as2js::exception_exit const&)
            {
                // NOTE: because of the exception we get a leak here
                //       (i.e. exception in a destructor!!!)
                success = true;
            }
            catch(...)
            {
                CPPUNIT_ASSERT(!"delete bad_lock; generated the wrong exception");
            }
            if(!success)
            {
                CPPUNIT_ASSERT(!"delete bad_lock; did not generate an exception");
            }
        }

        // check that the tree looks as expected
        CPPUNIT_ASSERT(root->get_children_size() == 2);
        CPPUNIT_ASSERT(directive_list_a->get_children_size() == 0);
        CPPUNIT_ASSERT(directive_list_b->get_children_size() == 1);
        CPPUNIT_ASSERT(directive_list_b->get_child(0) == assignment);
        CPPUNIT_ASSERT(assignment->get_children_size() == 2);
        CPPUNIT_ASSERT(assignment->get_child(0) == identifier_a);
        CPPUNIT_ASSERT(assignment->get_child(1) == power);
        CPPUNIT_ASSERT(identifier_a->get_children_size() == 0);
        CPPUNIT_ASSERT(power->get_children_size() == 1);
        // Although member is not in the tree anymore, its children
        // are still there as expected (because we hold a smart pointers
        // to all of that)
        //CPPUNIT_ASSERT(power->get_child(0) == member);
        CPPUNIT_ASSERT(power->get_child(0) == literal);
        CPPUNIT_ASSERT(!member->get_parent());
        CPPUNIT_ASSERT(member->get_children_size() == 2);
        CPPUNIT_ASSERT(member->get_child(0) == identifier_math);
        CPPUNIT_ASSERT(member->get_child(1) == identifier_e);
        CPPUNIT_ASSERT(identifier_math->get_children_size() == 0);
        CPPUNIT_ASSERT(identifier_math->get_parent() == member);
        CPPUNIT_ASSERT(identifier_e->get_children_size() == 0);
        CPPUNIT_ASSERT(identifier_e->get_parent() == member);
        CPPUNIT_ASSERT(literal->get_children_size() == 0);
    }

    // we again deleted as many nodes as we created
    CPPUNIT_ASSERT(counter == 0);
}


void As2JsNodeUnitTests::test_param()
{
    {
        as2js::Node::pointer_t match(new as2js::Node(as2js::Node::node_t::NODE_PARAM_MATCH));

        CPPUNIT_ASSERT(match->get_param_size() == 0);

        // zero is not acceptable
        CPPUNIT_ASSERT_THROW(match->set_param_size(0), as2js::exception_internal_error);

        match->set_param_size(5);
        CPPUNIT_ASSERT(match->get_param_size() == 5);

        // cannot change the size once set
        CPPUNIT_ASSERT_THROW(match->set_param_size(10), as2js::exception_internal_error);

        CPPUNIT_ASSERT(match->get_param_size() == 5);

        // first set the depth, try with an out of range index too
        for(int i(-5); i < 0; ++i)
        {
            CPPUNIT_ASSERT_THROW(match->set_param_depth(i, rand()), std::out_of_range);
        }
        ssize_t depths[5];
        for(int i(0); i < 5; ++i)
        {
            depths[i] = rand();
            match->set_param_depth(i, depths[i]);
        }
        for(int i(5); i <= 10; ++i)
        {
            CPPUNIT_ASSERT_THROW(match->set_param_depth(i, rand()), std::out_of_range);
        }

        // now test that what we saved can be read back, also with some out of range
        for(int i(-5); i < 0; ++i)
        {
            CPPUNIT_ASSERT_THROW(match->get_param_depth(i), std::out_of_range);
        }
        for(int i(0); i < 5; ++i)
        {
            CPPUNIT_ASSERT(match->get_param_depth(i) == depths[i]);
        }
        for(int i(5); i < 10; ++i)
        {
            CPPUNIT_ASSERT_THROW(match->get_param_depth(i), std::out_of_range);
        }

        // second set the index, try with an out of range index too
        for(int i(-5); i < 0; ++i)
        {
            CPPUNIT_ASSERT_THROW(match->set_param_index(i, rand() % 5), std::out_of_range);
            CPPUNIT_ASSERT_THROW(match->set_param_index(i, rand()), std::out_of_range);
        }
        size_t index[5];
        for(int i(0); i < 5; ++i)
        {
            index[i] = rand() % 5;
            match->set_param_index(i, index[i]);

            // if 'j' is invalid, then just throw
            // and do not change the valid value
            for(int k(0); k < 10; ++k)
            {
                int j(0);
                do
                {
                    j = rand();
                }
                while(j >= 0 && j <= 5);
                CPPUNIT_ASSERT_THROW(match->set_param_index(i, j), std::out_of_range);
            }
        }
        for(int i(5); i <= 10; ++i)
        {
            CPPUNIT_ASSERT_THROW(match->set_param_index(i, rand() % 5), std::out_of_range);
            CPPUNIT_ASSERT_THROW(match->set_param_index(i, rand()), std::out_of_range);
        }

        // now test that what we saved can be read back, also with some out of range
        for(int i(-5); i < 0; ++i)
        {
            CPPUNIT_ASSERT_THROW(match->get_param_index(i), std::out_of_range);
        }
        for(int i(0); i < 5; ++i)
        {
            CPPUNIT_ASSERT(match->get_param_index(i) == index[i]);
        }
        for(int i(5); i < 10; ++i)
        {
            CPPUNIT_ASSERT_THROW(match->get_param_index(i), std::out_of_range);
        }
    }
}


void As2JsNodeUnitTests::test_position()
{
    as2js::Position pos;
    pos.set_filename("file.js");
    int total_line(1);
    for(int page(1); page < 10; ++page)
    {
        int paragraphs(rand() % 10 + 10);
        int page_line(1);
        int paragraph(1);
        for(int line(1); line < 100; ++line)
        {
            CPPUNIT_ASSERT(pos.get_page() == page);
            CPPUNIT_ASSERT(pos.get_page_line() == page_line);
            CPPUNIT_ASSERT(pos.get_paragraph() == paragraph);
            CPPUNIT_ASSERT(pos.get_line() == total_line);

            std::stringstream pos_str;
            pos_str << pos;
            std::stringstream test_str;
            test_str << "file.js:" << total_line << ":";
            CPPUNIT_ASSERT(pos_str.str() == test_str.str());

            // create any valid type of node
            size_t const idx(rand() % g_node_types_size);
            as2js::Node::pointer_t node(new as2js::Node(g_node_types[idx].f_type));

            // set our current position in there
            node->set_position(pos);

            // verify that the node position is equal to ours
            as2js::Position const& node_pos(node->get_position());
            CPPUNIT_ASSERT(node_pos.get_page() == page);
            CPPUNIT_ASSERT(node_pos.get_page_line() == page_line);
            CPPUNIT_ASSERT(node_pos.get_paragraph() == paragraph);
            CPPUNIT_ASSERT(node_pos.get_line() == total_line);

            std::stringstream node_pos_str;
            node_pos_str << node_pos;
            std::stringstream node_test_str;
            node_test_str << "file.js:" << total_line << ":";
            CPPUNIT_ASSERT(node_pos_str.str() == node_test_str.str());

            // create a replacement now
            size_t const idx_replacement(rand() % g_node_types_size);
            as2js::Node::pointer_t replacement(node->create_replacement(g_node_types[idx_replacement].f_type));

            // verify that the replacement position is equal to ours
            // (and thus the node's)
            as2js::Position const& replacement_pos(node->get_position());
            CPPUNIT_ASSERT(replacement_pos.get_page() == page);
            CPPUNIT_ASSERT(replacement_pos.get_page_line() == page_line);
            CPPUNIT_ASSERT(replacement_pos.get_paragraph() == paragraph);
            CPPUNIT_ASSERT(replacement_pos.get_line() == total_line);

            std::stringstream replacement_pos_str;
            replacement_pos_str << replacement_pos;
            std::stringstream replacement_test_str;
            replacement_test_str << "file.js:" << total_line << ":";
            CPPUNIT_ASSERT(replacement_pos_str.str() == replacement_test_str.str());

            // verify that the node position has not changed
            as2js::Position const& node_pos2(node->get_position());
            CPPUNIT_ASSERT(node_pos2.get_page() == page);
            CPPUNIT_ASSERT(node_pos2.get_page_line() == page_line);
            CPPUNIT_ASSERT(node_pos2.get_paragraph() == paragraph);
            CPPUNIT_ASSERT(node_pos2.get_line() == total_line);

            std::stringstream node_pos2_str;
            node_pos2_str << node_pos2;
            std::stringstream node_test2_str;
            node_test2_str << "file.js:" << total_line << ":";
            CPPUNIT_ASSERT(node_pos2_str.str() == node_test2_str.str());

            // go to the next line, paragraph, etc.
            if(line % paragraphs == 0)
            {
                pos.new_paragraph();
                ++paragraph;
            }
            pos.new_line();
            ++total_line;
            ++page_line;
        }
        pos.new_page();
    }

}


void As2JsNodeUnitTests::test_links()
{
    for(int i(0); i < 10; ++i)
    {
        // create any valid type of node
        size_t const idx_node(rand() % g_node_types_size);
        as2js::Node::pointer_t node(new as2js::Node(g_node_types[idx_node].f_type));

        size_t const idx_bad_link(rand() % g_node_types_size);
        as2js::Node::pointer_t bad_link(new as2js::Node(g_node_types[idx_bad_link].f_type));

        // check various links

        { // instance
            as2js::Node::pointer_t link(new as2js::Node(as2js::Node::node_t::NODE_CLASS));
            node->set_instance(link);
            CPPUNIT_ASSERT(node->get_instance() == link);

            as2js::Node::pointer_t other_link(new as2js::Node(as2js::Node::node_t::NODE_CLASS));
            node->set_instance(other_link);
            CPPUNIT_ASSERT(node->get_instance() == other_link);
        }
        CPPUNIT_ASSERT(!node->get_instance()); // weak pointer, reset to null

        { // type
            as2js::Node::pointer_t link(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
            node->set_type_node(link);
            CPPUNIT_ASSERT(node->get_type_node() == link);

            as2js::Node::pointer_t other_link(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
            node->set_type_node(other_link);
            CPPUNIT_ASSERT(node->get_type_node() == other_link);
        }
        CPPUNIT_ASSERT(!node->get_type_node()); // weak pointer, reset to null

        { // attributes
            as2js::Node::pointer_t link(new as2js::Node(as2js::Node::node_t::NODE_ATTRIBUTES));
            node->set_attribute_node(link);
            CPPUNIT_ASSERT(node->get_attribute_node() == link);

            as2js::Node::pointer_t other_link(new as2js::Node(as2js::Node::node_t::NODE_ATTRIBUTES));
            node->set_attribute_node(other_link);
            CPPUNIT_ASSERT(node->get_attribute_node() == other_link);
        }
        CPPUNIT_ASSERT(node->get_attribute_node()); // NOT a weak pointer for attributes

        { // goto exit
            as2js::Node::pointer_t link(new as2js::Node(as2js::Node::node_t::NODE_LABEL));
            node->set_goto_exit(link);
            CPPUNIT_ASSERT(node->get_goto_exit() == link);

            as2js::Node::pointer_t other_link(new as2js::Node(as2js::Node::node_t::NODE_LABEL));
            node->set_goto_exit(other_link);
            CPPUNIT_ASSERT(node->get_goto_exit() == other_link);
        }
        CPPUNIT_ASSERT(!node->get_goto_exit()); // weak pointer, reset to null

        { // goto enter
            as2js::Node::pointer_t link(new as2js::Node(as2js::Node::node_t::NODE_LABEL));
            node->set_goto_enter(link);
            CPPUNIT_ASSERT(node->get_goto_enter() == link);

            as2js::Node::pointer_t other_link(new as2js::Node(as2js::Node::node_t::NODE_LABEL));
            node->set_goto_enter(other_link);
            CPPUNIT_ASSERT(node->get_goto_enter() == other_link);
        }
        CPPUNIT_ASSERT(!node->get_goto_enter()); // weak pointer, reset to null
    }
}


void As2JsNodeUnitTests::test_variables()
{
    for(int i(0); i < 10; ++i)
    {
        // create any valid type of node
        size_t const idx_node(rand() % g_node_types_size);
        as2js::Node::pointer_t node(new as2js::Node(g_node_types[idx_node].f_type));

        // create a node that is not a NODE_VARIABLE
        size_t idx_bad_link;
        do
        {
            idx_bad_link = rand() % g_node_types_size;
        }
        while(g_node_types[idx_bad_link].f_type == as2js::Node::node_t::NODE_VARIABLE);
        as2js::Node::pointer_t not_variable(new as2js::Node(g_node_types[idx_bad_link].f_type));
        CPPUNIT_ASSERT_THROW(node->add_variable(not_variable), as2js::exception_incompatible_node_type);

        // add 10 valid variables
        as2js::Node::pointer_t variables[10];
        for(size_t j(0); j < 10; ++j)
        {
            CPPUNIT_ASSERT(node->get_variable_size() == j);

            variables[j].reset(new as2js::Node(as2js::Node::node_t::NODE_VARIABLE));
            node->add_variable(variables[j]);
        }
        CPPUNIT_ASSERT(node->get_variable_size() == 10);

        // try with offsets that are too small
        for(int j(-10); j < 0; ++j)
        {
            CPPUNIT_ASSERT_THROW(node->get_variable(j), std::out_of_range);
        }

        // then verify that the variables are indeed valid
        for(int j(0); j < 10; ++j)
        {
            CPPUNIT_ASSERT(node->get_variable(j) == variables[j]);
        }

        // try with offsets that are too large
        for(int j(10); j <= 20; ++j)
        {
            CPPUNIT_ASSERT_THROW(node->get_variable(j), std::out_of_range);
        }
    }
}


void As2JsNodeUnitTests::test_labels()
{
    for(int i(0); i < 10; ++i)
    {
        // create a NODE_FUNCTION
        as2js::Node::pointer_t function(new as2js::Node(as2js::Node::node_t::NODE_FUNCTION));

        // create a node that is not a NODE_LABEL
        size_t idx_bad_label;
        do
        {
            idx_bad_label = rand() % g_node_types_size;
        }
        while(g_node_types[idx_bad_label].f_type == as2js::Node::node_t::NODE_LABEL);
        as2js::Node::pointer_t not_label(new as2js::Node(g_node_types[idx_bad_label].f_type));
        CPPUNIT_ASSERT_THROW(function->add_label(not_label), as2js::exception_incompatible_node_type);

        for(int j(0); j < 10; ++j)
        {
            // create a node that is not a NODE_LABEL
            as2js::Node::pointer_t label(new as2js::Node(as2js::Node::node_t::NODE_LABEL));

            // create a node that is not a NODE_FUNCTION
            size_t idx_bad_function;
            do
            {
                idx_bad_function = rand() % g_node_types_size;
            }
            while(g_node_types[idx_bad_function].f_type == as2js::Node::node_t::NODE_FUNCTION);
            as2js::Node::pointer_t not_function(new as2js::Node(g_node_types[idx_bad_function].f_type));
            CPPUNIT_ASSERT_THROW(not_function->add_label(label), as2js::exception_incompatible_node_type);

            // labels need to have a name
            CPPUNIT_ASSERT_THROW(function->add_label(label), as2js::exception_incompatible_node_data);

            // save the label with a name
            std::string label_name("label" + std::to_string(j));
            label->set_string(label_name);
            function->add_label(label);

            // trying to add two labels (or the same) with the same name err
            CPPUNIT_ASSERT_THROW(function->add_label(label), as2js::exception_already_defined);

            // verify that we can find that label
            CPPUNIT_ASSERT(function->find_label(label_name) == label);
        }
    }
}


void As2JsNodeUnitTests::test_attributes()
{
    for(int i(0); i < 10; ++i)
    {
        // create a node that is not a NODE_PROGRAM
        // (i.e. a node that accepts all attributes)
        size_t idx_node;
        do
        {
            idx_node = rand() % g_node_types_size;
        }
        while(g_node_types[idx_node].f_type == as2js::Node::node_t::NODE_PROGRAM);
        as2js::Node::pointer_t node(new as2js::Node(g_node_types[idx_node].f_type));

        // need to test all combinatorial cases...
        for(size_t j(0); j < g_groups_of_attributes_size; ++j)
        {
            // go through the list of attributes that generate conflicts
            for(as2js::Node::attribute_t const *attr_list(g_groups_of_attributes[j].f_attributes);
                                         *attr_list != as2js::Node::attribute_t::NODE_ATTR_max;
                                         ++attr_list)
            {
                if(*attr_list == as2js::Node::attribute_t::NODE_ATTR_TYPE)
                {
                    switch(node->get_type())
                    {
                    case as2js::Node::node_t::NODE_ADD:
                    case as2js::Node::node_t::NODE_ARRAY:
                    case as2js::Node::node_t::NODE_ARRAY_LITERAL:
                    case as2js::Node::node_t::NODE_AS:
                    case as2js::Node::node_t::NODE_ASSIGNMENT:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_ADD:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_DIVIDE:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_MAXIMUM:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_MINIMUM:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_MODULO:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_POWER:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
                    case as2js::Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
                    case as2js::Node::node_t::NODE_BITWISE_AND:
                    case as2js::Node::node_t::NODE_BITWISE_NOT:
                    case as2js::Node::node_t::NODE_BITWISE_OR:
                    case as2js::Node::node_t::NODE_BITWISE_XOR:
                    case as2js::Node::node_t::NODE_CALL:
                    case as2js::Node::node_t::NODE_CONDITIONAL:
                    case as2js::Node::node_t::NODE_DECREMENT:
                    case as2js::Node::node_t::NODE_DELETE:
                    case as2js::Node::node_t::NODE_DIVIDE:
                    case as2js::Node::node_t::NODE_EQUAL:
                    case as2js::Node::node_t::NODE_FALSE:
                    case as2js::Node::node_t::NODE_FLOAT64:
                    case as2js::Node::node_t::NODE_FUNCTION:
                    case as2js::Node::node_t::NODE_GREATER:
                    case as2js::Node::node_t::NODE_GREATER_EQUAL:
                    case as2js::Node::node_t::NODE_IDENTIFIER:
                    case as2js::Node::node_t::NODE_IN:
                    case as2js::Node::node_t::NODE_INCREMENT:
                    case as2js::Node::node_t::NODE_INSTANCEOF:
                    case as2js::Node::node_t::NODE_INT64:
                    case as2js::Node::node_t::NODE_IS:
                    case as2js::Node::node_t::NODE_LESS:
                    case as2js::Node::node_t::NODE_LESS_EQUAL:
                    case as2js::Node::node_t::NODE_LIST:
                    case as2js::Node::node_t::NODE_LOGICAL_AND:
                    case as2js::Node::node_t::NODE_LOGICAL_NOT:
                    case as2js::Node::node_t::NODE_LOGICAL_OR:
                    case as2js::Node::node_t::NODE_LOGICAL_XOR:
                    case as2js::Node::node_t::NODE_MATCH:
                    case as2js::Node::node_t::NODE_MAXIMUM:
                    case as2js::Node::node_t::NODE_MEMBER:
                    case as2js::Node::node_t::NODE_MINIMUM:
                    case as2js::Node::node_t::NODE_MODULO:
                    case as2js::Node::node_t::NODE_MULTIPLY:
                    case as2js::Node::node_t::NODE_NAME:
                    case as2js::Node::node_t::NODE_NEW:
                    case as2js::Node::node_t::NODE_NOT_EQUAL:
                    case as2js::Node::node_t::NODE_NULL:
                    case as2js::Node::node_t::NODE_OBJECT_LITERAL:
                    case as2js::Node::node_t::NODE_POST_DECREMENT:
                    case as2js::Node::node_t::NODE_POST_INCREMENT:
                    case as2js::Node::node_t::NODE_POWER:
                    case as2js::Node::node_t::NODE_PRIVATE:
                    case as2js::Node::node_t::NODE_PUBLIC:
                    case as2js::Node::node_t::NODE_RANGE:
                    case as2js::Node::node_t::NODE_ROTATE_LEFT:
                    case as2js::Node::node_t::NODE_ROTATE_RIGHT:
                    case as2js::Node::node_t::NODE_SCOPE:
                    case as2js::Node::node_t::NODE_SHIFT_LEFT:
                    case as2js::Node::node_t::NODE_SHIFT_RIGHT:
                    case as2js::Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
                    case as2js::Node::node_t::NODE_STRICTLY_EQUAL:
                    case as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL:
                    case as2js::Node::node_t::NODE_STRING:
                    case as2js::Node::node_t::NODE_SUBTRACT:
                    case as2js::Node::node_t::NODE_SUPER:
                    case as2js::Node::node_t::NODE_THIS:
                    case as2js::Node::node_t::NODE_TRUE:
                    case as2js::Node::node_t::NODE_TYPEOF:
                    case as2js::Node::node_t::NODE_UNDEFINED:
                    case as2js::Node::node_t::NODE_VIDENTIFIER:
                    case as2js::Node::node_t::NODE_VOID:
                        break;;

                    default:
                        // with any other types we would get an error
                        continue;

                    }
                }

                as2js::Node::attribute_set_t set;
                CPPUNIT_ASSERT(node->compare_all_attributes(set));

                // set that one attribute first
                node->set_attribute(*attr_list, true);

                CPPUNIT_ASSERT(!node->compare_all_attributes(set));
                set[static_cast<int>(*attr_list)] = true;
                CPPUNIT_ASSERT(node->compare_all_attributes(set));

                as2js::String str(g_attribute_names[static_cast<int>(*attr_list)]);

                // test against all the other attributes
                for(int a(0); a < static_cast<int>(as2js::Node::attribute_t::NODE_ATTR_max); ++a)
                {
                    // no need to test with itself, we do that earlier
                    if(static_cast<as2js::Node::attribute_t>(a) == *attr_list)
                    {
                        continue;
                    }

                    if(static_cast<as2js::Node::attribute_t>(a) == as2js::Node::attribute_t::NODE_ATTR_TYPE)
                    {
                        switch(node->get_type())
                        {
                        case as2js::Node::node_t::NODE_ADD:
                        case as2js::Node::node_t::NODE_ARRAY:
                        case as2js::Node::node_t::NODE_ARRAY_LITERAL:
                        case as2js::Node::node_t::NODE_AS:
                        case as2js::Node::node_t::NODE_ASSIGNMENT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_ADD:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_AND:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_OR:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_BITWISE_XOR:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_DIVIDE:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_AND:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_OR:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_LOGICAL_XOR:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_MAXIMUM:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_MINIMUM:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_MODULO:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_MULTIPLY:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_POWER:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_LEFT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_ROTATE_RIGHT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_LEFT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED:
                        case as2js::Node::node_t::NODE_ASSIGNMENT_SUBTRACT:
                        case as2js::Node::node_t::NODE_BITWISE_AND:
                        case as2js::Node::node_t::NODE_BITWISE_NOT:
                        case as2js::Node::node_t::NODE_BITWISE_OR:
                        case as2js::Node::node_t::NODE_BITWISE_XOR:
                        case as2js::Node::node_t::NODE_CALL:
                        case as2js::Node::node_t::NODE_CONDITIONAL:
                        case as2js::Node::node_t::NODE_DECREMENT:
                        case as2js::Node::node_t::NODE_DELETE:
                        case as2js::Node::node_t::NODE_DIVIDE:
                        case as2js::Node::node_t::NODE_EQUAL:
                        case as2js::Node::node_t::NODE_FALSE:
                        case as2js::Node::node_t::NODE_FLOAT64:
                        case as2js::Node::node_t::NODE_FUNCTION:
                        case as2js::Node::node_t::NODE_GREATER:
                        case as2js::Node::node_t::NODE_GREATER_EQUAL:
                        case as2js::Node::node_t::NODE_IDENTIFIER:
                        case as2js::Node::node_t::NODE_IN:
                        case as2js::Node::node_t::NODE_INCREMENT:
                        case as2js::Node::node_t::NODE_INSTANCEOF:
                        case as2js::Node::node_t::NODE_INT64:
                        case as2js::Node::node_t::NODE_IS:
                        case as2js::Node::node_t::NODE_LESS:
                        case as2js::Node::node_t::NODE_LESS_EQUAL:
                        case as2js::Node::node_t::NODE_LIST:
                        case as2js::Node::node_t::NODE_LOGICAL_AND:
                        case as2js::Node::node_t::NODE_LOGICAL_NOT:
                        case as2js::Node::node_t::NODE_LOGICAL_OR:
                        case as2js::Node::node_t::NODE_LOGICAL_XOR:
                        case as2js::Node::node_t::NODE_MATCH:
                        case as2js::Node::node_t::NODE_MAXIMUM:
                        case as2js::Node::node_t::NODE_MEMBER:
                        case as2js::Node::node_t::NODE_MINIMUM:
                        case as2js::Node::node_t::NODE_MODULO:
                        case as2js::Node::node_t::NODE_MULTIPLY:
                        case as2js::Node::node_t::NODE_NAME:
                        case as2js::Node::node_t::NODE_NEW:
                        case as2js::Node::node_t::NODE_NOT_EQUAL:
                        case as2js::Node::node_t::NODE_NULL:
                        case as2js::Node::node_t::NODE_OBJECT_LITERAL:
                        case as2js::Node::node_t::NODE_POST_DECREMENT:
                        case as2js::Node::node_t::NODE_POST_INCREMENT:
                        case as2js::Node::node_t::NODE_POWER:
                        case as2js::Node::node_t::NODE_PRIVATE:
                        case as2js::Node::node_t::NODE_PUBLIC:
                        case as2js::Node::node_t::NODE_RANGE:
                        case as2js::Node::node_t::NODE_ROTATE_LEFT:
                        case as2js::Node::node_t::NODE_ROTATE_RIGHT:
                        case as2js::Node::node_t::NODE_SCOPE:
                        case as2js::Node::node_t::NODE_SHIFT_LEFT:
                        case as2js::Node::node_t::NODE_SHIFT_RIGHT:
                        case as2js::Node::node_t::NODE_SHIFT_RIGHT_UNSIGNED:
                        case as2js::Node::node_t::NODE_STRICTLY_EQUAL:
                        case as2js::Node::node_t::NODE_STRICTLY_NOT_EQUAL:
                        case as2js::Node::node_t::NODE_STRING:
                        case as2js::Node::node_t::NODE_SUBTRACT:
                        case as2js::Node::node_t::NODE_SUPER:
                        case as2js::Node::node_t::NODE_THIS:
                        case as2js::Node::node_t::NODE_TRUE:
                        case as2js::Node::node_t::NODE_TYPEOF:
                        case as2js::Node::node_t::NODE_UNDEFINED:
                        case as2js::Node::node_t::NODE_VIDENTIFIER:
                        case as2js::Node::node_t::NODE_VOID:
                            break;;

                        default:
                            // with any other types we would get an error
                            continue;

                        }
                    }

                    // is attribute 'a' in conflict with attribute '*attr_list'?
                    if(in_conflict(j, *attr_list, static_cast<as2js::Node::attribute_t>(a)))
                    {
                        test_callback c;
                        c.f_expected_message_level = as2js::message_level_t::MESSAGE_LEVEL_ERROR;
                        c.f_expected_error_code = as2js::err_code_t::AS_ERR_INVALID_ATTRIBUTES;
                        c.f_expected_pos.set_filename("unknown-file");
                        c.f_expected_pos.set_function("unknown-func");
                        c.f_expected_message = "Attributes " + std::string(g_groups_of_attributes[j].f_names) + " are mutually exclusive. Only one of them can be used.";

//std::cerr << "next conflict: " << c.f_expected_message << "\n";
                        // if in conflict, trying to set the flag generates
                        // an error
                        CPPUNIT_ASSERT(!node->get_attribute(static_cast<as2js::Node::attribute_t>(a)));
                        node->set_attribute(static_cast<as2js::Node::attribute_t>(a), true);
                        // the set_attribute() did not change the attribute because it is
                        // in conflict with another attribute which is set at this time...
                        CPPUNIT_ASSERT(!node->get_attribute(static_cast<as2js::Node::attribute_t>(a)));
                    }
                    else
                    {
                        // before we set it, always false
//std::cerr << "next valid attr: " << static_cast<int>(*attr_list) << " against " << a << "\n";
                        CPPUNIT_ASSERT(!node->get_attribute(static_cast<as2js::Node::attribute_t>(a)));
                        node->set_attribute(static_cast<as2js::Node::attribute_t>(a), true);
                        CPPUNIT_ASSERT(node->get_attribute(static_cast<as2js::Node::attribute_t>(a)));
                        node->set_attribute(static_cast<as2js::Node::attribute_t>(a), false);
                        CPPUNIT_ASSERT(!node->get_attribute(static_cast<as2js::Node::attribute_t>(a)));
                    }
                }

                // we are done with that loop, restore the attribute to the default
                node->set_attribute(*attr_list, false);
            }
        }
    }
}


void As2JsNodeUnitTests::test_attribute_tree()
{
    // here we create a tree of nodes that we can then test with various
    // attributes using the set_attribute_tree() function
    //
    // the tree is very specific to make it easier to handle the test; there
    // is no need to test every single case (every attribute) since we do that
    // in other tests; this test is to make sure the tree is followed as
    // expected (all leaves are hit)
    //
    as2js::Node::pointer_t root(new as2js::Node(as2js::Node::node_t::NODE_ROOT));

    // block
    as2js::Node::pointer_t directive_list(new as2js::Node(as2js::Node::node_t::NODE_DIRECTIVE_LIST));
    root->append_child(directive_list);

    // { for( ...
    as2js::Node::pointer_t for_loop(new as2js::Node(as2js::Node::node_t::NODE_FOR));
    directive_list->append_child(for_loop);

    // { for( ... , ...
    as2js::Node::pointer_t init(new as2js::Node(as2js::Node::node_t::NODE_LIST));
    for_loop->append_child(init);

    as2js::Node::pointer_t var1(new as2js::Node(as2js::Node::node_t::NODE_VAR));
    init->append_child(var1);

    as2js::Node::pointer_t variable1(new as2js::Node(as2js::Node::node_t::NODE_VARIABLE));
    var1->append_child(variable1);

    // { for(i
    as2js::Node::pointer_t variable_name1(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    variable_name1->set_string("i");
    variable1->append_child(variable_name1);

    // { for(i := 
    as2js::Node::pointer_t value1(new as2js::Node(as2js::Node::node_t::NODE_SET));
    variable1->append_child(value1);

    // { for(i := ... + ...
    as2js::Node::pointer_t add1(new as2js::Node(as2js::Node::node_t::NODE_ADD));
    value1->append_child(add1);

    // { for(i := a + ...
    as2js::Node::pointer_t var_a1(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    var_a1->set_string("a");
    add1->append_child(var_a1);

    // { for(i := a + b
    as2js::Node::pointer_t var_b1(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    var_b1->set_string("b");
    add1->append_child(var_b1);

    // { for(i := a + b, 
    as2js::Node::pointer_t var2(new as2js::Node(as2js::Node::node_t::NODE_VAR));
    init->append_child(var2);

    as2js::Node::pointer_t variable2(new as2js::Node(as2js::Node::node_t::NODE_VARIABLE));
    var2->append_child(variable2);

    // { for(i := a + b, j
    as2js::Node::pointer_t variable_name2(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    variable_name2->set_string("j");
    variable2->append_child(variable_name2);

    // { for(i := a + b, j := 
    as2js::Node::pointer_t value2(new as2js::Node(as2js::Node::node_t::NODE_SET));
    variable2->append_child(value2);

    // { for(i := a + b, j := ... / ...
    as2js::Node::pointer_t divide2(new as2js::Node(as2js::Node::node_t::NODE_DIVIDE));
    value2->append_child(divide2);

    // { for(i := a + b, j := c / ...
    as2js::Node::pointer_t var_a2(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    var_a2->set_string("c");
    divide2->append_child(var_a2);

    // { for(i := a + b, j := c / d
    as2js::Node::pointer_t var_b2(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    var_b2->set_string("d");
    divide2->append_child(var_b2);

    // { for(i := a + b, j := c / d; ... < ...
    as2js::Node::pointer_t less(new as2js::Node(as2js::Node::node_t::NODE_LESS));
    for_loop->append_child(less);

    // { for(i := a + b, j := c / d; i < ...
    as2js::Node::pointer_t var_i2(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    var_i2->set_string("i");
    less->append_child(var_i2);

    // { for(i := a + b, j := c / d; i < 100;
    as2js::Node::pointer_t one_hunder(new as2js::Node(as2js::Node::node_t::NODE_INT64));
    one_hunder->set_int64(100);
    less->append_child(one_hunder);

    // { for(i := a + b, j := c / d; i < 100; ++...)
    as2js::Node::pointer_t increment(new as2js::Node(as2js::Node::node_t::NODE_INCREMENT));
    for_loop->append_child(increment);

    // { for(i := a + b, j := c / d; i < 100; ++i)
    as2js::Node::pointer_t var_i3(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    var_i3->set_string("i");
    increment->append_child(var_i3);

    // { for(i := a + b, j := c / d; i < 100; ++i) { ... } }
    as2js::Node::pointer_t block_list(new as2js::Node(as2js::Node::node_t::NODE_DIRECTIVE_LIST));
    for_loop->append_child(block_list);

    // { for(i := a + b, j := c / d; i < 100; ++i) { ...(...); } }
    as2js::Node::pointer_t func(new as2js::Node(as2js::Node::node_t::NODE_CALL));
    block_list->append_child(func);

    // { for(i := a + b, j := c / d; i < 100; ++i) { func(...); } }
    as2js::Node::pointer_t var_i4(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    var_i4->set_string("func");
    func->append_child(var_i4);

    // { for(i := a + b, j := c / d; i < 100; ++i) { func(...); } }
    as2js::Node::pointer_t param_list(new as2js::Node(as2js::Node::node_t::NODE_LIST));
    func->append_child(param_list);

    // { for(i := a + b, j := c / d; i < 100; ++i) { func(i, ...); } }
    as2js::Node::pointer_t var_i5(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    var_i5->set_string("i");
    param_list->append_child(var_i5);

    // { for(i := a + b, j := c / d; i < 100; ++i) { func(i, j); } }
    as2js::Node::pointer_t var_i6(new as2js::Node(as2js::Node::node_t::NODE_IDENTIFIER));
    var_i6->set_string("j");
    param_list->append_child(var_i6);

    // since we have a tree with parents we can test an invalid parent
    // which itself has a parent and get an error including the parent
    // information
    as2js::Node::pointer_t test_list(new as2js::Node(as2js::Node::node_t::NODE_DIRECTIVE_LIST));
    CPPUNIT_ASSERT_THROW(test_list->set_parent(var_i5, 0), as2js::exception_incompatible_node_type);

    // the DEFINED attribute applies to all types of nodes so it is easy to
    // use... (would the test benefit from testing other attributes?)
    root->set_attribute_tree(as2js::Node::attribute_t::NODE_ATTR_DEFINED, true);
    CPPUNIT_ASSERT(root->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(directive_list->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(for_loop->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(init->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(variable1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(variable_name1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(value1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(add1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var_a1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var_b1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(variable2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(variable_name2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(value2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(divide2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var_a2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var_b2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(less->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var_i2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(one_hunder->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(increment->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var_i3->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(block_list->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(func->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var_i4->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(param_list->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var_i5->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(var_i6->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));

    // now test the clearing of the attribute
    root->set_attribute_tree(as2js::Node::attribute_t::NODE_ATTR_DEFINED, false);
    CPPUNIT_ASSERT(!root->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!directive_list->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!for_loop->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!init->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!variable1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!variable_name1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!value1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!add1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var_a1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var_b1->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!variable2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!variable_name2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!value2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!divide2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var_a2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var_b2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!less->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var_i2->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!one_hunder->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!increment->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var_i3->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!block_list->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!func->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var_i4->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!param_list->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var_i5->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
    CPPUNIT_ASSERT(!var_i6->get_attribute(as2js::Node::attribute_t::NODE_ATTR_DEFINED));
}


// is attribute 'a' in conflict with attribute '*attr_list'?
bool As2JsNodeUnitTests::in_conflict(size_t j, as2js::Node::attribute_t attr, as2js::Node::attribute_t a) const
{
    for(as2js::Node::attribute_t const *conflict_list(g_groups_of_attributes[j].f_attributes);
                                       *conflict_list != as2js::Node::attribute_t::NODE_ATTR_max;
                                       ++conflict_list)
    {
        if(a == *conflict_list)
        {
            return true;
        }
    }

    // the following handles exceptions
    //
    // From the function type:
    //  . abstract, constructor, static, virtual
    //
    // We also get:
    //  . abstract / native
    //  . abstract / constructor / inline / virtual
    switch(attr)
    {
    case as2js::Node::attribute_t::NODE_ATTR_ABSTRACT:
        switch(a)
        {
        case as2js::Node::attribute_t::NODE_ATTR_NATIVE:
        case as2js::Node::attribute_t::NODE_ATTR_INLINE:
            return true;
            break;

        default:
            break;

        }
        break;

    case as2js::Node::attribute_t::NODE_ATTR_CONSTRUCTOR:
        switch(a)
        {
        case as2js::Node::attribute_t::NODE_ATTR_INLINE:
            return true;
            break;

        default:
            break;

        }
        break;

    case as2js::Node::attribute_t::NODE_ATTR_INLINE:
        switch(a)
        {
        case as2js::Node::attribute_t::NODE_ATTR_ABSTRACT:
        case as2js::Node::attribute_t::NODE_ATTR_CONSTRUCTOR:
        case as2js::Node::attribute_t::NODE_ATTR_NATIVE:
        case as2js::Node::attribute_t::NODE_ATTR_VIRTUAL:
            return true;
            break;

        default:
            break;

        }
        break;

    case as2js::Node::attribute_t::NODE_ATTR_NATIVE:
        switch(a)
        {
        case as2js::Node::attribute_t::NODE_ATTR_ABSTRACT:
        case as2js::Node::attribute_t::NODE_ATTR_INLINE:
            return true;
            break;

        default:
            break;

        }
        break;


    case as2js::Node::attribute_t::NODE_ATTR_VIRTUAL:
        switch(a)
        {
        case as2js::Node::attribute_t::NODE_ATTR_INLINE:
            return true;

        default:
            break;

        }
        break;

    default:
        break;

    }

    return false;
}


// vim: ts=4 sw=4 et
