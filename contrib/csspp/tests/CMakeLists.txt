#
# File:
#      CMakeLists.txt
#
# Description:
#      Build csspp tests.
#
# Documentation:
#      See the CMake documentation.
#
# License:
#      CSS Preprocessor -- Test Suite
#      Copyright (C) 2015-2017  Made to Order Software Corporation
#
#      This library is free software; you can redistribute it and/or
#      modify it under the terms of the GNU Lesser General Public
#      License as published by the Free Software Foundation; either
#      version 2 of the License, or (at your option) any later version.
#
#      This library is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#      Lesser General Public License for more details.
#
#      You should have received a copy of the GNU Lesser General Public
#      License along with this library; if not, write to the Free Software
#      Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
#

find_package( Catch )

if(CATCH_FOUND)

    project( csspp_tests )

    include_directories(
        ${CATCH_INCLUDE_DIR}
    )

    add_executable( ${PROJECT_NAME}
        catch_tests.cpp
        catch_assembler.cpp
        catch_color.cpp
        catch_compiler.cpp
        catch_csspp.cpp
        catch_error.cpp
        catch_expr_additive.cpp
        catch_expr_conditional.cpp
        catch_expr_equality.cpp
        catch_expr_list.cpp
        catch_expr_logical_and.cpp
        catch_expr_logical_or.cpp
        catch_expr_multiplicative.cpp
        catch_expr_power.cpp
        catch_expr_relational.cpp
        catch_expr_unary.cpp
        catch_internal_functions.cpp
        catch_lexer.cpp
        catch_node.cpp
        catch_nth_child.cpp
        catch_parser.cpp
        catch_position.cpp
        catch_unicode_range.cpp
    )

    target_link_libraries( ${PROJECT_NAME} csspp )

    # You can use the --success command line option to see all the tests
    # as they run; it is a LOT of output though, thus by default we don't
    # use it
    add_test( csspp_tests ${PROJECT_NAME} )

else(CATCH_FOUND)

    # You may use 'apt-get install catch' under Ubuntu
    message("No test will be created because you do not seem to have catch.hpp installed...")

endif(CATCH_FOUND)

# Local Variables:
# indent-tabs-mode: nil
# tab-width: 4
# End:

# vim: ts=4 sw=4 et
