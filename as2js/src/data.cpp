/* data.cpp -- written by Alexis WILKE for Made to Order Software Corp. (c) 2005-2011 */

/*

Copyright (c) 2005-2011 Made to Order Software Corp.

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

#include    "as2js/node.h"
//#include    <inttypes.h>


namespace as2js
{


struct type_name_t
{
    node_t          f_type;
    const char *    f_name;
};

#define    NODE_TYPE_NAME(node)    { NODE_##node, TO_STR_sub(node) }

static const type_name_t node_type_name[] =
{
    NODE_TYPE_NAME(EOF),
    NODE_TYPE_NAME(UNKNOWN),
    NODE_TYPE_NAME(ADD),
    NODE_TYPE_NAME(BITWISE_AND),
    NODE_TYPE_NAME(BITWISE_NOT),
    NODE_TYPE_NAME(ASSIGNMENT),
    NODE_TYPE_NAME(BITWISE_OR),
    NODE_TYPE_NAME(BITWISE_XOR),
    NODE_TYPE_NAME(CLOSE_CURVLY_BRACKET),
    NODE_TYPE_NAME(CLOSE_PARENTHESIS),
    NODE_TYPE_NAME(CLOSE_SQUARE_BRACKET),
    NODE_TYPE_NAME(COLON),
    NODE_TYPE_NAME(COMMA),
    NODE_TYPE_NAME(CONDITIONAL),
    NODE_TYPE_NAME(DIVIDE),
    NODE_TYPE_NAME(GREATER),
    NODE_TYPE_NAME(LESS),
    NODE_TYPE_NAME(LOGICAL_NOT),
    NODE_TYPE_NAME(MODULO),
    NODE_TYPE_NAME(MULTIPLY),
    NODE_TYPE_NAME(OPEN_CURVLY_BRACKET),
    NODE_TYPE_NAME(OPEN_PARENTHESIS),
    NODE_TYPE_NAME(OPEN_SQUARE_BRACKET),
    NODE_TYPE_NAME(MEMBER),
    NODE_TYPE_NAME(SEMICOLON),
    NODE_TYPE_NAME(SUBTRACT),
    NODE_TYPE_NAME(ARRAY),
    NODE_TYPE_NAME(ARRAY_LITERAL),
    NODE_TYPE_NAME(AS),
    NODE_TYPE_NAME(ASSIGNMENT_ADD),
    NODE_TYPE_NAME(ASSIGNMENT_BITWISE_AND),
    NODE_TYPE_NAME(ASSIGNMENT_BITWISE_OR),
    NODE_TYPE_NAME(ASSIGNMENT_BITWISE_XOR),
    NODE_TYPE_NAME(ASSIGNMENT_DIVIDE),
    NODE_TYPE_NAME(ASSIGNMENT_LOGICAL_AND),
    NODE_TYPE_NAME(ASSIGNMENT_LOGICAL_OR),
    NODE_TYPE_NAME(ASSIGNMENT_LOGICAL_XOR),
    NODE_TYPE_NAME(ASSIGNMENT_MAXIMUM),
    NODE_TYPE_NAME(ASSIGNMENT_MINIMUM),
    NODE_TYPE_NAME(ASSIGNMENT_MODULO),
    NODE_TYPE_NAME(ASSIGNMENT_MULTIPLY),
    NODE_TYPE_NAME(ASSIGNMENT_POWER),
    NODE_TYPE_NAME(ASSIGNMENT_ROTATE_LEFT),
    NODE_TYPE_NAME(ASSIGNMENT_ROTATE_RIGHT),
    NODE_TYPE_NAME(ASSIGNMENT_SHIFT_LEFT),
    NODE_TYPE_NAME(ASSIGNMENT_SHIFT_RIGHT),
    NODE_TYPE_NAME(ASSIGNMENT_SHIFT_RIGHT_UNSIGNED),
    NODE_TYPE_NAME(ASSIGNMENT_SUBTRACT),
    NODE_TYPE_NAME(ATTRIBUTES),
    NODE_TYPE_NAME(AUTO),
    NODE_TYPE_NAME(BREAK),
    NODE_TYPE_NAME(CALL),
    NODE_TYPE_NAME(CASE),
    NODE_TYPE_NAME(CATCH),
    NODE_TYPE_NAME(CLASS),
    NODE_TYPE_NAME(CONST),
    NODE_TYPE_NAME(CONTINUE),
    NODE_TYPE_NAME(DECREMENT),
    NODE_TYPE_NAME(DEFAULT),
    NODE_TYPE_NAME(DELETE),
    NODE_TYPE_NAME(DIRECTIVE_LIST),
    NODE_TYPE_NAME(DO),
    NODE_TYPE_NAME(ELSE),
    NODE_TYPE_NAME(EMPTY),
    NODE_TYPE_NAME(ENTRY),
    NODE_TYPE_NAME(ENUM),
    NODE_TYPE_NAME(EQUAL),
    NODE_TYPE_NAME(EXCLUDE),
    NODE_TYPE_NAME(EXTENDS),
    NODE_TYPE_NAME(FALSE),
    NODE_TYPE_NAME(FINALLY),
    NODE_TYPE_NAME(FLOAT64),
    NODE_TYPE_NAME(FOR),
    NODE_TYPE_NAME(FOR_IN),
    NODE_TYPE_NAME(FUNCTION),
    NODE_TYPE_NAME(GOTO),
    NODE_TYPE_NAME(GREATER_EQUAL),
    NODE_TYPE_NAME(IDENTIFIER),
    NODE_TYPE_NAME(IF),
    NODE_TYPE_NAME(IMPLEMENTS),
    NODE_TYPE_NAME(IMPORT),
    NODE_TYPE_NAME(IN),
    NODE_TYPE_NAME(INCLUDE),
    NODE_TYPE_NAME(INCREMENT),
    NODE_TYPE_NAME(INSTANCEOF),
    NODE_TYPE_NAME(INT64),
    NODE_TYPE_NAME(INTERFACE),
    NODE_TYPE_NAME(IS),
    NODE_TYPE_NAME(LABEL),
    NODE_TYPE_NAME(LESS_EQUAL),
    NODE_TYPE_NAME(LIST),
    NODE_TYPE_NAME(LOGICAL_AND),
    NODE_TYPE_NAME(LOGICAL_OR),
    NODE_TYPE_NAME(LOGICAL_XOR),
    NODE_TYPE_NAME(MATCH),
    NODE_TYPE_NAME(MAXIMUM),
    NODE_TYPE_NAME(MINIMUM),
    NODE_TYPE_NAME(NAME),
    NODE_TYPE_NAME(NAMESPACE),
    NODE_TYPE_NAME(NEW),
    NODE_TYPE_NAME(NOT_EQUAL),
    /*NODE_TYPE_NAME(NULL),*/
    { NODE_NULL, "NULL" },
    NODE_TYPE_NAME(OBJECT_LITERAL),
    NODE_TYPE_NAME(PACKAGE),
    NODE_TYPE_NAME(PARAM),
    NODE_TYPE_NAME(PARAMETERS),
    NODE_TYPE_NAME(PARAM_MATCH),
    NODE_TYPE_NAME(POST_DECREMENT),
    NODE_TYPE_NAME(POST_INCREMENT),
    NODE_TYPE_NAME(POWER),
    NODE_TYPE_NAME(PRIVATE),
    NODE_TYPE_NAME(PROGRAM),
    NODE_TYPE_NAME(PUBLIC),
    NODE_TYPE_NAME(RANGE),
    NODE_TYPE_NAME(REGULAR_EXPRESSION),
    NODE_TYPE_NAME(REST),
    NODE_TYPE_NAME(RETURN),
    NODE_TYPE_NAME(ROOT),
    NODE_TYPE_NAME(ROTATE_LEFT),
    NODE_TYPE_NAME(ROTATE_RIGHT),
    NODE_TYPE_NAME(SCOPE),
    NODE_TYPE_NAME(SET),
    NODE_TYPE_NAME(SHIFT_LEFT),
    NODE_TYPE_NAME(SHIFT_RIGHT),
    NODE_TYPE_NAME(SHIFT_RIGHT_UNSIGNED),
    NODE_TYPE_NAME(STRICTLY_EQUAL),
    NODE_TYPE_NAME(STRICTLY_NOT_EQUAL),
    NODE_TYPE_NAME(STRING),
    NODE_TYPE_NAME(SUPER),
    NODE_TYPE_NAME(SWITCH),
    NODE_TYPE_NAME(THIS),
    NODE_TYPE_NAME(THROW),
    NODE_TYPE_NAME(TRUE),
    NODE_TYPE_NAME(TRY),
    NODE_TYPE_NAME(TYPE),
    NODE_TYPE_NAME(TYPEOF),
    NODE_TYPE_NAME(UNDEFINED),
    NODE_TYPE_NAME(USE),
    NODE_TYPE_NAME(VAR),
    NODE_TYPE_NAME(VARIABLE),
    NODE_TYPE_NAME(VAR_ATTRIBUTES),
    NODE_TYPE_NAME(VIDENTIFIER),
    NODE_TYPE_NAME(VOID),
    NODE_TYPE_NAME(WHILE),
    NODE_TYPE_NAME(WITH),

    // end list
    { (node_t)0, 0 }
};


