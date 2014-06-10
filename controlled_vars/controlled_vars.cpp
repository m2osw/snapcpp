//
// File:	controlled_vars.cpp
// Object:	Generates the controlled_vars.h header file
//
// Copyright:	Copyright (c) 2005-2012 Made to Order Software Corp.
//		All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

//
// Usage:
//
// This tool can be compiled with a very simple make controller_vars
// if you can manually define the config.h (from config.h.in) which
// is quite simple. Otherwise use cmake as in:
//
//	mkdir BUILD
//	(cd BUILD; cmake ..)
//	make -C BUILD
//
// Then run it to get the mo_controlled.h file created as in:
//
// 	controlled_vars >controlled_vars.h
//
// The result is a set of lengthy template header files of basic types to be
// used with boundaries. Since these are templates, 99.9% of the code goes
// away when the compilation is done.
//
// OS detection in the controlled_vars.h file is done with macros:
// (see http://sourceforge.net/p/predef/wiki/OperatingSystems/
// for a complete list)
//
//  Operating System	Macro
//
//  Mac OS/X		__APPLE__
//  Visual Studio	_MSC_VER
//

#include	"config.h"
#include	<stdlib.h>
#include	<stdio.h>
#ifdef HAVE_UNISTD_H
#include	<unistd.h>
#endif
#include	<string.h>
#include	<stdint.h>



int no_bool_constructors = 0;


// current output file
FILE *out = nullptr;

namespace
{
uint32_t	FLAG_TYPE_INT		= 0x00000001;
uint32_t	FLAG_TYPE_FLOAT		= 0x00000002;
}

struct TYPES
{
	char const *	name;
	char const *	short_name;
	char const *	long_name;
	uint32_t	flags;
	char const *	condition;
};
typedef struct TYPES	types_t;

types_t const g_types[] =
{
	{ "bool",          "bool",        "int32_t",       FLAG_TYPE_INT,   0 }, /* this generates quite many problems as operator input */
	{ "char",          "char",        "int32_t",       FLAG_TYPE_INT,   0 },
	{ "signed char",   "schar",       "int32_t",       FLAG_TYPE_INT,   0 },
	{ "unsigned char", "uchar",       "int32_t",       FLAG_TYPE_INT,   0 },
	/* in C++, wchar_t is a basic type, not a typedef; however, some compilers still allow changing the default and wchar_t then becomes a typedef */
	{ "wchar_t",       "wchar",       "int32_t",       FLAG_TYPE_INT,   "#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))" },
	{ "int16_t",       "int16",       "int32_t",       FLAG_TYPE_INT,   0 },
	{ "uint16_t",      "uint16",      "int32_t",       FLAG_TYPE_INT,   0 },
	{ "int32_t",       "int32",       "int32_t",       FLAG_TYPE_INT,   0 },
	{ "uint32_t",      "uint32",      "int32_t",       FLAG_TYPE_INT,   0 },
	{ "long",          "plain_long",  "int64_t",       FLAG_TYPE_INT,   "#if UINT_MAX == ULONG_MAX" },
	{ "unsigned long", "plain_ulong", "uint64_t",      FLAG_TYPE_INT,   "#if UINT_MAX == ULONG_MAX" },
	{ "int64_t",       "int64",       "int64_t",       FLAG_TYPE_INT,   0 },
	{ "uint64_t",      "uint64",      "uint64_t",      FLAG_TYPE_INT,   0 },
	{ "float",         "float",       "double",        FLAG_TYPE_FLOAT, 0 },
	{ "double",        "double",      "double",        FLAG_TYPE_FLOAT, 0 }, /* "long double" would be problematic here */
	{ "long double",   "longdouble",  "long double",   FLAG_TYPE_FLOAT, 0 },
	// The following were for Apple computers with a PPC
	//{ "size_t",        "size",        "uint64_t",      FLAG_TYPE_INT,   "#ifdef __APPLE__" },
	//{ "time_t",        "time",        "int64_t",       FLAG_TYPE_INT,   "#ifdef __APPLE__" }
};
#define	TYPES_ALL	(sizeof(g_types) / sizeof(g_types[0]))


types_t const g_ptr_types[] =
{
	{ "signed char",   "schar",       "int32_t",       FLAG_TYPE_INT,   0 },
	{ "unsigned char", "uchar",       "int32_t",       FLAG_TYPE_INT,   0 },
	/* in C++, wchar_t is a basic type, not a typedef; however, some compilers still allow changing the default and wchar_t then becomes a typedef */
	{ "wchar_t",       "wchar",       "int32_t",       FLAG_TYPE_INT,   "#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))" },
	{ "int16_t",       "int16",       "int32_t",       FLAG_TYPE_INT,   0 },
	{ "uint16_t",      "uint16",      "int32_t",       FLAG_TYPE_INT,   0 },
	{ "int32_t",       "int32",       "int64_t",       FLAG_TYPE_INT,   0 },
	{ "uint32_t",      "uint32",      "int64_t",       FLAG_TYPE_INT,   0 },
	{ "long",          "plain_long",  "int64_t",       FLAG_TYPE_INT,   "#if UINT_MAX == ULONG_MAX" },
	{ "unsigned long", "plain_ulong", "uint64_t",      FLAG_TYPE_INT,   "#if UINT_MAX == ULONG_MAX" },
	{ "int64_t",       "int64",       "int64_t",       FLAG_TYPE_INT,   0 },
	{ "uint64_t",      "uint64",      "uint64_t",      FLAG_TYPE_INT,   0 },
	{ "size_t",        "size",        "uint64_t",      FLAG_TYPE_INT,   "#ifdef __APPLE__" },
};
#define	PTR_TYPES_ALL	(sizeof(g_ptr_types) / sizeof(g_ptr_types[0]))


namespace
{
uint32_t	FLAG_HAS_VOID		= 0x00000001;
uint32_t	FLAG_HAS_DOINIT		= 0x00000002;
uint32_t	FLAG_HAS_INITFLG	= 0x00000004;
uint32_t	FLAG_HAS_DEFAULT	= 0x00000008;
uint32_t	FLAG_HAS_LIMITS		= 0x00000010;
uint32_t	FLAG_HAS_FLOAT		= 0x00000020;
uint32_t	FLAG_HAS_DEBUG_ALREADY	= 0x00000040;
uint32_t	FLAG_HAS_ENUM         	= 0x00000080;

uint32_t	FLAG_HAS_RETURN_T	= 0x00010000;
uint32_t	FLAG_HAS_RETURN_BOOL	= 0x00020000;
uint32_t	FLAG_HAS_NOINIT		= 0x00040000;
uint32_t	FLAG_HAS_LIMITED	= 0x00080000;
uint32_t	FLAG_HAS_NOFLOAT	= 0x00100000;
uint32_t	FLAG_HAS_PTR		= 0x00200000;
uint32_t	FLAG_HAS_RETURN_PRIMARY	= 0x00400000;
uint32_t	FLAG_HAS_REFERENCE	= 0x00800000;
uint32_t	FLAG_HAS_CONST    	= 0x01000000;
}


struct OP_T
{
	char const *	name;
	uint32_t	flags;
};
typedef struct OP_T	op_t;

