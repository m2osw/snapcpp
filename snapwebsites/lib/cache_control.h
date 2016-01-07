// Snap Websites Servers -- parse and memorize cache control settings
// Copyright (C) 2011-2016  Made to Order Software Corp.
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

//#include "cache_control.h"
//#include "snap_uri.h"
//#include "snap_signals.h"
//#include "snap_exception.h"
//#include "snap_version.h"
//#include "http_cookie.h"
//#include "udp_client_server.h"

#include <controlled_vars/controlled_vars_auto_init.h>
#include <controlled_vars/controlled_vars_limited_auto_enum_init.h>

//#include <QtCassandra/QCassandra.h>
//#include <QtCassandra/QCassandraContext.h>

//#include <stdlib.h>

#include <QString>

namespace snap
{


class cache_control_settings
{
public:
    // From spec: "HTTP/1.1 servers SHOULD NOT send Expires dates
    //            more than one year in the future."
    //
    static int64_t const            AGE_MAXIMUM = 365 * 24 * 60 * 60;

                    cache_control_settings();
                    cache_control_settings(QString const & info, bool const internal_setup);

    // general handling
    void                            reset_cache_info();
    void                            set_cache_info(QString const & info, bool const internal_setup);

    // response only (server)
    void                            set_must_revalidate(bool must_revalidate);
    bool                            get_must_revalidate() const;

    void                            set_private(bool private_cache);
    bool                            get_private() const;

    void                            set_proxy_revalidate(bool proxy_revalidate);
    bool                            get_proxy_revalidate() const;

    void                            set_public(bool public_cache);
    bool                            get_public() const;

    // request and response (client and server)
    void                            set_max_age(int64_t max_age);
    void                            set_max_age(QString const & max_age);
    void                            update_max_age(int64_t max_age);
    int64_t                         get_max_age() const;

    void                            set_no_cache(bool no_cache);
    bool                            get_no_cache() const;

    void                            set_no_store(bool no_store);
    bool                            get_no_store() const;

    void                            set_no_transform(bool no_transform);
    bool                            get_no_transform() const;

    void                            set_s_maxage(int64_t s_maxage);
    void                            set_s_maxage(QString const & s_maxage);
    void                            update_s_maxage(int64_t s_maxage);
    int64_t                         get_s_maxage() const;

    // request only (client)
    void                            set_max_stale(int64_t max_stale);
    void                            set_max_stale(QString const & max_stale);
    int64_t                         get_max_stale() const;

    void                            set_min_fresh(int64_t min_fresh);
    void                            set_min_fresh(QString const & min_fresh);
    int64_t                         get_min_fresh() const;

    void                            set_only_if_cached(bool only_if_cached);
    bool                            get_only_if_cached() const;

    static int64_t                  string_to_seconds(QString const & max_age);
    static int64_t                  minimum(int64_t a, int64_t b);

private:
    typedef controlled_vars::auto_init<int64_t, -1>    m1int64_t;

    // in alphabetical order
    controlled_vars::zint64_t       f_max_age;
    m1int64_t                       f_max_stale;
    m1int64_t                       f_min_fresh;
    controlled_vars::tlbool_t       f_must_revalidate;
    controlled_vars::flbool_t       f_no_cache;
    controlled_vars::flbool_t       f_no_store;
    controlled_vars::flbool_t       f_no_transform;
    controlled_vars::flbool_t       f_only_if_cached;
    controlled_vars::flbool_t       f_private;
    controlled_vars::flbool_t       f_proxy_revalidate;
    controlled_vars::flbool_t       f_public;
    m1int64_t                       f_s_maxage;
};


} // namespace snap
// vim: ts=4 sw=4 et
