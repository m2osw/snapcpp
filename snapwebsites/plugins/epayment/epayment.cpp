// Snap Websites Server -- handle an array of electronic payment facilities...
// Copyright (C) 2011-2015  Made to Order Software Corp.
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

#include "epayment.h"

#include "../editor/editor.h"
//#include "../output/output.h"

#include "not_reached.h"

#include <iostream>

#include "poison.h"


SNAP_PLUGIN_START(epayment, 1, 0)


/* \brief Get a fixed epayment name.
 *
 * The epayment plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    switch(name)
    {
    case SNAP_NAME_EPAYMENT_INVOICE_STATUS:
        return "epayment::invoice_status";

    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED:
        return "canceled";

    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED:
        return "completed";

    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED:
        return "created";

    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED:
        return "failed";

    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID:
        return "paid";

    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING:
        return "pending";

    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING:
        return "processing";

    case SNAP_NAME_EPAYMENT_PRICE:
        return "epayment::price";

    case SNAP_NAME_EPAYMENT_PRODUCT_DESCRIPTION:
        return "epayment::product_name";

    case SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH:
        return "types/taxonomy/system/content-types/epayment/product";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_EPAYMENT_...");

    }
    NOTREACHED();
}









/** \brief Initialize the epayment plugin.
 *
 * This function is used to initialize the epayment plugin object.
 */
epayment::epayment()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the epayment plugin.
 *
 * Ensure the epayment object is clean before it is gone.
 */
epayment::~epayment()
{
}


/** \brief Initialize the epayment.
 *
 * This function terminates the initialization of the epayment plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void epayment::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(epayment, "layout", layout::layout, generate_header_content, _1, _2, _3, _4);
}


/** \brief Get a pointer to the epayment plugin.
 *
 * This function returns an instance pointer to the epayment plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the epayment plugin.
 */
epayment *epayment::instance()
{
    return g_plugin_epayment_factory.instance();
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
QString epayment::description() const
{
    return "The e-Payment plugin offers one common way to process an"
          " electronic or not so electronic payment online (i.e. you"
          " may accept checks, for example...)";
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
int64_t epayment::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2014, 12, 29, 15, 38, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added
 *                        to the database by this update (in micro-seconds).
 */
void epayment::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    content::content::instance()->add_xml(get_plugin_name());
}


/** \brief Setup page for the editor.
 *
 * The editor has a set of dynamic parameters that the users are offered
 * to setup. These parameters need to be sent to the user and we use this
 * function for that purpose.
 *
 * \todo
 * Look for a way to generate the editor data only if necessary (too
 * complex for now.)
 *
 * \param[in,out] ipath  The path being managed.
 * \param[in,out] header  The header being generated.
 * \param[in,out] metadata  The metadata being generated.
 * \param[in] ctemplate  The template in case path does not exist.
 */
void epayment::on_generate_header_content(content::path_info_t& ipath, QDomElement& header, QDomElement& metadata, QString const& ctemplate)
{
    static_cast<void>(ipath);
    static_cast<void>(metadata);
    static_cast<void>(ctemplate);

    QDomDocument doc(header.ownerDocument());

    // make sure this is a product, if so, add product fields
    links::link_info product_info(content::get_name(content::SNAP_NAME_CONTENT_PAGE_TYPE), true, ipath.get_key(), ipath.get_branch());
    QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(product_info));
    links::link_info product_child_info;
    if(link_ctxt->next_link(product_child_info))
    {
        // the link_info returns a full key with domain name
        // use a path_info_t to retrieve the cpath instead
        content::path_info_t type_ipath;
        type_ipath.set_path(product_child_info.key());
        if(type_ipath.get_cpath().startsWith(get_name(SNAP_NAME_EPAYMENT_PRODUCT_TYPE_PATH)))
        {
            // if the content is the main page then define the titles and body here
            FIELD_SEARCH
                (content::field_search::COMMAND_MODE, content::field_search::SEARCH_MODE_EACH)
                (content::field_search::COMMAND_ELEMENT, metadata)
                (content::field_search::COMMAND_PATH_INFO_REVISION, ipath)

                // /snap/head/metadata/epayment
                (content::field_search::COMMAND_CHILD_ELEMENT, "epayment")

                // /snap/head/metadata/epayment/product-name
                (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_EPAYMENT_PRODUCT_DESCRIPTION))
                (content::field_search::COMMAND_SELF)
                (content::field_search::COMMAND_IF_FOUND, 1)
                    // use page title as a fallback
                    (content::field_search::COMMAND_FIELD_NAME, content::get_name(content::SNAP_NAME_CONTENT_TITLE))
                    (content::field_search::COMMAND_SELF)
                (content::field_search::COMMAND_LABEL, 1)
                (content::field_search::COMMAND_SAVE, "product-description")

                // /snap/head/metadata/epayment/product-price
                (content::field_search::COMMAND_FIELD_NAME, get_name(SNAP_NAME_EPAYMENT_PRICE))
                (content::field_search::COMMAND_SELF)
                (content::field_search::COMMAND_SAVE, "product-price")

                // generate!
                ;
        }
    }

    // TODO: find a way to include e-Payment data only if required
    //       (it may already be done! search on add_javascript() for info.)
    content::content::instance()->add_javascript(doc, "epayment");
    content::content::instance()->add_css(doc, "epayment");
}


