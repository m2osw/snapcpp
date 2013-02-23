// Snap Websites Server -- advanced parser
// Copyright (C) 2011-2012  Made to Order Software Corp.
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

#include "snap_parser.h"
#include <stdio.h>
#include <QList>
#include <QPointer>

namespace snap
{
namespace parser
{

token_id_none_def TOKEN_ID_NONE;
token_id_integer_def TOKEN_ID_INTEGER;
token_id_float_def TOKEN_ID_FLOAT;
token_id_identifier_def TOKEN_ID_IDENTIFIER;
token_id_keyword_def TOKEN_ID_KEYWORD;
token_id_string_def TOKEN_ID_STRING;
token_id_literal_def TOKEN_ID_LITERAL;
token_id_empty_def TOKEN_ID_EMPTY;

/** \brief Set the input string for the lexer.
 *
 * This lexer accepts a standard QString as input. It will be what gets parsed.
 *
 * The input is never modified. It is parsed using the next_token() function.
 *
 * By default, the input is an empty string.
 *
 * \param[in] input  The input string to be parsed by this lexer.
 */
void lexer::set_input(const QString& input)
{
	f_input = input;
	f_pos = f_input.begin();
	f_line = 1;
}

/** \brief Read the next token.
 *
 * At this time we support the follow tokens:
 *
 * \li TOKEN_ID_NONE_ENUM -- the end of the input was reached
 *
 * \li TOKEN_ID_INTEGER_ENUM -- an integer ([0-9]+) number; always positive since
 *				the parser returns '-' as a separate literal
 *
 * \li TOKEN_ID_FLOAT_ENUM -- a floating point number with optinal exponent
 *				([0-9]+\.[0-9]+([eE][+-]?[0-9]+)?); always positive since
 *				the parser returns '-' as a separate literal
 *
 * \li TOKEN_ID_IDENTIFIER_ENUM -- supports C like identifiers ([a-z_][a-z0-9_]*)
 *
 * \li TOKEN_ID_KEYWORD_ENUM -- an identifier that matches one of our keywords
 *				as defined in the keyword map
 *
 * \li TOKEN_ID_STRING_ENUM -- a string delimited by double quotes ("); support
 *				backslashes; returns the content of the string
 *				(the quotes are removed)
 *
 * \li TOKEN_ID_LITERAL_ENUM -- anything else except what gets removed (spaces,
 *				new lines, C or C++ like comments)
 *
 * \li TOKEN_ID_ERROR_ENUM -- an error occured, you can get the error message for
 *				more information
 *
 * The TOKEN_ID_LITERAL_ENUM may either return a character ('=' operator) or a
 * string ("/=" operator). The special literals are defined here:
 *
 * \li ++ - increment
 * \li += - add & assign
 * \li -- - decrement
 * \li -= - subtract & assign
 * \li *= - multiply & assign
 * \li ** - power
 * \li **= - power & assign
 * \li /= - divide & assign
 * \li %= - divide & assign
 * \li ~= - bitwise not & assign
 * \li &= - bitwise and & assign
 * \li && - logical and
 * \li &&= - logical and & assign
 * \li |= - bitwise or & assign
 * \li || - logical or
 * \li ||= - logical or & assign
 * \li ^= - bitwise xor & assign
 * \li ^^ - logical xor
 * \li ^^= - logical xor & assign
 * \li != - not equal
 * \li !== - exactly not equal
 * \li !< - rotate left
 * \li !> - rotate left
 * \li ?= - assign default if undefined
 * \li == - equal
 * \li === - exactly equal
 * \li <= - smaller or equal
 * \li << - shift left
 * \li <<= - shift left and assign
 * \li <? - minimum
 * \li <?= - minimum and assign
 * \li >= - larger or equal
 * \li >> - shift right
 * \li >>> - unsigned shift right
 * \li >>= - shift right and assign
 * \li >>>= - unsigned shift right and assign
 * \li >? - maximum
 * \li >?= - maximum and assign
 * \li := - required assignment
 * \li :: - namespace
 *
 * If the returned token says TOKEN_ID_NONE_ENUM then you reached the
 * end of the input. When it says TOKEN_ID_ERROR_ENUM, then the input
 * is invalid and the error message and line number can be retrieved
 * to inform the user.
 *
 * The parser supports any type of new lines (Unix, Windows and Mac.)
 *
 * \todo
 * Check for overflow on integers and doubles
 *
 * \todo
 * Should we include default keywords? (i.e. true, false, if, else,
 * etc.) so those cannot be used as identifiers in some places?
 *
 * \return The read token.
 */
token lexer::next_token()
{
	token		result;

// restart is called whenever we find a comment or
// some other entry that just gets "deleted" from the input
// (i.e. new line, space...)
//
// Note: I don't use a do ... while(repeat); because in some cases
// we are inside several levels of switch() for() while() loops.
restart:

	// we reached the end of input
	if(f_pos == f_input.end()) {
		return result;
	}

	switch(f_pos->unicode()) {
	case '\n':
		++f_pos;
		++f_line;
		goto restart;

	case '\r':
		++f_pos;
		++f_line;
		if(f_pos != f_input.end() && *f_pos == '\n') {
			// skip "\r\n" as one end of line
			++f_pos;
		}
		goto restart;

	case ' ':
	case '\t':
		++f_pos;
		goto restart;

	case '+':
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // add and assign
				result.set_value("+=");
				++f_pos;
				break;

			case '+': // increment
				result.set_value("++");
				++f_pos;
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '-':
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // subtract and assign
				result.set_value("-=");
				++f_pos;
				break;

			case '-': // decrement
				result.set_value("--");
				++f_pos;
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '*':
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '/': // invalid C comment end marker
				// in this case we don't have to restart since we
				// reached the end of the input
				f_error_code = static_cast<int>(LEXER_ERROR_INVALID_C_COMMENT);
				f_error_message = "comment terminator without introducer";
				f_error_line = f_line;
				result.set_id(TOKEN_ID_ERROR_ENUM);
				break;

			case '=': // multiply and assign
				result.set_value("*=");
				++f_pos;
				break;

			case '*': // power
				result.set_value("**");
				++f_pos;
				if(f_pos != f_input.end()) {
					if(*f_pos == '=') {
						// power and assign
						result.set_value("**=");
						++f_pos;
					}
				}
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '/': // divide
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '/': // C++ comment -- skip up to eol
				for(++f_pos; f_pos != f_input.end(); ++f_pos) {
					if(*f_pos == '\n' || *f_pos == '\r') {
						goto restart;
					}
				}
				// in this case we don't have to restart since we
				// reached the end of the input
				result.set_id(TOKEN_ID_NONE_ENUM);
				break;

			case '*': // C comment -- skip up to */
				for(++f_pos; f_pos != f_input.end(); ++f_pos) {
					if(f_pos + 1 != f_input.end() && *f_pos == '*' && f_pos[1] == '/') {
						f_pos += 2;
						goto restart;
					}
				}
				// in this case the comment was not terminated
				f_error_code = static_cast<int>(LEXER_ERROR_INVALID_C_COMMENT);
				f_error_message = "comment not terminated";
				f_error_line = f_line;
				result.set_id(TOKEN_ID_ERROR_ENUM);
				break;

			case '=': // divide and assign
				result.set_value("/=");
				++f_pos;
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '%': // modulo
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // modulo and assign
				result.set_value("%=");
				++f_pos;
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '~': // bitwise not
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // bitwise not and assign
				result.set_value("~=");
				++f_pos;
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '&': // bitwise and
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // bitwise and & assign
				result.set_value("&=");
				++f_pos;
				break;

			case '&': // logical and
				result.set_value("&&");
				++f_pos;
				if(f_pos != f_input.end()) {
					if(*f_pos == '=') {
						// logical and & assign
						result.set_value("&&=");
						++f_pos;
					}
				}
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '|': // bitwise or
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // bitwise or & assign
				result.set_value("|=");
				++f_pos;
				break;

			case '|': // logical or
				result.set_value("||");
				++f_pos;
				if(f_pos != f_input.end()) {
					if(*f_pos == '=') {
						// logical or and assign
						result.set_value("||=");
						++f_pos;
					}
				}
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '^': // bitwise xor
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // bitwise xor & assign
				result.set_value("^=");
				++f_pos;
				break;

			case '^': // logical xor
				result.set_value("^^");
				++f_pos;
				if(f_pos != f_input.end()) {
					if(*f_pos == '=') {
						// logical xor and assign
						result.set_value("^^=");
						++f_pos;
					}
				}
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '!': // logical not
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // not equal
				result.set_value("!=");
				++f_pos;
				if(f_pos != f_input.end()) {
					if(*f_pos == '=') {
						// exactly not equal (type checked)
						result.set_value("!==");
						++f_pos;
					}
				}
				break;

			case '<': // rotate left
				result.set_value("!<");
				++f_pos;
				break;

			case '>': // rotate right
				result.set_value("!>");
				++f_pos;
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '?': // ? by itself is used here and there generally similar to C/C++
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // assign if left hand side not set
				result.set_value("?=");
				++f_pos;
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '=': // assign
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // equality check (compare)
				result.set_value("==");
				++f_pos;
				if(f_pos != f_input.end()) {
					if(*f_pos == '=') {
						// exactly equal (type checked)
						result.set_value("===");
						++f_pos;
					}
				}
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '<': // greater than
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // smaller or equal
				result.set_value("<=");
				++f_pos;
				break;

			case '<': // shift left
				result.set_value("<<");
				++f_pos;
				if(f_pos != f_input.end()) {
					if(*f_pos == '=') {
						// shift left and assign
						result.set_value("<<=");
						++f_pos;
					}
				}
				break;

			case '?': // minimum
				result.set_value("<?");
				++f_pos;
				if(f_pos != f_input.end()) {
					if(*f_pos == '=') {
						// minimum and assign
						result.set_value("<?=");
						++f_pos;
					}
				}
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '>': // less than
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // larger or equal
				result.set_value(">=");
				++f_pos;
				break;

			case '>': // shift right
				result.set_value(">>");
				++f_pos;
				if(f_pos != f_input.end()) {
					switch(f_pos->unicode()) {
					case '=':
						// shift right and assign
						result.set_value(">>=");
						++f_pos;
						break;

					case '>':
						// unsigned shift right
						result.set_value(">>>");
						++f_pos;
						if(f_pos != f_input.end()) {
							if(*f_pos == '=') {
								// unsigned right shift and assign
								result.set_value(">>>=");
								++f_pos;
							}
						}
						break;

					default:
						// ignore other characters
						break;

					}
				}
				break;

			case '?': // maximum
				result.set_value(">?");
				++f_pos;
				if(f_pos != f_input.end()) {
					if(*f_pos == '=') {
						// maximum and assign
						result.set_value(">?=");
						++f_pos;
					}
				}
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case ':':
		result.set_id(TOKEN_ID_LITERAL_ENUM);
		result.set_value(*f_pos);
		++f_pos;
		if(f_pos != f_input.end()) {
			switch(f_pos->unicode()) {
			case '=': // required
				result.set_value(":=");
				++f_pos;
				break;

			case ':': // namespace
				result.set_value("::");
				++f_pos;
				break;

			default:
				// ignore other characters
				break;

			}
		}
		break;

	case '"':
		{
			++f_pos;
			QString::const_iterator start(f_pos);
			while(f_pos != f_input.end() && *f_pos != '"') {
				if(*f_pos == '\n' || *f_pos == '\r') {
					// strings cannot continue after the end of a line
					break;
				}
				if(*f_pos == '\\') {
					++f_pos;
					if(f_pos == f_input.end()) {
						// this is an invalid backslash
						break;
					}
				}
				++f_pos;
			}
			if(f_pos == f_input.end()) {
				f_error_code = static_cast<int>(LEXER_ERROR_INVALID_STRING);
				f_error_message = "invalid string";
				f_error_line = f_line;
				result.set_id(TOKEN_ID_ERROR_ENUM);
			}
			else {
				result.set_id(TOKEN_ID_STRING_ENUM);
				result.set_value(QString(start, f_pos - start));
				++f_pos; // skip the closing quote
			}
		}
		break;

	case '0':
		// hexadecimal?
		if(f_pos + 1 != f_input.end() && (f_pos[1] == 'x' || f_pos[1] == 'X')
		&& f_pos + 2 != f_input.end() && ((f_pos[2] >= '0' && f_pos[2] <= '9')
									|| (f_pos[2] >= 'a' && f_pos[2] <= 'f')
									|| (f_pos[2] >= 'A' && f_pos[2] <= 'F'))) {
			bool ok;
			f_pos += 2; // skip the 0x or 0X
			QString::const_iterator start(f_pos);
			// parse number
			while(f_pos != f_input.end() && ((*f_pos >= '0' && *f_pos <= '9')
					|| (*f_pos >= 'a' && *f_pos <= 'f')
					|| (*f_pos >= 'A' && *f_pos <= 'F'))) {
				++f_pos;
			}
			result.set_id(TOKEN_ID_INTEGER_ENUM);
			QString value(start, f_pos - start);
			result.set_value(value.toULongLong(&ok, 16));
			if(!ok) {
				// as far as I know the only reason it can fail is because
				// it is too large (since we parsed a valid number!)
				f_error_code = static_cast<int>(LEXER_ERROR_INVALID_NUMBER);
				f_error_message = "number too large";
				f_error_line = f_line;
				result.set_id(TOKEN_ID_ERROR_ENUM);
			}
			break;
		}
		// no octal support at this point, octal is not available in
		// JavaScript by default!
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		{
			bool ok;
			// TODO: test overflows
			QString::const_iterator start(f_pos);
			// number
			do {
				++f_pos;
			} while(f_pos != f_input.end() && *f_pos >= '0' && *f_pos <= '9');
			if(*f_pos == '.') {
				// skip the decimal point
				++f_pos;

				// floating point
				while(f_pos != f_input.end() && *f_pos >= '0' && *f_pos <= '9') {
					++f_pos;
				}
				// TODO: add exponent support
				result.set_id(TOKEN_ID_FLOAT_ENUM);
				QString value(start, f_pos - start);
				result.set_value(value.toDouble(&ok));
			}
			else {
				result.set_id(TOKEN_ID_INTEGER_ENUM);
				QString value(start, f_pos - start);
				result.set_value(value.toULongLong(&ok));
			}
			if(!ok) {
				// as far as I know the only reason it can fail is because
				// it is too large (since we parsed a valid number!)
				f_error_code = static_cast<int>(LEXER_ERROR_INVALID_NUMBER);
				f_error_message = "number too large";
				f_error_line = f_line;
				result.set_id(TOKEN_ID_ERROR_ENUM);
			}
		}
		break;

	default:
		// TBD: add support for '$' for JavaScript?
		if((*f_pos >= 'a' && *f_pos <= 'z')
		|| (*f_pos >= 'A' && *f_pos <= 'Z')
		|| *f_pos == '_') {
			// identifier
			QString::const_iterator start(f_pos);
			++f_pos;
			while(f_pos != f_input.end()
				&& ((*f_pos >= 'a' && *f_pos <= 'z')
					|| (*f_pos >= 'A' && *f_pos <= 'Z')
					|| (*f_pos >= '0' && *f_pos <= '9')
					|| *f_pos == '_')) {
				++f_pos;
			}
			QString identifier(start, f_pos - start);
			if(f_keywords.contains(identifier)) {
				result.set_id(TOKEN_ID_KEYWORD_ENUM);
				result.set_value(f_keywords[identifier]);
			}
			else {
				result.set_id(TOKEN_ID_IDENTIFIER_ENUM);
				result.set_value(identifier);
			}
		}
		else {
			// in all other cases return a QChar
			result.set_id(TOKEN_ID_LITERAL_ENUM);
			result.set_value(*f_pos);
			++f_pos;
		}
		break;

	}

	return result;
}

void lexer::add_keyword(keyword& k)
{
	f_keywords[k.identifier()] = k.number();
}


int	keyword::g_next_number = 0;

keyword::keyword(lexer& parent, const QString& keyword_identifier, int index_number)
	: f_number(index_number == 0 ? ++g_next_number : index_number),
	  f_identifier(keyword_identifier)
{
	parent.add_keyword(*this);
}



rule::rule_data_t::rule_data_t()
	: f_token(TOKEN_ID_NONE_ENUM),
	  f_choices(NULL)
{
}

rule::rule_data_t::rule_data_t(const rule_data_t& s)
	: f_token(s.f_token),
	  f_value(s.f_value),
	  f_keyword(s.f_keyword),
	  f_choices(s.f_choices)
{
}

rule::rule_data_t::rule_data_t(choices& c)
	: f_token(TOKEN_ID_CHOICES_ENUM),
	  f_choices(&c)
{
}

rule::rule_data_t::rule_data_t(token_t token)
	: f_token(token),
	  f_choices(NULL)
{
}

rule::rule_data_t::rule_data_t(const QString& value)
	: f_token(TOKEN_ID_LITERAL_ENUM),
	  f_value(value),
	  f_choices(NULL)
{
}

rule::rule_data_t::rule_data_t(const keyword& k)
	: f_token(TOKEN_ID_KEYWORD_ENUM),
	  f_keyword(k),
	  f_choices(NULL)
{
}



rule::rule(choices& c)
	: f_parent(&c),
	  //f_tokens() -- auto-init
	  f_reducer(NULL)
{
}

rule::rule(const rule& r)
	: f_parent(r.f_parent),
	  f_tokens(r.f_tokens),
	  f_reducer(r.f_reducer)
{
}

void rule::add_rules(choices& c)
{
	rule_data_t data(c);
	data.f_token = TOKEN_ID_RULES_ENUM;
	f_tokens.push_back(data);
}

void rule::add_choices(choices& c)
{
	f_tokens.push_back(rule_data_t(c));
}

void rule::add_token(token_t token)
{
	f_tokens.push_back(rule_data_t(token));
}

void rule::add_literal(const QString& value)
{
	f_tokens.push_back(rule_data_t(value));
}

void rule::add_keyword(const keyword& k)
{
	f_tokens.push_back(rule_data_t(k));
}

rule& rule::operator >> (const token_id& token)
{
	add_token(token);
	return *this;
}

rule& rule::operator >> (const QString& literal)
{
	add_literal(literal);
	return *this;
}

rule& rule::operator >> (const char *literal)
{
	add_literal(literal);
	return *this;
}

rule& rule::operator >> (const keyword& k)
{
	add_keyword(k);
	return *this;
}

rule& rule::operator >> (choices& c)
{
	add_choices(c);
	return *this;
}

rule& rule::operator >= (rule::reducer_t function)
{
	set_reducer(function);
	return *this;
}

rule& operator >> (const token_id& token_left, const token_id& token_right)
{
	rule *r(new rule);
	r->add_token(token_left);
	r->add_token(token_right);
	return *r;
}

rule& operator >> (const token_id& token, const QString& literal)
{
	rule *r(new rule);
	r->add_token(token);
	r->add_literal(literal);
	return *r;
}

rule& operator >> (const token_id& token, const char *literal)
{
	rule *r(new rule);
	r->add_token(token);
	r->add_literal(literal);
	return *r;
}

rule& operator >> (const token_id& token, const keyword& k)
{
	rule *r(new rule);
	r->add_token(token);
	r->add_keyword(k);
	return *r;
}

rule& operator >> (const token_id& token, choices& c)
{
	rule *r(new rule);
	r->add_token(token);
	r->add_choices(c);
	return *r;
}

rule& operator >> (const QString& literal, const token_id& token)
{
	rule *r(new rule);
	r->add_literal(literal);
	r->add_token(token);
	return *r;
}

rule& operator >> (const QString& literal_left, const QString& literal_right)
{
	rule *r(new rule);
	r->add_literal(literal_left);
	r->add_literal(literal_right);
	return *r;
}

rule& operator >> (const QString& literal, const keyword& k)
{
	rule *r(new rule);
	r->add_literal(literal);
	r->add_keyword(k);
	return *r;
}

rule& operator >> (const QString& literal, choices& c)
{
	rule *r(new rule);
	r->add_literal(literal);
	r->add_choices(c);
	return *r;
}

rule& operator >> (const keyword& k, const token_id& token)
{
	rule *r(new rule);
	r->add_keyword(k);
	r->add_token(token);
	return *r;
}

rule& operator >> (const keyword& k, const QString& literal)
{
	rule *r(new rule);
	r->add_keyword(k);
	r->add_literal(literal);
	return *r;
}

rule& operator >> (const keyword& k_left, const keyword& k_right)
{
	rule *r(new rule);
	r->add_keyword(k_left);
	r->add_keyword(k_right);
	return *r;
}

rule& operator >> (const keyword& k, choices& c)
{
	rule *r(new rule);
	r->add_keyword(k);
	r->add_choices(c);
	return *r;
}

rule& operator >> (choices& c, const token_id& token)
{
	rule *r(new rule);
	r->add_choices(c);
	r->add_token(token);
	return *r;
}

rule& operator >> (choices& c, const QString& literal)
{
	rule *r(new rule);
	r->add_choices(c);
	r->add_literal(literal);
	return *r;
}

rule& operator >> (choices& c, const keyword& k)
{
	rule *r(new rule);
	r->add_choices(c);
	r->add_keyword(k);
	return *r;
}

rule& operator >> (choices& c_left, choices& c_right)
{
	rule *r(new rule);
	r->add_choices(c_left);
	r->add_choices(c_right);
	return *r;
}

rule& operator >= (const token_id& token, rule::reducer_t function)
{
	rule *r(new rule);
	r->add_token(token);
	r->set_reducer(function);
	return *r;
}

rule& operator >= (const QString& literal, rule::reducer_t function)
{
	rule *r(new rule);
	r->add_literal(literal);
	r->set_reducer(function);
	return *r;
}

rule& operator >= (const keyword& k, rule::reducer_t function)
{
	rule *r(new rule);
	r->add_keyword(k);
	r->set_reducer(function);
	return *r;
}

rule& operator >= (choices& c, rule::reducer_t function)
{
	rule *r(new rule);
	r->add_choices(c);
	r->set_reducer(function);
	return *r;
}

QString rule::to_string() const
{
	QString		result;

	for(QVector<rule_data_t>::const_iterator ri = f_tokens.begin();
											ri != f_tokens.end(); ++ri) {
		if(ri != f_tokens.begin()) {
			result += " ";
		}
		const rule_data_t& r(*ri);
		switch(r.f_token) {
		case TOKEN_ID_NONE_ENUM:
			result += "\xA4";  // currency sign used as the EOI marker
			break;

		case TOKEN_ID_INTEGER_ENUM:
			result += "TOKEN_ID_INTEGER";
			break;

		case TOKEN_ID_FLOAT_ENUM:
			result += "TOKEN_ID_FLOAT";
			break;

		case TOKEN_ID_IDENTIFIER_ENUM:
			result += "TOKEN_ID_IDENTIFIER";
			break;

		case TOKEN_ID_KEYWORD_ENUM:
			result += "keyword_" + r.f_keyword.identifier();
			break;

		case TOKEN_ID_STRING_ENUM:
			result += "TOKEN_ID_STRING";
			break;

		case TOKEN_ID_LITERAL_ENUM:
			result += "\"" + r.f_value + "\"";
			break;

		case TOKEN_ID_EMPTY_ENUM:
			// put the empty set for empty
			result += "\xF8";
			break;

		case TOKEN_ID_CHOICES_ENUM:
			result += r.f_choices->name();
			break;

		case TOKEN_ID_NODE_ENUM:
			result += " /* INVALID -- TOKEN_ID_NODE!!! */ ";
			break;

		case TOKEN_ID_ERROR_ENUM:
			result += " /* INVALID -- TOKEN_ID_ERROR!!! */ ";
			break;

		default:
			result += " /* INVALID -- unknown token identifier!!! */ ";
			break;

		}
	}

	if(f_reducer != NULL) {
		// show that we have a reducer
		result += " { ... }";
	}

	return result;
}




choices::choices(grammar *parent, const char *choice_name)
	: f_name(choice_name)
	  //f_rules() -- auto-init
{
	if(parent != NULL) {
		parent->add_choices(*this);
	}
}

choices::~choices()
{
	clear();
}

void choices::clear()
{
	int max(f_rules.count());
	for(int r = 0; r < max; ++r) {
		delete f_rules[r];
	}
	f_rules.clear();
}


choices& choices::operator = (const choices& rhs)
{
	if(this != &rhs) {
		//f_name -- not changed, rhs.f_name is probably "internal"

		clear();

		// copy rhs rules
		int max(rhs.f_rules.count());
		for(int r = 0; r < max; ++r) {
			f_rules.push_back(new rule(*rhs.f_rules[r]));
		}
	}

	return *this;
}

choices& choices::operator >>= (choices& rhs)
{
	if(this == &rhs) {
		throw std::runtime_error("a rule cannot just be represented as itself");
	}

	rule *r(new rule);
	r->add_choices(rhs);
	f_rules.push_back(r);

	return *this;
}

choices& choices::operator >>= (rule& r)
{
	// in this case there are no choices
	if(r[0].get_token().get_id() == TOKEN_ID_RULES_ENUM) {
		this->operator = (r[0].get_choices());
	}
	else {
		f_rules.push_back(&r);
	}

	return *this;
}

choices& choices::operator >>= (const token_id& token)
{
	rule *r = new rule;
	r->add_token(token);
	f_rules.push_back(r);

	return *this;
}

choices& choices::operator >>= (const QString& literal)
{
	rule *r = new rule;
	r->add_literal(literal);
	f_rules.push_back(r);

	return *this;
}

choices& choices::operator >>= (const keyword& k)
{
	rule *r = new rule;
	r->add_keyword(k);
	f_rules.push_back(r);

	return *this;
}


rule& choices::operator | (rule& r)
{
	// left hand-side is this
	rule *l(new rule);
	l->add_choices(*this);

	return *l | r;
}

rule& operator | (const token_id& token, rule& r_right)
{
	choices *c(new choices(NULL, "internal"));
	rule *r_left(new rule);
	r_left->add_token(token);
	c->add_rule(*r_left);
	c->add_rule(r_right);
	rule *r(new rule);
	r->add_rules(*c);
	return *r;
}

rule& operator | (rule& r_left, rule& r_right)
{
	// append to existing list?
	if(r_left[0].get_token().get_id() == TOKEN_ID_RULES_ENUM) {
		r_left[0].get_choices().add_rule(r_right);
		return r_left;
	}

	choices *c(new choices(NULL, "internal"));
	c->add_rule(r_left);
	c->add_rule(r_right);
	rule *r(new rule);
	r->add_rules(*c);
	return *r;
}

rule& operator | (rule& r, choices& c)
{
	rule *l(new rule);
	l->add_choices(c);

	return r | *l;
}

void choices::add_rule(rule& r)
{
	f_rules.push_back(&r);
}



QString choices::to_string() const
{
	QString		result(f_name + ": ");

	for(QVector<rule *>::const_iterator ri = f_rules.begin();
										ri != f_rules.end(); ++ri) {
		if(ri != f_rules.begin()) {
			result += "\n    | ";
		}
		const rule *r(*ri);
		result += r->to_string();
	}

	return result;
}






grammar::grammar()
	//: f_choices() -- auto-init
{
}

void grammar::add_choices(choices& c)
{
	f_choices.push_back(&c);
}

struct parser_state;
typedef QVector<parser_state *> state_array_t;
typedef QMap<parser_state *, int> state_map_t;
struct parser_state
{
	parser_state(parser_state *parent, choices& c, int r)
		: f_parent(parent),
		  f_choices(&c),
		  f_rule(r)
		  //f_position(0) -- auto-init
		  //f_node() -- auto-init
		  //f_add_on_reduce() -- auto-init
	{
		if(parent != NULL) {
			parent->f_children.push_back(this);
		}
	}

