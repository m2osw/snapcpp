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

#include "snapwebsites.h"

/** \file
 * \brief Header of the ecommerce plugin.
 *
 * The file defines the various ecommerce plugin classes.
 */

namespace snap
{
namespace ecommerce
{


//enum name_t
//{
//    SNAP_NAME_ECOMMERCE_NAME
//};
//char const *get_name(name_t name) __attribute__ ((const));


//class ecommerce_exception : public snap_exception
//{
//public:
//    ecommerce_exception(char const *       what_msg) : snap_exception("ecommerce", what_msg) {}
//    ecommerce_exception(std::string const& what_msg) : snap_exception("ecommerce", what_msg) {}
//    ecommerce_exception(QString const&     what_msg) : snap_exception("ecommerce", what_msg) {}
//};








class ecommerce : public plugins::plugin
{
public:
                                ecommerce();
                                ~ecommerce();

    static ecommerce *          instance();
    virtual QString             description() const;
    virtual int64_t             do_update(int64_t last_updated);

    void                        on_bootstrap(snap_child *snap);

    //SNAP_SIGNAL_WITH_MODE(set_locale, (), (), START_AND_DONE);
    //SNAP_SIGNAL_WITH_MODE(set_timezone, (), (), START_AND_DONE);

private:
    zpsnap_child_t              f_snap;
};


} // namespace ecommerce
} // namespace snap
// vim: ts=4 sw=4 et
