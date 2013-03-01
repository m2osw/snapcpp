// Snap Websites Server -- manage sendmail (record, display)
// Copyright (C) 2013  Made to Order Software Corp.
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

#include "sendmail.h"
#include "plugins.h"
#include "log.h"
#include "not_reached.h"
#include "../content/content.h"
#include <QtCassandra/QCassandraValue.h>
#include <iostream>
#include <magic.h>


SNAP_PLUGIN_START(sendmail, 1, 0)


/** \brief Get a fixed sendmail plugin name.
 *
 * The sendmail plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
const char *get_name(name_t name)
{
	switch(name) {
	//case SNAP_NAME_SENDMAIL_TABLE:
	//	return "sendmail";

	default:
		// invalid index
		throw snap_exception();

	}
	NOTREACHED();
}


/** \brief Initialize an email attachment object.
 *
 * You can create an email attachment object, initializes it, and then
 * add it to an email object. The number of attachments is not limited
 * although you should remember that most mail servers limit the total
 * size of an email. It may be 5, 10 or 20Mb, but if you go over the
 * email will fail.
 */
sendmail::email::email_attachment::email_attachment()
	//: f_header()
	//, f_data()
{
}


/** \brief The content of the binary file to attach to this email.
 *
 * This function is used to attach one binary file to the email.
 *
 * If you know the MIME type of the data, it is smart to define it when
 * calling this function so that way you avoid asking the magic library
 * for it. This will save time as the magic library is much slower.
 *
 * \exception sendmail_exception_no_magic
 * If the data MIME type cannot be determined because the magic
 * library cannot be initialized, the function raises this exception.
 *
 * \param[in] data  The data to attach to this email.
 *
 * \sa add_header();
 */
void sendmail::email::email_attachment::set_data(const QByteArray& data, QString mime_type)
{
	f_data = data;

	// RAII class to make sure magic gets closed on error
	class magic
	{
	public:
		magic()
			: f_magic(magic_open(MAGIC_COMPRESS | MAGIC_MIME))
		{
			if(f_magic == NULL)
			{
				throw sendmail_exception_no_magic();
			}
		}

		~magic()
		{
			magic_close(f_magic);
		}

		QString get_mime_type(const QByteArray& data)
		{
			return magic_buffer(f_magic, data.data(), data.length());
		}

	private:
		magic_t f_magic;
	};

	// if user did not define the MIME type then ask the magic library
	if(mime_type.isEmpty())
	{
		magic m;
		mime_type = m.get_mime_type(f_data);
	}
	f_header["Content-Type"] = mime_type;
}


/** \brief Header of this attachment.
 *
 * Each attachment can be assigned a set of headers such as the Content-Type
 * (which is automatically set by the set_data() function.)
 *
 * Headers in an attachment are similar to the headers in the main email
 * only it cannot include certain entries such as the To:, Cc:, etc.
 *
 * In most cases you want to include the filename if the attachment represents
 * a file. Plain text and HTML will generally only need the Content-Type which
 * is already set by a call to the set_data() funciton.
 *
 * \node
 * The Content-Transfer-Encoding is managed internally and you are not
 * expected to set this value. The Content-Disposition is generally set
 * to "attachment" for files that are attached to the email.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a header cannot be empty. This exception is raised if the name
 * is empty.
 *
 * \todo
 * As we develop a functioning version of sendmail we want to add tests to
 * prevent a set of fields that we will handle internally and thus we do
 * not want users to be able to set here.
 *
 * \param[in] name  A valid header name.
 * \param[in] value  The value of this header.
 *
 * \sa set_data()
 */
void sendmail::email::email_attachment::add_header(const QString& name, const QString& value)
{
	if(name.isEmpty())
	{
		throw sendmail_exception_invalid_argument();
	}

	f_header[name] = value;
}


