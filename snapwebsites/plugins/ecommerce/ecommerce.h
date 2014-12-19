// Snap Websites Server -- handle a cart, checkout, wishlist, affiliates...
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

#include "../path/path.h"

/** \file
 * \brief Header of the ecommerce plugin.
 *
 * The file defines the various ecommerce plugin classes.
 */

namespace snap
{
namespace ecommerce
{


enum name_t
{
    SNAP_NAME_ECOMMERCE_CART_PRODUCTS,
    SNAP_NAME_ECOMMERCE_CART_PRODUCTS_POST_FIELD,
    SNAP_NAME_ECOMMERCE_JAVASCRIPT_CART,
    SNAP_NAME_ECOMMERCE_PRICE,
    SNAP_NAME_ECOMMERCE_PRODUCT_DESCRIPTION,
    SNAP_NAME_ECOMMERCE_PRODUCT_TYPE_PATH
};
char const *get_name(name_t name) __attribute__ ((const));


//class ecommerce_exception : public snap_exception
//{
//public:
//    ecommerce_exception(char const *       what_msg) : snap_exception("ecommerce", what_msg) {}
//    ecommerce_exception(std::string const& what_msg) : snap_exception("ecommerce", what_msg) {}
//    ecommerce_exception(QString const&     what_msg) : snap_exception("ecommerce", what_msg) {}
//};








class ecommerce : public plugins::plugin
                , public path::path_execute
{
public:
                                ecommerce();
    virtual                     ~ecommerce();

    static ecommerce *          instance();
    virtual QString             description() const;
    virtual int64_t             do_update(int64_t last_updated);

    void                        on_bootstrap(snap_child *snap);
    void                        on_generate_header_content(content::path_info_t& path, QDomElement& header, QDomElement& metadata, QString const& ctemplate);
    void                        on_process_post(QString const& uri_path);
    void                        on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info);
    virtual bool                on_path_execute(content::path_info_t& ipath);

private:
    void                        content_update(int64_t variables_timestamp);

    zpsnap_child_t              f_snap;
};


} // namespace ecommerce
} // namespace snap
// vim: ts=4 sw=4 et