name_t epayment::get_invoice_status(content::path_info_t& invoice_ipath)
{
    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraRow::pointer_t row(content_table->row(invoice_ipath.get_key()));
    QString const status(row->cell(get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS))->value().stringValue());

    // convert string to ID, makes it easier to test the status
    if(status == get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED))
    {
        return SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED;
    }
    if(status == get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED))
    {
        return SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED;
    }
    if(status == get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED))
    {
        return SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED;
    }
    if(status == get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED))
    {
        return SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED;
    }
    if(status == get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID))
    {
        return SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID;
    }
    if(status == get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING))
    {
        return SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING;
    }
    if(status == get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING))
    {
        return SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING;
    }

    throw snap_logic_exception(QString("invoice \"%1\" has unknown status \"%2\".").arg(invoice_ipath.get_key()).arg(status));
}


/** \brief Signal used to change the invoice status.
 *
 * Other plugins that want to react whenever an invoice changes its status
 * can make use of this signal. For example, once an invoice is marked PAID
 * and the cart included items that need to be shipped, the corresponding
 * plugin can make the invoice visible to the administrator who is
 * responsible for the handling.
 *
 * Another example is about users who purchase software. Once the invoice
 * is marked as PAID, the software becomes downloadable by the user.
 *
 * \todo
 * We need to see whether we want to enforce the status change in the
 * sense that the status cannot go from PAID back to CANCELED or PENDING.
 *
 * \exception snap_logic_exception
 * This exception is raised when the function is called with an invalid
 * status.
 *
 * \param[in,out] invoice_ipath  The path to the invoice changing its status.
 * \param[in] status  The new status as an epayment name_t.
 *
 * \return true if the status changed, false if the status does not change
 *         or an error is detected and we can continue.
 */
bool epayment::set_invoice_status_impl(content::path_info_t& invoice_ipath, name_t const status)
{
    // make sure the status is properly defined
    switch(status)
    {
    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_CANCELED:
    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_COMPLETED:
    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_CREATED:
    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_FAILED:
    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_PAID:
    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_PENDING:
    case SNAP_NAME_EPAYMENT_INVOICE_STATUS_PROCESSING:
        break;

    default:
        // status is contolled as the few types defined in this switch;
        // anything else is not allowed
        throw snap_logic_exception("invalid SNAP_NAME_EPAYMENT_INVOICE_STATUS_...");

    }

    content::content *content_plugin(content::content::instance());
    QtCassandra::QCassandraTable::pointer_t content_table(content_plugin->get_content_table());
    QtCassandra::QCassandraRow::pointer_t row(content_table->row(invoice_ipath.get_key()));
    QString const current_status(row->cell(get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS))->value().stringValue());
    QString const new_status(get_name(status));
    if(current_status == new_status)
    {
        // status not changing, avoid any additional work
        return false;
    }
    row->cell(get_name(SNAP_NAME_EPAYMENT_INVOICE_STATUS))->setValue(new_status);

    return true;
}



// List of bitcoin libraries and software
//   https://en.bitcoin.it/wiki/Software

SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et