/** \brief Initialize an email object.
 *
 * This function initializes an email object making it ready to be
 * setup before processing.
 *
 * The function takes no parameter, although a certain number of
 * parameters are required and must be defined before the email
 * can be sent:
 *
 * \li From -- the name/email of the user sending this email.
 * \li To -- the name/email of the user to whom this email is being sent,
 *           there may be multiple recipients and they may be defined
 *           in Cc or Bcc as well as the To list. The To can also be
 *           defined as a list alias name in which case the backend
 *           will send the email to all the subscribers of that list.
 * \li Subject -- the subject must include something.
 * \li Content -- at least one attachment must be added as the body.
 *
 * Attachments support text emails, HTML pages, and any file (image,
 * PDF, etc.). There is no specific limit to the number of attachments
 * or the size per se, although more email systems do limit the size
 * of an email so we do enforce some limit (i.e. 25Mb).
 */
sendmail::email::email()
	//: f_from("") -- auto-init, but required
	//, f_cumulative("") -- auto-init
	//, f_list() -- auto-init, but at least one required
	//, f_subject("") -- auto-init, but required
	//, f_header() -- auto-init
	//, f_attachments() -- auto-init, but at least one required
{
}


/** \brief Clean up the email object.
 *
 * This function ensures that an email object is cleaned up before
 * getting freed.
 */
sendmail::email::~email()
{
}


/** \brief Save the name and email address of the sender.
 *
 * This function saves the name and address of the sender. It has to
 * be valid according to RFC 2822.
 *
 * \param[in] from  The name and email address of the sender
 */
void sendmail::email::set_from(const QString& from)
{
	f_from = from;
}


/** \brief Mark this email as being cumulative.
 *
 * A cumulative email is not sent immediately. Instead it is stored
 * and sent at a later time once certain thresholds are reached.
 * There are two thresholds used at this time: a time threshold, a
 * user may want to receive at most one email every few days; and
 * a count threshold, a user may want to receive an email for every
 * X events.
 *
 * Also, our system is capable to cumulate using an overwrite so
 * the receiver gets one email even if the same object was modified
 * multiple times. For example an administrator may want to know
 * when a type of pages gets modified, but he doesn't want to know
 * of each little change (i.e. the editor may change the page 5
 * times in a row as he/she finds things to tweak over and over
 * again.) The name of the \p object passed as a parameter allows
 * the mail system to cumulate using an overwrite and thus mark
 * that this information should really only be sent once (i.e.
 * instead of saying 10 times that page X changed, the mail system
 * can say it once [although we will include how many edits were
 * made as an additional piece of information.])
 *
 * Note that the user may mark all emails that he/she receives as
 * cumulative or non-cumulative so this flag is useful but it can
 * be ignored by the receivers. The priority can be used by the
 * receiver to decide what to do with an email. (i.e. send urgent
 * emails immediately.)
 *
 * \note
 * You may call the set_cumulative() function with an empty string
 * to turn off the cumulative feature for that email.
 *
 * \param[in] object  The name of the object being worked on.
 */
void sendmail::email::set_cumulative(const QString& object)
{
	f_cumulative = object;
}


/** \brief The priority is a somewhat arbitrary value defining the email urgency.
 *
 * Many mail system define a priority but it really isn't defined in the
 * RFC 2822 so the value is not well defined.
 *
 * The priority is saved in the X-Priority header.
 *
 * \param[in] priority  The priority of this email.
 */
void sendmail::email::set_priority(email_priority_t priority)
{
	QString name;
	switch(priority)
	{
	case EMAIL_PRIORITY_BULK:
		name = "Bulk";
		break;

	case EMAIL_PRIORITY_LOW:
		name = "Low";
		break;

	case EMAIL_PRIORITY_NORMAL:
		name = "Normal";
		break;

	case EMAIL_PRIORITY_HIGH:
		name = "High";
		break;

	case EMAIL_PRIORITY_URGENT:
		name = "Urgent";
		break;

	default:
		throw sendmail_exception_invalid_argument();

	}

	f_header["X-Priority"] = QString("%1 (%2)").arg(static_cast<int>(priority)).arg(name);
	f_header["X-MSMail-Priority"] = name;
	f_header["Importance"] = name;
}


