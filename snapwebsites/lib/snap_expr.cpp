// Snap Websites Server -- C-like expression scripting support
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

#include "snap_expr.h"

#include "snap_parser.h"
#include "qstring_stream.h"
#include "not_reached.h"

#include <controlled_vars/controlled_vars_limited_need_init.h>

#include <QtSerialization/QSerializationComposite.h>
#include <QtSerialization/QSerializationFieldTag.h>
#include <QtSerialization/QSerializationFieldString.h>
#include <QtSerialization/QSerializationFieldBasicTypes.h>
#include <QtSerialization/QSerializationWriter.h>

#include <iostream>

//#include <QList>
#include <QBuffer>

#include "poison.h"

namespace snap
{
namespace snap_expr
{



namespace
{
/** \brief Context to access the database.
 *
 * Different functions available in this expression handling program let
 * you access the database. For example, the cell() function let you
 * read the content of a cell and save it in a variable, or compare
 * to a specific value, or use it as part of some key.
 *
 * The pointer is set using the set_cassandra_context() which is a
 * static function and should only be called once.
 */
QtCassandra::QCassandraContext::pointer_t   g_context;
}




variable_t::variable_t(QString const& name)
    : f_name(name)
    //, f_type(EXPR_VARIABLE_TYPE_NULL) -- auto-init
    //, f_value() -- auto-init (empty, a.k.a. NULL)
{
}


QString variable_t::get_name() const
{
    return f_name;
}


variable_t::variable_type_t variable_t::get_type() const
{
    return f_type;
}


QtCassandra::QCassandraValue const& variable_t::get_value() const
{
    return f_value;
}


void variable_t::set_value(variable_type_t type, QtCassandra::QCassandraValue const& value)
{
    f_type = static_cast<int>(type);        // FIXME cast
    f_value = value;
}


void variable_t::set_value()
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_NULL);        // FIXME cast
    f_value.setNullValue();
}


void variable_t::set_value(bool value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_BOOL);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(char value)
{
    // with g++ this is unsigned...
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_UINT8);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(signed char value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_INT8);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(unsigned char value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_UINT8);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(int16_t value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_INT16);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(uint16_t value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_UINT16);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(int32_t value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_INT32);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(uint32_t value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_UINT32);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(int64_t value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_INT64);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(uint64_t value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_UINT64);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(float value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_FLOAT);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(double value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_DOUBLE);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(QString const& value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_STRING);        // FIXME cast
    f_value = value;
}


void variable_t::set_value(QByteArray const& value)
{
    f_type = static_cast<int>(EXPR_VARIABLE_TYPE_BINARY);        // FIXME cast
    f_value = value;
}


bool variable_t::is_true() const
{
    switch(f_type)
    {
    case EXPR_VARIABLE_TYPE_NULL:
        return false;

    case EXPR_VARIABLE_TYPE_BOOL:
        return f_value.boolValue();

    case EXPR_VARIABLE_TYPE_INT8:
    case EXPR_VARIABLE_TYPE_UINT8:
        return f_value.signedCharValue() != 0;

    case EXPR_VARIABLE_TYPE_INT16:
    case EXPR_VARIABLE_TYPE_UINT16:
        return f_value.int16Value() != 0;

    case EXPR_VARIABLE_TYPE_INT32:
    case EXPR_VARIABLE_TYPE_UINT32:
        return f_value.int32Value() != 0;

    case EXPR_VARIABLE_TYPE_INT64:
    case EXPR_VARIABLE_TYPE_UINT64:
        return f_value.int32Value() != 0;

    case EXPR_VARIABLE_TYPE_FLOAT:
        return f_value.floatValue() != 0.0f;

    case EXPR_VARIABLE_TYPE_DOUBLE:
        return f_value.doubleValue() != 0.0;

    case EXPR_VARIABLE_TYPE_STRING:
    case EXPR_VARIABLE_TYPE_BINARY:
        return !f_value.nullValue();

    }
    NOTREACHED();
}


bool variable_t::get_bool() const
{
    switch(get_type())
    {
    case variable_t::EXPR_VARIABLE_TYPE_BOOL:
        return get_value().boolValue();

    default:
        throw snap_expr_exception_invalid_parameter_type("parameter must be a Boolean");

    }
    NOTREACHED();
}


int64_t variable_t::get_integer() const
{
    switch(get_type())
    {
    case variable_t::EXPR_VARIABLE_TYPE_INT8:
        return get_value().signedCharValue();

    case variable_t::EXPR_VARIABLE_TYPE_UINT8:
        return get_value().unsignedCharValue();

    case variable_t::EXPR_VARIABLE_TYPE_INT16:
        return get_value().int16Value();

    case variable_t::EXPR_VARIABLE_TYPE_UINT16:
        return get_value().uint16Value();

    case variable_t::EXPR_VARIABLE_TYPE_INT32:
        return get_value().int32Value();

    case variable_t::EXPR_VARIABLE_TYPE_UINT32:
        return get_value().uint32Value();

    case variable_t::EXPR_VARIABLE_TYPE_INT64:
        return get_value().int64Value();

    case variable_t::EXPR_VARIABLE_TYPE_UINT64:
        return get_value().uint64Value();

    default:
        throw snap_expr_exception_invalid_parameter_type("parameter must be an integer");

    }
    NOTREACHED();
}


QString variable_t::get_string() const
{
    switch(get_type())
    {
    case variable_t::EXPR_VARIABLE_TYPE_STRING:
        return get_value().stringValue();

    default:
        throw snap_expr_exception_invalid_parameter_type("parameter must be a string");

    }
    NOTREACHED();
}







using namespace parser;


class expr_node : public expr_node_base, public parser_user_data, public QtSerialization::QSerializationObject
{
public:
    static int const LIST_TEST_NODE_MAJOR_VERSION = 1;
    static int const LIST_TEST_NODE_MINOR_VERSION = 0;

    typedef QSharedPointer<expr_node>           expr_node_pointer_t;
    typedef QVector<expr_node_pointer_t>        expr_node_vector_t;
    typedef QMap<QString, expr_node_pointer_t>  expr_node_map_t;

    static functions_t::function_call_table_t const internal_functions[];

    enum node_type_t
    {
        NODE_TYPE_UNKNOWN, // used when loading

        // Operations
        NODE_TYPE_OPERATION_LIST, // comma operator
        NODE_TYPE_OPERATION_LOGICAL_NOT,
        NODE_TYPE_OPERATION_BITWISE_NOT,
        NODE_TYPE_OPERATION_NEGATE,
        NODE_TYPE_OPERATION_FUNCTION,
        NODE_TYPE_OPERATION_MULTIPLY,
        NODE_TYPE_OPERATION_DIVIDE,
        NODE_TYPE_OPERATION_MODULO,
        NODE_TYPE_OPERATION_ADD,
        NODE_TYPE_OPERATION_SUBTRACT,
        NODE_TYPE_OPERATION_SHIFT_LEFT,
        NODE_TYPE_OPERATION_SHIFT_RIGHT,
        NODE_TYPE_OPERATION_LESS,
        NODE_TYPE_OPERATION_LESS_OR_EQUAL,
        NODE_TYPE_OPERATION_GREATER,
        NODE_TYPE_OPERATION_GREATER_OR_EQUAL,
        NODE_TYPE_OPERATION_MINIMUM,
        NODE_TYPE_OPERATION_MAXIMUM,
        NODE_TYPE_OPERATION_EQUAL,
        NODE_TYPE_OPERATION_NOT_EQUAL,
        NODE_TYPE_OPERATION_BITWISE_AND,
        NODE_TYPE_OPERATION_BITWISE_XOR,
        NODE_TYPE_OPERATION_BITWISE_OR,
        NODE_TYPE_OPERATION_LOGICAL_AND,
        NODE_TYPE_OPERATION_LOGICAL_XOR,
        NODE_TYPE_OPERATION_LOGICAL_OR,
        NODE_TYPE_OPERATION_CONDITIONAL,
        NODE_TYPE_OPERATION_ASSIGNMENT, // save to variable
        NODE_TYPE_OPERATION_VARIABLE, // load from variable

        // Literals
        NODE_TYPE_LITERAL_BOOLEAN,
        NODE_TYPE_LITERAL_INTEGER,
        NODE_TYPE_LITERAL_FLOATING_POINT,
        NODE_TYPE_LITERAL_STRING,