/**********************************************************************/
/**********************************************************************/
/***  DATA DISPLAY  ***************************************************/
/**********************************************************************/
/**********************************************************************/

static void DisplayStr(FILE *out, String str)
{
    fprintf(out, ": '");
    long len = str.GetLength();
    const long *s = str.Get();
    while(len > 0) {
        len--;
        if((unsigned long) *s < 0x7f) {
            fprintf(out, "%c", (char) *s);
        }
        else {
            fprintf(out, "\\U%lX", *s);
        }
        s++;
    }
    fprintf(out, "'");
}



const char *Data::GetTypeName() const
{
    const type_name_t *tn;
    tn = node_type_name;
    while(tn->f_name != 0) {
        if(tn->f_type == f_type) {
            return tn->f_name;
        }
        tn++;
    }
    return "<undefined type name>";
}


void Data::Display(FILE *out) const
{
    const char *name;

    name = GetTypeName();
    fprintf(out, "%04d: %s", f_type, name);
    if(f_type > ' ' && f_type < 0x7F) {
        fprintf(out, " = '%c'", f_type);
    }

    switch(f_type) {
    case NODE_IDENTIFIER:
    case NODE_VIDENTIFIER:
    case NODE_STRING:
    case NODE_GOTO:
    case NODE_LABEL:
    case NODE_IMPORT:
    case NODE_CLASS:
    case NODE_INTERFACE:
    case NODE_ENUM:
        DisplayStr(out, f_str);
        break;

    case NODE_PACKAGE:
    {
        DisplayStr(out, f_str);
        int flags = f_int.Get();
        if((flags & NODE_PACKAGE_FLAG_FOUND_LABELS) != 0) {
            fprintf(out, " FOUND-LABELS");
        }
    }
        break;

    case NODE_INT64:
#if __WORDSIZE == 64
        fprintf(out, ": %ld, 0x%016lX", f_int.Get(), f_int.Get());
#else
        fprintf(out, ": %lld, 0x%016llX", f_int.Get(), f_int.Get());
#endif
        break;

    case NODE_FLOAT64:
        fprintf(out, ": %f", f_float.Get());
        break;

    case NODE_FUNCTION:
    {
        DisplayStr(out, f_str);
        int flags = f_int.Get();
        if((flags & NODE_FUNCTION_FLAG_GETTER) != 0) {
            fprintf(out, " GETTER");
        }
        if((flags & NODE_FUNCTION_FLAG_SETTER) != 0) {
            fprintf(out, " SETTER");
        }
    }
        break;

    case NODE_PARAM:
    {
        DisplayStr(out, f_str);
        int flags = f_int.Get();
        if((flags & NODE_PARAMETERS_FLAG_CONST) != 0) {
            fprintf(out, " CONST");
        }
        if((flags & NODE_PARAMETERS_FLAG_IN) != 0) {
            fprintf(out, " IN");
        }
        if((flags & NODE_PARAMETERS_FLAG_OUT) != 0) {
            fprintf(out, " OUT");
        }
        if((flags & NODE_PARAMETERS_FLAG_NAMED) != 0) {
            fprintf(out, " NAMED");
        }
        if((flags & NODE_PARAMETERS_FLAG_REST) != 0) {
            fprintf(out, " REST");
        }
        if((flags & NODE_PARAMETERS_FLAG_UNCHECKED) != 0) {
            fprintf(out, " UNCHECKED");
        }
        if((flags & NODE_PARAMETERS_FLAG_UNPROTOTYPED) != 0) {
            fprintf(out, " UNPROTOTYPED");
        }
        if((flags & NODE_PARAMETERS_FLAG_REFERENCED) != 0) {
            fprintf(out, " REFERENCED");
        }
        if((flags & NODE_PARAMETERS_FLAG_PARAMREF) != 0) {
            fprintf(out, " PARAMREF");
        }
    }
        break;

    case NODE_PARAM_MATCH:
    {
        fprintf(out, ":");
        int flags = f_int.Get();
        if((flags & NODE_PARAM_MATCH_FLAG_UNPROTOTYPED) != 0) {
            fprintf(out, " UNPROTOTYPED");
        }
    }
        break;

    case NODE_VARIABLE:
    case NODE_VAR_ATTRIBUTES:
    {
        DisplayStr(out, f_str);
        int flags = f_int.Get();
        if((flags & NODE_VAR_FLAG_CONST) != 0) {
            fprintf(out, " CONST");
        }
        if((flags & NODE_VAR_FLAG_LOCAL) != 0) {
            fprintf(out, " LOCAL");
        }
        if((flags & NODE_VAR_FLAG_MEMBER) != 0) {
            fprintf(out, " MEMBER");
        }
        if((flags & NODE_VAR_FLAG_ATTRIBUTES) != 0) {
            fprintf(out, " ATTRIBUTES");
        }
        if((flags & NODE_VAR_FLAG_ENUM) != 0) {
            fprintf(out, " ENUM");
        }
        if((flags & NODE_VAR_FLAG_COMPILED) != 0) {
            fprintf(out, " COMPILED");
        }
        if((flags & NODE_VAR_FLAG_INUSE) != 0) {
            fprintf(out, " INUSE");
        }
        if((flags & NODE_VAR_FLAG_ATTRS) != 0) {
            fprintf(out, " ATTRS");
        }
        if((flags & NODE_VAR_FLAG_DEFINED) != 0) {
            fprintf(out, " DEFINED");
        }
        if((flags & NODE_VAR_FLAG_DEFINING) != 0) {
            fprintf(out, " DEFINING");
        }
        if((flags & NODE_VAR_FLAG_TOADD) != 0) {
            fprintf(out, " TOADD");
        }
    }
        break;

    default:
        break;

    }

    size_t const size = f_user_data.size();
    if(size > 0)
    {
        fprintf(out, " Raw Data:");
        for(size_t idx(0); idx < size; ++idx)
        {
            fprintf(out, " %08X", f_user_data[idx]);
        }
    }
}


