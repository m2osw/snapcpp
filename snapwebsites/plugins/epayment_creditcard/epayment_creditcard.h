// Snap Websites Server -- epayment credit card extension
// Copyright (C) 2014-2016  Made to Order Software Corp.
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

namespace snap
{
namespace epayment_creditcard
{


enum class name_t
{
    SNAP_NAME_EPAYMENT_CREDITCARD_DEFAULT_COUNTRY,
    SNAP_NAME_EPAYMENT_CREDITCARD_SETTINGS_PATH,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_ADDRESS2,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_COUNTRY,
    SNAP_NAME_EPAYMENT_CREDITCARD_SHOW_PROVINCE
};
char const * get_name(name_t name) __attribute__ ((const));


class epayment_creditcard_exception : public snap_exception
{
public:
    explicit epayment_creditcard_exception(char const *        what_msg) : snap_exception("server-access", what_msg) {}
    explicit epayment_creditcard_exception(std::string const & what_msg) : snap_exception("server-access", what_msg) {}
    explicit epayment_creditcard_exception(QString const &     what_msg) : snap_exception("server-access", what_msg) {}
};

class epayment_creditcard_exception_status_missing : public epayment_creditcard_exception
{
public:
    explicit epayment_creditcard_exception_status_missing(char const *        what_msg) : epayment_creditcard_exception(what_msg) {}
    explicit epayment_creditcard_exception_status_missing(std::string const & what_msg) : epayment_creditcard_exception(what_msg) {}
    explicit epayment_creditcard_exception_status_missing(QString const &     what_msg) : epayment_creditcard_exception(what_msg) {}
};







class epayment_creditcard
        : public plugins::plugin
{
public:
                                epayment_creditcard();
                                ~epayment_creditcard();

    // plugins::plugin implementation
    static epayment_creditcard *instance();
    virtual QString             settings_path() const;
    virtual QString             icon() const;
    virtual QString             description() const;
    virtual QString             dependencies() const;
    virtual int64_t             do_update(int64_t last_updated);
    virtual void                bootstrap(snap_child * snap);

    // server signals
    void                        on_process_post(QString const & uri_path);

    // editor signals
    void                        on_dynamic_editor_widget(content::path_info_t & ipath, QString const & name, QDomDocument & editor_widgets);

private:
    void                        content_update(int64_t variables_timestamp);

    zpsnap_child_t              f_snap;
};


} // namespace epayment_creditcard
} // namespace snap
// vim: ts=4 sw=4 et
