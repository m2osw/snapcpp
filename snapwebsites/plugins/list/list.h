// Snap Websites Server -- list management (sort criteria)
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

#include "../layout/layout.h"


namespace snap
{
namespace list
{


enum name_t
{
    SNAP_NAME_LIST_SETUP
};
char const *get_name(name_t name) __attribute__ ((const));


class list_exception : public snap_exception
{
public:
    list_exception(char const *what_msg)        : snap_exception("list", std::string(what_msg)) {}
    list_exception(std::string const& what_msg) : snap_exception("list", what_msg) {}
    list_exception(QString const& what_msg)     : snap_exception("list", what_msg.toStdString()) {}
};

class list_exception_invalid : public list_exception
{
public:
    list_exception_invalid(char const *what_msg)        : list_exception(what_msg) {}
    list_exception_invalid(std::string const& what_msg) : list_exception(what_msg) {}
    list_exception_invalid(QString const& what_msg)     : list_exception(what_msg) {}
};




class list_atom
{
public:
    enum comparator_t
    {
        LIST_ATOM_COMPARATOR_STRING,                // load data as UTF-8 strings, make lowercase and compare
        LIST_ATOM_COMPARATOR_STRING_WITH_CASE,      // load data as UTF-8 strings, and compare as is
        LIST_ATOM_COMPARATOR_ANY_STRING,            // load data using dbutils, make lowercase and compare
        LIST_ATOM_COMPARATOR_ANY_STRING_WITH_CASE,  // load data using dbutils, and compare as is
        LIST_ATOM_COMPARATOR_INT8,                  // load data as int8_t and compare as is
        LIST_ATOM_COMPARATOR_UINT8,                 // load data as uint8_t and compare as is
        LIST_ATOM_COMPARATOR_INT16,                 // load data as int16_t and compare as is
        LIST_ATOM_COMPARATOR_UINT16,                // load data as uint16_t and compare as is
        LIST_ATOM_COMPARATOR_INT32,                 // load data as int32_t and compare as is
        LIST_ATOM_COMPARATOR_UINT32,                // load data as uint32_t and compare as is
        LIST_ATOM_COMPARATOR_INT64,                 // load data as int64_t and compare as is
        LIST_ATOM_COMPARATOR_UINT64                 // load data as uint64_t and compare as is
    };
    typedef controlled_vars::limited_auto_init<comparator_t, LIST_ATOM_COMPARATOR_STRING, LIST_ATOM_COMPARATOR_UINT64, LIST_ATOM_COMPARATOR_STRING> safe_comparator_t;

    void                        set_comparator(comparator_t comparator);
    void                        set_column_name(QString const& name);
    void                        set_descending(bool descending);

    comparator_t                get_comparator() const;
    QString const&              get_column_name() const;
    bool                        get_descending() const;

    // internal functions used to save the data serialized
    // (you should only use the serialization of the list class)
    void                        unserialize(QtSerialization::QReader& r);
    virtual void                readTag(QString const& name, QtSerialization::QReader& r);
    void                        serialize(QtSerialization::QWriter& w) const;

private:
    QString                     f_column_name;
    controlled_vars::fbool_t    f_descending;
    safe_comparator_t           f_comparator;
};
typedef QVector<list_atom> list_atom_vector_t;






class list : public plugins::plugin, public layout::layout_content, public QtSerialization::QSerializationObject
{
public:
    static int const LIST_ATOMS_MAJOR_VERSION = 1;
    static int const LIST_ATOMS_MINOR_VERSION = 0;

                        list();
                        ~list();

    static list *       instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    virtual void        on_generate_main_content(content::path_info_t& path, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                on_generate_page_content(content::path_info_t& path, QDomElement& page, QDomElement& body, QString const& ctemplate);

    void                unserialize(QString const& data);
    virtual void        readTag(QString const& name, QtSerialization::QReader& r);
    QString             serialize() const;

private:
    void                content_update(int64_t variables_timestamp);

    zpsnap_child_t      f_snap;
    list_atom_vector_t  f_list_atoms;
};


} // namespace list
} // namespace snap
// vim: ts=4 sw=4 et