op_t const g_generic_operators[] =
{
	{ "=",   FLAG_HAS_NOINIT | FLAG_HAS_LIMITED },
	{ "*=",  FLAG_HAS_LIMITED },
	{ "/=",  FLAG_HAS_LIMITED },
	{ "%=",  FLAG_HAS_LIMITED | FLAG_HAS_NOFLOAT },
	{ "+=",  FLAG_HAS_LIMITED },
	{ "-=",  FLAG_HAS_LIMITED },
	{ "<<=", FLAG_HAS_LIMITED | FLAG_HAS_NOFLOAT },
	{ ">>=", FLAG_HAS_LIMITED | FLAG_HAS_NOFLOAT },
	{ "&=",  FLAG_HAS_LIMITED | FLAG_HAS_NOFLOAT },
	{ "|=",  FLAG_HAS_LIMITED | FLAG_HAS_NOFLOAT },
	{ "^=",  FLAG_HAS_LIMITED | FLAG_HAS_NOFLOAT },
	{ "*",   FLAG_HAS_RETURN_T | FLAG_HAS_CONST },
	{ "/",   FLAG_HAS_RETURN_T | FLAG_HAS_CONST },
	{ "%",   FLAG_HAS_RETURN_T | FLAG_HAS_CONST | FLAG_HAS_NOFLOAT },
	{ "+",   FLAG_HAS_RETURN_T | FLAG_HAS_CONST },
	{ "-",   FLAG_HAS_RETURN_T | FLAG_HAS_CONST },
	{ "<<",  FLAG_HAS_RETURN_T | FLAG_HAS_CONST | FLAG_HAS_NOFLOAT },
	{ ">>",  FLAG_HAS_RETURN_T | FLAG_HAS_CONST | FLAG_HAS_NOFLOAT },
	{ "&",   FLAG_HAS_RETURN_T | FLAG_HAS_CONST | FLAG_HAS_NOFLOAT },
	{ "|",   FLAG_HAS_RETURN_T | FLAG_HAS_CONST | FLAG_HAS_NOFLOAT },
	{ "^",   FLAG_HAS_RETURN_T | FLAG_HAS_CONST | FLAG_HAS_NOFLOAT },
	{ "==",  FLAG_HAS_RETURN_BOOL | FLAG_HAS_CONST },
	{ "!=",  FLAG_HAS_RETURN_BOOL | FLAG_HAS_CONST },
	{ "<",   FLAG_HAS_RETURN_BOOL | FLAG_HAS_CONST },
	{ "<=",  FLAG_HAS_RETURN_BOOL | FLAG_HAS_CONST },
	{ ">",   FLAG_HAS_RETURN_BOOL | FLAG_HAS_CONST },
	{ ">=",  FLAG_HAS_RETURN_BOOL | FLAG_HAS_CONST }
};
#define	GENERIC_OPERATORS_MAX		(sizeof(g_generic_operators) / sizeof(g_generic_operators[0]))


op_t const g_generic_ptr_operators[] =
{
	{ "=",   FLAG_HAS_NOINIT | FLAG_HAS_PTR },
	{ "+=",  FLAG_HAS_RETURN_PRIMARY },
	{ "-=",  FLAG_HAS_RETURN_PRIMARY },
	{ "+",   FLAG_HAS_RETURN_PRIMARY },
	{ "-",   FLAG_HAS_RETURN_PRIMARY },
	{ "==",  FLAG_HAS_RETURN_BOOL | FLAG_HAS_PTR },
	{ "!=",  FLAG_HAS_RETURN_BOOL | FLAG_HAS_PTR },
	{ "<",   FLAG_HAS_RETURN_BOOL | FLAG_HAS_PTR },
	{ "<=",  FLAG_HAS_RETURN_BOOL | FLAG_HAS_PTR },
	{ ">",   FLAG_HAS_RETURN_BOOL | FLAG_HAS_PTR },
	{ ">=",  FLAG_HAS_RETURN_BOOL | FLAG_HAS_PTR }
};
#define	GENERIC_PTR_OPERATORS_MAX	(sizeof(g_generic_ptr_operators) / sizeof(g_generic_ptr_operators[0]))



