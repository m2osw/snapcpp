// Snap Websites Server -- advanced parser
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
#pragma once

#include <controlled_vars/controlled_vars_auto_init.h>
#include <controlled_vars/controlled_vars_limited_auto_init.h>

#include <QVariant>
#include <QVector>
#include <QSharedPointer>

namespace snap {
namespace parser {

class snap_parser_no_current_choices : public std::exception {};
class snap_parser_state_has_children : public std::exception {};
class snap_parser_unexpected_token : public std::exception {};



enum token_t {
	TOKEN_ID_NONE_ENUM = 0,		// "not a token" (also end of input)

	TOKEN_ID_INTEGER_ENUM,
	TOKEN_ID_FLOAT_ENUM,
	TOKEN_ID_IDENTIFIER_ENUM,
	TOKEN_ID_KEYWORD_ENUM,
	TOKEN_ID_STRING_ENUM,
	TOKEN_ID_LITERAL_ENUM,		// literal character(s)

	TOKEN_ID_EMPTY_ENUM,		// special empty token
	TOKEN_ID_CHOICES_ENUM,		// pointer to a choices object
	TOKEN_ID_RULES_ENUM,		// pointer to a choices object (see rules operator |() )
	TOKEN_ID_NODE_ENUM,			// pointer to a node object
	TOKEN_ID_ERROR_ENUM			// an error occured
};

struct token_id { token_id(token_t t) : f_type(t) {} operator token_t () const { return f_type; } private: token_t f_type; };
struct token_id_none_def : public token_id { token_id_none_def() : token_id(TOKEN_ID_NONE_ENUM) {} };
struct token_id_integer_def : public token_id { token_id_integer_def() : token_id(TOKEN_ID_INTEGER_ENUM) {} };
struct token_id_float_def : public token_id { token_id_float_def() : token_id(TOKEN_ID_FLOAT_ENUM) {} };
struct token_id_identifier_def : public token_id { token_id_identifier_def() : token_id(TOKEN_ID_IDENTIFIER_ENUM) {} };
struct token_id_keyword_def : public token_id { token_id_keyword_def() : token_id(TOKEN_ID_KEYWORD_ENUM) {} };
struct token_id_string_def : public token_id { token_id_string_def() : token_id(TOKEN_ID_STRING_ENUM) {} };
struct token_id_literal_def : public token_id { token_id_literal_def() : token_id(TOKEN_ID_LITERAL_ENUM) {} };
struct token_id_empty_def : public token_id { token_id_empty_def() : token_id(TOKEN_ID_EMPTY_ENUM) {} };

extern token_id_none_def TOKEN_ID_NONE;
extern token_id_integer_def TOKEN_ID_INTEGER;
extern token_id_float_def TOKEN_ID_FLOAT;
extern token_id_identifier_def TOKEN_ID_IDENTIFIER;
extern token_id_keyword_def TOKEN_ID_KEYWORD;
extern token_id_string_def TOKEN_ID_STRING;
extern token_id_literal_def TOKEN_ID_LITERAL;
extern token_id_empty_def TOKEN_ID_EMPTY;




class token
{
public:
	token(token_t id = TOKEN_ID_NONE) : f_id(id) {}
	token(const token& t) : f_id(t.f_id), f_value(t.f_value) {}
	token& operator = (const token& t)
	{
		if(this != &t) {
			f_id = t.f_id;
			f_value = t.f_value;
		}
		return *this;
	}
	// polymorphic type so user data works as expected
	virtual ~token() {}

	void set_id(token_t id) { f_id = id; }
	token_t get_id() const { return f_id; }

	void set_value(const QVariant& value) { f_value = value; }
	QVariant get_value() const { return f_value; }

private:
	token_t			f_id;
	QVariant		f_value;
};
typedef QVector<QSharedPointer<token> >	vector_token_t;

class keyword;

class lexer
{
public:
	enum lexer_error_t {
		LEXER_ERROR_NONE,

