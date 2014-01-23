// Snap Websites Server -- manage debug, info, warning, error messages
// Copyright (C) 2013-2014  Made to Order Software Corp.
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
#include <controlled_vars/controlled_vars_limited_need_init.h>

namespace snap
{
namespace messages
{

class messages_exception : public snap_exception {};
class messages_exception_invalid_field_name : public messages_exception {};
class messages_exception_already_defined : public messages_exception {};

enum name_t
{
    SNAP_NAME_MESSAGES_MESSAGES,
    SNAP_NAME_MESSAGES_WARNING_HEADER
};
const char *get_name(name_t name) __attribute__ ((const));


class messages : public plugins::plugin, public layout::layout_content, public QtSerialization::QSerializationObject
{
public:
    static const int MESSAGES_MAJOR_VERSION = 1;
    static const int MESSAGES_MINOR_VERSION = 0;

    class message : public QtSerialization::QSerializationObject
    {
    public:
        enum message_type_enum_t
        {
            MESSAGE_TYPE_ERROR,
            MESSAGE_TYPE_WARNING,
            MESSAGE_TYPE_INFO,
            MESSAGE_TYPE_DEBUG
        };
        typedef controlled_vars::limited_need_init<message_type_enum_t, MESSAGE_TYPE_ERROR, MESSAGE_TYPE_DEBUG> message_type_t;

                            message();
                            message(message_type_t t, const QString& title, const QString& body);
                            message(const message& rhs);

        message_type_enum_t get_type() const;
        int                 get_id() const;
        const QString&      get_title() const;
        const QString&      get_body() const;

        // internal functions used to save the data serialized
        void                unserialize(QtSerialization::QReader& r);
        virtual void        readTag(const QString& name, QtSerialization::QReader& r);
        void                serialize(QtSerialization::QWriter& w) const;

    private:
        message_type_t              f_type;
        controlled_vars::mint32_t   f_id;
        QString                     f_title;
        QString                     f_body;
    };

                        messages();
                        ~messages();

    static messages *   instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    virtual void        on_generate_main_content(layout::layout *l, content::path_info_t& path, QDomElement& page, QDomElement& body, const QString& ctemplate);
    void                on_generate_page_content(layout::layout *l, content::path_info_t& path, QDomElement& page, QDomElement& body, const QString& ctemplate);
    void                on_attach_to_session();
    void                on_detach_from_session();

    void                set_http_error(snap_child::http_code_t err_code, QString err_name, const QString& err_description, const QString& err_details, bool err_security);
    void                set_error(QString err_name, const QString& err_description, const QString& err_details, bool err_security);
    void                set_warning(QString warning_name, const QString& warning_description, const QString& warning_details);
    void                set_info(QString info_name, const QString& info_description);
    void                set_debug(QString debug_name, const QString& debug_description);

    const message&      get_last_message() const;
    int                 get_error_count() const;
    int                 get_warning_count() const;

    // internal functions used to save the data serialized
    void                unserialize(const QString& data);
    virtual void        readTag(const QString& name, QtSerialization::QReader& r);
    QString             serialize() const;

private:
    void content_update(int64_t variables_timestamp);

    zpsnap_child_t              f_snap;
    QVector<message>            f_messages;
    controlled_vars::zint32_t   f_error_count;
    controlled_vars::zint32_t   f_warning_count;
};

} // namespace messages
} // namespace snap
// vim: ts=4 sw=4 et