void create_operator(const char *name, const char *op, const char *type, long flags, char const *long_type)
{
	const char *right;
	int direct;

	fprintf(out, "\t");
	if((flags & FLAG_HAS_RETURN_BOOL) != 0) {
		fprintf(out, "bool");
		direct = 1;
	}
	else if((flags & FLAG_HAS_ENUM) != 0 && long_type) {
		fprintf(out, "%s", long_type);
		direct = 1;
	}
	else if((flags & FLAG_HAS_RETURN_T) != 0) {
		fprintf(out, "T");
		direct = 1;
	}
	else if((flags & FLAG_HAS_RETURN_PRIMARY) != 0) {
		fprintf(out, "primary_type_t");
		direct = 1;
	}
	else {
		fprintf(out, "%s_init&", name);
		direct = 0;
	}
	fprintf(out, " operator %s (", op);
	if(type == 0) {
		fprintf(out, "%s_init const& n", name);
		right = "n.f_value";
	}
	else {
		fprintf(out, "%s v", type);
		right = "v";
	}
	fprintf(out, ")%s {", (flags & FLAG_HAS_CONST) != 0 ? " const" : "");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		if((flags & FLAG_HAS_NOINIT) == 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		else {
			fprintf(out, " f_initialized = true;");
		}
		if(type == 0) {
			fprintf(out, " if(!n.f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
	}
	if((flags & FLAG_HAS_LIMITS) != 0 && (flags & FLAG_HAS_LIMITED) != 0) {
		char buf[4];
		int i;
		const char *fmt;
		for(i = 0; op[i + 1] != '\0'; ++i) {
			buf[i] = op[i];
		}
		buf[i] = '\0';
		if(i == 0) { // op == "="
			// the first %s is set to "" (i.e. ignored)
			fmt = "%s%s";
		}
		else {
			fmt = "f_value %s %s";
		}
		if(direct) {
			fprintf(out, " return f_value = check(");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
			fprintf(out, fmt, buf, right);
#pragma GCC diagnostic pop
			fprintf(out, ");");
		}
		else {
			fprintf(out, " f_value = check(");
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
			fprintf(out, fmt, buf, right);
#pragma GCC diagnostic pop
			fprintf(out, ");");
			fprintf(out, " return *this;");
		}
	}
	else {
		if(direct) {
			fprintf(out, " return f_value %s %s;", op, right);
		}
		else {
			fprintf(out, " f_value %s %s;", op, right);
			fprintf(out, " return *this;");
		}
	}
	fprintf(out, " }\n");
}


void create_ptr_operator(const char *name, const char *op, const char *type, long flags)
{
	const char *right;
	int direct;

	fprintf(out, "\t");
	if((flags & FLAG_HAS_RETURN_BOOL) != 0) {
		fprintf(out, "bool");
		direct = 1;
	}
	else if((flags & FLAG_HAS_RETURN_T) != 0) {
		fprintf(out, "T");
		direct = 1;
	}
	else if((flags & FLAG_HAS_RETURN_PRIMARY) != 0) {
		fprintf(out, "primary_type_t");
		direct = 1;
	}
	else {
		fprintf(out, "%s_init&", name);
		direct = 0;
	}
	fprintf(out, " operator %s (", op);
	if(type == 0) {
		fprintf(out, "const %s_init& n", name);
		right = "n.f_ptr";
	}
	else {
		fprintf(out, "%s v", type);
		right = "v";
	}
	fprintf(out, ") {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		if((flags & FLAG_HAS_NOINIT) == 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		else {
			fprintf(out, " f_initialized = true;");
		}
		if(type == 0) {
			fprintf(out, " if(!n.f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
	}
	if(direct) {
		fprintf(out, " return f_ptr %s %s;", op, right);
	}
	else {
		fprintf(out, " f_ptr %s %s;", op, right);
		fprintf(out, " return *this;");
	}
	fprintf(out, " }\n");
}


void create_ptr_operator_for_ptr(const char *name, const char *op, const char *type, long flags)
{
	const char *right;
	int direct;

	fprintf(out, "\t");
	if(*op == 'r') {
		fprintf(out, "void");
		direct = 2;
	}
	else if((flags & FLAG_HAS_RETURN_BOOL) != 0) {
		fprintf(out, "bool");
		direct = 1;
	}
	else if((flags & FLAG_HAS_RETURN_T) != 0) {
		fprintf(out, "T");
		direct = 1;
	}
	else if((flags & FLAG_HAS_RETURN_PRIMARY) != 0) {
		fprintf(out, "primary_type_t");
		direct = 1;
	}
	else {
		fprintf(out, "%s_init&", name);
		direct = 0;
	}
	if(*op == 'r') {
		fprintf(out, " reset(");
		op = "=";
	}
	else {
		fprintf(out, " operator %s (", op);
	}
	if(type == 0) {
		fprintf(out, "const %s_init%sp", name, ((flags & FLAG_HAS_REFERENCE) != 0 ? "& " : " *"));
		if((flags & FLAG_HAS_REFERENCE) != 0) {
			right = "p.f_ptr";
		}
		else {
			right = "p->f_ptr";
		}
	}
	else {
		fprintf(out, "%s p", type);
		if((flags & FLAG_HAS_REFERENCE) != 0) {
			right = "&p";
		}
		else {
			right = "p";
		}
	}
	fprintf(out, ") {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		if((flags & FLAG_HAS_NOINIT) == 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		if(type == 0) {
			fprintf(out, " if(!p%sf_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");", (flags & FLAG_HAS_REFERENCE) == 0 ? "->" : ".");
		}
	}
	if(type == 0) {
		// this is a bit extra since we're testing the input and
		// not the data of this object
		fprintf(out, " if(%sp == 0) throw controlled_vars_error_null_pointer(\"dereferencing a null pointer\");", (flags & FLAG_HAS_REFERENCE) == 0 ? "" : "&");
	}
	if((flags & FLAG_HAS_INITFLG) != 0 && (flags & FLAG_HAS_NOINIT) != 0) {
		fprintf(out, " f_initialized = true;");
	}
	switch(direct) {
	default: //case 0:
		fprintf(out, " f_ptr %s %s;", op, right);
		fprintf(out, " return *this;");
		break;

	case 1:
		fprintf(out, " return f_ptr %s %s;", op, right);
		break;

	case 2:
		fprintf(out, " f_ptr %s %s;", op, right);
		break;

	}
	fprintf(out, " }\n");
}


void create_all_operators(const char *name, long flags)
{
	const op_t	*op;
	unsigned long	o, t, f;

	for(o = 0; o < GENERIC_OPERATORS_MAX; ++o) {
		op = g_generic_operators + o;
		f = flags | op->flags;
		// test to avoid the auto_init& operator %= (auto_init& v);
		// and other integer only operators.
		if((f & FLAG_HAS_FLOAT) == 0 || (f & FLAG_HAS_NOFLOAT) == 0) {
			create_operator(name, op->name, 0, f, nullptr);
		}
		/* IMPORTANT:
		 *	Here we were skipping the type bool, now there is a
		 *	command line option and by default we do not skip it.
		 */
		for(t = (no_bool_constructors == 1 ? 1 : 0); t < TYPES_ALL; ++t) {
			// test to avoid all the operators that are not float compatible
			// (i.e. bitwise operators, modulo)
			if((f & FLAG_HAS_NOFLOAT) == 0 || ((f & FLAG_HAS_FLOAT) == 0 && (g_types[t].flags & FLAG_TYPE_FLOAT) == 0)) {
				if(g_types[t].condition) {
					fprintf(out, "%s\n", g_types[t].condition);
				}
				create_operator(name, op->name, g_types[t].name, f, nullptr);
				if(g_types[t].condition) {
					fprintf(out, "#endif\n");
				}
			}
		}
	}
}


void create_all_enum_operators(const char *name, long flags)
{
	const op_t	*op;
	unsigned long	o, t, f;

	for(o = 0; o < GENERIC_OPERATORS_MAX; ++o) {
		op = g_generic_operators + o;
		f = flags | op->flags;
		// test to avoid the auto_init& operator %= (auto_init& v);
		// and other integer only operators.
		if(((f & FLAG_HAS_FLOAT) == 0 || (f & FLAG_HAS_NOFLOAT) == 0)
		&& (f & FLAG_HAS_LIMITED) == 0) {
			create_operator(name, op->name, 0, f, "int32_t");
		}
		/* IMPORTANT:
		 *	Here we were skipping the type bool, now there is a
		 *	command line option and by default we do not skip it
		 *	except for comparison tests which are in conflict
		 *	with testing with the enumeration type, somehow.
		 */
		t = no_bool_constructors == 1
			|| strcmp(op->name, "==") == 0
			|| strcmp(op->name, "!=") == 0
			|| strcmp(op->name, "<") == 0
			|| strcmp(op->name, "<=") == 0
			|| strcmp(op->name, ">") == 0
			|| strcmp(op->name, ">=") == 0
				? 1 : 0;
		for(; t < TYPES_ALL; ++t) {
			// test to avoid all the operators that are not float compatible
			// (i.e. bitwise operators, modulo)
			if(((f & FLAG_HAS_NOFLOAT) == 0 || ((f & FLAG_HAS_FLOAT) == 0 && (g_types[t].flags & FLAG_TYPE_FLOAT) == 0))
			&& (f & FLAG_HAS_LIMITED) == 0) {
				if(g_types[t].condition) {
					fprintf(out, "%s\n", g_types[t].condition);
				}
				create_operator(name, op->name, g_types[t].name, f, g_types[t].long_name);
				if(g_types[t].condition) {
					fprintf(out, "#endif\n");
				}
			}
		}
	}
	create_operator(name, "==", "T", f | FLAG_HAS_CONST, nullptr);
	create_operator(name, "!=", "T", f | FLAG_HAS_CONST, nullptr);
	create_operator(name, "<", "T", f | FLAG_HAS_CONST, nullptr);
	create_operator(name, "<=", "T", f | FLAG_HAS_CONST, nullptr);
	create_operator(name, ">", "T", f | FLAG_HAS_CONST, nullptr);
	create_operator(name, ">=", "T", f | FLAG_HAS_CONST, nullptr);

	// our create_operator does not support the following so we do it
	// here as is:
	fprintf(out, "template<class Q = T>\n");
	fprintf(out, "typename std::enable_if<!std::is_fundamental<Q>::value, Q>::type operator == (bool v) const { return f_value == v; }\n");
	fprintf(out, "template<class Q = T>\n");
	fprintf(out, "typename std::enable_if<!std::is_fundamental<Q>::value, Q>::type operator >= (bool v) const { return f_value == v; }\n");
	fprintf(out, "template<class Q = T>\n");
	fprintf(out, "typename std::enable_if<!std::is_fundamental<Q>::value, Q>::type operator > (bool v) const { return f_value == v; }\n");
	fprintf(out, "template<class Q = T>\n");
	fprintf(out, "typename std::enable_if<!std::is_fundamental<Q>::value, Q>::type operator <= (bool v) const { return f_value == v; }\n");
	fprintf(out, "template<class Q = T>\n");
	fprintf(out, "typename std::enable_if<!std::is_fundamental<Q>::value, Q>::type operator < (bool v) const { return f_value == v; }\n");
	fprintf(out, "template<class Q = T>\n");
	fprintf(out, "typename std::enable_if<!std::is_fundamental<Q>::value, Q>::type operator != (bool v) const { return f_value == v; }\n");

}


void create_unary_operators(const char *name, long flags)
{
	int		i;
	const char	*s;

	// NOTE: max i can be either 2 or 4
	//	 at this time, we don't want to have the T * operators
	//	 instead we'll have a set of ptr() functions
	for(i = 0; i < 2; ++i) {
		fprintf(out, "\toperator T%s ()%s {",
				i & 2 ? " *" : "",
				i & 1 ? "" : " const");
		// NOTE: we want to change the following test for T *
		//	 but it requires a reference!!!
		//	 (also, we use ptr() instead for now)
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " return %sf_value;", i & 2 ? "&" : "");
		fprintf(out, " }\n");
	}

	// C++ casts can be annoying to write so make a value() function available too
	fprintf(out, "\tT value() const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
	}
	fprintf(out, " return f_value; }\n");

	for(i = 0; i < 2; ++i) {
		s = i & 1 ? "" : "const ";
		fprintf(out, "\t%sT * ptr() %s{", s, s);
		// NOTE: we want to change the following test for T *
		//	 but it requires a reference!!!
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " return &f_value; }\n");
	}

	fprintf(out, "\tbool operator ! () const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
	}
	fprintf(out, " return !f_value; }\n");

	const char *op = "~+-";
	if(flags & FLAG_HAS_FLOAT) {
		op = "+-";
	}
	int max = strlen(op);
	for(i = 0; i < max; ++i) {
		fprintf(out, "\tT operator %c () const {", op[i]);
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " return %cf_value; }\n", op[i]);
	}

	const char *limits;
	if((flags & FLAG_HAS_LIMITS) != 0) {
		limits = ", min, max";
	}
	else {
		limits = "";
	}

	// NOTE: operator ++/-- () -> ++/--var
	//	 operator ++/-- (int)  -> var++/--
	for(i = 0; i < 4; ++i) {
		fprintf(out, "\t%s_init%s operator %s (%s) {",
			name,
			i & 1 ? "" : "&",
			i & 2 ? "--" : "++",
			i & 1 ? "int" : "");
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		if(i & 1) {
			fprintf(out, " %s_init<T%s> result(*this);", name, limits);
		}
		if((flags & FLAG_HAS_LIMITS) != 0) {
			// in this case we only need to check against one bound
			if(i & 2) {
				fprintf(out, " if(f_value <= min)");
			}
			else {
				fprintf(out, " if(f_value >= max)");
			}
			fprintf(out, " throw controlled_vars_error_out_of_bounds(\"%s would render value out of bounds\");", i & 2 ? "--" : "++");
			fprintf(out, " %sf_value;", i & 2 ? "--" : "++");
		}
		else {
			fprintf(out, " %sf_value;", i & 2 ? "--" : "++");
		}
		if(i & 1) {
			fprintf(out, " return result;");
		}
		else {
			fprintf(out, " return *this;");
		}
		fprintf(out, " }\n");
	}
}