	~parser_state()
	{
//fprintf(stderr, "destructor! %p\n", this);
		clear();
	}

	void clear()
	{
		if(!f_children.empty()) {
			throw std::runtime_error("clearing a state that has children is not allowed");
		}
		// if we have a parent make sure we're removed from the list
		// of children of that parent
		if(f_parent != NULL) {
			int p = f_parent->f_children.indexOf(this);
			if(p < 0) {
				throw std::runtime_error("clearing a state with a parent that doesn't know about us is not allowed");
			}
			f_parent->f_children.remove(p);
			f_parent = NULL;
		}
		// delete all the states to be executed on reduce
		// if they're still here, they can be removed
		while(!f_add_on_reduce.empty()) {
			delete f_add_on_reduce.last();
			f_add_on_reduce.pop_back();
		}
		// useful for debug purposes
		f_choices = NULL;
		f_rule = -1;
		f_position = -1;
	}

	void reset(parser_state *parent, choices& c, int r)
	{
		f_parent = parent;
		if(parent != NULL) {
			parent->f_children.push_back(this);
		}
		f_choices = &c;
		f_rule = r;
		f_position = 0;
		f_node.clear();
		f_add_on_reduce.clear();
	}

	static parser_state *alloc(state_array_t& free_states, parser_state *parent, choices& c, int r)
	{
		parser_state *state;
		if(free_states.empty()) {
			state = new parser_state(parent, c, r);
		}
		else {
			state = free_states.last();
			free_states.pop_back();
			state->reset(parent, c, r);
		}
		return state;
	}

