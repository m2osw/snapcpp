// Snap Websites Server -- handle the access to attachments
// Copyright (C) 2014  Made to Order Software Corp.
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

#include "attachment.h"

#include "../messages/messages.h"

#include "not_reached.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(attachment, 1, 0)

// using the SNAP_NAME_CONTENT_... for this one.
/* \brief Get a fixed attachment name.
 *
 * The attachment plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
//char const *get_name(name_t name)
//{
//    switch(name)
//    {
//    case SNAP_NAME_ATTACHMENT_...:
//        return "attachment::...";
//
//    default:
//        // invalid index
//        throw snap_logic_exception("invalid SNAP_NAME_ATTACHMENT_...");
//
//    }
//    NOTREACHED();
//}









/** \brief Initialize the attachment plugin.
 *
 * This function is used to initialize the attachment plugin object.
 */
attachment::attachment()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the attachment plugin.
 *
 * Ensure the attachment object is clean before it is gone.
 */
attachment::~attachment()
{
}


/** \brief Initialize the attachment.
 *
 * This function terminates the initialization of the attachment plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void attachment::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(attachment, "path", path::path, can_handle_dynamic_path, _1, _2);
    SNAP_LISTEN(attachment, "content", content::content, page_cloned, _1);
    SNAP_LISTEN(attachment, "content", content::content, copy_branch_cells, _1, _2, _3);
}


/** \brief Get a pointer to the attachment plugin.
 *
 * This function returns an instance pointer to the attachment plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the attachment plugin.
 */