void create_unary_enum_operators(const char *name, long flags)
{
	int		i;
	const char	*s;

	static_cast<void>(name);

	// NOTE: max i can be either 2 or 4
	//	 at this time, we don't want to have the T * operators
	//	 instead we'll have a set of ptr() functions
	for(i = 0; i < 2; ++i) {
		fprintf(out, "\toperator T%s ()%s {",
				i & 2 ? " *" : "",
				i & 1 ? "" : " const");
		// NOTE: we want to change the following test for T *
		//	 but it requires a reference!!!
		//	 (also, we use ptr() instead for now)
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " return %sf_value;", i & 2 ? "&" : "");
		fprintf(out, " }\n");
	}

	// C++ casts can be annoying to write so make a value() function available too
	fprintf(out, "\tT value() const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
	}
	fprintf(out, " return f_value; }\n");

	for(i = 0; i < 2; ++i) {
		s = i & 1 ? "" : "const ";
		fprintf(out, "\t%sT * ptr() %s{", s, s);
		// NOTE: we want to change the following test for T *
		//	 but it requires a reference!!!
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " return &f_value; }\n");
	}

	// This does not work with 'operator T () const'
	//fprintf(out, "\toperator bool () const {");
	//if((flags & FLAG_HAS_INITFLG) != 0) {
	//	fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
	//}
	//fprintf(out, " return f_value != static_cast<T>(0); }\n");

	fprintf(out, "\tbool operator ! () const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
	}
	fprintf(out, " return !f_value; }\n");

	const char *op = "~+-";
	if(flags & FLAG_HAS_FLOAT) {
		op = "+-";
	}
	int max = strlen(op);
	for(i = 0; i < max; ++i) {
		fprintf(out, "\tint operator %c () const {", op[i]);
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " return %cf_value; }\n", op[i]);
	}
}


void create_unary_ptr_operators(const char *name, long flags)
{
	int		i;
	const char	*s;

	for(i = 0; i < 2; ++i) {
		fprintf(out, "\toperator primary_type_t ()%s {",
				i & 1 ? "" : " const");
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " return %sf_ptr;", i & 2 ? "&" : "");
		fprintf(out, " }\n");
	}

	// C++ casts can be annoying to write so make a value() function available too
	fprintf(out, "\tprimary_type_t value() const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
	}
	fprintf(out, " return f_ptr; }\n");

	for(i = 0; i < 2; ++i) {
		s = i & 1 ? "" : "const ";

		fprintf(out, "\tT *get() %s {", s);
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " return f_ptr; }\n");

		fprintf(out, "\tprimary_type_t *ptr() %s {", s);
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " return &f_ptr; }\n");

		fprintf(out, "\tT *operator -> () %s {", s);
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " if(f_ptr == 0) throw controlled_vars_error_null_pointer(\"dereferencing a null pointer\");");
		fprintf(out, " return f_ptr; }\n");

		fprintf(out, "\t%s T& operator * () %s {",s, s);
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " if(f_ptr == 0) throw controlled_vars_error_null_pointer(\"dereferencing a null pointer\");");
		fprintf(out, " return *f_ptr; }\n");

		fprintf(out, "\t%s T& operator [] (int index) %s {",s, s);
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		fprintf(out, " if(f_ptr == 0) throw controlled_vars_error_null_pointer(\"dereferencing a null pointer\");");
		// unfortunately we cannot check bounds as these were not indicated to us
		fprintf(out, " return f_ptr[index]; }\n");
	}

	fprintf(out, "\tvoid swap(%s_init& p) {", name);
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
	}
	fprintf(out, " primary_type_t n(f_ptr); f_ptr = p.f_ptr; p.f_ptr = n; }\n");

	fprintf(out, "\toperator bool () const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
	}
	fprintf(out, " return f_ptr != 0; }\n");

	fprintf(out, "\tbool operator ! () const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
	}
	fprintf(out, " return f_ptr == 0; }\n");

	// NOTE: operator ++/-- () -> ++/--var
	//	 operator ++/-- (int)  -> var++/--
	for(i = 0; i < 4; ++i) {
		fprintf(out, "\t%s_init%s operator %s (%s) {",
			name,
			i & 1 ? "" : "&",
			i & 2 ? "--" : "++",
			i & 1 ? "int" : "");
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " if(!f_initialized) throw controlled_vars_error_not_initialized(\"uninitialized variable\");");
		}
		if(i & 1) {
			fprintf(out, " %s_init<T> result(*this);", name);
		}
		fprintf(out, " %sf_ptr;", i & 2 ? "--" : "++");
		if(i & 1) {
			fprintf(out, " return result;");
		}
		else {
			fprintf(out, " return *this;");
		}
		fprintf(out, " }\n");
	}
}


void create_all_ptr_operators(const char *name, long flags)
{
	const op_t	*op;
	unsigned long	o, t, f;

	// if no default, then the default reset uses null()
	fprintf(out, "\tvoid reset() {%s f_ptr = %s; }\n",
			(flags & FLAG_HAS_INITFLG) == 0 ? "" : " f_initialized = true;",
			(flags & FLAG_HAS_DEFAULT) != 0 ? "init_value::DEFAULT_VALUE()" : "null()");
	create_ptr_operator_for_ptr(name, "reset", "T&", flags | FLAG_HAS_REFERENCE | FLAG_HAS_NOINIT);
	create_ptr_operator_for_ptr(name, "reset", "primary_type_t", flags | FLAG_HAS_NOINIT);
	create_ptr_operator_for_ptr(name, "reset", 0, flags | FLAG_HAS_REFERENCE | FLAG_HAS_NOINIT);
	create_ptr_operator_for_ptr(name, "reset", 0, flags | FLAG_HAS_NOINIT);

	for(o = 0; o < GENERIC_PTR_OPERATORS_MAX; ++o) {
		op = g_generic_ptr_operators + o;
		f = flags | op->flags;
		if((f & FLAG_HAS_PTR) != 0) {
			create_ptr_operator_for_ptr(name, op->name, "T&", f | FLAG_HAS_REFERENCE);
			create_ptr_operator_for_ptr(name, op->name, "primary_type_t", f);
			create_ptr_operator_for_ptr(name, op->name, 0, f | FLAG_HAS_REFERENCE);
			create_ptr_operator_for_ptr(name, op->name, 0, f);
		}
		else {
			/* IMPORTANT:
			 *	Here we were skipping the type bool, now there is a
			 *	command line option and by default we don't skip it.
			 */
			for(t = 0; t < PTR_TYPES_ALL; ++t) {
				// test to avoid all the operators that are not float compatible
				// (i.e. bitwise operators, modulo)
				if(g_ptr_types[t].condition) {
					fprintf(out, "%s\n", g_ptr_types[t].condition);
				}
				create_ptr_operator(name, op->name, g_ptr_types[t].name, f);
				if(g_ptr_types[t].condition) {
					fprintf(out, "#endif\n");
				}
			}
		}
	}
}


void create_typedef(const char *name, const char *short_name)
{
	const char	*t;
	unsigned int	idx;

	// here we include the size_t and time_t types (these were removed though)
	// UPDATE: We do not include bool because now it is managed as an
	//         enumeration instead
	for(idx = 1; idx < TYPES_ALL; ++idx) {
		t = g_types[idx].name;
		if(g_types[idx].flags & FLAG_TYPE_FLOAT) {
			// skip integer types
			if(strcmp(name, "auto") == 0
			|| strcmp(name, "ptr_auto") == 0) {
				continue;
			}
		}
		else {
			// skip floating point types
			if(strcmp(name, "fauto") == 0) {
				continue;
			}
		}
		if(g_types[idx].condition) {
			fprintf(out, "%s\n", g_types[idx].condition);
		}
		fprintf(out, "typedef %s_init<%s> %s%s_t;\n", name, g_types[idx].name, short_name, g_types[idx].short_name);
		if(g_types[idx].condition) {
			fprintf(out, "#endif\n");
		}
	}
}