/**********************************************************************/
/**********************************************************************/
/***  DATA CONVERSION  ************************************************/
/**********************************************************************/
/**********************************************************************/

bool Data::ToBoolean()
{
    switch(f_type) {
    case NODE_TRUE:
    case NODE_FALSE:
        // already a boolean
        break;

    case NODE_NULL:
    case NODE_UNDEFINED:
        f_type = NODE_FALSE;
        break;

    case NODE_INT64:
        f_type = f_int.Get() != 0 ? NODE_TRUE : NODE_FALSE;
        break;

    case NODE_FLOAT64:
    {
        double value = f_float.Get();
        f_type = value != 0 && !isnan(value) ? NODE_TRUE : NODE_FALSE;
    }
        break;

    case NODE_STRING:
        f_type = f_str.IsEmpty() ? NODE_FALSE : NODE_TRUE;
        break;

    // At this time Data doesn't support any of these:
    //case CHARACTER:
    //case NAMESPACE:
    //case COMPOUNDATTRIBUTE:
    //case CLASS:
    //case SIMPLEINSTANCE:
    //case METHODCLOSURE:
    //case DATE:
    //case REGEXP:
    //case PACKAGE:
    //    f_type = NODE_TRUE;
    //    break;

    default:
        // failure (can't convert)
        return false;

    }

    return true;
}