	static void free(state_array_t& current, state_array_t& free_states, parser_state *s)
	{
		// recursively free all the children
		while(!s->f_children.empty()) {
			free(current, free_states, s->f_children.last());
			//s->f_children.pop_back(); -- automatic in clear()
		}
		s->clear();
		int pos = current.indexOf(s);
		if(pos != -1) {
			current.remove(pos);
		}
		free_states.push_back(s);
	}

	static parser_state *copy(state_array_t& free_states, parser_state *source)
	{
		parser_state *state(alloc(free_states, source->f_parent, *source->f_choices, source->f_rule));
		state->f_line = source->f_line;
		state->f_position = source->f_position;
		if(source->f_node != NULL) {
			state->f_node = QSharedPointer<token_node>(new token_node(*source->f_node));
		}
		state->copy_reduce_states(free_states, source->f_add_on_reduce);
		return state;
	}

	void copy_reduce_states(state_array_t& free_states, state_array_t& add_on_reduce)
	{
		int max = add_on_reduce.size();
		for(int i = 0; i < max; ++i) {
			// we need to set the correct parent in the copy
			// and it is faster to correct in the source before the copy
			f_add_on_reduce.push_back(copy(free_states, add_on_reduce[i]));
		}
	}

	void add_token(token& t)
	{
		if(f_node == NULL) {
			f_node = QSharedPointer<token_node>(new token_node);
			f_node->set_line(f_line);
		}
		f_node->add_token(t);
	}
	void add_node(QSharedPointer<token_node> n)
	{
		if(f_node == NULL) {
			f_node = QSharedPointer<token_node>(new token_node);
			f_node->set_line(f_line);
		}
		f_node->add_node(n);
	}

