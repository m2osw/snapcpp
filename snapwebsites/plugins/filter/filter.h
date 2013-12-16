// Snap Websites Server -- filter
// Copyright (C) 2011-2013  Made to Order Software Corp.
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
#include "not_reached.h"

namespace snap
{
namespace filter
{

class filter : public plugins::plugin
{
public:
    enum token_t
    {
        TOK_UNDEFINED,
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

        parameter_t()
            : f_type(TOK_UNDEFINED)
            //, f_name("") -- auto-init
            //, f_value("") -- auto-init
        {
        }

        void reset()
        {
            f_type = TOK_INVALID;
            f_name = "";
            f_value = "";
        }

        bool operator == (const parameter_t& rhs) const
        {
            return f_name == rhs.f_name;
        }
        bool operator < (const parameter_t& rhs) const
        {
            return f_name < rhs.f_name;
        }

        static const char *token_name(token_t type)
        {
            switch(type)
            {
            case TOK_UNDEFINED:
                return "undefined";

            case TOK_IDENTIFIER:
                return "identifier";

            case TOK_STRING:
                return "string";

            case TOK_INTEGER:
                return "integer";

            case TOK_REAL:
                return "real";

            case TOK_SEPARATOR:
                return "separator";

            case TOK_INVALID:
                return "invalid";

            }
            NOTREACHED();
        }
    };

    // TODO: change that in a class
    struct token_info_t
    {
        QString                     f_name;
        QVector<parameter_t>        f_parameters;
        controlled_vars::fbool_t    f_found;
        controlled_vars::fbool_t    f_error;
        QString                     f_replacement;

        bool is_namespace(const char *name)
        {
            return f_name.startsWith(name);
        }

        bool is_token(const char *name)
        {
            // in a way, once marked as found a token is viewed as used up
            // and thus it doesn't match anymore; same with errors
            bool result(!f_found && !f_error && f_name == name);
            if(result)
            {
                f_found = true;
            }
            return result;
        }

        bool verify_args(int min, int max)
        {
            if(min < 0 || max < -1)
            {
                throw std::runtime_error("detected a minimum and/or maximum smaller than zero in token_info_t::args()");
            }
            if(min > max && max != -1)
            {
                throw std::runtime_error("detected a minimum larger than the maximum in token_info_t::args()");
            }
            const int size(f_parameters.size());
            const bool valid(size >= min && (max == -1 || size <= max));
            if(!valid)
            {
                f_error = true;
                f_found = true;
                f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> " + f_name + " expects ";
                if(min == max)
                {
                    if(min == 0)
                    {
                        f_replacement += "no arguments";
                    }
                    else if(min == 1)
                    {
                        f_replacement += "exactly 1 argument";
                    }
                    else
                    {
                        f_replacement += QString("exactly %1 arguments").arg(min);
                    }
                }
                else
                {
                    if(min == 0)
                    {
                        if(max == 1)
                        {
                            f_replacement += "at most 1 argument";
                        }
                        else
                        {
                            f_replacement += QString("at most %1 arguments").arg(max);
                        }
                    }
                    else if(max == -1)
                    {
                        if(min == 1)
                        {
                            f_replacement += "at least 1 argument";
                        }
                        else
                        {
                            f_replacement += QString("at least %1 arguments").arg(min);
                        }
                    }
                    else
                    {
                        f_replacement += QString("between %1 and %2 arguments").arg(min).arg(max);
                    }
                }
                f_replacement += "</span>";
            }
            return valid;
        }

        parameter_t get_arg(const QString& name, int position = -1, token_t type = TOK_UNDEFINED)
        {
            parameter_t null;
            QVector<parameter_t>::const_iterator it(f_parameters.end());
            if(!name.isEmpty())
            {
                parameter_t p;
                p.f_name = name;
                it = std::find(f_parameters.begin(), f_parameters.end(), p);
                if(it == f_parameters.end() && position == -1)
                {
                    f_error = true;
                    f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> " + name + " is missing from the list of parameters, you may need to name your parameters.</span>";
                    return null;
                }
            }
            if(it == f_parameters.end() && position != -1)
            {
                if(position >= 0 && position < f_parameters.size())
                {
                    it = f_parameters.begin() + position;
                }
            }
            if(it == f_parameters.end())
            {
                f_error = true;
                f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> parameter \"" + name + "\" (position: " + QString("%1").arg(position) + ") was not found in the list.</span>";
                return null;
            }
            if(type != TOK_UNDEFINED && it->f_type != type)
            {
                f_error = true;
                f_replacement = "<span class=\"filter-error\"><span class=\"filter-error-word\">ERROR:</span> parameter \"" + name + "\" (position: " + QString("%1").arg(position) + ") is a " + parameter_t::token_name(it->f_type) + " not of the expected type: " + parameter_t::token_name(type) + ".</span>";
                return null;
            }
            return *it;
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

    static filter *     instance();
    virtual QString     description() const;

    void                on_bootstrap(::snap::snap_child *snap);
    void                on_xss_filter(QDomNode& node, QString const& accepted_tags, QString const& accepted_attributes);
    void                on_token_filter(QString const& cpath, QDomDocument& xml);

    SNAP_SIGNAL(replace_token, (filter *f, QString const& cpath, QDomDocument& xml, token_info_t& token), (f, cpath, xml, token));

private:
    snap_child *    f_snap;
};

} // namespace filter
} // namespace snap
#endif
// SNAP_FILTER_H
// vim: ts=4 sw=4 et