/** \brief Clear the specified list.
 *
 * The sendmail email object supports a few lists of email addressed
 * used to define all the email addresses where the data has to be
 * sent. These lists can be cleared by this function to make sure that
 * it doesn't get filled with unwanted data.
 *
 * \param[in] list  The list to clear.
 */
void sendmail::email::clear_list(email_list_t list)
{
	if(list < EMAIL_LIST_TO || list > EMAIL_LIST_BCC)
	{
		throw sendmail_exception_invalid_argument();
	}

	f_list[list].clear();
}


/** \brief Add one email address to a list.
 *
 * This function adds the specified \p to email address to this email
 * object.
 *
 * If you have multiple email addresses, you may want to use a QStringList
 * instead of just a QString.
 *
 * \param[in] list  The list where the email is added.
 * \param[in] address  The email to be added.
 */
void sendmail::email::add_to_list(email_list_t list, const QString& address)
{
	if(list < EMAIL_LIST_TO || list > EMAIL_LIST_BCC)
	{
		throw sendmail_exception_invalid_argument();
	}

	f_list[list].push_back(address);
}


/** \brief Add a set of address to one of the lists of this email object.
 *
 * This function adds all the \p addresses to the specified list item.
 *
 * \param[in] list  The list where the addresses will be added.
 * \param[in] addresses  The list of addresses to be added.
 */
void sendmail::email::add_to_list(email_list_t list, const QStringList& addresses)
{
	if(list < EMAIL_LIST_TO || list > EMAIL_LIST_BCC)
	{
		throw sendmail_exception_invalid_argument();
	}

	int max(addresses.count());
	for(int i(0); i < max; ++i)
	{
		f_list[list].push_back(addresses.at(i));
	}
}


/** \brief Set the email subject.
 *
 * This function sets the subject of the email. Anything is permitted although
 * you should not send emails with an empty subject.
 *
 * The system takes care of encoding the subject if required. It will also trim
 * it and remove any unwanted characters (tabs, new lines, etc.)
 *
 * The subject line is also silently truncated to a reasonable size.
 *
 * \param[in] subject  The subject of the email.
 */
void sendmail::email::set_subject(const QString& subject)
{
	f_subject = subject;
}


/** \brief Add a header to the email.
 *
 * The system takes care of most of the email headers but this function gives
 * you the possibility to add more.
 *
 * Note that the priority should instead be set with the set_priority() function.
 *
 * The content type should not be set. The system automatically takes care of
 * that for you including required encoding information, attachments, etc.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a header cannot be empty. This exception is raised if the name
 * is empty.
 *
 * \param[in] name  A valid header name.
 * \param[in] value  The value of this header.
 */
void sendmail::email::add_header(const QString& name, const QString& value)
{
	if(name.isEmpty())
	{
		throw sendmail_exception_invalid_argument();
	}

	f_header[name] = value;
}


/** \brief Add an attachment to this email.
 *
 * All data appearing in the body of the email is defined using attachment.
 * This includes the normal plain text body if you use one. See the
 * email_attachment class for details on how to create an attachment
 * for an email.
 *
 * Note that if you want to add a plain text and an HTML version to your
 * email, these are sub-attachments to one attachment of the email defined
 * as alternatives. If only that one attachment is added to an email then
 * it won't be made a sub-attachment in the final email buffer.
 *
 * \note
 * It is important to note that the attachments are written in the email
 * in the order they are defined here. It is quite customary to add the
 * plain text first, then the HTML version, then the different files to
 * attach to the email.
 *
 * \param[in] data  The email attachment.
 */
void sendmail::email::add_attachment(const email_attachment& data)
{
	f_attachment.push_back(data);
}


SNAP_PLUGIN_END()

// vim: ts=4 sw=4