void create_class(const char *name, const char *short_name, long flags)
{
	unsigned int	idx;
	char const 	*init;
	char const	*limits;

	if((flags & FLAG_HAS_LIMITS) != 0) {
		// we'd need to check that min <= max which should be possible
		// (actually BOOST does it...)
		limits = ", T min, T max";
	}
	else {
		limits = "";
	}

	fprintf(out, "/** \\brief Documentation available online.\n");
	fprintf(out, " * Please go to http://snapwebsites.org/project/controlled-vars\n");
	fprintf(out, " */\n");
	if((flags & FLAG_HAS_DEFAULT) != 0) {
		fprintf(out, "template<class T%s, T init_value = 0>", limits);
		if((flags & FLAG_HAS_LIMITS) != 0) {
			// the init_value should be checked using a static test
			// (which is possible, BOOST does it, but good luck to
			// replicate that work in a couple lines of code!)
			init = " f_value = check(init_value);";
		}
		else {
			init = " f_value = init_value;";
		}
	}
	else {
		fprintf(out, "template<class T%s>", limits);
		if((flags & FLAG_HAS_LIMITS) != 0) {
			// here we can use the min value if zero is not part of the range
			init = " f_value = 0.0 >= min && 0.0 <= max ? 0.0 : min;";
		}
		else {
			init = " f_value = 0.0;";
		}
	}
	fprintf(out, " class %s_init {\n", name);
	fprintf(out, "public:\n");
	fprintf(out, "\ttypedef T primary_type_t;\n");

	// Define the default value
	if((flags & FLAG_HAS_DEFAULT) != 0) {
		fprintf(out, "\tstatic T const DEFAULT_VALUE = init_value;\n");
	}

	// Define the limits
	if((flags & FLAG_HAS_LIMITS) != 0) {
		fprintf(out, "\tstatic primary_type_t const MIN_BOUND = min;\n");
		fprintf(out, "\tstatic primary_type_t const MAX_BOUND = max;\n");
		fprintf(out, "\tCONTROLLED_VARS_STATIC_ASSERT(min <= max);\n");
		if((flags & FLAG_HAS_DEFAULT) != 0) {
			fprintf(out, "\tCONTROLLED_VARS_STATIC_ASSERT(init_value >= min && init_value <= max);\n");
		}

		// a function to check the limits
		fprintf(out, "\ttemplate<class L> T check(L v) {\n");
		fprintf(out, "#ifdef CONTROLLED_VARS_LIMITED\n");
		fprintf(out, "#ifdef __GNUC__\n");
		fprintf(out, "#pragma GCC diagnostic push\n");
		fprintf(out, "#pragma GCC diagnostic ignored \"-Wlogical-op\"\n");
		fprintf(out, "#endif\n");
		fprintf(out, "\t\tif(v < min || v > max)");
		fprintf(out, " throw controlled_vars_error_out_of_bounds(\"value out of bounds\");\n");
		fprintf(out, "#ifdef __GNUC__\n");
		fprintf(out, "#pragma GCC diagnostic pop\n");
		fprintf(out, "#endif\n");
		fprintf(out, "#endif\n");
		fprintf(out, "\t\treturn static_cast<primary_type_t>(v);\n");
		fprintf(out, "\t}\n");
	}

	// Constructors
	if((flags & FLAG_HAS_VOID) != 0) {
		fprintf(out, "\t%s_init() {%s%s }\n", name,
			(flags & FLAG_HAS_DOINIT) != 0 ? init : "",
			(flags & FLAG_HAS_INITFLG) != 0 ? " f_initialized = false;" : "");
	}
	// in older versions of g++ we did not want the bool
	// type in constructors; it works fine in newer versions though
	// use the --no-bool-constructor option to revert to the
	// old behavior
	//
	// old command: (use idx = 1 to skip the bool type)
	// we don't want the bool type in the constructors...
	// it creates some problems
	// here we exclude the bool type
	for(idx = (no_bool_constructors == 1 ? 1 : 0); idx < TYPES_ALL; ++idx) {
		if(g_types[idx].condition) {
			fprintf(out, "%s\n", g_types[idx].condition);
		}
		fprintf(out, "\t%s_init(%s v) {", name, g_types[idx].name);
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " f_initialized = true;");
		}
		// The static cast is nice to have with cl which otherwise generates
		// warnings about values being truncated all over the place.
		fprintf(out, " f_value =");
		if((flags & FLAG_HAS_LIMITS) != 0) {
			fprintf(out, " check(v); }\n");
		}
		else {
			fprintf(out, " static_cast<primary_type_t>(v); }\n");
		}
		if(g_types[idx].condition) {
			fprintf(out, "#endif\n");
		}
	}

	// Unary operators
	create_unary_operators(name, flags);

	// Binary Operators
	create_all_operators(name, flags);

	if((flags & FLAG_HAS_DEBUG_ALREADY) == 0) {
		fprintf(out, "#ifdef CONTROLLED_VARS_DEBUG\n");
	}
	fprintf(out, "\tbool is_initialized() const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " return f_initialized;");
	}
	else {
		fprintf(out, " return true;");
	}
	fprintf(out, " }\n");
	if((flags & FLAG_HAS_DEBUG_ALREADY) == 0) {
		fprintf(out, "#endif\n");
	}

	fprintf(out, "private:\n");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, "\tbool f_initialized;\n");
	}
	fprintf(out, "\tT f_value;\n");
	fprintf(out, "};\n");

	if((flags & FLAG_HAS_LIMITS) == 0) {
		create_typedef(name, short_name);
	}
}


void create_class_enum(const char *name, long flags)
{
	char const 	*init;
	char const	*limits;

	flags |= FLAG_HAS_ENUM;

	if((flags & FLAG_HAS_FLOAT) != 0) {
		fprintf(stderr, "internal error: create_class_enum() called with FLAG_HAS_FLOAT.\n");
		exit(1);
	}

	if((flags & FLAG_HAS_LIMITS) != 0) {
		// we'd need to check that min <= max which should be possible
		// (actually BOOST does it...)
		limits = ", T min, T max";
	}
	else {
		limits = "";
	}

	fprintf(out, "/** \\brief Documentation available online.\n");
	fprintf(out, " * Please go to http://snapwebsites.org/project/controlled-vars\n");
	fprintf(out, " */\n");
	if((flags & FLAG_HAS_DEFAULT) != 0) {
		if((flags & FLAG_HAS_ENUM) != 0) {
			// we allow an "auto-init" of enumerations although
			// really we probably should not allow those at all
			// because all enumerations should be limited
			fprintf(out, "template<class T%s, T init_value = static_cast<T>(0)>", limits);
		}
		else {
			fprintf(out, "template<class T%s, T init_value = static_cast<T>(0)>", limits);
		}
		if((flags & FLAG_HAS_LIMITS) != 0) {
			// the init_value should be checked using a static test
			// (which is possible, BOOST does it, but good luck to
			// replicate that work in a couple lines of code!)
			init = " f_value = check(init_value);";
		}
		else {
			init = " f_value = init_value;";
		}
	}
	else {
		fprintf(out, "template<class T%s>", limits);
		if((flags & FLAG_HAS_LIMITS) != 0) {
			// here we can use the min value if zero is not part of the range
			init = " f_value = 0.0 >= min && 0.0 <= max ? 0.0 : min;";
		}
		else {
			init = " f_value = 0.0;";
		}
	}
	fprintf(out, " class %s_init {\n", name);
	fprintf(out, "public:\n");
	fprintf(out, "\ttypedef T primary_type_t;\n");

	// Define the default value
	if((flags & FLAG_HAS_DEFAULT) != 0) {
		fprintf(out, "\tstatic T const DEFAULT_VALUE = init_value;\n");
	}

	// Define the limits
	if((flags & FLAG_HAS_LIMITS) != 0) {
		fprintf(out, "\tstatic primary_type_t const MIN_BOUND = min;\n");
		fprintf(out, "\tstatic primary_type_t const MAX_BOUND = max;\n");
		fprintf(out, "\tCONTROLLED_VARS_STATIC_ASSERT(min <= max);\n");
		if((flags & FLAG_HAS_DEFAULT) != 0) {
			fprintf(out, "\tCONTROLLED_VARS_STATIC_ASSERT(init_value >= min && init_value <= max);\n");
		}

		// a function to check the limits
		fprintf(out, "\tT check(T v) {\n");
		fprintf(out, "#ifdef CONTROLLED_VARS_LIMITED\n");
		fprintf(out, "#ifdef __GNUC__\n");
		fprintf(out, "#pragma GCC diagnostic push\n");
		fprintf(out, "#pragma GCC diagnostic ignored \"-Wlogical-op\"\n");
		fprintf(out, "#endif\n");
		fprintf(out, "\t\tif(v < min || v > max)");
		fprintf(out, " throw controlled_vars_error_out_of_bounds(\"value out of bounds\");\n");
		fprintf(out, "#ifdef __GNUC__\n");
		fprintf(out, "#pragma GCC diagnostic pop\n");
		fprintf(out, "#endif\n");
		fprintf(out, "#endif\n");
		fprintf(out, "\t\treturn v;\n");
		fprintf(out, "\t}\n");
	}

	// Constructors
	if((flags & FLAG_HAS_VOID) != 0) {
		fprintf(out, "\t%s_init() {%s%s }\n", name,
			(flags & FLAG_HAS_DOINIT) != 0 ? init : "",
			(flags & FLAG_HAS_INITFLG) != 0 ? " f_initialized = false;" : "");
	}

	// create only one constructor for enumerations, but the correct
	// one!
	fprintf(out, "\t%s_init(T v) {", name);
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " f_initialized = true;");
	}
	fprintf(out, " f_value =");
	if((flags & FLAG_HAS_LIMITS) != 0) {
		fprintf(out, " check(v); }\n");
	}
	else {
		fprintf(out, " v; }\n");
	}

	// create only one assignment operator
	fprintf(out, "\t%s_init& operator = (T v) {", name);
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " f_initialized = true;");
	}
	fprintf(out, " f_value =");
	if((flags & FLAG_HAS_LIMITS) != 0) {
		fprintf(out, " check(v); return *this; }\n");
	}
	else {
		fprintf(out, " v; return *this; }\n");
	}

	// Unary operators
	create_unary_enum_operators(name, flags);

	// Binary Operators
	create_all_enum_operators(name, flags);

	if((flags & FLAG_HAS_DEBUG_ALREADY) == 0) {
		fprintf(out, "#ifdef CONTROLLED_VARS_DEBUG\n");
	}
	fprintf(out, "\tbool is_initialized() const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " return f_initialized;");
	}
	else {
		fprintf(out, " return true;");
	}
	fprintf(out, " }\n");
	if((flags & FLAG_HAS_DEBUG_ALREADY) == 0) {
		fprintf(out, "#endif\n");
	}

	fprintf(out, "private:\n");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, "\tbool f_initialized;\n");
	}
	fprintf(out, "\tT f_value;\n");
	fprintf(out, "};\n");

	//if((flags & FLAG_HAS_LIMITS) == 0) {
	//	create_typedef(name, short_name);
	//}
}