	QString toString()
	{
		QString result;

		result = QString("0x%1-%2 [r:%3, p:%4/%5]").arg(reinterpret_cast<qulonglong>(this), 0, 16).arg(f_choices->name().toUtf8().data()).arg(f_rule).arg(f_position).arg((*f_choices)[f_rule].count());
		if(f_parent != NULL) {
			result += QString(" (parent 0x%5-%6)").arg(reinterpret_cast<qulonglong>(f_parent), 0, 16).arg(f_parent->f_choices->name().toUtf8().data());
		}

		return result;
	}

	/** \brief Display an array of states.
	 *
	 * This function displays the array of states as defined by the parameter
	 * \p a. This prints all the parents of each element and also the list
	 * of add on reduce if any.
	 *
	 * \param[in] a  The array to be displayed.
	 */
	static void display_array(const state_array_t& a)
	{
		fprintf(stderr, "+++ ARRAY (%d items)\n", a.size());
		for(state_array_t::const_iterator it(a.begin()); it != a.end(); ++it) {
			parser_state *state(*it);
			//fprintf(stderr, "  state = %p\n", state); // for crash
			fprintf(stderr, "  current: %s\n", state->toString().toUtf8().data());
			for(state_array_t::const_iterator r(state->f_add_on_reduce.begin()); r != state->f_add_on_reduce.end(); ++r) {
				parser_state *s(*r);
				fprintf(stderr, "      add on reduce: %s\n", s->toString().toUtf8().data());
			}
			while(state->f_parent != NULL) {
				state = state->f_parent;
				fprintf(stderr, "    parent: %s\n", state->toString().toUtf8().data());
			}
		}
		fprintf(stderr, "---\n");
	}