bool Data::ToNumber()
{
    switch(f_type) {
    case NODE_INT64:
    case NODE_FLOAT64:
        break;

    case NODE_TRUE:
        f_type = NODE_INT64;
        f_int.Set(1);
        break;

    case NODE_NULL:
    case NODE_FALSE:
        f_type = NODE_INT64;
        f_int.Set(0);
        break;

    case NODE_UNDEFINED:
        f_type = NODE_FLOAT64;
        f_float.Set(FP_NAN);
        break;

    default:
        // failure (can't convert)
        return false;

    }

    return true;
}


bool Data::ToString()
{
    char    buf[256];

    buf[sizeof(buf) - 1] = '\0';

    switch(f_type) {
    case NODE_STRING:
        break;

    case NODE_UNDEFINED:
        f_type = NODE_STRING;
        f_str = "undefined";
        break;

    case NODE_NULL:
        f_type = NODE_STRING;
        f_str = "null";
        break;

    case NODE_TRUE:
        f_type = NODE_STRING;
        f_str = "true";
        break;

    case NODE_FALSE:
        f_type = NODE_STRING;
        f_str = "false";
        break;

    case NODE_INT64:
        f_type = NODE_STRING;
#if __WORDSIZE == 64
        snprintf(buf, sizeof(buf) - 1, "%ld", f_int.Get());
#else
        snprintf(buf, sizeof(buf) - 1, "%lld", f_int.Get());
#endif
        f_str = buf;
        break;

    case NODE_FLOAT64:
    {
        double value = f_float.Get();
        f_type = NODE_STRING;
        if(isnan(value)) {
            f_str = "NaN";
        }
        else if(value == 0.0) {
            f_str = "0";
        }
        else if(isinf(value) < 0) {
            f_str = "-Infinity";
        }
        else if(isinf(value) > 0) {
            f_str = "Infinity";
        }
        else {
            snprintf(buf, sizeof(buf) - 1, "%g", value);
            f_str = buf;
        }
    }
        break;

    default:
        // failure (can't convert)
        return false;

    }

    return true;
}


