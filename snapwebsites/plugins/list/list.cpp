// Snap Websites Server -- advanced handling of lists
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

#include "list.h"

//#include "../messages/messages.h"

#include "not_reached.h"

#include <QtSerialization/QSerializationComposite.h>
#include <QtSerialization/QSerializationFieldString.h>
#include <QtSerialization/QSerializationFieldBasicTypes.h>

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(list, 1, 0)

/** \brief Get a fixed list name.
 *
 * The list plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name) {
    case SNAP_NAME_LIST_SETUP:
        return "list::setup";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_OUTPUT_...");

    }
    NOTREACHED();
}





void list_atom::set_comparator(comparator_t comparator)
{
    f_comparator = static_cast<int>(comparator);
}


void list_atom::set_column_name(QString const& name)
{
    f_column_name = name;
}


void list_atom::set_descending(bool descending)
{
    f_descending = descending;
}


list_atom::comparator_t list_atom::get_comparator() const
{
    return f_comparator;
}


QString const& list_atom::get_column_name() const
{
    return f_column_name;
}


bool list_atom::get_descending() const
{
    return f_descending;
}


void list_atom::unserialize(QtSerialization::QReader& r)
{
    QtSerialization::QComposite comp;
    QtSerialization::QFieldString tag_column_name(comp, "column_name", f_column_name);
    int8_t descending;
    QtSerialization::QFieldInt8 tag_descending(comp, "descending", descending);
    int32_t comparator;
    QtSerialization::QFieldInt32 tag_comparator(comp, "comparator", comparator);
    r.read(comp);

    f_descending = descending;
    f_comparator = comparator;
}


void list_atom::readTag(QString const& name, QtSerialization::QReader& r)
{
    static_cast<void>(name);
    static_cast<void>(r);
}


void list_atom::serialize(QtSerialization::QWriter& w) const
{
    QtSerialization::QWriter::QTag tag(w, "list_atom");
    QtSerialization::writeTag(w, "column_name", f_column_name);
    QtSerialization::writeTag(w, "descending", static_cast<int8_t>(f_descending));
    QtSerialization::writeTag(w, "comparator", static_cast<int32_t>(f_comparator));
}




/** \brief Initialize the list plugin.
 *
 * This function is used to initialize the list plugin object.
 */
list::list()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the list plugin.
 *
 * Ensure the list object is clean before it is gone.
 */
list::~list()
{
}


/** \brief Initialize the list.
 *
 * This function terminates the initialization of the list plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void list::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(list, "layout", layout::layout, generate_page_content, _1, _2, _3, _4);
}


/** \brief Get a pointer to the list plugin.
 *
 * This function returns an instance pointer to the list plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the list plugin.
 */
list *list::instance()
{
    return g_plugin_list_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString list::description() const
{
    return "Generate lists of pages using a set of parameters as defined"
          " by the system (some lists are defined internally) and the end"
          " users.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t list::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 2, 4, 16, 29, 30, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void list::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Generate the page main content.
 *
 * This function generates the main content of the page. Other
 * plugins will also have the event called if they subscribed and
 * thus will be given a chance to add their own content to the
 * main page. This part is the one that (in most cases) appears
 * as the main content on the page although the content of some
 * columns may be interleaved with this content.
 *
 * Note that this is NOT the HTML output. It is the <page> tag of
 * the snap XML file format. The theme layout XSLT will be used
 * to generate the final output.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  A fallback path in case ipath is not satisfactory.
 */
void list::on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    static_cast<void>(ipath);
    static_cast<void>(page);
    static_cast<void>(body);
    static_cast<void>(ctemplate);
}


/** \brief Generate the page common content.
 *
 * This function generates some content that is expected in a page
 * by default.
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 * \param[in] ctemplate  The body being generated.
 */
void list::on_generate_page_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate)
{
    static_cast<void>(ipath);
    static_cast<void>(page);
    static_cast<void>(body);
    static_cast<void>(ctemplate);
}


/** \brief Unserialize a set of list atoms.
 *
 * This function unserializes a set of list atoms that was serialized using
 * the serialize() function. This is considered an internal function as it
 * is called by the unserialize() function of the list object.
 *
 * \param[in] data  The data to unserialize.
 *
 * \sa serialize()
 */
void list::unserialize(QString const& data)
{
    // QBuffer takes a non-const QByteArray so we have to create a copy
    QByteArray non_const_data(data.toUtf8());
    QBuffer in(&non_const_data);
    in.open(QIODevice::ReadOnly);
    QtSerialization::QReader reader(in);
    QtSerialization::QComposite comp;
    QtSerialization::QFieldTag list_tag(comp, "list", this);
    reader.read(comp);
}


/** \brief Read the contents of one tag from the reader.
 *
 * This function reads the contents of one message tag. It calls
 * the attachment unserialize() as required whenever an attachment
 * is found in the stream.
 *
 * \param[in] name  The name of the tag being read.
 * \param[in] r  The reader used to read the input data.
 */
void list::readTag(QString const& name, QtSerialization::QReader& r)
{
    if(name == "atoms")
    {
        list_atom atom;
        atom.unserialize(r);
        f_list_atoms.push_back(atom);
    }
}


/** \brief Serialize a list of list atoms to a writer.
 *
 * This function serializes the current list of atoms so it can be
 * saved in the database in the form of a string.
 *
 * \return A string representing the list of atoms.
 *
 * \sa unserialize()
 */
QString list::serialize() const
{
    QByteArray result;
    QBuffer archive(&result);
    archive.open(QIODevice::WriteOnly);
    {
        QtSerialization::QWriter w(archive, "list", LIST_ATOMS_MAJOR_VERSION, LIST_ATOMS_MINOR_VERSION);
        QtSerialization::QWriter::QTag tag(w, "atoms");
        int const max_atoms(f_list_atoms.count());
        for(int i(0); i < max_atoms; ++i)
        {
            f_list_atoms[i].serialize(w);
        }
        // end the writer so everything gets saved in the buffer (result)
    }

    return QString::fromUtf8(result.data());
}




SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