	int								f_line;
	parser_state *					f_parent;
	state_array_t					f_children;

	choices *						f_choices;
	controlled_vars::zint32_t		f_rule;
	controlled_vars::zint32_t		f_position;

	QSharedPointer<token_node>		f_node;
	state_array_t					f_add_on_reduce;
};

/** \brief Move to the next token in a rule.
 *
 * Each state includes a position in one specific rule. This function moves
 * that pointer to the next position.
 *
 * When the end of the rule is reached, then the rule gets reduced. This means
 * calling the user reduce function and removing the rule from the current list
 * and replacing it with its parent.
 *
 * Reducing means removing the current state and putting it the list of
 * free state after we added the node tree to its parent. The parent is
 * then added to the list of current state as it becomes current again.
 *
 * When reducing a rule and moving up to the parent, the parent may then need
 * reduction too! Thus, the function loops and reduce this state and all of
 * its parent until a state that cannot be reduced anymore.
 *
 * This function also detects recursive rules and place those in the current
 * stack of states as expected. Note that next_token() is called on the
 * recursive rule too. This is a recursive function call, but it is very
 * unlikely to be called more than twice.
 *
 * \param[in] state  The state being moved.
 * \param[in] current  The list of current states
 * \param[in] free_states  The list of free states
 */
void next_token(parser_state *state, state_array_t& current, state_array_t& free_states)
{
	bool repeat;
	do {
		repeat = false;
		// move forward to the next token in this rule
		++state->f_position;
		if(state->f_position >= (*state->f_choices)[state->f_rule].count()) {
			if(state->f_position == (*state->f_choices)[state->f_rule].count()) {
				repeat = true;
				// we reached the end of the rule, we can reduce it!
				// call user function
				(*state->f_choices)[state->f_rule].reduce(state->f_node);
				// put our node as the rule result in the parent
				// add the recusive children in the current stack
				//int max = state->f_add_on_reduce.size();
				parser_state *p(state->f_parent);
				int max = p->f_add_on_reduce.size();
				for(int i = 0; i < max; ++i) {
					parser_state *s(parser_state::copy(free_states, p->f_add_on_reduce[i]));
					s->add_node(state->f_node);
					current.push_back(s);
					next_token(s, current, free_states);
				}

				if(p->f_children.size() > 1) {
					// the parent has several children which means we may get
					// more than one reduce... to support that possibility
					// duplicate the parent now
					parser_state *new_parent(parser_state::copy(free_states, p));
					p = new_parent;
				}
				p->add_node(state->f_node);

				// remove this state from the current set of rules
				parser_state::free(current, free_states, state);

				// continue with the parent which will get its
				// position increased on the next iteration
				state = p;
				current.push_back(state);
			}
			else {
				// forget about that state; we're reducing it for the second time?!
				parser_state::free(current, free_states, state);
			}
		}
		// else -- the user is not finished with this state
	} while(repeat);
}

bool grammar::parse(lexer& input, choices& start)
{
	// the result of the parser against the lexer is a tree of tokens
	//
	// to run the parser, we need a state, this can be defined locally
	// because we do not need it in the result;
	//
	// create the root rule
	choices root(this, "root");
	root >>= start >> TOKEN_ID_NONE;
	parser_state *s = new parser_state(NULL, root, 0);
	s->f_line = 1;

	state_array_t free_states;
	state_array_t current;
	current.push_back(s);
	while(!current.empty()) {
		// we're working on the 'check' vector which is
		// a copy of the current vector so the current
		// vector can change in size
//fprintf(stderr, "=================================================================\n");
//parser_state::display_array(current);

		uint32_t line(input.line());
		bool retry;
		//state_array_t new_states;
		do {
			retry = false;
			state_array_t check(current);
			for(state_array_t::const_iterator it(check.begin());
							it != check.end(); ++it) {
				// it is a state, check whether the current entry
				// is a token or a rule
				parser_state *state(*it);
				const rule::rule_ref ref((*state->f_choices)[state->f_rule][state->f_position]);
				token_t token_id(ref.get_token().get_id());

				// only take care of choices in this loop (terminators are
				// handled in the next loop)
				if(token_id == TOKEN_ID_CHOICES_ENUM) {
					// follow the choice by adding all of the rules it points to
					choices *c(&ref.get_choices());

					int max = c->count();
					for(int r = 0; r < max; ++r) {

						parser_state *child;
						if(free_states.empty()) {
							child = new parser_state(state, *c, r);
						}
						else {
							child = free_states.last();
							free_states.pop_back();
							child->reset(state, *c, r);
						}
						child->f_line = line;

						// check whether this is recursive; very important
						// to avoid infinite loop; recurvise rules are used
						// only when the concern rule gets reduced
						// the child position is always 0 here (it's a new child)
						controlled_vars::zbool_t recursive;
						const rule::rule_ref child_ref((*c)[r][0]);
						token_t child_token_id(child_ref.get_token().get_id());
						if(child_token_id == TOKEN_ID_CHOICES_ENUM) {
							// if the new child state starts with a 'choices'
							// and that's a 'choices' we already added
							// (including this very child,) then
							// that child is recursive
							choices *child_choices(&child_ref.get_choices());
							parser_state *p(child); // start from ourselves
							while(p != NULL) {
								if(child_choices == p->f_choices) {
									if(p->f_parent == NULL) {
										throw std::runtime_error("invalid recursion (root cannot be recursive)");
									}
									// p may be ourselves so we cannot put that
									// there, use the parent instead
									p->f_parent->f_add_on_reduce.push_back(child);
									recursive = true;
									break;
								}
								p = p->f_parent;
							}
						}

						// if recursive it was already added to all the
						// states were it needs to be; otherwise we add it
						// to the current stack
						if(!recursive) {
							//new_states.push_back(child);
							current.push_back(child);
						}
					}
					current.remove(current.indexOf(state));
					retry = true;
				}
				else if(token_id == TOKEN_ID_EMPTY_ENUM) {
					// we have to take care of empty rules here since anything
					// coming after an empty rule has to be added to the list
					// of rules here (it is very important because of the
					// potential for recursive rules)
					token t(TOKEN_ID_EMPTY_ENUM);
					state->add_token(t);
					next_token(state, current, free_states);
					retry = true;
				}
			}
		} while(retry);

		// get the first token
		token t(input.next_token());

		state_array_t check(current);
		for(state_array_t::const_iterator it(check.begin());
						it != check.end(); ++it) {
			// it is a state, check whether the current entry
			// is a token or a rule
			parser_state *state(*it);
			const rule::rule_ref ref((*state->f_choices)[state->f_rule][state->f_position]);
			token_t token_id(ref.get_token().get_id());
			if(token_id == TOKEN_ID_CHOICES_ENUM
			|| token_id == TOKEN_ID_EMPTY_ENUM) {
				throw std::runtime_error("this should never happen since the previous for() loop removed all of those!");
			}
			else {
				bool remove(false);
				if(t.get_id() != token_id) {
					remove = true;
				}
				else {
					switch(token_id) {
					case TOKEN_ID_LITERAL_ENUM:
						// a literal must match exactly
						if(t.get_value().toString() != ref.get_value()) {
							remove = true;
						}
						break;

					case TOKEN_ID_KEYWORD_ENUM:
						// a keyword must match exactly
						if(t.get_value().toInt() != ref.get_keyword().number()) {
							remove = true;
						}
						break;

					case TOKEN_ID_IDENTIFIER_ENUM:
					case TOKEN_ID_STRING_ENUM:
					case TOKEN_ID_INTEGER_ENUM:
					case TOKEN_ID_FLOAT_ENUM:
						// this is a match whatever the value
						break;

					case TOKEN_ID_NONE_ENUM:
						// this state is the root state, this means the result
						// is really the child node of this current state
						f_result = qSharedPointerDynamicCast<token_node, token>((*state->f_node)[0]);
						return true;

					default:
						// at this point other tokens are rejected here
						throw snap_parser_unexpected_token();

					}
				}
				if(remove) {
					parser_state::free(current, free_states, state);
				}
				else {
					// save this token as it was accepted
					state->add_token(t);
					next_token(state, current, free_states);
				}
			}
		}
	}

	return false;
}



} // namespace parser
} // namespace snap

// vim: ts=4 sw=4