attachment *attachment::instance()
{
    return g_plugin_attachment_factory.instance();
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
QString attachment::description() const
{
    return "Handle the output of attachments, which includes sending the"
        " proper compressed file and in some cases transforming the file"
        " on the fly before sending it to the user (i.e. resizing an image"
        " to \"better\" sizes for the page being presented.)";
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
int64_t attachment::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 10, 2, 23, 58, 12, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void attachment::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Allow a second opinion on who can handle this path.
 *
 * This function is used here to allow the attachment plugin to handle
 * attachments that have a different filename (i.e. have some extensions
 * that could be removed for us to find the wanted file.)
 *
 * Although we could use an "easier" mechanism such as query string
 * entries to tweak the files, it is much less natural than supporting
 * "random" filenames for extensions.
 *
 * The attachment plugin support is limited to ".gz". However, other
 * core plugins support other magical extensions (i.e. image and
 * javascript.)
 *
 * \param[in] ipath  The path being checked.
 * \param[in] plugin_info  The current information about this path plugin.
 */
void attachment::on_can_handle_dynamic_path(content::path_info_t& ipath, path::dynamic_plugin_t& plugin_info)
{
    // is that path already going to be handled by someone else?
    // (avoid wasting time if that's the case)
    if(plugin_info.get_plugin()
    || plugin_info.get_plugin_if_renamed())
    {
        return;
    }

    // TODO: will other plugins check for their own extension schemes?
    //       (I would imagine that this plugin will support more than
    //       just the .gz extension...)
    QString cpath(ipath.get_cpath());
    if(!cpath.endsWith(".gz") || cpath.endsWith("/.gz"))
    {
        return;
    }

    cpath = cpath.left(cpath.length() - 3);
    content::path_info_t attachment_ipath;
    attachment_ipath.set_path(cpath);
    QtCassandra::QCassandraTable::pointer_t revision_table(content::content::instance()->get_revision_table());
    if(!revision_table->exists(attachment_ipath.get_revision_key())
    || !revision_table->row(attachment_ipath.get_revision_key())->exists(content::get_name(content::SNAP_NAME_CONTENT_ATTACHMENT)))
    {
        return;
    }

    QtCassandra::QCassandraValue attachment_key(revision_table->row(attachment_ipath.get_revision_key())->cell(content::get_name(content::SNAP_NAME_CONTENT_ATTACHMENT))->value());
    if(attachment_key.nullValue())
    {
        return;
    }

    QtCassandra::QCassandraTable::pointer_t files_table(content::content::instance()->get_files_table());
    if(!files_table->exists(attachment_key.binaryValue())
    || !files_table->row(attachment_key.binaryValue())->exists(content::get_name(content::SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED)))
    {
        // TODO: also offer a dynamic version which compress the
        //       file on the fly (but we wouldd have to save it and
        //       that could cause problems with the backend if we
        //       were to not use the maximum compression?)
        return;
    }

    // tell the path plugin that we know how to handle this one
    plugin_info.set_plugin_if_renamed(this, attachment_ipath.get_cpath());
    ipath.set_parameter("attachment_field", content::get_name(content::SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED));
}


/** \brief Execute a page: generate the complete attachment of that page.
 *
 * This function displays the page that the user is trying to view. It is
 * supposed that the page permissions were already checked and thus that
 * its contents can be displayed to the current user.
 *
 * Note that the path was canonicalized by the path plugin and thus it does
 * not require any further corrections.
 *
 * \param[in,out] ipath  The canonicalized path being managed.
 *
 * \return true if the content is properly generated, false otherwise.
 */
bool attachment::on_path_execute(content::path_info_t& ipath)
{
    // TODO: we probably do not want to check for attachments to send if the
    //       action is not "view"...

    // attachments should never be saved with a compression extension
    //
    // HOWEVER, we'd like to offer a way for the system to allow extensions
    // but if we are here the system already found the page and thus found
    // it with[out] the extension as defined in the database...
    //
    QString field_name;
    content::path_info_t attachment_ipath;
    QString const renamed(ipath.get_parameter("renamed_path"));
    if(renamed.isEmpty())
    {
        attachment_ipath = ipath;
        field_name = content::get_name(content::SNAP_NAME_CONTENT_FILES_DATA);
    }
    else
    {
        // TODO: that data may NOT be available yet in which case a plugin
        //       needs to offer it... how do we do that?!
        attachment_ipath.set_path(renamed);
        field_name = ipath.get_parameter("attachment_field");
    }

    QtCassandra::QCassandraTable::pointer_t revision_table(content::content::instance()->get_revision_table());
    QtCassandra::QCassandraValue attachment_key(revision_table->row(attachment_ipath.get_revision_key())->cell(content::get_name(content::SNAP_NAME_CONTENT_ATTACHMENT))->value());
    if(attachment_key.nullValue())
    {
        // somehow the file key is not available
        f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "Attachment Not Found",
                QString("The attachment \"%1\" was not found.").arg(ipath.get_key()),
                QString("Could not find field \"%1\" of file \"%2\" (maybe renamed \"%3\").")
                        .arg(field_name)
                        .arg(QString::fromAscii(attachment_key.binaryValue().toHex()))
                        .arg(renamed));
        NOTREACHED();
    }

    QtCassandra::QCassandraTable::pointer_t files_table(content::content::instance()->get_files_table());
    if(!files_table->exists(attachment_key.binaryValue())
    || !files_table->row(attachment_key.binaryValue())->exists(field_name))
    {
        // somehow the file data is not available
        f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "Attachment Not Found",
                QString("The attachment \"%1\" was not found.").arg(ipath.get_key()),
                QString("Could not find field \"%1\" of file \"%2\".")
                        .arg(content::get_name(content::SNAP_NAME_CONTENT_FILES_DATA))
                        .arg(QString::fromAscii(attachment_key.binaryValue().toHex())));
        NOTREACHED();
    }

    QtCassandra::QCassandraRow::pointer_t file_row(files_table->row(attachment_key.binaryValue()));

    // TODO: If the user is loading the file as an attachment,
    //       we need those headers

    //int pos(cpath.lastIndexOf('/'));
    //QString basename(cpath.mid(pos + 1));
    //f_snap->set_header("Content-Disposition", "attachment; filename=" + basename);

    //f_snap->set_header("Content-Transfer-Encoding", "binary");

    // get the attachment MIME type and tweak it if it is a known text format
    QtCassandra::QCassandraValue attachment_mime_type(file_row->cell(content::get_name(content::SNAP_NAME_CONTENT_FILES_MIME_TYPE))->value());
    QString content_type(attachment_mime_type.stringValue());
    if(content_type == "text/javascript"
    || content_type == "text/css"
    || content_type == "text/xml")
    {
        // TBD -- we probably should check what's defined inside those
        //        files before assuming it's using UTF-8.
        content_type += "; charset=utf-8";
    }
    f_snap->set_header("Content-Type", content_type);

    // the actual file data now
    QtCassandra::QCassandraValue data(file_row->cell(field_name)->value());
    f_snap->output(data.binaryValue());

    return true;
}