//create_class_ptr("ptr_auto", "zp", FLAG_HAS_VOID | FLAG_HAS_DOINIT | FLAG_HAS_DEFAULT)
//create_class_ptr("ptr_need", "mp", 0);
//create_class_ptr("ptr_no", "np", FLAG_HAS_VOID | FLAG_HAS_INITFLG | FLAG_HAS_DEBUG_ALREADY);
void create_class_ptr(const char *name, const char *short_name, long flags)
{
	unsigned int	idx;
	const char 	*init;

	fprintf(out, "/** \\brief Documentation available online.\n");
	fprintf(out, " * Please go to http://snapwebsites.org/project/controlled-vars\n");
	fprintf(out, " */\n");
	if((flags & FLAG_HAS_DEFAULT) != 0) {
		fprintf(out, "template<class T> class trait_%s_null { public: static T *DEFAULT_VALUE() { return 0; } };\n", name);
		fprintf(out, "template<class T, typename init_value = trait_%s_null<T> >", name);
		init = " f_ptr = DEFAULT_VALUE();";
	}
	else {
		fprintf(out, "template<class T>");
		init = " f_ptr = 0;";
	}
	fprintf(out, " class %s_init {\n", name);
	fprintf(out, "public:\n");
	fprintf(out, "\ttypedef T *primary_type_t;\n");

	// Define the default value
	if((flags & FLAG_HAS_DEFAULT) != 0) {
		fprintf(out, "\tstatic T *DEFAULT_VALUE() { return init_value::DEFAULT_VALUE(); }\n");
	}
	fprintf(out, "\tstatic T *null() { return 0; }\n");

	// Constructors
	if((flags & FLAG_HAS_VOID) != 0) {
		fprintf(out, "\t%s_init() {%s%s }\n", name,
			(flags & FLAG_HAS_DOINIT) != 0 ? init : "",
			(flags & FLAG_HAS_INITFLG) != 0 ? " f_initialized = false;" : "");
	}
	// for points, the different types are:
	//	T pointer
	//	T reference
	//	class by pointer
	//	class by reference
	for(idx = 0; idx < 4; ++idx) {
		int mode = idx % 2; // 0 - pointer, 1 - reference
		int type = idx / 2; // 0 - T, 1 - class

		static const char *ptr_modes[] = { " *", "& " };
		fprintf(out, "\t%s_init(%s%s%s%sp) {",
				name,
				(type == 0 ? "" : "const "),
				(type == 0 ? "T" : name),
				(type == 0 ? "" : "_init"),
				ptr_modes[mode]);
		if((flags & FLAG_HAS_INITFLG) != 0) {
			fprintf(out, " f_initialized = true;");
		}
		if(type == 0) {
			fprintf(out, " f_ptr = %sp; }\n", mode == 0 ? "" : "&");
		}
		else {
			switch(mode) {
			default: //case 0:
				fprintf(out, " f_ptr = p == 0 ? 0 : p->f_ptr; }\n");
				break;

			case 1:
				fprintf(out, " f_ptr = &p == 0 ? 0 : p.f_ptr; }\n");
				break;

			}
		}
	}

	// Unary operators
	create_unary_ptr_operators(name, flags);

	// Binary Operators
	create_all_ptr_operators(name, flags);

	if((flags & FLAG_HAS_DEBUG_ALREADY) == 0) {
		fprintf(out, "#ifdef CONTROLLED_VARS_DEBUG\n");
	}
	fprintf(out, "\tbool is_initialized() const {");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, " return f_initialized;");
	}
	else {
		fprintf(out, " return true;");
	}
	fprintf(out, " }\n");
	if((flags & FLAG_HAS_DEBUG_ALREADY) == 0) {
		fprintf(out, "#endif\n");
	}

	fprintf(out, "private:\n");
	if((flags & FLAG_HAS_INITFLG) != 0) {
		fprintf(out, "\tbool f_initialized;\n");
	}
	fprintf(out, "\tprimary_type_t f_ptr;\n");
	fprintf(out, "};\n");

	create_typedef(name, short_name);
}


void create_direct_typedef(const char *short_name)
{
	unsigned int	idx;

	// here we include the bool, size_t and time_t types
	// UPDATE: I removed the bool because it is handled as an enumeration
	for(idx = 1; idx < TYPES_ALL; ++idx) {
		if(g_types[idx].condition) {
			fprintf(out, "%s\n", g_types[idx].condition);
		}
		fprintf(out, "typedef %s %s%s_t;\n", g_types[idx].name, short_name, g_types[idx].short_name);
		if(g_types[idx].condition) {
			fprintf(out, "#endif\n");
		}
	}
}


void create_file(const char *filename)
{
	if(out != nullptr) {
		fclose(out);
	}
	out = fopen(filename, "w");
	if(out == nullptr) {
		fprintf(stderr, "error:controlled_vars: cannot create file \"%s\"\n", filename);
		exit(1);
	}
}


namespace
{
uint32_t PRINT_FLAG_INCLUDE_STDEXCEPT     = 0x0001;
uint32_t PRINT_FLAG_INCLUDE_INIT          = 0x0002;
uint32_t PRINT_FLAG_INCLUDE_EXCEPTION     = 0x0004;
uint32_t PRINT_FLAG_NO_NAMESPACE          = 0x0008;
uint32_t PRINT_FLAG_INCLUDE_STATIC_ASSERT = 0x0010;
uint32_t PRINT_FLAG_ENUM                  = 0x0020;
}

