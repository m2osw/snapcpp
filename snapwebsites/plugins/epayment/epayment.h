// Snap Websites Server -- handle electronic and not so electronic payments
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

#include "../content/content.h"

/** \file
 * \brief Header of the epayment plugin.
 *
 * The file defines the various epayment plugin classes.
 */

namespace snap
{
namespace epayment
{


enum name_t
{
    SNAP_NAME_EPAYMENT_PRICE,
    SNAP_NAME_EPAYMENT_PRODUCT_DESCRIPTION,
    SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH
};
char const *get_name(name_t name) __attribute__ ((const));


//class epayment_exception : public snap_exception
//{
//public:
//    epayment_exception(char const *       what_msg) : snap_exception("epayment", what_msg) {}
//    epayment_exception(std::string const& what_msg) : snap_exception("epayment", what_msg) {}
//    epayment_exception(QString const&     what_msg) : snap_exception("epayment", what_msg) {}
//};








class epayment : public plugins::plugin
{
public:
                                epayment();
                                ~epayment();

    static epayment *           instance();
    virtual QString             description() const;
    virtual int64_t             do_update(int64_t last_updated);

    void                        on_bootstrap(snap_child *snap);
    void                        on_generate_header_content(content::path_info_t& path, QDomElement& header, QDomElement& metadata, QString const& ctemplate);

    SNAP_SIGNAL_WITH_MODE(generate_invoice, (content::path_info_t& invoice_ipath, uint64_t& invoice_number), (invoice_ipath, invoice_number), NEITHER);

private:
    void                        content_update(int64_t variables_timestamp);

    zpsnap_child_t              f_snap;
};


} // namespace epayment
} // namespace snap
// vim: ts=4 sw=4 et
