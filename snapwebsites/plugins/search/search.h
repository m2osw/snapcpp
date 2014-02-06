// Snap Websites Server -- search capability
// Copyright (C) 2012-2014  Made to Order Software Corp.
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

#include "../layout/layout.h"

namespace snap
{
namespace search
{

enum name_t
{
    SNAP_NAME_SEARCH_STATUS
};
char const *get_name(name_t name) __attribute__ ((const));



class search : public plugins::plugin
{
public:
                            search();
                            ~search();

    static search *         instance();
    virtual QString         description() const;
    virtual int64_t         do_update(int64_t last_updated);

    void                    on_bootstrap(::snap::snap_child *snap);
    void                    on_improve_signature(QString const& path, QString& signature);
    void                    on_generate_page_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate);

private:
    void                    content_update(int64_t variables_timestamp);

    zpsnap_child_t          f_snap;
};

} // namespace search
} // namespace snap
// vim: ts=4 sw=4 et