void print_header(const char *filename, const char *upper, int flags)
{
	fprintf(out, "// WARNING: do not edit; this is an auto-generated\n");
	fprintf(out, "// WARNING: file; please, use the generator named\n");
	fprintf(out, "// WARNING: controlled_vars to re-generate\n");
	fprintf(out, "//\n");
	fprintf(out, "// File:	%s\n", filename);
	fprintf(out, "// Object:	Help you by constraining basic types like classes.\n");
	fprintf(out, "//\n");
	fprintf(out, "// Copyright:	Copyright (c) 2005-2012 Made to Order Software Corp.\n");
	fprintf(out, "//		All Rights Reserved.\n");
	fprintf(out, "//\n");
	fprintf(out, "// http://snapwebsites.org/\n");
	fprintf(out, "// contact@m2osw.com\n");
	fprintf(out, "//\n");
	fprintf(out, "// Permission is hereby granted, free of charge, to any person obtaining a copy\n");
	fprintf(out, "// of this software and associated documentation files (the \"Software\"), to deal\n");
	fprintf(out, "// in the Software without restriction, including without limitation the rights\n");
	fprintf(out, "// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n");
	fprintf(out, "// copies of the Software, and to permit persons to whom the Software is\n");
	fprintf(out, "// furnished to do so, subject to the following conditions:\n");
	fprintf(out, "//\n");
	fprintf(out, "// The above copyright notice and this permission notice shall be included in\n");
	fprintf(out, "// all copies or substantial portions of the Software.\n");
	fprintf(out, "//\n");
	fprintf(out, "// THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n");
	fprintf(out, "// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n");
	fprintf(out, "// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n");
	fprintf(out, "// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n");
	fprintf(out, "// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n");
	fprintf(out, "// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN\n");
	fprintf(out, "// THE SOFTWARE.\n");
	fprintf(out, "//\n");
	fprintf(out, "#ifndef CONTROLLED_VARS_%s%s_H\n", upper, (flags & PRINT_FLAG_INCLUDE_INIT) != 0 ? "_INIT" : "");
	fprintf(out, "#define CONTROLLED_VARS_%s%s_H\n", upper, (flags & PRINT_FLAG_INCLUDE_INIT) != 0 ? "_INIT" : "");
	fprintf(out, "#ifdef _MSC_VER\n");
	fprintf(out, "#pragma warning(push)\n");
	fprintf(out, "#pragma warning(disable: 4005 4018 4244 4800)\n");
	fprintf(out, "#if _MSC_VER > 1000\n");
	fprintf(out, "#pragma once\n");
	fprintf(out, "#endif\n");
	fprintf(out, "#elif defined(__GNUC__)\n");
	fprintf(out, "#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)\n");
	fprintf(out, "#pragma once\n");
	fprintf(out, "#endif\n");
	fprintf(out, "#endif\n");
	if((flags & PRINT_FLAG_NO_NAMESPACE) == 0) {
		if((flags & PRINT_FLAG_INCLUDE_EXCEPTION) != 0) {
			fprintf(out, "#include \"controlled_vars_exceptions.h\"\n");
		}
		else {
			fprintf(out, "#include <limits.h>\n");
			fprintf(out, "#include <sys/types.h>\n");
			//fprintf(out, "#ifndef BOOST_CSTDINT_HPP\n");
			fprintf(out, "#include <stdint.h>\n");
			//fprintf(out, "#endif\n");
		}
		if((flags & PRINT_FLAG_INCLUDE_STATIC_ASSERT) != 0) {
			fprintf(out, "#include \"controlled_vars_static_assert.h\"\n");
		}
		if((flags & PRINT_FLAG_INCLUDE_STDEXCEPT) != 0) {
			fprintf(out, "#include <stdexcept>\n");
		}
		if((flags & PRINT_FLAG_ENUM) != 0) {
			fprintf(out, "#include <type_traits>\n");
		}
		fprintf(out, "namespace controlled_vars {\n");
	}
}


void print_footer(int flags)
{
	if((flags & PRINT_FLAG_NO_NAMESPACE) == 0) {
		fprintf(out, "} // namespace controlled_vars\n");
	}
	fprintf(out, "#ifdef _MSC_VER\n");
	fprintf(out, "#pragma warning(pop)\n");
	fprintf(out, "#endif\n");
	fprintf(out, "#endif\n");
}

typedef void (*print_func)();


void print_exceptions()
{
	fprintf(out, "class controlled_vars_error : public std::logic_error {\n");
	fprintf(out, "public:\n");
	fprintf(out, "\texplicit controlled_vars_error(const std::string& what_msg) : logic_error(what_msg) {}\n");
	fprintf(out, "};\n");
	fprintf(out, "class controlled_vars_error_not_initialized : public controlled_vars_error {\n");
	fprintf(out, "public:\n");
	fprintf(out, "\texplicit controlled_vars_error_not_initialized(const std::string& what_msg) : controlled_vars_error(what_msg) {}\n");
	fprintf(out, "};\n");
	fprintf(out, "class controlled_vars_error_out_of_bounds : public controlled_vars_error {\n");
	fprintf(out, "public:\n");
	fprintf(out, "\texplicit controlled_vars_error_out_of_bounds(const std::string& what_msg) : controlled_vars_error(what_msg) {}\n");
	fprintf(out, "};\n");
	fprintf(out, "class controlled_vars_error_null_pointer : public controlled_vars_error {\n");
	fprintf(out, "public:\n");
	fprintf(out, "\texplicit controlled_vars_error_null_pointer(const std::string& what_msg) : controlled_vars_error(what_msg) {}\n");
	fprintf(out, "};\n");
}


void print_static_assert()
{
	fprintf(out, "// The following is 100%% coming from boost/static_assert.hpp\n");
	fprintf(out, "// At this time we only support MSC and GNUC\n");
	fprintf(out, "#if defined(_MSC_VER)||defined(__GNUC__)\n");
	fprintf(out, "#define CONTROLLED_VARS_JOIN(X,Y) CONTROLLED_VARS_DO_JOIN(X,Y)\n");
	fprintf(out, "#define CONTROLLED_VARS_DO_JOIN(X,Y) CONTROLLED_VARS_DO_JOIN2(X,Y)\n");
	fprintf(out, "#define CONTROLLED_VARS_DO_JOIN2(X,Y) X##Y\n");
	fprintf(out, "template<bool x> struct STATIC_ASSERTION_FAILURE;\n");
	fprintf(out, "template<> struct STATIC_ASSERTION_FAILURE<true>{enum{value=1};};\n");
	fprintf(out, "template<int x> struct static_assert_test{};\n");
	fprintf(out, "#if defined(__GNUC__)&&((__GNUC__>3)||((__GNUC__==3)&&(__GNUC_MINOR__>=4)))\n");
	fprintf(out, "#define CONTROLLED_VARS_STATIC_ASSERT_BOOL_CAST(x) ((x)==0?false:true)\n");
	fprintf(out, "#else\n");
	fprintf(out, "#define CONTROLLED_VARS_STATIC_ASSERT_BOOL_CAST(x) (bool)(x)\n");
	fprintf(out, "#endif\n");
	fprintf(out, "#ifdef _MSC_VER\n");
	fprintf(out, "#define CONTROLLED_VARS_STATIC_ASSERT(B) "
			   "typedef ::controlled_vars::static_assert_test<"
			      "sizeof(::controlled_vars::STATIC_ASSERTION_FAILURE<CONTROLLED_VARS_STATIC_ASSERT_BOOL_CAST(B)>)>"
				 "CONTROLLED_VARS_JOIN(controlled_vars_static_assert_typedef_,__COUNTER__)\n");
	fprintf(out, "#else\n");
	fprintf(out, "#define CONTROLLED_VARS_STATIC_ASSERT(B) "
			   "typedef ::controlled_vars::static_assert_test<"
			      "sizeof(::controlled_vars::STATIC_ASSERTION_FAILURE<CONTROLLED_VARS_STATIC_ASSERT_BOOL_CAST(B)>)>"
				 "CONTROLLED_VARS_JOIN(controlled_vars_static_assert_typedef_,__LINE__)\n");
	fprintf(out, "#endif\n");
	fprintf(out, "#else\n");
	fprintf(out, "#define CONTROLLED_VARS_STATIC_ASSERT(B)\n");
	fprintf(out, "#endif\n");
}


void print_auto()
{
	create_class("auto", "z", FLAG_HAS_VOID | FLAG_HAS_DOINIT | FLAG_HAS_DEFAULT);
}


void print_auto_enum()
{
	create_class_enum("auto_enum", FLAG_HAS_VOID | FLAG_HAS_DOINIT | FLAG_HAS_DEFAULT);
	fprintf(out, "typedef auto_enum_init<bool, false> fbool_t;\n");
	fprintf(out, "typedef fbool_t zbool_t;\n");
	fprintf(out, "typedef auto_enum_init<bool, true> tbool_t;\n");
}