        // Variable
        NODE_TYPE_VARIABLE
    };
    typedef controlled_vars::limited_need_init<node_type_t, NODE_TYPE_UNKNOWN, NODE_TYPE_VARIABLE> safe_node_type_t;

    expr_node(node_type_t type)
        : f_type(static_cast<int>(type))        // FIXME cast
        , f_variable("")
        //, f_children() -- auto-init
    {
    }

    node_type_t get_type() const
    {
        return f_type;
    }

    void set_name(QString const& name)
    {
        f_name = name;
    }

    QString get_name() const
    {
        return f_name;
    }

    variable_t const& get_variable() const
    {
        verify_variable();
        return f_variable;
    }

    void set_variable(variable_t const& variable)
    {
        verify_variable();
        f_variable = variable;
    }

    void add_child(expr_node_pointer_t child)
    {
        verify_children();
        f_children.push_back(child);
    }

    int children_size() const
    {
        verify_children();
        return f_children.size();
    }

    expr_node_pointer_t get_child(int idx)
    {
        verify_children();
        if(static_cast<uint32_t>(idx) >= static_cast<uint32_t>(f_children.size()))
        {
            throw snap_logic_exception(QString("expr_node::get_child(%1) called with an out of bounds index (max: %2)")
                                        .arg(idx).arg(f_children.size()));
        }
        return f_children[idx];
    }

    void read(QtSerialization::QReader& r)
    {
        QtSerialization::QComposite comp;
        qint32 type;
        QtSerialization::QFieldInt32 node_type(comp, "type", type); // f_type is an enum...
        QtSerialization::QFieldString node_name(comp, "name", f_name);
        qint64 value_int;
        QtSerialization::QFieldInt64 node_int(comp, "int", value_int);
        double value_dbl;
        QtSerialization::QFieldDouble node_flt(comp, "flt", value_dbl);
        QString value_str;
        QtSerialization::QFieldString node_str(comp, "str", value_str);
        QtSerialization::QFieldTag vars(comp, "node", this);
        r.read(comp);
        f_type = type;
        switch(f_type)
        {
        case NODE_TYPE_LITERAL_BOOLEAN:
            f_variable.set_value(variable_t::EXPR_VARIABLE_TYPE_BOOL, static_cast<bool>(value_int));
            break;

        case NODE_TYPE_LITERAL_INTEGER:
            f_variable.set_value(variable_t::EXPR_VARIABLE_TYPE_INT64, static_cast<int64_t>(value_int));
            break;

        case NODE_TYPE_LITERAL_FLOATING_POINT:
            f_variable.set_value(variable_t::EXPR_VARIABLE_TYPE_DOUBLE, value_dbl);
            break;

        case NODE_TYPE_LITERAL_STRING:
            f_variable.set_value(variable_t::EXPR_VARIABLE_TYPE_STRING, value_str);
            break;

        default:
            // only literals have a value when serializing
            break;

        }
    }

    virtual void readTag(QString const& name, QtSerialization::QReader& r)
    {
        if(name == "node")
        {
            // create a variable with an invalid name
            expr_node_pointer_t child(new expr_node(NODE_TYPE_UNKNOWN));
            // read the data from the reader
            child->read(r);
            // add to the variable vector
            add_child(child);
        }
    }

    void write(QtSerialization::QWriter& w) const
    {
        QtSerialization::QWriter::QTag tag(w, "node");

        QtSerialization::writeTag(w, "type", static_cast<qint32>(f_type));

        if(!f_name.isEmpty())
        {
            QtSerialization::writeTag(w, "name", f_name);
        }

        switch(f_type)
        {
        case NODE_TYPE_LITERAL_INTEGER:
            QtSerialization::writeTag(w, "int", static_cast<qint64>(f_variable.get_value().int64Value()));
            break;

        case NODE_TYPE_LITERAL_FLOATING_POINT:
            QtSerialization::writeTag(w, "flt", f_variable.get_value().doubleValue());
            break;

        case NODE_TYPE_LITERAL_STRING:
            QtSerialization::writeTag(w, "str", f_variable.get_value().stringValue());
            break;

        default:
            // only literals have a value when serializing
            break;

        }

        int const max_children(f_children.size());
        for(int i(0); i < max_children; ++i)
        {
            f_children[i]->write(w);
        }
    }

    // SFINAE test
    // Source: http://stackoverflow.com/questions/257288/is-it-possible-to-write-a-c-template-to-check-for-a-functions-existence
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
    template<typename T>
    class has_function
    {
    private:
        typedef char yes[1];
        typedef char no [2];

        template <typename U, U> struct type_check;

        // integers() is always available at this point
        //template <typename C> static yes &test_integers(type_check<void(&)(int64_t, int64_t), C::integers> *);
        //template <typename C> static no  &test_integers(...);

        template <typename C> static yes &test_floating_points(type_check<double(&)(double, double), C::floating_points> *);
        template <typename C> static no  &test_floating_points(...);

        template <typename C> static yes &test_string_integer(type_check<QString(&)(QString const&, int64_t), C::string_integer> *);
        template <typename C> static no  &test_string_integer(...);

        template <typename C> static yes &test_strings(type_check<QString(&)(QString const&, QString const&), C::strings> *);
        template <typename C> static no  &test_strings(...);

    public:
        //static bool const has_integers        = sizeof(test_integers       <T>(nullptr)) == sizeof(yes);
        static bool const has_floating_points = sizeof(test_floating_points<T>(nullptr)) == sizeof(yes);
        static bool const has_string_integer  = sizeof(test_string_integer <T>(nullptr)) == sizeof(yes);
        static bool const has_strings         = sizeof(test_strings        <T>(nullptr)) == sizeof(yes);
    };
#pragma GCC diagnostic pop

    // select structure depending on bool
    template<bool C, typename T = void>
    struct enable_if
    {
        typedef T type;
    };

    // if false, do nothing, whatever T
    template<typename T>
    struct enable_if<false, T>
    {
    };