/** \brief Get the current status of a flag.
 *
 * This function returns true or false depending on the current status
 * of the specified flag.
 *
 * The function verifies that the specified flag (\p f) corresponds to
 * the type of data you are dealing with.
 *
 * If the flag was never set, this function returns false.
 *
 * \param[in] f  The flag to retrieve.
 *
 * \return true if the flag was set, false otherwise.
 */
bool Data::get_flag(flag_t f) const
{
    verify_flag_attribute(f);
    return f_flags_and_attributes[f];
}


/** \brief Set a flag.
 *
 * This function sets the specified flag \p f to the specified value \p v
 * in this Node object.
 *
 * The function verifies that the specified flag (\p f) corresponds to
 * the type of data you are dealing with.
 *
 * \param[in] f  The flag to set.
 * \param[in] v  The new value for the flag.
 */
void Data::set_flag(flag_t f, bool v)
{
    verify_flag_attribute(f);
    f_flags_and_attributes[f] = v;
}


/** \brief Verify that f corresponds to the data type.
 *
 * This function verifies that f corresponds to a valid flag according
 * to the type of this Data object.
 */
void Data::verify_flag_attribute(flag_t f)
{
    switch(f)
    {
    case NODE_CATCH_FLAG_TYPED:
        if(f_type != NODE_CATCH)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    case NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES:
        if(f_type != NODE_DIRECTIVE_LIST)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    case NODE_FOR_FLAG_FOREACH:
        if(f_type != NODE_FOR)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    case NODE_FUNCTION_FLAG_GETTER:
    case NODE_FUNCTION_FLAG_SETTER:
    case NODE_FUNCTION_FLAG_OUT:
    case NODE_FUNCTION_FLAG_VOID:
    case NODE_FUNCTION_FLAG_NEVER:
    case NODE_FUNCTION_FLAG_NOPARAMS:
    case NODE_FUNCTION_FLAG_OPERATOR:
        if(f_type != NODE_FUNCTION)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    case NODE_IDENTIFIER_FLAG_WITH:
    case NODE_IDENTIFIER_FLAG_TYPED:
        if(f_type != NODE_IDENTIFIER
        && f_type != NODE_VIDENTIFIER
        && f_type != NODE_STRING)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    case NODE_IMPORT_FLAG_IMPLEMENTS:
        if(f_type != NODE_IMPORT)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    case NODE_PACKAGE_FLAG_FOUND_LABELS:
    case NODE_PACKAGE_FLAG_REFERENCED:
        if(f_type != NODE_PACKAGE)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    case NODE_PARAM_MATCH_FLAG_UNPROTOTYPED:
        if(f_type != NODE_PARAM_MATCH)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break

    case NODE_PARAMETERS_FLAG_CONST:
    case NODE_PARAMETERS_FLAG_IN:
    case NODE_PARAMETERS_FLAG_OUT:
    case NODE_PARAMETERS_FLAG_NAMED:
    case NODE_PARAMETERS_FLAG_REST:
    case NODE_PARAMETERS_FLAG_UNCHECKED:
    case NODE_PARAMETERS_FLAG_UNPROTOTYPED:
    case NODE_PARAMETERS_FLAG_REFERENCED:    // referenced from a parameter or a variable
    case NODE_PARAMETERS_FLAG_PARAMREF:      // referenced from another parameter
    case NODE_PARAMETERS_FLAG_CATCH:         // a parameter defined in a catch()
        if(f_type != NODE_PARAMETERS)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    case NODE_SWITCH_FLAG_DEFAULT:           // we found a 'default:' label in that switch
        if(f_type != NODE_SWITCH)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    case NODE_VAR_FLAG_CONST:
    case NODE_VAR_FLAG_LOCAL:
    case NODE_VAR_FLAG_MEMBER:
    case NODE_VAR_FLAG_ATTRIBUTES:
    case NODE_VAR_FLAG_ENUM:                 // there is a NODE_SET and it somehow needs to be copied
    case NODE_VAR_FLAG_COMPILED:             // Expression() was called on the NODE_SET
    case NODE_VAR_FLAG_INUSE:                // this variable was referenced
    case NODE_VAR_FLAG_ATTRS:                // currently being read for attributes (to avoid loops)
    case NODE_VAR_FLAG_DEFINED:              // was already parsed
    case NODE_VAR_FLAG_DEFINING:             // currently defining, can't read
    case NODE_VAR_FLAG_TOADD:                // to be added in the directive list
        if(f_type != NODE_VARIABLE
        && f_type != NODE_VAR
        && f_type != NODE_PARAM)
        {
            throw internal_error("flag / type missmatch in Data::verify_flag()");
        }
        break;

    // member visibility
    case NODE_ATTR_PUBLIC:
    case NODE_ATTR_PRIVATE:
    case NODE_ATTR_PROTECTED:
    case NODE_ATTR_INTERNAL:

    // function member type
    case NODE_ATTR_STATIC:
    case NODE_ATTR_ABSTRACT:
    case NODE_ATTR_VIRTUAL:
    case NODE_ATTR_ARRAY:

    // function/variable is defined in your system (execution env.)
    case NODE_ATTR_INTRINSIC:

    // operator overload (function member)
    case NODE_ATTR_CONSTRUCTOR:

    // function & member constrains
    case NODE_ATTR_FINAL:
    case NODE_ATTR_ENUMERABLE:

    // conditional compilation
    case NODE_ATTR_TRUE:
    case NODE_ATTR_FALSE:
    case NODE_ATTR_UNUSED:                      // if definition is used, error!

    // class attribute (whether a class can be enlarged at run time)
    case NODE_ATTR_DYNAMIC:

    // switch attributes
    case NODE_ATTR_FOREACH:
    case NODE_ATTR_NOBREAK:
    case NODE_ATTR_AUTOBREAK:
        // TBD -- we'll need to see whether we want to limit the attributes
        //        on a per node type basis and how we can do that properly
        if(f_type == NODE_PROGRAM)
        {
            throw internal_error("attribute / type missmatch in Data::verify_flag()");
        }
        break;

    // attributes were defined
    case NODE_ATTR_DEFINED:
        // all nodes can receive this flag
        break;

    // default: -- do not define so the compiler can tell us if
    //             an enumeration is missing in this case
    }
}


}
// namespace as2js

// vim: ts=4 sw=4 et