		LEXER_ERROR_INVALID_STRING,
		LEXER_ERROR_INVALID_C_COMMENT,
		LEXER_ERROR_INVALID_NUMBER,

		LEXER_ERROR_max
	};

					lexer() { f_pos = f_input.begin(); }
	bool			eoi() const { return f_pos == f_input.end(); }
	uint32_t		line() const { return f_line; }
	void			set_input(const QString& input);
	void			add_keyword(keyword& k);
	token			next_token();
	lexer_error_t	get_error_code() const { return f_error_code; }
	QString			get_error_message() const { return f_error_message; }
	uint32_t		get_error_line() const { return f_error_line; }

private:
	// list of keywords / identifiers
	typedef QMap<QString, int> keywords_map_t;

	QString						f_input;
	QString::const_iterator		f_pos;
	controlled_vars::zuint32_t	f_line;
	keywords_map_t				f_keywords;
	typedef controlled_vars::limited_auto_init<lexer_error_t, LEXER_ERROR_NONE, static_cast<lexer_error_t>(LEXER_ERROR_max - 1), LEXER_ERROR_NONE> controlled_error_t;
	controlled_error_t			f_error_code;
	QString						f_error_message;
	controlled_vars::zuint32_t	f_error_line;
};

class keyword
{
public:
	keyword() : f_number(0) {}
	keyword(lexer& parent, const QString& keyword_identifier, int index_number = 0);

	QString identifier() const { return f_identifier; }
	int number() const { return f_number; }

private:
	static int	g_next_number;

	int			f_number;
	QString		f_identifier;
};

class choices;
class token_node;

class rule
{
public:
	typedef void (*reducer_t)(const rule& r, QSharedPointer<token_node>& t);

	rule() : f_parent(NULL), f_reducer(NULL) {}
	rule(choices& c);
	rule(const rule& r);

	void add_rules(choices& c); // choices of rules
	void add_choices(choices& c); // sub-rule
	void add_token(token_t token); // any value accepted
	void add_literal(const QString& value);
	void add_keyword(const keyword& k);
	void set_reducer(reducer_t reducer)
	{
		f_reducer = reducer;
	}

	int count() const { return f_tokens.count(); }

	class rule_ref
	{
	public:
		rule_ref(const rule *r, int position)
			: f_rule(r), f_position(position)
		{
		}
		rule_ref(const rule_ref& ref)
			: f_rule(ref.f_rule), f_position(ref.f_position)
		{
		}

		token get_token() const { return f_rule->f_tokens[f_position].f_token; }
		QString get_value() const { return f_rule->f_tokens[f_position].f_value; }
		keyword get_keyword() const { return f_rule->f_tokens[f_position].f_keyword; }
		choices& get_choices() const { return *f_rule->f_tokens[f_position].f_choices; }

	private:
		const rule *f_rule;
		int			f_position;
	};

	const rule_ref operator [] (int position) const
	{
		return rule_ref(this, position);
	}

	void reduce(QSharedPointer<token_node> n) const
	{
		if(f_reducer != NULL) {
			f_reducer(*this, n);
		}
	}

	rule& operator >> (const token_id& token);
	rule& operator >> (const QString& literal);
	rule& operator >> (const char *literal);
	rule& operator >> (const keyword& k);
	rule& operator >> (choices& c);
	rule& operator >= (rule::reducer_t function);

	QString to_string() const;

private:
	struct rule_data_t {
		rule_data_t();
		rule_data_t(const rule_data_t& s);
		rule_data_t(choices& c);
		rule_data_t(token_t token);
		rule_data_t(const QString& value); // i.e. literal
		rule_data_t(const keyword& k);

		token_t		f_token;
		QString		f_value;	// required value if not empty
		keyword		f_keyword;	// the keyword
		choices *	f_choices;	// sub-rule if not null & token TOKEN_ID_CHOICES_ENUM
	};