void print_limited_auto()
{
	create_class("limited_auto", "lz", FLAG_HAS_VOID | FLAG_HAS_DOINIT | FLAG_HAS_DEFAULT | FLAG_HAS_LIMITS);
}


void print_limited_auto_enum()
{
	create_class_enum("limited_auto_enum", FLAG_HAS_VOID | FLAG_HAS_DOINIT | FLAG_HAS_DEFAULT | FLAG_HAS_LIMITS);
	fprintf(out, "typedef limited_auto_enum_init<bool, false, true, false> flbool_t;\n");
	fprintf(out, "typedef flbool_t zlbool_t;\n");
	fprintf(out, "typedef limited_auto_enum_init<bool, false, true, true> tlbool_t;\n");
}


void print_ptr_auto()
{
	create_class_ptr("ptr_auto", "zp", FLAG_HAS_VOID | FLAG_HAS_DOINIT | FLAG_HAS_DEFAULT);
}


void print_fauto()
{
	create_class("fauto", "z", FLAG_HAS_VOID | FLAG_HAS_DOINIT | FLAG_HAS_FLOAT);
}


void print_limited_fauto()
{
	create_class("limited_fauto", "lz", FLAG_HAS_VOID | FLAG_HAS_DOINIT | FLAG_HAS_FLOAT | FLAG_HAS_LIMITS);
}


void print_need()
{
	create_class("need", "m", 0);
}


void print_need_enum()
{
	create_class_enum("need_enum", 0);
	fprintf(out, "typedef need_enum_init<bool> mbool_t;\n");
}


void print_limited_need()
{
	create_class("limited_need", "lm", FLAG_HAS_LIMITS);
}


void print_limited_need_enum()
{
	create_class_enum("limited_need_enum", FLAG_HAS_LIMITS);
	fprintf(out, "typedef limited_need_enum_init<bool, false, true> mlbool_t;\n");
}


void print_ptr_need()
{
	create_class_ptr("ptr_need", "mp", 0);
}


void print_no_init()
{
	fprintf(out, "#ifdef CONTROLLED_VARS_DEBUG\n");
	create_class("no", "r", FLAG_HAS_VOID | FLAG_HAS_INITFLG | FLAG_HAS_DEBUG_ALREADY);
	fprintf(out, "#else\n");
	create_direct_typedef("r");
	fprintf(out, "#endif\n");
}


void print_no_init_enum()
{
	// Anything here?
	fprintf(out, "#ifdef CONTROLLED_VARS_DEBUG\n");
	create_class_enum("no_enum", FLAG_HAS_VOID | FLAG_HAS_INITFLG | FLAG_HAS_DEBUG_ALREADY);
	fprintf(out, "typedef no_enum_init<bool> rbool_t;\n");
	fprintf(out, "#else\n");
	fprintf(out, "typedef bool rbool_t;\n");
	fprintf(out, "#endif\n");
}


void print_limited_no_init()
{
	fprintf(out, "#ifdef CONTROLLED_VARS_DEBUG\n");
	create_class("limited_no", "r", FLAG_HAS_VOID | FLAG_HAS_INITFLG | FLAG_HAS_LIMITS | FLAG_HAS_DEBUG_ALREADY);
	//fprintf(out, "#else\n");
	// in non-debug, this is essentially the same template, but we
	// expect the users to declare their types "properly" (i.e. using
	// a typedef whenever CONTROLLED_VARS_DEBUG is not defined.)
	//create_direct_typedef("rl");
	fprintf(out, "#endif\n");
}


void print_limited_no_init_enum()
{
	fprintf(out, "#ifdef CONTROLLED_VARS_DEBUG\n");
	create_class_enum("limited_no_enum", FLAG_HAS_VOID | FLAG_HAS_INITFLG | FLAG_HAS_LIMITS | FLAG_HAS_DEBUG_ALREADY);
	fprintf(out, "typedef limited_no_enum_init<bool, false, true> rlbool_t;\n");
	fprintf(out, "#else\n");
	fprintf(out, "typedef bool rlbool_t;\n");
	fprintf(out, "#endif\n");
}


void print_ptr_no_init()
{
	create_class_ptr("ptr_no", "rp", FLAG_HAS_VOID | FLAG_HAS_INITFLG | FLAG_HAS_DEBUG_ALREADY);
}


void print_file(const char *name, int flags, print_func func)
{
	char filename[256], upper[256];

	// create an uppercase version of the name
	const char *n = name;
	char *u = upper;
	while(*n != '\0') {
		*u++ = *n++ & 0x5F;
	}
	*u = '\0';

	//printf("Working on \"%s\"\n", upper);

	// create the output file
	sprintf(filename, "controlled_vars_%s%s.h", name, (flags & PRINT_FLAG_INCLUDE_INIT) != 0 ? "_init" : "");
	create_file(filename);

	// print out the header
	print_header(filename, upper, flags);

	// print out the contents
	(*func)();

	// print closure
	print_footer(flags);
}

void print_include_all()
{
	create_file("controlled_vars.h");
	print_header("controlled_vars.h", "", PRINT_FLAG_NO_NAMESPACE);
	// we don't have to include the exception header,
	// it will be by several of the following headers
	fprintf(out, "#include \"controlled_vars_auto_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_auto_enum_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_limited_auto_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_limited_auto_enum_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_fauto_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_limited_fauto_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_need_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_need_enum_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_limited_need_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_limited_need_enum_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_no_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_no_enum_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_limited_no_init.h\"\n");
	fprintf(out, "#include \"controlled_vars_limited_no_enum_init.h\"\n");
	print_footer(PRINT_FLAG_NO_NAMESPACE);
}

int main(int argc, char *argv[])
{
	for(int i(1); i < argc; ++i) {
		if(strcmp(argv[i], "--no-bool-constructors") == 0) {
			no_bool_constructors = 1;
		}
	}

	print_file("exceptions", PRINT_FLAG_INCLUDE_STDEXCEPT, print_exceptions);
	print_file("static_assert", 0, print_static_assert);

	print_file("auto",              PRINT_FLAG_INCLUDE_INIT, print_auto);
	print_file("auto_enum",         PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_ENUM, print_auto_enum);
	print_file("limited_auto",      PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_INCLUDE_EXCEPTION | PRINT_FLAG_INCLUDE_STATIC_ASSERT, print_limited_auto);
	print_file("limited_auto_enum", PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_ENUM | PRINT_FLAG_INCLUDE_EXCEPTION | PRINT_FLAG_INCLUDE_STATIC_ASSERT, print_limited_auto_enum);
	print_file("ptr_auto",          PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_INCLUDE_EXCEPTION, print_ptr_auto);
	print_file("fauto",             PRINT_FLAG_INCLUDE_INIT, print_fauto);
	print_file("limited_fauto",     PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_INCLUDE_EXCEPTION | PRINT_FLAG_INCLUDE_STATIC_ASSERT, print_limited_fauto);
	print_file("need",              PRINT_FLAG_INCLUDE_INIT, print_need);
	print_file("need_enum",         PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_ENUM, print_need_enum);
	print_file("limited_need",      PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_INCLUDE_EXCEPTION | PRINT_FLAG_INCLUDE_STATIC_ASSERT, print_limited_need);
	print_file("limited_need_enum", PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_ENUM | PRINT_FLAG_INCLUDE_EXCEPTION | PRINT_FLAG_INCLUDE_STATIC_ASSERT, print_limited_need_enum);
	print_file("ptr_need",          PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_INCLUDE_EXCEPTION, print_ptr_need);
	print_file("no",                PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_INCLUDE_EXCEPTION, print_no_init);
	print_file("no_enum",           PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_ENUM | PRINT_FLAG_INCLUDE_EXCEPTION, print_no_init_enum);
	print_file("limited_no",        PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_INCLUDE_EXCEPTION | PRINT_FLAG_INCLUDE_STATIC_ASSERT, print_limited_no_init);
	print_file("limited_no_enum",   PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_ENUM | PRINT_FLAG_INCLUDE_EXCEPTION | PRINT_FLAG_INCLUDE_STATIC_ASSERT, print_limited_no_init_enum);
	print_file("ptr_no",            PRINT_FLAG_INCLUDE_INIT | PRINT_FLAG_INCLUDE_EXCEPTION, print_ptr_no_init);

	print_include_all();

	return 0;
}