    template<typename F>
    typename enable_if<has_function<F>::has_floating_points>::type do_float(variable_t& result, double a, double b)
    {
        QtCassandra::QCassandraValue value;
        value.setFloatValue(F::floating_points(a, b));
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_FLOAT, value);
    }

    template<typename F>
    typename enable_if<!has_function<F>::has_floating_points>::type do_float(variable_t& result, double a, double b)
    {
    }

    template<typename F>
    typename enable_if<has_function<F>::has_floating_points>::type do_double(variable_t& result, double a, double b)
    {
        QtCassandra::QCassandraValue value;
        value.setDoubleValue(F::floating_points(a, b));
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_DOUBLE, value);
    }

    template<typename F>
    typename enable_if<!has_function<F>::has_floating_points>::type do_double(variable_t& result, double a, double b)
    {
    }

    template<typename F>
    typename enable_if<has_function<F>::has_string_integer>::type do_string_integer(variable_t& result, QString const& a, int64_t b)
    {
        QtCassandra::QCassandraValue value;
        value.setStringValue(F::string_integer(a, b));
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    template<typename F>
    typename enable_if<!has_function<F>::has_string_integer>::type do_string_integer(variable_t& result, QString const& a, int64_t b)
    {
    }

    template<typename F>
    typename enable_if<has_function<F>::has_strings>::type do_strings(variable_t& result, QString const& a, QString const& b)
    {
        QtCassandra::QCassandraValue value;
        value.setStringValue(F::strings(a, b));
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    template<typename F>
    typename enable_if<!has_function<F>::has_strings>::type do_strings(variable_t& result, QString const& a, QString const& b)
    {
    }

    class op_multiply
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a * b; }
        static double floating_points(double a, double b) { return a * b; }
        static QString string_integer(QString const& a, int64_t b) { return a.repeated(static_cast<int>(b)); }
    };

    class op_divide
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a / b; }
        static double floating_points(double a, double b) { return a / b; }
    };

    class op_modulo
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a % b; }
    };

    class op_add
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a + b; }
        static double floating_points(double a, double b) { return a + b; }
        static QString strings(QString const& a, QString const& b) { return a + b; }
    };

    class op_subtract
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a - b; }
        static double floating_points(double a, double b) { return a - b; }
    };

    class op_shift_left
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a << b; }
    };

    class op_shift_right
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a >> b; }
        static int64_t integers(uint64_t a, int64_t b) { return a >> b; }
        static int64_t integers(int64_t a, uint64_t b) { return a >> b; }
        static int64_t integers(uint64_t a, uint64_t b) { return a >> b; }
    };

    class op_less
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a < b; }
        static int64_t floating_points(double a, double b) { return a < b; }
        static int64_t strings(QString const& a, QString const& b) { return a < b; }
    };

    class op_less_or_equal
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a <= b; }
        static int64_t floating_points(double a, double b) { return a <= b; }
        static int64_t strings(QString const& a, QString const& b) { return a <= b; }
    };

    class op_greater
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a > b; }
        static int64_t floating_points(double a, double b) { return a > b; }
        static int64_t strings(QString const& a, QString const& b) { return a > b; }
    };

    class op_greater_or_equal
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a >= b; }
        static int64_t floating_points(double a, double b) { return a >= b; }
        static int64_t strings(QString const& a, QString const& b) { return a >= b; }
    };

    class op_minimum
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a < b ? a : b; }
        static double floating_points(double a, double b) { return a < b ? a : b; }
        static QString strings(QString const& a, QString const& b) { return a < b ? a : b; }
    };

    class op_maximum
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a > b ? a : b; }
        static double floating_points(double a, double b) { return a > b ? a : b; }
        static QString strings(QString const& a, QString const& b) { return a > b ? a : b; }
    };

    class op_equal
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a == b; }
        static int64_t floating_points(double a, double b) { return a == b; }
        static int64_t strings(QString const& a, QString const& b) { return a == b; }
    };

    class op_not_equal
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a != b; }
        static int64_t floating_points(double a, double b) { return a != b; }
        static int64_t strings(QString const& a, QString const& b) { return a != b; }
    };

    class op_bitwise_and
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a & b; }
    };

    class op_bitwise_xor
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a ^ b; }
    };

    class op_bitwise_or
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a | b; }
    };

    class op_logical_and
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a && b; }
    };

    class op_logical_xor
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return (a != 0) ^ (b != 0); }
    };

    class op_logical_or
    {
    public:
        static int64_t integers(int64_t a, int64_t b) { return a || b; }
    };

    void execute(variable_t& result, variable_t::variable_map_t& variables, functions_t& functions)
    {
        variable_t::variable_vector_t sub_results;
        int const max_children(f_children.size());
        for(int i(0); i < max_children; ++i)
        {
            variable_t cr;
            f_children[i]->execute(cr, variables, functions);
            sub_results.push_back(cr);
        }

        switch(f_type)
        {
        case NODE_TYPE_UNKNOWN:
            throw snap_logic_exception("expr_node::execute() called with an incompatible result type: NODE_TYPE_UNKNOWN");

        case NODE_TYPE_OPERATION_LIST:
            result = sub_results.last();
            break;

        case NODE_TYPE_OPERATION_LOGICAL_NOT:
            logical_not(result, sub_results);
            break;

        case NODE_TYPE_OPERATION_BITWISE_NOT:
            bitwise_not(result, sub_results);
            break;

        case NODE_TYPE_OPERATION_NEGATE:
            negate(result, sub_results);
            break;

        case NODE_TYPE_OPERATION_FUNCTION:
            call_function(result, sub_results, functions);
            break;

        case NODE_TYPE_OPERATION_MULTIPLY:
            binary_operation<op_multiply>("*", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_DIVIDE:
            binary_operation<op_divide>("/", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_MODULO:
            binary_operation<op_modulo>("%", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_ADD:
            binary_operation<op_add>("+", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_SUBTRACT:
            binary_operation<op_subtract>("-", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_SHIFT_LEFT:
            binary_operation<op_shift_left>("<<", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_SHIFT_RIGHT:
            binary_operation<op_shift_right>(">>", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_LESS:
            binary_operation<op_less>("<", result, sub_results, true);
            break;

        case NODE_TYPE_OPERATION_LESS_OR_EQUAL:
            binary_operation<op_less_or_equal>("<=", result, sub_results, true);
            break;

        case NODE_TYPE_OPERATION_GREATER:
            binary_operation<op_greater>(">", result, sub_results, true);
            break;

        case NODE_TYPE_OPERATION_GREATER_OR_EQUAL:
            binary_operation<op_greater_or_equal>(">=", result, sub_results, true);
            break;

        case NODE_TYPE_OPERATION_MINIMUM:
            binary_operation<op_minimum>("<?", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_MAXIMUM:
            binary_operation<op_maximum>(">?", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_EQUAL:
            binary_operation<op_equal>("==", result, sub_results, true);
            break;

        case NODE_TYPE_OPERATION_NOT_EQUAL:
            binary_operation<op_not_equal>("!=", result, sub_results, true);
            break;

        case NODE_TYPE_OPERATION_BITWISE_AND:
            binary_operation<op_bitwise_and>("&", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_BITWISE_XOR:
            binary_operation<op_bitwise_xor>("^", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_BITWISE_OR:
            binary_operation<op_bitwise_or>("|", result, sub_results, false);
            break;

        case NODE_TYPE_OPERATION_LOGICAL_AND:
            binary_operation<op_logical_and>("&&", result, sub_results, true);
            break;

        case NODE_TYPE_OPERATION_LOGICAL_XOR:
            binary_operation<op_logical_xor>("^^", result, sub_results, true);
            break;

        case NODE_TYPE_OPERATION_LOGICAL_OR:
            binary_operation<op_logical_or>("||", result, sub_results, true);
            break;

        case NODE_TYPE_OPERATION_CONDITIONAL:
            conditional(result, sub_results);
            break;

        case NODE_TYPE_OPERATION_ASSIGNMENT:
            assignment(result, sub_results, variables);
            break;

        case NODE_TYPE_OPERATION_VARIABLE:
            load_variable(result, variables);
            break;

        case NODE_TYPE_LITERAL_BOOLEAN:
            result = f_variable;
            break;

        case NODE_TYPE_LITERAL_INTEGER:
            result = f_variable;
            break;

        case NODE_TYPE_LITERAL_FLOATING_POINT:
            result = f_variable;
            break;

        case NODE_TYPE_LITERAL_STRING:
            result = f_variable;
            break;

        case NODE_TYPE_VARIABLE:
            // a program cannot include variables as instructions
            throw snap_logic_exception(QString("expr_node::execute() called with an incompatible type: %1").arg(f_type));

        }
    }

    void logical_not(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        verify_unary(sub_results);
        QtCassandra::QCassandraValue value;
        switch(sub_results[0].get_type())
        {
        case variable_t::EXPR_VARIABLE_TYPE_BOOL:
            value.setBoolValue(!sub_results[0].get_value().boolValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT8:
            value.setBoolValue(sub_results[0].get_value().signedCharValue() == 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT8:
            value.setBoolValue(sub_results[0].get_value().unsignedCharValue() == 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT16:
            value.setBoolValue(sub_results[0].get_value().int16Value() == 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT16:
            value.setBoolValue(sub_results[0].get_value().uint16Value() == 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT32:
            value.setBoolValue(sub_results[0].get_value().int32Value() == 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT32:
            value.setBoolValue(sub_results[0].get_value().uint32Value() == 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT64:
            value.setBoolValue(sub_results[0].get_value().int64Value() == 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT64:
            value.setBoolValue(sub_results[0].get_value().uint64Value() == 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_FLOAT:
            value.setBoolValue(sub_results[0].get_value().floatValue() == 0.0f);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_DOUBLE:
            value.setBoolValue(sub_results[0].get_value().doubleValue() == 0.0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_STRING:
            value.setBoolValue(sub_results[0].get_value().stringValue().length() == 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_BINARY:
            value.setBoolValue(sub_results[0].get_value().binaryValue().size() == 0);
            break;

        default:
            throw snap_logic_exception(QString("expr_node::logical_not() called with an incompatible sub_result type: %1").arg(static_cast<int>(sub_results[0].get_type())));

        }
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    void bitwise_not(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        verify_unary(sub_results);
        QtCassandra::QCassandraValue value;
        switch(sub_results[0].get_type())
        {
        case variable_t::EXPR_VARIABLE_TYPE_BOOL:
            value.setBoolValue(~sub_results[0].get_value().boolValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT8:
            value.setSignedCharValue(~sub_results[0].get_value().signedCharValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT8:
            value.setUnsignedCharValue(~sub_results[0].get_value().unsignedCharValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT16:
            value.setInt16Value(~sub_results[0].get_value().int16Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT16:
            value.setUInt16Value(~sub_results[0].get_value().uint16Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT32:
            value.setInt32Value(~sub_results[0].get_value().int32Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT32:
            value.setUInt32Value(~sub_results[0].get_value().uint32Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT64:
            value.setInt64Value(~sub_results[0].get_value().int64Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT64:
            value.setUInt64Value(~sub_results[0].get_value().uint64Value());
            break;

        //case variable_t::EXPR_VARIABLE_TYPE_STRING:
            // TODO:
            // swap case (A -> a and A -> a)
        default:
            throw snap_logic_exception(QString("expr_node::bitwise_not() called with an incompatible sub_result type: %1").arg(static_cast<int>(sub_results[0].get_type())));

        }
        result.set_value(sub_results[0].get_type(), value);
    }

    void negate(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        verify_unary(sub_results);
        QtCassandra::QCassandraValue value;
        switch(sub_results[0].get_type())
        {
        case variable_t::EXPR_VARIABLE_TYPE_INT8:
            value.setSignedCharValue(-sub_results[0].get_value().signedCharValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT8:
            value.setUnsignedCharValue(-sub_results[0].get_value().unsignedCharValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT16:
            value.setInt16Value(-sub_results[0].get_value().int16Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT16:
            value.setUInt16Value(-sub_results[0].get_value().uint16Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT32:
            value.setInt32Value(-sub_results[0].get_value().int32Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT32:
            value.setUInt32Value(-sub_results[0].get_value().uint32Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT64:
            value.setInt64Value(-sub_results[0].get_value().int64Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT64:
            value.setUInt64Value(-sub_results[0].get_value().uint64Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_FLOAT:
            value.setFloatValue(-sub_results[0].get_value().floatValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_DOUBLE:
            value.setDoubleValue(-sub_results[0].get_value().doubleValue());
            break;

        default:
            throw snap_logic_exception(QString("expr_node::negate() called with an incompatible sub_result type: %1").arg(static_cast<int>(sub_results[0].get_type())));

        }
        result.set_value(sub_results[0].get_type(), value);
    }

    static void call_cell(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(!g_context)
        {
            throw snap_expr_exception_invalid_number_of_parameters("cell() function not available, g_context is NULL");
        }
        if(sub_results.size() != 3)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call cell() expected exactly 3");
        }
        QString const table_name(sub_results[0].get_string());
        QString const row_name(sub_results[1].get_string());
        QString const cell_name(sub_results[2].get_string());
        QtCassandra::QCassandraValue value(g_context->table(table_name)->row(row_name)->cell(cell_name)->value());
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_BINARY, value);
    }

    static void call_cell_exists(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(!g_context)
        {
            throw snap_expr_exception_invalid_number_of_parameters("cell_exists() function not available, g_context is NULL");
        }
        if(sub_results.size() != 3)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call cell_exists() expected exactly 3");
        }
        QString const table_name(sub_results[0].get_string());
        QString const row_name(sub_results[1].get_string());
        QString const cell_name(sub_results[2].get_string());
        QtCassandra::QCassandraValue value;
        value.setBoolValue(g_context->table(table_name)->row(row_name)->exists(cell_name));
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    static void call_int64(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call string() expected exactly 1");
        }
        int64_t r(0);
        QtCassandra::QCassandraValue const& v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::EXPR_VARIABLE_TYPE_NULL:
            //r = 0; -- an "empty" number...
            break;

        case variable_t::EXPR_VARIABLE_TYPE_BOOL:
            r = v.boolValue() ? 1 : 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT8:
            r = v.signedCharValue();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT8:
            r = v.unsignedCharValue();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT16:
            r = v.int16Value();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT16:
            r = v.uint16Value();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT32:
            r = v.int32Value();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT32:
            r = v.uint32Value();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT64:
            r = v.int64Value();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT64:
            r = v.uint64Value();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = v.floatValue();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = v.doubleValue();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_STRING:
            r = v.stringValue().toLongLong();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_BINARY:
            r = v.int64Value();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setInt64Value(r);
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_INT64, value);
    }

    static void call_row_exists(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(!g_context)
        {
            throw snap_expr_exception_invalid_number_of_parameters("row_exists() function not available, g_context is NULL");
        }
        if(sub_results.size() != 2)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call row_exists() expected exactly 2");
        }
        QString const table_name(sub_results[0].get_string());
        QString const row_name(sub_results[1].get_string());
        QtCassandra::QCassandraValue value;
        value.setBoolValue(g_context->table(table_name)->exists(row_name));
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    static void call_string(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call string() expected exactly 1");
        }
        QString str;
        QtCassandra::QCassandraValue const& v(sub_results[0].get_value());
        switch(sub_results[0].get_type())
        {
        case variable_t::EXPR_VARIABLE_TYPE_NULL:
            //str = ""; -- an empty string is the default
            break;

        case variable_t::EXPR_VARIABLE_TYPE_BOOL:
            str = v.boolValue() ? "true" : "false";
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT8:
            str = QString("%1").arg(static_cast<int>(v.signedCharValue()));
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT8:
            str = QString("%1").arg(static_cast<int>(v.unsignedCharValue()));
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT16:
            str = QString("%1").arg(v.int16Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT16:
            str = QString("%1").arg(v.uint16Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT32:
            str = QString("%1").arg(v.int32Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT32:
            str = QString("%1").arg(v.uint32Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT64:
            str = QString("%1").arg(v.int64Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT64:
            str = QString("%1").arg(v.uint64Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_FLOAT:
            str = QString("%1").arg(v.floatValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_DOUBLE:
            str = QString("%1").arg(v.doubleValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_STRING:
            str = v.stringValue();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_BINARY:
            str = v.stringValue();
            break;

        }
        QtCassandra::QCassandraValue value;
        value.setStringValue(str);
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    static void call_strlen(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call strlen() expected exactly 1");
        }
        QString const str(sub_results[0].get_string());
        QtCassandra::QCassandraValue value;
        value.setInt64Value(str.length());
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_INT64, value);
    }

    static void call_substr(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        int const size(sub_results.size());
        if(size < 2 || size > 3)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call substr() expected 2 or 3");
        }
        QString const str(sub_results[0].get_string());
        int const start(sub_results[1].get_integer());
        QtCassandra::QCassandraValue value;
        if(size == 3)
        {
            int const end(sub_results[2].get_integer());
            value.setStringValue(str.mid(start, end));
        }
        else
        {
            value.setStringValue(str.mid(start));
        }
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_STRING, value);
    }

    static void call_table_exists(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
        if(!g_context)
        {
            throw snap_expr_exception_invalid_number_of_parameters("table_exists() function not available, g_context is NULL");
        }
        if(sub_results.size() != 1)
        {
            throw snap_expr_exception_invalid_number_of_parameters("invalid number of parameters to call table_exists() expected exactly 1");
        }
        QString const table_name(sub_results[0].get_string());
        QtCassandra::QCassandraValue value;
        value.setBoolValue(g_context->findTable(table_name) != nullptr);
        result.set_value(variable_t::EXPR_VARIABLE_TYPE_BOOL, value);
    }

    void call_function(variable_t& result, variable_t::variable_vector_t const& sub_results, functions_t& functions)
    {
        functions_t::function_call_t f(functions.get_function(f_name));
        if(f == nullptr)
        {
            if(functions.get_has_internal_functions())
            {
                throw snap_expr_exception_unknown_function(QString("unknown function \"%s\" in list execution environment").arg(f_name));
            }
            functions.add_functions(internal_functions);
            f = functions.get_function(f_name);
            if(f == nullptr)
            {
                throw snap_expr_exception_unknown_function(QString("unknown function \"%s\" in list execution environment").arg(f_name));
            }
        }
        f(result, sub_results);
    }

    template<typename F>
    void binary_operation(char const *op, variable_t& result, variable_t::variable_vector_t const& sub_results, bool return_bool)
    {
        verify_binary(sub_results);

        variable_t::variable_type_t type(
                return_bool ? variable_t::EXPR_VARIABLE_TYPE_BOOL
                            : std::max(sub_results[0].get_type(), sub_results[1].get_type()));  // FIXME cast

        bool lstring_type(false), rstring_type(false);
        bool lfloating_point(false), rfloating_point(false);
        bool lsigned_value(true), rsigned_value(true);
        QString ls, rs;
        double lf(0.0), rf(0.0);
        int64_t li(0), ri(0);

        bool valid(true);
        switch(sub_results[0].get_type())
        {
        case variable_t::EXPR_VARIABLE_TYPE_BOOL:
            li = static_cast<int64_t>(sub_results[0].get_value().boolValue() != 0);
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT8:
            li = static_cast<int64_t>(sub_results[0].get_value().signedCharValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT8:
            lsigned_value = false;
            li = static_cast<int64_t>(sub_results[0].get_value().unsignedCharValue());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT16:
            li = static_cast<int64_t>(sub_results[0].get_value().int16Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT16:
            lsigned_value = false;
            li = static_cast<int64_t>(sub_results[0].get_value().uint16Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT32:
            li = static_cast<int64_t>(sub_results[0].get_value().int32Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT32:
            lsigned_value = false;
            li = static_cast<int64_t>(sub_results[0].get_value().uint32Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT64:
            li = static_cast<int64_t>(sub_results[0].get_value().int64Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT64:
            lsigned_value = false;
            li = static_cast<int64_t>(sub_results[0].get_value().uint64Value());
            break;

        case variable_t::EXPR_VARIABLE_TYPE_DOUBLE:
            lfloating_point = true;
            lf = sub_results[0].get_value().doubleValue();
            break;

        case variable_t::EXPR_VARIABLE_TYPE_STRING:
            lstring_type = true;
            ls = sub_results[0].get_value().stringValue();
            break;

        default:
            valid = false;
            break;

        }

        if(valid)
        {
            switch(sub_results[1].get_type())
            {
            case variable_t::EXPR_VARIABLE_TYPE_BOOL:
                ri = static_cast<bool>(sub_results[1].get_value().boolValue() != 0);
                break;

            case variable_t::EXPR_VARIABLE_TYPE_INT8:
                ri = static_cast<int64_t>(sub_results[1].get_value().signedCharValue());
                break;

            case variable_t::EXPR_VARIABLE_TYPE_UINT8:
                rsigned_value = false;
                ri = static_cast<int64_t>(sub_results[1].get_value().unsignedCharValue());
                break;

            case variable_t::EXPR_VARIABLE_TYPE_INT16:
                ri = static_cast<int64_t>(sub_results[1].get_value().int16Value());
                break;

            case variable_t::EXPR_VARIABLE_TYPE_UINT16:
                rsigned_value = false;
                ri = static_cast<int64_t>(sub_results[1].get_value().uint16Value());
                break;

            case variable_t::EXPR_VARIABLE_TYPE_INT32:
                ri = static_cast<int64_t>(sub_results[1].get_value().int32Value());
                break;

            case variable_t::EXPR_VARIABLE_TYPE_UINT32:
                rsigned_value = false;
                ri = static_cast<int64_t>(sub_results[1].get_value().uint32Value());
                break;

            case variable_t::EXPR_VARIABLE_TYPE_INT64:
                ri = static_cast<int64_t>(sub_results[1].get_value().int64Value());
                break;

            case variable_t::EXPR_VARIABLE_TYPE_UINT64:
                rsigned_value = false;
                ri = static_cast<int64_t>(sub_results[1].get_value().uint64Value());
                break;

            case variable_t::EXPR_VARIABLE_TYPE_DOUBLE:
                rfloating_point = true;
                rf = sub_results[1].get_value().doubleValue();
                break;

            case variable_t::EXPR_VARIABLE_TYPE_STRING:
                rstring_type = true;
                rs = sub_results[1].get_value().stringValue();
                break;

            default:
                valid = false;
                break;

            }
        }

        if(valid)
        {
            QtCassandra::QCassandraValue value;
            switch(type)
            {
            case variable_t::EXPR_VARIABLE_TYPE_BOOL:
                value.setBoolValue(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::EXPR_VARIABLE_TYPE_INT8:
                value.setSignedCharValue(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::EXPR_VARIABLE_TYPE_UINT8:
                value.setUnsignedCharValue(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::EXPR_VARIABLE_TYPE_INT16:
                value.setInt16Value(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::EXPR_VARIABLE_TYPE_UINT16:
                value.setUInt16Value(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::EXPR_VARIABLE_TYPE_INT32:
                value.setInt32Value(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::EXPR_VARIABLE_TYPE_UINT32:
                value.setUInt32Value(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::EXPR_VARIABLE_TYPE_INT64:
                value.setInt64Value(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::EXPR_VARIABLE_TYPE_UINT64:
                value.setUInt64Value(F::integers(li, ri));
                result.set_value(type, value);
                return;

            case variable_t::EXPR_VARIABLE_TYPE_FLOAT:
                if(has_function<F>::has_floating_points)
                {
                    if(!lfloating_point)
                    {
                        lf = lsigned_value ? li : static_cast<uint64_t>(li);
                    }
                    if(!rfloating_point)
                    {
                        rf = rsigned_value ? ri : static_cast<uint64_t>(ri);
                    }
                    do_float<F>(result, lf, rf);
                    return;
                }
                break;

            case variable_t::EXPR_VARIABLE_TYPE_DOUBLE:
                if(has_function<F>::has_floating_points)
                {
                    if(!lfloating_point)
                    {
                        lf = lsigned_value ? li : static_cast<uint64_t>(li);
                    }
                    if(!rfloating_point)
                    {
                        rf = rsigned_value ? ri : static_cast<uint64_t>(ri);
                    }
                    do_double<F>(result, lf, rf);
                    return;
                }
                break;

            case variable_t::EXPR_VARIABLE_TYPE_STRING:
                if(has_function<F>::has_strings)
                {
                    if(!lstring_type)
                    {
                        ls = QString("%1").arg(lfloating_point ? lf : (lsigned_value ? li : static_cast<uint64_t>(li)));
                    }
                    if(!rstring_type)
                    {
                        rs = QString("%1").arg(rfloating_point ? rf : (rsigned_value ? ri : static_cast<uint64_t>(ri)));
                    }
                    // do "string" + "string"
                    do_strings<F>(result, ls, rs);
                    return;
                }
                else if(has_function<F>::has_string_integer
                     && variable_t::EXPR_VARIABLE_TYPE_STRING == sub_results[0].get_type()
                     && !rfloating_point && !rstring_type)
                {
                    // very special case to support: "string" * 123
                    do_string_integer<F>(result, ls, ri);
                    return;
                }
                break;

            default:
                // anything else is invalid at this point
                break;

            }
        }

        throw snap_logic_exception(QString("expr_node::binary_operation(\"%3\") called with incompatible sub_result types: %1 x %2")
                            .arg(static_cast<int>(sub_results[0].get_type()))
                            .arg(static_cast<int>(sub_results[1].get_type()))
                            .arg(op));
    }

    void conditional(variable_t& result, variable_t::variable_vector_t const& sub_results)
    {
#ifdef DEBUG
        if(sub_results.size() != 3)
        {
            throw snap_logic_exception("expr_node::conditional() found a conditional operator with a number of results which is not 3");
        }
#endif
        bool r(false);
        switch(sub_results[0].get_type())
        {
        case variable_t::EXPR_VARIABLE_TYPE_BOOL:
            r = sub_results[0].get_value().signedCharValue() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT8:
            r = sub_results[0].get_value().signedCharValue() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT8:
            r = sub_results[0].get_value().unsignedCharValue() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT16:
            r = sub_results[0].get_value().int16Value() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT16:
            r = sub_results[0].get_value().uint16Value() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT32:
            r = sub_results[0].get_value().int32Value() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT32:
            r = sub_results[0].get_value().uint32Value() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_INT64:
            r = sub_results[0].get_value().int64Value() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_UINT64:
            r = sub_results[0].get_value().uint64Value() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_FLOAT:
            r = sub_results[0].get_value().floatValue() != 0.0f;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_DOUBLE:
            r = sub_results[0].get_value().doubleValue() != 0.0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_STRING:
            r = sub_results[0].get_value().stringValue().length() != 0;
            break;

        case variable_t::EXPR_VARIABLE_TYPE_BINARY:
            r = sub_results[0].get_value().binaryValue().size() != 0;
            break;

        default:
            throw snap_logic_exception(QString("expr_node::logical_not() called with an incompatible sub_result type: %1").arg(static_cast<int>(sub_results[0].get_type())));

        }
        if(r)
        {
            result = sub_results[1];
        }
        else
        {
            result = sub_results[2];
        }
    }

    void assignment(variable_t& result, variable_t::variable_vector_t const& sub_results, variable_t::variable_map_t& variables)
    {
#ifdef DEBUG
        if(sub_results.size() != 1)
        {
            throw snap_logic_exception("expr_node::execute() found an assignment operator with a number of results which is not 1");
        }
#endif
        variables[f_name] = sub_results[0];
        // result is a copy of the variable contents
        result = variables[f_name];
    }

    void load_variable(variable_t& result, variable_t::variable_map_t& variables)
    {
        if(variables.contains(f_name))
        {
            result = variables[f_name];
        }
    }

private:
    void verify_variable() const
    {
#ifdef DEBUG
        switch(f_type)
        {
        case NODE_TYPE_OPERATION_ASSIGNMENT:
        case NODE_TYPE_LITERAL_BOOLEAN:
        case NODE_TYPE_LITERAL_INTEGER:
        case NODE_TYPE_LITERAL_FLOATING_POINT:
        case NODE_TYPE_LITERAL_STRING:
        case NODE_TYPE_OPERATION_VARIABLE:
            break;

        default:
            throw snap_logic_exception(QString("expr_node::[gs]et_name(\"...\") called on a node which does not support a name... (type: %1)").arg(f_type));

        }
#endif
    }

    void verify_children() const
    {
#ifdef DEBUG
        switch(f_type)
        {
        case NODE_TYPE_OPERATION_LIST:
        case NODE_TYPE_OPERATION_LOGICAL_NOT:
        case NODE_TYPE_OPERATION_BITWISE_NOT:
        case NODE_TYPE_OPERATION_NEGATE:
        case NODE_TYPE_OPERATION_FUNCTION:
        case NODE_TYPE_OPERATION_MULTIPLY:
        case NODE_TYPE_OPERATION_DIVIDE:
        case NODE_TYPE_OPERATION_MODULO:
        case NODE_TYPE_OPERATION_ADD:
        case NODE_TYPE_OPERATION_SUBTRACT:
        case NODE_TYPE_OPERATION_SHIFT_LEFT:
        case NODE_TYPE_OPERATION_SHIFT_RIGHT:
        case NODE_TYPE_OPERATION_LESS:
        case NODE_TYPE_OPERATION_LESS_OR_EQUAL:
        case NODE_TYPE_OPERATION_GREATER:
        case NODE_TYPE_OPERATION_GREATER_OR_EQUAL:
        case NODE_TYPE_OPERATION_MINIMUM:
        case NODE_TYPE_OPERATION_MAXIMUM:
        case NODE_TYPE_OPERATION_EQUAL:
        case NODE_TYPE_OPERATION_NOT_EQUAL:
        case NODE_TYPE_OPERATION_BITWISE_AND:
        case NODE_TYPE_OPERATION_BITWISE_XOR:
        case NODE_TYPE_OPERATION_BITWISE_OR:
        case NODE_TYPE_OPERATION_LOGICAL_AND:
        case NODE_TYPE_OPERATION_LOGICAL_XOR:
        case NODE_TYPE_OPERATION_LOGICAL_OR:
        case NODE_TYPE_OPERATION_CONDITIONAL:
        case NODE_TYPE_OPERATION_ASSIGNMENT:
            break;

        default:
            throw snap_logic_exception(QString("expr_node::add_child/children_size/get_child() called on a node which does not support children... (type: %1)").arg(f_type));

        }
#endif
    }

    void verify_unary(variable_t::variable_vector_t const& sub_results)
    {
#ifdef DEBUG
        if(sub_results.size() != 1)
        {
            throw snap_logic_exception(QString("expr_node::execute() found an unary operator (%1) with a number of result which is not 1").arg(f_type));
        }
#endif
    }

    void verify_binary(variable_t::variable_vector_t const& sub_results)
    {
#ifdef DEBUG
        if(sub_results.size() != 2)
        {
            throw snap_logic_exception(QString("expr_node::execute() found an binary operator (%1) with a number of result which is not 2").arg(f_type));
        }
#endif
    }

    safe_node_type_t        f_type;

    QString                 f_name;
    variable_t              f_variable;

    expr_node_vector_t      f_children;
};


functions_t::function_call_table_t const expr_node::internal_functions[] =
{
    { // read the content of a cell
        "cell",
        expr_node::call_cell
    },
    { // check whether a cell exists in a table and row
        "cell_exists",
        expr_node::call_cell_exists
    },
    { // check whether a row exists in a table
        "int64",
        expr_node::call_int64
    },
    { // check whether a row exists in a table
        "row_exists",
        expr_node::call_row_exists
    },
    { // convert the parameter to a string
        "string",
        expr_node::call_string
    },
    { // return length of a string
        "strlen",
        expr_node::call_strlen
    },
    { // retrieve part of a string
        "substr",
        expr_node::call_substr
    },
    { // check whether a named table exists
        "table_exists",
        expr_node::call_table_exists
    },
    {
        nullptr,
        nullptr
    }
};


/** \brief Merge qualified names in one single identifier.
 *
 * This function is used to transform qualified names in one single long
 * identifier. So
 *
 * \code
 * a :: b
 *   :: c
 * \endcode
 *
 * will result in "a::b::c" as one long identifier.
 *
 * \param[in] r  The rule calling this function.
 * \param[in,out] t  The token tree to be tweaked.
 */
void list_qualified_name(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    QSharedPointer<parser::token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    (*t)[0]->set_value((*n)[0]->get_value().toString() + "::" + (*t)[2]->get_value().toString());
}


void list_expr_binary_operation(QSharedPointer<token_node>& t, expr_node::node_type_t operation)
{
    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    expr_node::expr_node_pointer_t l(qSharedPointerDynamicCast<expr_node, parser_user_data>(n0->get_user_data()));

    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t r(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    QSharedPointer<expr_node> v(new expr_node(operation));
    v->add_child(l);
    v->add_child(r);
    t->set_user_data(v);
}

#define LIST_EXPR_BINARY_OPERATION(a, b) \
    void list_expr_##a(rule const& r, QSharedPointer<token_node>& t) \
    { \
        static_cast<void>(r); \
        list_expr_binary_operation(t, expr_node::NODE_TYPE_OPERATION_##b); \
    }

LIST_EXPR_BINARY_OPERATION(multiplicative_multiply, MULTIPLY)
LIST_EXPR_BINARY_OPERATION(multiplicative_divide, DIVIDE)
LIST_EXPR_BINARY_OPERATION(multiplicative_modulo, MODULO)
LIST_EXPR_BINARY_OPERATION(additive_add, ADD)
LIST_EXPR_BINARY_OPERATION(additive_subtract, SUBTRACT)
LIST_EXPR_BINARY_OPERATION(shift_left, SHIFT_LEFT)
LIST_EXPR_BINARY_OPERATION(shift_right, SHIFT_RIGHT)
LIST_EXPR_BINARY_OPERATION(relational_less, LESS)
LIST_EXPR_BINARY_OPERATION(relational_less_or_equal, LESS_OR_EQUAL)
LIST_EXPR_BINARY_OPERATION(relational_greater, GREATER)
LIST_EXPR_BINARY_OPERATION(relational_greater_or_equal, GREATER_OR_EQUAL)
LIST_EXPR_BINARY_OPERATION(relational_minimum, MINIMUM)
LIST_EXPR_BINARY_OPERATION(relational_maximum, MAXIMUM)
LIST_EXPR_BINARY_OPERATION(equality_equal, EQUAL)
LIST_EXPR_BINARY_OPERATION(equality_not_equal, NOT_EQUAL)
LIST_EXPR_BINARY_OPERATION(bitwise_and, BITWISE_AND)
LIST_EXPR_BINARY_OPERATION(bitwise_xor, BITWISE_XOR)
LIST_EXPR_BINARY_OPERATION(bitwise_or, BITWISE_OR)
LIST_EXPR_BINARY_OPERATION(logical_and, LOGICAL_AND)
LIST_EXPR_BINARY_OPERATION(logical_xor, LOGICAL_XOR)
LIST_EXPR_BINARY_OPERATION(logical_or, LOGICAL_OR)



void list_expr_unary_operation(QSharedPointer<token_node>& t, expr_node::node_type_t operation)
{
    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    expr_node::expr_node_pointer_t i(qSharedPointerDynamicCast<expr_node, parser_user_data>(n->get_user_data()));

    expr_node::expr_node_pointer_t v(new expr_node(operation));
    v->add_child(i);
    t->set_user_data(v);
}

#define LIST_EXPR_UNARY_OPERATION(a, b) \
    void list_expr_##a(rule const& r, QSharedPointer<token_node>& t) \
    { \
        static_cast<void>(r); \
        list_expr_unary_operation(t, expr_node::NODE_TYPE_OPERATION_##b); \
    }

LIST_EXPR_UNARY_OPERATION(logical_not, LOGICAL_NOT)
LIST_EXPR_UNARY_OPERATION(bitwise_not, BITWISE_NOT)
LIST_EXPR_UNARY_OPERATION(negate, NEGATE)


void list_expr_conditional(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    expr_node::expr_node_pointer_t c(qSharedPointerDynamicCast<expr_node, parser_user_data>(n0->get_user_data()));

    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t at(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    QSharedPointer<token_node> n4(qSharedPointerDynamicCast<token_node, token>((*t)[4]));
    expr_node::expr_node_pointer_t af(qSharedPointerDynamicCast<expr_node, parser_user_data>(n4->get_user_data()));

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::NODE_TYPE_OPERATION_CONDITIONAL));
    v->add_child(c);
    v->add_child(at);
    v->add_child(af);
    t->set_user_data(v);
}


void list_expr_list(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    expr_node::expr_node_pointer_t l(qSharedPointerDynamicCast<expr_node, parser_user_data>(n0->get_user_data()));

    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t i(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    if(l->get_type() == expr_node::NODE_TYPE_OPERATION_LIST)
    {
        // just add to the existing list
        l->add_child(i);
        t->set_user_data(l);
    }
    else
    {
        // not a list yet, create it
        expr_node::expr_node_pointer_t v(new expr_node(expr_node::NODE_TYPE_OPERATION_LIST));
        v->add_child(l);
        v->add_child(i);
        t->set_user_data(v);
    }
}


void list_expr_identity(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[1]));
    expr_node::expr_node_pointer_t i(qSharedPointerDynamicCast<expr_node, parser_user_data>(n->get_user_data()));

    // we completely ignore the + and (...) they served there purpose already
    t->set_user_data(i);
}


void list_expr_function(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    // the function name is a string
    QSharedPointer<token_node> n0(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    QString const func_name((*n0)[0]->get_value().toString());//(*t)[0]->get_value().toString());

    // at this point this node is an expr_list which only returns the
    // last item as a result, we want all the parameters when calling a
    // function so we remove the parent node below (see loop)
    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t l(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    QSharedPointer<expr_node> v(new expr_node(expr_node::NODE_TYPE_OPERATION_FUNCTION));
    v->set_name(func_name);

    if(l->get_type() == expr_node::NODE_TYPE_OPERATION_LIST)
    {
        int const max_children(l->children_size());
        for(int i(0); i < max_children; ++i)
        {
            v->add_child(l->get_child(i));
        }
    }
    else
    {
        v->add_child(l);
    }

    t->set_user_data(v);
}


void list_expr_true(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::NODE_TYPE_LITERAL_BOOLEAN));
    QtCassandra::QCassandraValue value;
    value.setBoolValue(true);
    variable_t variable;
    variable.set_value(variable_t::EXPR_VARIABLE_TYPE_BOOL, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_false(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::NODE_TYPE_LITERAL_BOOLEAN));
    QtCassandra::QCassandraValue value;
    value.setBoolValue(false);
    variable_t variable;
    variable.set_value(variable_t::EXPR_VARIABLE_TYPE_BOOL, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_string(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    QString const str((*t)[0]->get_value().toString());

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::NODE_TYPE_LITERAL_STRING));
    QtCassandra::QCassandraValue value;
    value.setStringValue(str);
    variable_t variable;
    variable.set_value(variable_t::EXPR_VARIABLE_TYPE_STRING, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_integer(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    int64_t const integer((*t)[0]->get_value().toLongLong());

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::NODE_TYPE_LITERAL_INTEGER));
    QtCassandra::QCassandraValue value;
    value.setInt64Value(integer);
    variable_t variable;
    variable.set_value(variable_t::EXPR_VARIABLE_TYPE_INT64, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_float(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    double const floating_point((*t)[0]->get_value().toDouble());

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::NODE_TYPE_LITERAL_FLOATING_POINT));
    QtCassandra::QCassandraValue value;
    value.setDoubleValue(floating_point);
    variable_t variable;
    variable.set_value(variable_t::EXPR_VARIABLE_TYPE_DOUBLE, value);
    v->set_variable(variable);
    t->set_user_data(v);
}


void list_expr_variable(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    QString const name((*t)[0]->get_value().toString());

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::NODE_TYPE_OPERATION_VARIABLE));
    v->set_name(name);
    t->set_user_data(v);
}


void list_expr_assignment(rule const& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    QString const name((*t)[0]->get_value().toString());

    QSharedPointer<token_node> n2(qSharedPointerDynamicCast<token_node, token>((*t)[2]));
    expr_node::expr_node_pointer_t i(qSharedPointerDynamicCast<expr_node, parser_user_data>(n2->get_user_data()));

    expr_node::expr_node_pointer_t v(new expr_node(expr_node::NODE_TYPE_OPERATION_ASSIGNMENT));
    v->set_name(name);
    v->add_child(i);
    t->set_user_data(v);
}


void list_expr_copy_result(const rule& r, QSharedPointer<token_node>& t)
{
    static_cast<void>(r);

    QSharedPointer<token_node> n(qSharedPointerDynamicCast<token_node, token>((*t)[0]));
    // we don't need the dynamic cast since we don't need to access the data
    t->set_user_data(n->get_user_data());
}



/** \brief Compile a list test to binary.
 *
 * This function transforms a "list test" to byte code (similar to
 * Java byte code if you wish, althgouh really our result is an XML
 * string.) A list test is used to test whether a page is part of a
 * list or not.
 *
 * The \p script parameter is the C-like expression to be transformed.
 *
 * If the script is invalid, the function returns false and the \p program
 * parameter is left untouched.
 *
 * The script is actually a C-like expression.
 *
 * The following defines what is supported in the script (note that this
 * simplified yacc does not show you the priority of the binary operator;
 * we use the C order to the letter):
 *
 * \code
 * // start the parser here
 * start: expr_list
 *
 * // expression
 * expr: '(' expr_list ')'
 *     | expr binary_operator expr
 *     | unary_operator expr
 *     | expr '?' expr ':' expr
 *     | qualified_identifier '(' expr_list ')'
 *     | TOKEN_ID_IDENTIFIER
 *     | TOKEN_ID_STRING
 *     | TOKEN_ID_INTEGER
 *     | TOKEN_ID_FLOAT
 *     | 'true'
 *     | 'false'
 *
 * qualified_identifier: TOKEN_ID_IDENTIFIER
 *                     | qualified_identifier '::' TOKEN_ID_IDENTIFIER
 *
 * expr_list: expr
 *          | expr_list ',' expr
 *
 * // operators
 * unary_operator: '!' | '~' | '+' | '-'
 *
 * binary_operator: '*' | '/' | '%'
 *                | '+' | '-'
 *                | '<<' | '>>'
 *                | '<' | '>' | '<=' | '>=' | '~=' | '<?' | '>?'
 *                | '==' | '!='
 *                | '&'
 *                | '^'
 *                | '|'
 *                | '&&'
 *                | '^^'
 *                | '||'
 *                | ':='
 * \endcode
 *
 * Functions we support:
 *
 * \li cell( \<path>, \<name> [, \<type>] )
 *
 * Read the content of a cell named \<name>. If \<type> is not specified,
 * the cell content is viewed as binary. The program does NOT know what
 * type the cell is. You may cast the content at a later time as in:
 *
 * \code
 * int32_t(cell(path, "stats::counter"))
 * \endcode
 *
 * To specify the table name, use a qualified name as in:
 *
 * \code
 * data::cell(path, "content::modified", int32_t)
 * \endcode
 *
 * \<path> may represent a list of paths in which case only the first path
 * is used by default. The following paths are simply ignored.
 *
 * \li strlen( \<string> )
 *
 * Return the length of the \<string>.
 *
 * \li substr( \<string>, \<start>, \<length> )
 *
 * Return the portion of the \<string> starting at \<start> and with at
 * most \<length> characters.
 * \param[in] script  The script to be compiled to binary code bytes.
 * \param[out] result  The resulting binary code of \p script.
 *
 * \return A pointer to the program tree or nullptr.
 */
expr_node::expr_node_pointer_t compile_expression(QString const& script)
{
    // LEXER

    // lexer object
    parser::lexer lexer;
    lexer.set_input(script);
    parser::keyword keyword_true(lexer, "true");
    parser::keyword keyword_false(lexer, "false");

    // GRAMMAR
    parser::grammar g;

    // qualified_name
    choices qualified_name(&g, "qualified_name");
    qualified_name >>= TOKEN_ID_IDENTIFIER // keep identifier as is
                     | qualified_name >> "::" >> TOKEN_ID_IDENTIFIER
                                                         >= list_qualified_name
    ;

    // forward definitions
    choices expr(&g, "expr");
    choices conditional_expr(&g, "conditional_expr");

    // expr_list
    choices expr_list(&g, "expr_list");
    expr_list >>= expr
                                                    >= list_expr_copy_result
                | expr_list >> "," >> expr
                                                    >= list_expr_list
    ;

    // unary_expr
    choices unary_expr(&g, "unary_expr");
    unary_expr >>= "!" >> unary_expr
                                                    >= list_expr_logical_not
                 | "~" >> unary_expr
                                                    >= list_expr_bitwise_not
                 | "+" >> unary_expr
                                                    >= list_expr_identity
                 | "-" >> unary_expr
                                                    >= list_expr_negate
                 | "(" >> expr_list >> ")"
                                                    >= list_expr_identity
                 | qualified_name >> "(" >> expr_list >> ")"
                                                    >= list_expr_function
                 | TOKEN_ID_IDENTIFIER
                                                    >= list_expr_variable
                 | keyword_true
                                                    >= list_expr_true
                 | keyword_false
                                                    >= list_expr_false
                 | TOKEN_ID_STRING
                                                    >= list_expr_string
                 | TOKEN_ID_INTEGER
                                                    >= list_expr_integer
                 | TOKEN_ID_FLOAT
                                                    >= list_expr_float
    ;

    // multiplicative_expr
    choices multiplicative_expr(&g, "multiplicative_expr");
    multiplicative_expr >>= unary_expr
                                                    >= list_expr_copy_result
                          | multiplicative_expr >> "*" >> unary_expr
                                                    >= list_expr_multiplicative_multiply
                          | multiplicative_expr >> "/" >> unary_expr
                                                    >= list_expr_multiplicative_divide
                          | multiplicative_expr >> "%" >> unary_expr
                                                    >= list_expr_multiplicative_modulo
    ;

    // additive_expr
    choices additive_expr(&g, "additive_expr");
    additive_expr >>= multiplicative_expr
                                                    >= list_expr_copy_result
                    | additive_expr >> "+" >> multiplicative_expr
                                                    >= list_expr_additive_add
                    | additive_expr >> "-" >> multiplicative_expr
                                                    >= list_expr_additive_subtract
    ;

    // shift_expr
    choices shift_expr(&g, "shift_expr");
    shift_expr >>= additive_expr
                                                    >= list_expr_copy_result
                 | shift_expr >> "<<" >> additive_expr
                                                    >= list_expr_shift_left
                 | shift_expr >> ">>" >> additive_expr
                                                    >= list_expr_shift_right
    ;

    // relational_expr
    choices relational_expr(&g, "relational_expr");
    relational_expr >>= shift_expr
                                                    >= list_expr_copy_result
                   | relational_expr >> "<" >> shift_expr
                                                    >= list_expr_relational_less
                   | relational_expr >> "<=" >> shift_expr
                                                    >= list_expr_relational_less_or_equal
                   | relational_expr >> ">" >> shift_expr
                                                    >= list_expr_relational_greater
                   | relational_expr >> ">=" >> shift_expr
                                                    >= list_expr_relational_greater_or_equal
                   | relational_expr >> "<?" >> shift_expr
                                                    >= list_expr_relational_minimum
                   | relational_expr >> ">?" >> shift_expr
                                                    >= list_expr_relational_maximum
    ;

    // equality_expr
    choices equality_expr(&g, "equality_expr");
    equality_expr >>= relational_expr
                                                    >= list_expr_copy_result
                   | equality_expr >> "==" >> relational_expr
                                                    >= list_expr_equality_equal
                   | equality_expr >> "!=" >> relational_expr
                                                    >= list_expr_equality_not_equal
    ;

    // bitwise_and_expr
    choices bitwise_and_expr(&g, "bitwise_and_expr");
    bitwise_and_expr >>= equality_expr
                                                    >= list_expr_copy_result
                       | bitwise_and_expr >> "&" >> equality_expr
                                                    >= list_expr_bitwise_and
    ;

    // bitwise_xor_expr
    choices bitwise_xor_expr(&g, "bitwise_xor_expr");
    bitwise_xor_expr >>= bitwise_and_expr
                                                    >= list_expr_copy_result
                       | bitwise_xor_expr >> "^" >> bitwise_and_expr
                                                    >= list_expr_bitwise_xor
    ;

    // bitwise_or_expr
    choices bitwise_or_expr(&g, "bitwise_or_expr");
    bitwise_or_expr >>= bitwise_xor_expr
                                                    >= list_expr_copy_result
                      | bitwise_or_expr >> "|" >> bitwise_xor_expr
                                                    >= list_expr_bitwise_or
    ;

    // logical_and_expr
    choices logical_and_expr(&g, "logical_and_expr");
    logical_and_expr >>= bitwise_or_expr
                                                    >= list_expr_copy_result
                       | logical_and_expr >> "&&" >> bitwise_or_expr
                                                    >= list_expr_logical_and
    ;

    // logical_xor_expr
    choices logical_xor_expr(&g, "logical_xor_expr");
    logical_xor_expr >>= logical_and_expr
                                                    >= list_expr_copy_result
                       | logical_xor_expr >> "^^" >> logical_and_expr
                                                    >= list_expr_logical_xor
    ;

    // logical_or_expr
    choices logical_or_expr(&g, "logical_or_expr");
    logical_or_expr >>= logical_xor_expr
                                                    >= list_expr_copy_result
                      | logical_or_expr >> "||" >> logical_xor_expr
                                                    >= list_expr_logical_or
    ;

    // conditional_expr
    // logical-OR-expression ? expression : conditional-expression
    conditional_expr >>= logical_or_expr
                                                    >= list_expr_copy_result
                      | conditional_expr >> "?" >> expr >> ":" >> logical_or_expr
                                                    >= list_expr_conditional
    ;

    // assignment
    // (this is NOT a C compatible assignment, hence I used ":=")
    choices assignment(&g, "assignment");
    assignment >>= conditional_expr
                                                    >= list_expr_copy_result
                 | TOKEN_ID_IDENTIFIER >> ":=" >> conditional_expr
                                                    >= list_expr_assignment
    ;

    // expr
    expr >>= assignment
                                                    >= list_expr_copy_result
    ;

//std::cerr << expr.to_string() << "\n";
//std::cerr << expr_list.to_string() << "\n";
//std::cerr << unary_expr.to_string() << "\n";

    if(!g.parse(lexer, expr))
    {
        std::cerr << "error #" << static_cast<int>(lexer.get_error_code())
                  << " on line " << lexer.get_error_line()
                  << ": " << lexer.get_error_message()
                  << std::endl;

        expr_node::expr_node_pointer_t null;
        return null;
    }

    QSharedPointer<token_node> r(g.get_result());
    return qSharedPointerDynamicCast<expr_node, parser_user_data>(r->get_user_data());
}



bool expr::compile(QString const& expression)
{
    f_program_tree = compile_expression(expression);
    return f_program_tree;
}


QByteArray expr::serialize() const
{
    QByteArray result;

    QBuffer archive(&result);
    archive.open(QIODevice::WriteOnly);
    QtSerialization::QWriter w(archive, "expr", expr_node::LIST_TEST_NODE_MAJOR_VERSION, expr_node::LIST_TEST_NODE_MINOR_VERSION);
    static_cast<expr_node *>(&*f_program_tree)->write(w);

    return result;
}


void expr::unserialize(QByteArray const& serialized_code)
{
    QByteArray non_const(serialized_code);
    QBuffer archive(&non_const);
    archive.open(QIODevice::ReadOnly);
    QtSerialization::QReader r(archive);
    static_cast<expr_node *>(&*f_program_tree)->read(r);
}


void expr::execute(variable_t& result, variable_t::variable_map_t& variables, functions_t& functions)
{
    if(!f_program_tree)
    {
        throw snap_expr_exception_unknown_function("cannot execute an empty program");
    }
    variable_t pi("pi");
    pi.set_value(M_PI);
    variables["pi"] = pi;
    static_cast<expr_node *>(&*f_program_tree)->execute(result, variables, functions);
}


void expr::set_cassandra_context(QtCassandra::QCassandraContext::pointer_t context)
{
    g_context = context;
}


} // namespace snap_expr
} // snap

// vim: ts=4 sw=4 et