	choices *				f_parent;
	QVector<rule_data_t>	f_tokens;
	reducer_t				f_reducer;
};
// these have to be defined as friends of the class to enable
// all possible cases
rule& operator >> (const token_id& token_left, const token_id& token_right);
rule& operator >> (const token_id& token, const QString& literal);
rule& operator >> (const token_id& token, const char *literal);
rule& operator >> (const token_id& token, const keyword& k);
rule& operator >> (const token_id& token, choices& c);
rule& operator >> (const QString& literal, const token_id& token);
rule& operator >> (const QString& literal_left, const QString& literal_right);
rule& operator >> (const QString& literal, const keyword& k);
rule& operator >> (const QString& literal, choices& c);
rule& operator >> (const keyword& k, const token_id& token);
rule& operator >> (const keyword& k, const QString& literal);
rule& operator >> (const keyword& k_left, const keyword& k_right);
rule& operator >> (const keyword& k, choices& c);
rule& operator >> (choices& c, const token_id& token);
rule& operator >> (choices& c, const QString& literal);
rule& operator >> (choices& c, const keyword& k);
rule& operator >> (choices& c_left, choices& c_right);

// now a way to add a reducer function
rule& operator >= (const token_id& token, rule::reducer_t function);
rule& operator >= (const QString& literal, rule::reducer_t function);
rule& operator >= (const keyword& k, rule::reducer_t function);
rule& operator >= (choices& c, rule::reducer_t function);

rule& operator | (const token_id& token, rule& r_right);
rule& operator | (rule& r_left, rule& r_right);
rule& operator | (rule& r, choices& c);


class grammar;

class choices
{
public:
	choices(grammar *parent, const char *choice_name = "");
	~choices();

	const QString& name() const { return f_name; }
	int count() { return f_rules.count(); }
	void clear();

	choices& operator = (const choices& rhs);

	choices& operator >>= (const token_id& token);
	choices& operator >>= (const QString& literal);
	choices& operator >>= (const keyword& k);
	choices& operator >>= (choices& rhs);
	choices& operator >>= (rule& rhs);

	rule& operator | (rule& r);

	void add_rule(rule& r);
	const rule& operator [] (int rule) const
	{
		return *f_rules[rule];
	}

	// for debug purposes
	QString to_string() const;

private:
	QString				f_name;
	QVector<rule *>		f_rules;
};
typedef QVector<choices *>			choices_array_t;


// base class that parsers derive from to create user data to be
// saved in token_node objects (see below)
// must always be used with QSharedPointer<>
class parser_user_data
{
public:
	virtual ~parser_user_data() {}

private:
};


// token holder that can be saved in a tree like manner via the QObject
// child/parent functionality
class token_node : public token
{
// Q_OBJECT is not used because we don't have signals, slots or properties
public:
	token_node() : token(TOKEN_ID_NODE_ENUM) {}

	void add_token(token& t) { f_tokens.push_back(QSharedPointer<token>(new token(t))); }
	void add_node(QSharedPointer<token_node> n) { f_tokens.push_back(n); }
	vector_token_t& tokens() { return f_tokens; }
	size_t size() const { return f_tokens.size(); }
	QSharedPointer<token> operator [] (int index) { return f_tokens[index]; }
	const QSharedPointer<token> operator [] (int index) const { return f_tokens[index]; }
	void set_line(uint32_t line) { f_line = line; }
	uint32_t get_line() const { return f_line; }

	void set_user_data(QSharedPointer<parser_user_data> data) { f_user_data = data; }
	QSharedPointer<parser_user_data> get_user_data() const { return f_user_data; }

private:
	controlled_vars::zint32_t			f_line;
	vector_token_t						f_tokens;
	QSharedPointer<parser_user_data>	f_user_data;
};

class grammar
{
public:
	grammar();

	void add_choices(choices& c);

	bool parse(lexer& input, choices& start);
	QSharedPointer<token_node> get_result() const { return f_result; }

private:
	choices_array_t				f_choices;
	QSharedPointer<token_node>	f_result;
};



} // namespace parser
} // namespace snap
// vim: ts=4 sw=4
