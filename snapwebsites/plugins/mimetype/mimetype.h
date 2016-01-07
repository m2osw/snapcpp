// Snap Websites Server -- handle the basic display of the website content
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

#include "../output/output.h"


namespace snap
{
namespace mimetype
{


//enum class name_t
//{
//    SNAP_NAME_MIMETYPE_ACCEPTED
//};
//char const * get_name(name_t name) __attribute__ ((const));


class mimetype_exception : public snap_exception
{
public:
    mimetype_exception(char const *        what_msg) : snap_exception("mimetype", what_msg) {}
    mimetype_exception(std::string const & what_msg) : snap_exception("mimetype", what_msg) {}
    mimetype_exception(QString const &     what_msg) : snap_exception("mimetype", what_msg) {}
};

class mimetype_exception_invalid_content_xml : public mimetype_exception
{
public:
    mimetype_exception_invalid_content_xml(char const *        what_msg) : mimetype_exception(what_msg) {}
    mimetype_exception_invalid_content_xml(std::string const & what_msg) : mimetype_exception(what_msg) {}
    mimetype_exception_invalid_content_xml(QString const &     what_msg) : mimetype_exception(what_msg) {}
};







class mimetype : public plugins::plugin
{
public:
                        mimetype();
                        ~mimetype();

    // plugins::plugin implementation
    static mimetype *   instance();
    virtual QString     description() const;
    virtual QString     dependencies() const;
    virtual int64_t     do_update(int64_t last_updated);
    virtual void        bootstrap(snap_child * snap);

private:
    void                content_update(int64_t variables_timestamp);

    zpsnap_child_t      f_snap;
};


} // namespace mimetype
} // namespace snap
// vim: ts=4 sw=4 et
