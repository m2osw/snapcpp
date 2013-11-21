// Snap Websites Server -- filter
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
#ifndef SNAP_FILTER_H
#define SNAP_FILTER_H

#include "snap_child.h"
#include "plugins.h"
#include <QtXml/QDomDocument>

namespace snap
{
namespace filter
{

class filter : public plugins::plugin
{
public:
	enum token_t
	{
		TOK_IDENTIFIER,
		TOK_STRING,
		TOK_INTEGER,
		TOK_REAL,
		TOK_SEPARATOR,
		TOK_INVALID
	};

	// TODO: change that in a class
	struct parameter_t
	{
		token_t                 f_type;
		QString                 f_name;
		QString                 f_value;

		void reset()
		{
			f_type = TOK_INVALID;
			f_name = "";
			f_value = "";
		}
	};

	// TODO: change that in a class
	struct token_info_t
	{
		QString                 	f_name;
		QVector<parameter_t>    	f_parameters;
		controlled_vars::fbool_t	f_found;
		QString						f_replacement;

		bool is_token(const char *name)
		{
			// in a way, once marked as found a token is viewed as used up
			// and thus it doesn't match anymore
			bool result(!f_found && f_name == name);
			if(result)
			{
				f_found = true;
			}
			return result;
		}

		void reset()
		{
			f_name = "";
			f_parameters.clear();
			f_found = false;
			f_replacement = "";
		}
	};

	filter();
	~filter();

	static filter *		instance();
	virtual QString		description() const;

	void	on_bootstrap(::snap::snap_child *snap);
	void	on_xss_filter(QDomNode& node,
						  const QString& accepted_tags,
						  const QString& accepted_attributes);
	void 	on_token_filter(QDomDocument& xml);

	SNAP_SIGNAL(replace_token, (filter *f, QDomDocument& xml, token_info_t& token), (f, xml, token));

private:
	snap_child *	f_snap;
};

} // namespace filter
} // namespace snap
#endif
// SNAP_FILTER_H
// vim: ts=4 sw=4