/** \brief Someone just cloned a page.
 *
 * Check whether the clone represents a file. If so, we want to add a
 * reference from that file to this new page.
 *
 * This must happen in pretty much all cases.
 *
 * \param[in] tree  The tree of pages that got cloned.
 */
void attachment::on_page_cloned(content::content::cloned_tree_t const& tree)
{
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t branch_table(content_plugin->get_branch_table());
    QtCassandra::QCassandraTable::pointer_t files_table(content_plugin->get_files_table());

    char const *attachment_reference(content::get_name(content::SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE));
    QString const content_attachment_reference(QString("%1::").arg(attachment_reference));
    size_t const max_pages(tree.f_pages.size());
    for(size_t idx(0); idx < max_pages; ++idx)
    {
        content::content::cloned_page_t const& page(tree.f_pages[idx]);
        size_t const max_branches(page.f_branches.size());
        for(size_t branch(0); branch < max_branches; ++branch)
        {
            snap_version::version_number_t const b(page.f_branches[branch].f_branch);
            content::path_info_t page_ipath(page.f_destination);
            page_ipath.force_branch(b);

            QtCassandra::QCassandraRow::pointer_t branch_row(branch_table->row(page_ipath.get_branch_key()));
            QtCassandra::QCassandraColumnRangePredicate column_predicate;
            column_predicate.setStartColumnName(content_attachment_reference);
            column_predicate.setEndColumnName(QString("%1;").arg(attachment_reference));
            column_predicate.setCount(100);
            column_predicate.setIndex(); // behave like an index
            for(;;)
            {
                branch_row->clearCache();
                branch_row->readCells(column_predicate);
                QtCassandra::QCassandraCells const branch_cells(branch_row->cells());
                if(branch_cells.isEmpty())
                {
                    // done
                    break;
                }
                // handle one batch
                for(QtCassandra::QCassandraCells::const_iterator nc(branch_cells.begin());
                                                                 nc != branch_cells.end();
                                                                 ++nc)
                {
                    QtCassandra::QCassandraCell::pointer_t branch_cell(*nc);
                    QByteArray cell_key(branch_cell->columnKey());

                    // this key starts with SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE + "::"
                    // and then represents an md5
                    QByteArray md5(cell_key.mid( content_attachment_reference.length() ));

                    // with that md5 we can access the files table
                    signed char const one(1);
                    files_table->row(md5)->cell(QString("%1::%2").arg(content::get_name(content::SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(page_ipath.get_key()))->setValue(one);
                }
            }
        }
    }
}


void attachment::on_copy_branch_cells(QtCassandra::QCassandraCells& source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    static_cast<void>(destination_branch);

    QtCassandra::QCassandraTable::pointer_t files_table(content::content::instance()->get_files_table());

    std::string content_attachment_reference(content::get_name(content::SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE));
    content_attachment_reference += "::";

    QtCassandra::QCassandraCells left_cells;

    // handle one batch
    for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
            nc != source_cells.end();
            ++nc)
    {
        QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
        QByteArray cell_key(source_cell->columnKey());

        if(cell_key.startsWith(content_attachment_reference.c_str()))
        {
            // copy our fields as is
            destination_row->cell(cell_key)->setValue(source_cell->value());

            // make sure the (new) list is checked so we actually get a list
            content::path_info_t ipath;
            ipath.set_path(destination_row->rowName());

            // this key starts with SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE + "::"
            // and then represents an md5
            QByteArray md5(cell_key.mid( content_attachment_reference.length() ));

            // with that md5 we can access the files table
            signed char const one(1);
            files_table->row(md5)->cell(QString("%1::%2").arg(content::get_name(content::SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(ipath.get_key()))->setValue(one);
        }
        else
        {
            // keep the other branch fields as is, other plugins can handle
            // them as required by implementing this signal
            //
            // note that the map is a map a shared pointers so it is fast
            // to make a copy like this
            left_cells[cell_key] = source_cell;
        }
    }

    // overwrite the source with the cells we allow to copy "further"
    source_cells = left_cells;
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
