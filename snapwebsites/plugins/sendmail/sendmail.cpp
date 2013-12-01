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
#include "mkgmtime.h"
#include "qdomxpath.h"
#include "not_reached.h"
#include "../content/content.h"
#include "../users/users.h"
#include "process.h"
#include <QtCassandra/QCassandraValue.h>
#include <QtSerialization/QSerializationComposite.h>
#include <QtSerialization/QSerializationFieldString.h>
#include <QtSerialization/QSerializationFieldTag.h>
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
    case SNAP_NAME_SENDMAIL:
        return "sendmail";

    case SNAP_NAME_SENDMAIL_CONTENT_TYPE:
        return "Content-Type";

    case SNAP_NAME_SENDMAIL_EMAIL:
        return "sendmail::email";

    case SNAP_NAME_SENDMAIL_EMAILS_TABLE:
        return "emails";

    case SNAP_NAME_SENDMAIL_FREQUENCY:
        return "sendmail::frequency";

    case SNAP_NAME_SENDMAIL_FREQUENCY_DAILY:
        return "daily";

    case SNAP_NAME_SENDMAIL_FREQUENCY_IMMEDIATE:
        return "immediate";

    case SNAP_NAME_SENDMAIL_FREQUENCY_MONTHLY:
        return "monthly";

    case SNAP_NAME_SENDMAIL_FREQUENCY_WEEKLY:
        return "weekly";

    case SNAP_NAME_SENDMAIL_FROM:
        return "From";

    case SNAP_NAME_SENDMAIL_IMPORTANT:
        return "Importance";

    case SNAP_NAME_SENDMAIL_INDEX:
        return "*index*";

    case SNAP_NAME_SENDMAIL_LISTS:
        return "lists";

    case SNAP_NAME_SENDMAIL_NEW:
        return "new";

    case SNAP_NAME_SENDMAIL_PING:
        return "PING";

    case SNAP_NAME_SENDMAIL_SENDING_STATUS:
        return "sendmail::sending_status";

    case SNAP_NAME_SENDMAIL_STATUS:
        return "sendmail::status";

    case SNAP_NAME_SENDMAIL_STATUS_DELETED:
        return "deleted";

    case SNAP_NAME_SENDMAIL_STATUS_LOADING:
        return "loading";

    case SNAP_NAME_SENDMAIL_STATUS_NEW:
        return "new";

    case SNAP_NAME_SENDMAIL_STATUS_READ:
        return "read";

    case SNAP_NAME_SENDMAIL_STATUS_SENDING:
        return "sending";

    case SNAP_NAME_SENDMAIL_STATUS_SENT:
        return "sent";

    case SNAP_NAME_SENDMAIL_STATUS_SPAM:
        return "spam";

    case SNAP_NAME_SENDMAIL_STOP:
        return "STOP";

    case SNAP_NAME_SENDMAIL_SUBJECT:
        return "Subject";

    case SNAP_NAME_SENDMAIL_TO:
        return "To";

    case SNAP_NAME_SENDMAIL_X_PRIORITY:
        return "X-Priority";

    case SNAP_NAME_SENDMAIL_X_MSMAIL_PRIORITY:
        return "X-MSMail-Priority";

    //default:
    //    // invalid index
    //    throw snap_exception("Invalid SNAP_NAME_SENDMAIL_...");

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


/** \brief Clean up an email attachment.
 *
 * This function is here primarily to have a clean virtual table.
 */
sendmail::email::email_attachment::~email_attachment()
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
                throw sendmail_exception_no_magic("Magic MIME type cannot be opened (magic_open() failed)");
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
    f_header[get_name(SNAP_NAME_SENDMAIL_CONTENT_TYPE)] = mime_type;
}


/** \brief The email attachment data.
 *
 * This function retrieves the attachment data from this email attachment
 * object. This is generally UTF-8 characters when we are dealing with
 * text (HTML or plain text.)
 *
 * The data type is defined in the Content-Type header which is automatically
 * defined by the mime_type parameter of the set_data() function call. To
 * retrieve the MIME type, use the following:
 *
 * \code
 * QString mime_type(attachment->get_header(get_name(SNAP_NAME_SENDMAIL_CONTENT_TYPE)));
 * \endcode
 */
QByteArray sendmail::email::email_attachment::get_data() const
{
    return f_data;
}


/** \brief Retrieve the value of a header.
 *
 * This function returns the value of the named header. If the header
 * is not currently defined, this function returns an empty string.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a header cannot be empty. This exception is raised if
 * \p name is empty.
 *
 * \param[in] name  A valid header name.
 *
 * \return The current value of that header or an empty string if undefined.
 */
QString sendmail::email::email_attachment::get_header(const QString& name) const
{
    if(name.isEmpty())
    {
        throw sendmail_exception_invalid_argument("Cannot retrieve a header with an empty name");
    }
    if(f_header.contains(name))
    {
        return f_header[name];
    }
    return "";
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
        throw sendmail_exception_invalid_argument("When adding a header the name cannot be empty");
    }

    f_header[name] = value;
}


/** \brief Get all the headers defined in this email attachment.
 *
 * This function returns the map of the headers defined in this email
 * attachment. This can be used to quickly scan all the headers.
 *
 * \note
 * It is important to remember that since this function returns a reference
 * to the map of headers, it may break if you call add_header() while going
 * through the references.
 *
 * \return A direct reference to the internal header map.
 */
const sendmail::email::header_map_t& sendmail::email::email_attachment::get_all_headers() const
{
    return f_header;
}


/** \brief Unserialize an email attachment.
 *
 * This function unserializes an email attachment that was serialized using
 * the serialize() function. This is considered an internal function as it
 * is called by the unserialize() function of the email object.
 *
 * \param[in] r  The reader used to read the input data.
 *
 * \sa serialize()
 */
void sendmail::email::email_attachment::unserialize(QtSerialization::QReader& r)
{
    QtSerialization::QComposite comp;
    QtSerialization::QFieldTag tag_header(comp, "header", this);
    QString attachment_data;
    QtSerialization::QFieldString tag_data(comp, "data", attachment_data);
    r.read(comp);
    f_data = QByteArray::fromBase64(attachment_data.toUtf8().data());
}


/** \brief Read the contents one tag from the reader.
 *
 * This function reads the contents of the attachment tag. It handles
 * the attachment header fields.
 *
 * \param[in] name  The name of the tag being read.
 * \param[in] r  The reader used to read the input data.
 */
void sendmail::email::email_attachment::readTag(const QString& name, QtSerialization::QReader& r)
{
    if(name == "header")
    {
        QtSerialization::QComposite comp;
        QString header_name;
        QtSerialization::QFieldString tag_name(comp, "name", header_name);
        QString header_value;
        QtSerialization::QFieldString tag_value(comp, "value", header_value);
        r.read(comp);
        f_header[header_name] = header_value;
    }
}


/** \brief Serialize an attachment to a writer.
 *
 * This function serialize an attachment so it can be saved in the database
 * in the form of a string.
 *
 * \param[in,out] w  The writer where the data gets saved.
 */
void sendmail::email::email_attachment::serialize(QtSerialization::QWriter& w) const
{
    QtSerialization::QWriter::QTag tag(w, "attachment");
    for(header_map_t::const_iterator it(f_header.begin()); it != f_header.end(); ++it)
    {
        QtSerialization::QWriter::QTag header(w, "header");
        QtSerialization::writeTag(w, "name", it.key());
        QtSerialization::writeTag(w, "value", it.value());
    }
    // the data may be binary and thus it cannot be saved as is
    // so we encode it using base64
    QtSerialization::writeTag(w, "data", f_data.toBase64().data());
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
    //: f_cumulative("") -- auto-init
    //, f_site_key("") -- auto-init, set when posting email
    //, f_email_path("") -- auto-init
    //, f_email_key("") -- auto-init, generated when posting email
    : f_time(static_cast<int64_t>(time(NULL)))
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
 * If you are call this function multiple times, only the last \p from
 * information is kept.
 *
 * \note
 * The set_from() function is the same as calling the add_header() with
 * "From" as the field name and \p from as the value. To retrieve that
 * field, you have to use the get_header() function.
 *
 * \exception sendmail_exception_invalid_argument
 * If the \p from parameter is not a valid email address (as per RCF
 * 2822) or there isn't exactly one email address in that parameter,
 * then this exception is raised.
 *
 * \param[in] from  The name and email address of the sender
 */
void sendmail::email::set_from(const QString& from)
{
    tld_email_list emails;
    if(emails.parse(from.toStdString(), 0) != TLD_RESULT_SUCCESS)
    {
        throw sendmail_exception_invalid_argument("invalid From: email");
    }
    if(emails.count() != 1)
    {
        throw sendmail_exception_invalid_argument("multiple From: emails");
    }
    f_header[get_name(SNAP_NAME_SENDMAIL_FROM)] = from;
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


/** \brief Set the site key of the site sending this email.
 *
 * The site key is saved in the email whenever the post_email() function
 * is called. You do not have to define it, it will anyway be overwritten.
 *
 * The site key is used to check whether an email is being sent to a group
 * and that group is a mailing list. In that case we've got to have the
 * name of the mailing list defined as "\<site-key>: \<list-name>" thus we
 * need access to the site key that generates the email at the time we
 * manage the email (which is from the backend that has no clue what the
 * site key is when reached).
 *
 * \param[in] site_key  The name (key/URI) of the site being built.
 */
void sendmail::email::set_site_key(const QString& site_key)
{
    f_site_key = site_key;
}


/** \brief Retrieve the site key of the site that generated this email.
 *
 * This function retrieves the value set by the set_site_key() function.
 * It returns an empty string until the post_email() function is called
 * because before that it is not set.
 *
 * The main reason for having the site key is to search the list of
 * email lists when the email gets sent to the end user.
 *
 * \return The site key of the site that generated the email.
 */
const QString& sendmail::email::get_site_key() const
{
    return f_site_key;
}


/** \brief Define the path to the email in the system.
 *
 * This function sets up the path of the email subject, body, and optional
 * attachments.
 *
 * Other attachments can also be added to the email. However, when a path
 * is defined, the title and body of that page are used as the subject and
 * the body of the email.
 *
 * \note
 * At the time an email gets sent, the permissions of a page are not
 * checked.
 *
 * \param[in] email_path  The path to a page that will be used as the email subject, body, and attachments
 */
void sendmail::email::set_email_path(const QString& email_path)
{
    f_email_path = email_path;
}


/** \brief Retrieve the path to the page used to generate the email.
 *
 * This email path is set to a page that represents the subject (title) and
 * body of the email. It may also have attachments linked to it.
 *
 * If the path is empty, then the email is generated using the email object
 * and its attachment, the first attachment being the body of the email.
 *
 * \return The path to the page to be used to generate the email subject and
 *         title.
 */
const QString& sendmail::email::get_email_path() const
{
    return f_email_path;
}


/** \brief Set the email key.
 *
 * When a new email is posted, it is assigned a unique number used as a
 * key in different places.
 *
 * \param[in] email_key  The name (key/URI) of the site being built.
 */
void sendmail::email::set_email_key(const QString& email_key)
{
    f_email_key = email_key;
}


/** \brief Retrieve the email key.
 *
 * This function retrieves the value set by the set_email_key() function.
 *
 * The email key is set when you call the post_email() function. It is a
 * random number that we also save in the email object so we can keep using
 * it as we go.
 *
 * \return The email key.
 */
const QString& sendmail::email::get_email_key() const
{
    return f_email_key;
}


/** \brief Retrieve the time when the email was posted.
 *
 * This function retrieves the time when the email was first posted.
 *
 * \return The time when the email was posted.
 */
time_t sendmail::email::get_time() const
{
    return static_cast<time_t>(f_time);
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
        throw sendmail_exception_invalid_argument("Unknown priority");

    }

    f_header[get_name(SNAP_NAME_SENDMAIL_X_PRIORITY)] = QString("%1 (%2)").arg(static_cast<int>(priority)).arg(name);
    f_header[get_name(SNAP_NAME_SENDMAIL_X_MSMAIL_PRIORITY)] = name;
    f_header[get_name(SNAP_NAME_SENDMAIL_IMPORTANT)] = name;
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
 * Note that if the email is setup with a path to a page, the title of that
 * page is used as the default subject. If the set_subject() function is
 * called with a valid subject (not empty) then the page title is ignored.
 *
 * \note
 * The set_subject() function is the same as calling the add_header() with
 * "Subject" as the field name and \p subject as the value.
 *
 * \param[in] subject  The subject of the email.
 */
void sendmail::email::set_subject(const QString& subject)
{
    f_header[get_name(SNAP_NAME_SENDMAIL_SUBJECT)] = subject;
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
 * The To, Cc, and Bcc fields are defined in this way. If multiple destinations
 * are defined, you must first add them to \p value before calling this function.
 *
 * \warning
 * Also the function is called 'add', because you may add as many headers as you
 * need, the function does NOT cumulate data within one field. Instead it
 * overwrites the content of the field. This is one way to replace an unwanted
 * value or force the content of a field for a given email.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a header cannot be empty. This exception is raised if
 * \p name is empty. The field name is also validated by the TLD library
 * and must be composed of letters, digits, the dash character, and it
 * has to start with a letter. The case is not important, however.
 * Also, if the field represents an email or a list of emails, the
 * value is also checked for validity.
 *
 * \param[in] name  A valid header name.
 * \param[in] value  The value of this header.
 */
void sendmail::email::add_header(const QString& name, const QString& value)
{
    tld_email_field_type type(tld_email_list::email_field_type(name.toStdString()));
    if(type == TLD_EMAIL_FIELD_TYPE_INVALID)
    {
        // this includes the case where the field name is empty
        throw sendmail_exception_invalid_argument("Invalid header name");
    }
    if(type != TLD_EMAIL_FIELD_TYPE_UNKNOWN)
    {
        // The Bcc fields may be empty
        if(type != TLD_EMAIL_FIELD_TYPE_ADDRESS_LIST_OPT
        || !value.isEmpty())
        {
            // if not unknown then we should check the field value
            // as a list of emails
            tld_email_list emails;
            if(emails.parse(value.toStdString(), 0) != TLD_RESULT_SUCCESS)
            {
                // TODO: this can happen if a TLD becomes obsolete and
                //       a user did not update one's email address.
                throw sendmail_exception_invalid_argument("Invalid header field of emails");
            }
            if(type == TLD_EMAIL_FIELD_TYPE_MAILBOX
            && emails.count() != 1)
            {
                throw sendmail_exception_invalid_argument("Header field expects exactly one email");
            }
        }
    }

    f_header[name] = value;
}


/** \brief Retrieve the value of a header.
 *
 * This function returns the value of the named header. If the header
 * is not currently defined, this function returns an empty string.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a header cannot be empty. This exception is raised if
 * \p name is empty.
 *
 * \param[in] name  A valid header name.
 *
 * \return The current value of that header or an empty string if undefined.
 */
QString sendmail::email::get_header(const QString& name) const
{
    if(name.isEmpty())
    {
        throw sendmail_exception_invalid_argument("Cannot retrieve a header with an empty name");
    }
    if(f_header.contains(name))
    {
        return f_header[name];
    }

    return "";
}


/** \brief Get all the headers defined in this email.
 *
 * This function returns the map of the headers defined in this email. This
 * can be used to quickly scan all the headers.
 *
 * \note
 * It is important to remember that since this function returns a reference
 * to the map of headers, it may break if you call add_header() while going
 * through the references.
 *
 * \return A direct reference to the internal header map.
 */
const sendmail::email::header_map_t& sendmail::email::get_all_headers() const
{
    return f_header;
}


/** \brief Add the body attachment to this email.
 *
 * When creating an email with a path to a page (which is close to mandatory
 * if you want to have translation and let users of your system to be able
 * to edit the email in all languages.)
 *
 * This function should be private because it should only be used internally.
 * Unfortunately, the function is used from the outside. But you've been
 * warn. Really, this is using a push_front() instead of a push_back() it
 * is otherwise the same as the add_attachment() function and you may want
 * to read that function's documentation too.
 *
 * \param[in] data  The attachment to add as the body of this email.
 *
 * \sa email::add_attachment()
 */
void sendmail::email::set_body_attachment(const email_attachment& data)
{
    f_attachment.push_front(data);
}


/** \brief Add an attachment to this email.
 *
 * All data appearing in the body of the email is defined using attachments.
 * This includes the normal plain text body if you use one. See the
 * email_attachment class for details on how to create an attachment
 * for an email.
 *
 * Note that if you want to add a plain text and an HTML version to your
 * email, these are sub-attachments to one attachment of the email defined
 * as alternatives. If only that one attachment is added to an email then
 * it won't be made a sub-attachment in the final email buffer.
 *
 * \b IMPORTANT \b NOTE: the body and subject of emails are most often defined
 * using a path to a page. This means the first attachment is to be viewed
 * as an attachment, not the main body. Also, the attachments of the page
 * are also viewed as attachments of the email and will appear before the
 * attachments added here.
 *
 * \note
 * It is important to note that the attachments are written in the email
 * in the order they are defined here. It is quite customary to add the
 * plain text first, then the HTML version, then the different files to
 * attach to the email.
 *
 * \param[in] data  The email attachment.
 *
 * \sa email::set_body_attachment()
 */
void sendmail::email::add_attachment(const email_attachment& data)
{
    f_attachment.push_back(data);
}


/** \brief Retrieve the number of attachments defined in this email.
 *
 * This function defines the number of attachments that were added to this
 * email. This is useful to retrieve the attachments with the
 * get_attachment() function.
 *
 * \return The number of attachments defined in this email.
 *
 * \sa add_attachment()
 * \sa get_attachment()
 */
int sendmail::email::get_attachment_count() const
{
    return f_attachment.count();
}


/** \brief Retrieve the specified attachement.
 *
 * This function gives you a read/write reference to the specified
 * attachment. This is used by plugins that need to access email
 * data to filter it one way or the other (i.e. change all the tags
 * with their corresponding values.)
 *
 * The \p index parameter must be a number between 0 and
 * get_attachment_count() minus one. If no attachments were added
 * then this function cannot be called.
 *
 * \param[in] index  The index of the attachment to retrieve.
 *
 * \return A reference to the corresponding attachment.
 */
sendmail::email::email_attachment& sendmail::email::get_attachment(int index) const
{
    // make sure we use the const version of the [] operator so
    // Qt does not make a deep copy of the element
    return const_cast<email_attachment&>(f_attachment[index]);
}


/** \brief Add a parameter to the email.
 *
 * Whenever you create an email, you may be able to offer additional
 * parameters that are to be used as token replacement in the email.
 * For example, when creating a new user, we ask the user to verify his
 * email address. This is done by creating a session identifier and then
 * asking the user to go to the special page /verify/&lt;session&gt;. That
 * way we know that the user received the email (although it may not
 * exactly be the right person...)
 *
 * The name of the parameter should be something like "users::verify",
 * i.e. it should be namespace specific to not clash with sendmail or
 * other plugins parameters.
 *
 * \warning
 * Also the function is called 'add', because you may add as many parameters
 * as you are available, the function does NOT cumulate data within one field.
 * Instead it overwrites the content of the field. This is one way to replace
 * an unwanted value or force the content of a field for a given email.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a parameter cannot be empty. This exception is raised if
 * \p name is empty.
 *
 * \param[in] name  A valid parameter name.
 * \param[in] value  The value of this header.
 */
void sendmail::email::add_parameter(const QString& name, const QString& value)
{
    if(name.isEmpty())
    {
        throw sendmail_exception_invalid_argument("Cannot add a parameter with an empty name");
    }

    f_parameter[name] = value;
}


/** \brief Retrieve the value of a named parameter.
 *
 * This function returns the value of the named parameter. If the parameter
 * is not currently defined, this function returns an empty string.
 *
 * \exception sendmail_exception_invalid_argument
 * The name of a parameter cannot be empty. This exception is raised if
 * \p name is empty.
 *
 * \param[in] name  A valid parameter name.
 *
 * \return The current value of that parameter or an empty string if undefined.
 */
QString sendmail::email::get_parameter(const QString& name) const
{
    if(name.isEmpty())
    {
        throw sendmail_exception_invalid_argument("Cannot retrieve a parameter with an empty name");
    }
    if(f_parameter.contains(name))
    {
        return f_parameter[name];
    }

    return "";
}


/** \brief Get all the parameters defined in this email.
 *
 * This function returns the map of the parameters defined in this email.
 * This can be used to quickly scan all the parameters.
 *
 * \note
 * It is important to remember that since this function returns a reference
 * to the map of parameters, it may break if you call add_parameter() while
 * going through the references.
 *
 * \return A direct reference to the internal parameter map.
 */
const sendmail::email::header_map_t& sendmail::email::get_all_parameters() const
{
    return f_parameter;
}


/** \brief Unserialize an email message.
 *
 * This function unserializes an email message that was serialized using
 * the serialize() function.
 *
 * You are expected to first create an email object and then call this
 * function with the data parameter set as the string that the serialize()
 * function returned.
 *
 * You may setup some default headers such as the X-Mailer value in your
 * email object before calling this function. If such header information
 * is defined in the serialized data then it will be overwritten with
 * that data. Otherwise it will remain the same.
 *
 * The function doesn't return anything. Instead it unserializes the
 * \p data directly in this email object.
 *
 * \param[in] data  The serialized email data to transform.
 *
 * \sa serialize()
 */
void sendmail::email::unserialize(const QString& data)
{
    // QBuffer takes a non-const QByteArray so we have to create a copy
    QByteArray non_const_data(data.toUtf8().data());
    QBuffer in(&non_const_data);
    in.open(QIODevice::ReadOnly);
    QtSerialization::QReader reader(in);
    QtSerialization::QComposite comp;
    QtSerialization::QFieldTag rules(comp, "email", this);
    reader.read(comp);
}


/** \brief Read the contents of one tag from the reader.
 *
 * This function reads the contents of the main email tag. It calls
 * the attachment unserialize() as required whenever an attachment
 * is found in the stream.
 *
 * \param[in] name  The name of the tag being read.
 * \param[in] r  The reader used to read the input data.
 */
void sendmail::email::readTag(const QString& name, QtSerialization::QReader& r)
{
    //if(name == "email")
    //{
    //    QtSerialization::QComposite comp;
    //    QtSerialization::QFieldTag info(comp, "content", this);
    //    r.read(comp);
    //}
    if(name == "email")
    {
        QtSerialization::QComposite comp;
        QtSerialization::QFieldString tag_cumulative(comp, "cumulative", f_cumulative);
        QtSerialization::QFieldString tag_site_key(comp, "site_key", f_site_key);
        QtSerialization::QFieldString tag_email_path(comp, "email_path", f_email_path);
        QtSerialization::QFieldString tag_email_key(comp, "email_key", f_email_key);
        QtSerialization::QFieldTag tag_header(comp, "header", this);
        QtSerialization::QFieldTag tag_attachment(comp, "attachment", this);
        QtSerialization::QFieldTag tag_parameter(comp, "parameter", this);
        r.read(comp);
    }
    else if(name == "header")
    {
        QtSerialization::QComposite comp;
        QString header_name;
        QtSerialization::QFieldString tag_name(comp, "name", header_name);
        QString header_value;
        QtSerialization::QFieldString tag_value(comp, "value", header_value);
        r.read(comp);
        f_header[header_name] = header_value;
    }
    else if(name == "attachment")
    {
        email_attachment attachment;
        attachment.unserialize(r);
        add_attachment(attachment);
    }
    else if(name == "parameter")
    {
        QtSerialization::QComposite comp;
        QString parameter_name;
        QtSerialization::QFieldString tag_name(comp, "name", parameter_name);
        QString parameter_value;
        QtSerialization::QFieldString tag_value(comp, "value", parameter_value);
        r.read(comp);
        f_parameter[parameter_name] = parameter_value;
    }
}


/** \brief Transform the email in one string.
 *
 * This function transform the email data in one string so it can easily
 * be saved in the Cassandra database. This is done so it can be sent to
 * the recipients using the backend process preferably on a separate
 * computer (i.e. a computer that is not being accessed by your web
 * clients.)
 *
 * The unserialize() function can be used to restore an email that was
 * previously serialized with this function.
 *
 * \return The email object in the form of a string.
 *
 * \sa unserialize()
 */
QString sendmail::email::serialize() const
{
    QByteArray result;
    QBuffer archive(&result);
    archive.open(QIODevice::WriteOnly);
    {
        QtSerialization::QWriter w(archive, "email", EMAIL_MAJOR_VERSION, EMAIL_MINOR_VERSION);
        QtSerialization::QWriter::QTag tag(w, "email");
        if(!f_cumulative.isEmpty())
        {
            QtSerialization::writeTag(w, "cumulative", f_cumulative);
        }
        QtSerialization::writeTag(w, "site_key", f_site_key);
        QtSerialization::writeTag(w, "email_path", f_email_path);
        QtSerialization::writeTag(w, "email_key", f_email_key);
        for(header_map_t::const_iterator it(f_header.begin()); it != f_header.end(); ++it)
        {
            QtSerialization::QWriter::QTag header(w, "header");
            QtSerialization::writeTag(w, "name", it.key());
            QtSerialization::writeTag(w, "value", it.value());
        }
        const int amax(f_attachment.count());
        for(int i(0); i < amax; ++i)
        {
            f_attachment[i].serialize(w);
        }
        for(header_map_t::const_iterator it(f_parameter.begin()); it != f_parameter.end(); ++it)
        {
            QtSerialization::QWriter::QTag parameter(w, "parameter");
            QtSerialization::writeTag(w, "name", it.key());
            QtSerialization::writeTag(w, "value", it.value());
        }
        // end the writer so everything gets saved in the buffer (result)
    }

    return QString::fromUtf8(result.data());
}


/** \brief Initialize the sendmail plugin.
 *
 * This function is used to initialize the sendmail plugin object.
 */
sendmail::sendmail()
    //: f_snap(NULL) -- auto-init
{
}


/** \brief Clean up the sendmail plugin.
 *
 * Ensure the sendmail object is clean before it is gone.
 */
sendmail::~sendmail()
{
}


/** \brief Initialize sendmail.
 *
 * This function terminates the initialization of the sendmail plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void sendmail::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN(sendmail, "server", server, register_backend_action, _1);
    SNAP_LISTEN(sendmail, "filter", filter::filter, replace_token, _1, _2, _3);
}


/** \brief Get a pointer to the sendmail plugin.
 *
 * This function returns an instance pointer to the sendmail plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the sendmail plugin.
 */
sendmail *sendmail::instance()
{
    return g_plugin_sendmail_factory.instance();
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
QString sendmail::description() const
{
    return "Handle sending emails from your website environment."
        " This version of sendmail requires a backend process to"
        " actually process the emails and send them out.";
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
int64_t sendmail::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2013, 11, 18, 1, 5, 0, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the content plugin.
 *
 * This function is the first update for the content plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void sendmail::initial_update(int64_t variables_timestamp)
{
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our administration pages, etc.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void sendmail::content_update(int64_t variables_timestamp)
{
    content::content::instance()->add_xml(get_name(SNAP_NAME_SENDMAIL));
}


/** \brief Initialize the emails table.
 *
 * This function creates the "emails" table if it doesn't exist yet. Otherwise
 * it simple returns the existing Cassandra table.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The table is used for several purposes:
 *
 * \li List of emails to be sent
 *
 * Whenever a plugin sends an email, it makes use of this table via the
 * post_email() function. This function adds an entry under the "new" row
 * which is used to post new emails to the backend. The backend is started
 * with the special "sendmail" action to actually handle the emails.
 *
 * \li Email lists to handle multi-users send
 *
 * This table has a special entry named "lists" which is a list of emails
 * that are used by end users to create mailing lists. For example, all your
 * staff could be listed under a list named "staff@m2osw.com". Sending an
 * email to "staff" results in an email sent to all the people listed in that
 * list. Note that if you create such lists in your mail server then this
 * Snap! Websites feature is not required.
 *
 * Since a list needs to be specific to a website (or at least a
 * well defined group of websites) the names in such lists include the name
 * of the website (i.e. m2osw.com:staff; the name of the site is taken from
 * the f_snap->get_site_key() function.) The name looks like this:
 *
 * \code
 * <site-key>: <list-name>
 * \endcode
 *
 * Where the site key parameter comes from the email site key (set when you
 * call the post_email() function) and the list name parameter comes from
 * the actual To: email list.
 *
 * \li List of user email addresses
 *
 * Each user has one entry in the table which is keyed by their email address
 * (since emails are considered unique.) The fact that we save the emails in
 * this table rather than the user table is to avoid problems (i.e. growth
 * of the user table could result in some unwanted slowness.)
 *
 * The list includes each email using the email key as the cell name and the
 * email data as the email contents. Later we may want to have a key which
 * includes the date or the name of the sender in order to be able to quickly
 * list the emails to the user in such or such order.
 *
 * The table also manages information about the emails such as whether it
 * was looked at, deleted, spam, etc. Spam emails do not get deleted so that
 * way we can easily eliminate duplicates (Although we may want to make use
 * of a spam user at some point.)
 *
 * Speaking of duplicates, when we detect that another email was sent to
 * tell something to the user (i.e. you got new comments) and the user did
 * not yet come back on the site, then the new version should not be saved.
 * This feature makes use of the cumulative flag.
 *
 * Emails that were sent (i.e. using the /usr/bin/sendmail tool) are marked
 * as sent so we avoid sending them again.
 *
 * \return The pointer to the users table.
 */
QSharedPointer<QtCassandra::QCassandraTable> sendmail::get_emails_table()
{
    return f_snap->create_table(get_name(SNAP_NAME_SENDMAIL_EMAILS_TABLE), "E-Mails table.");
}


/** \brief Prepare the email for the filter_email signal.
 *
 * This function readies the email parameter (\p e) for the filter_email
 * signal.
 *
 * At this point this function processes the email using the token plugin
 * so as to convert all the tags in the body (text and HTML) before
 * proceeding further.
 *
 * \param[in] e  The email to be filtered by other plugins.
 *
 * \return true if the signal is to be propagated to all the other plugins.
 */
bool sendmail::filter_email_impl(email& e)
{
    return true;
}


/** \brief Post an email.
 *
 * This function posts an email.
 *
 * The email is not sent immediately, instead it gets added to the
 * Cassandra database and processed later by the sendmail backend
 * (i.e. using "snapbackend -a sendmail".)
 *
 * Note that the message is not processed here at all. The backend
 * is fully responsible. The processing determines all the users
 * being emailed, when to send the email, whether the user wants
 * HTML or not, etc.
 *
 * \note
 * We save the original data to the Cassandra database so it can be
 * processed by the backend instead of the frontend. That way we can save
 * time in the frontend.
 *
 * \exception sendmail_exception_invalid_argument
 * An invalid argument exception is raised if no content was specified before
 * you call this function. The email is considered empty if no attachments
 * were added and no email path was defined.
 *
 * \param[in] e  Email to post.
 */
void sendmail::post_email(const email& e)
{
    // we do not accept to send an email email
    if(e.get_attachment_count() == 0 && e.get_email_path().isEmpty())
    {
        throw sendmail_exception_invalid_argument("An email must have at least one attachment or the email path defined");
    }

    email copy(e);
    copy.set_site_key(f_snap->get_site_key());
    QString key(f_snap->get_unique_number());
    copy.set_email_key(key);
    QSharedPointer<QtCassandra::QCassandraTable> table(get_emails_table());
    QtCassandra::QCassandraValue value;
    QString data(copy.serialize());
    value.setStringValue(data);
    table->row(get_name(SNAP_NAME_SENDMAIL_NEW))->cell(key)->setValue(value);

    // signal the listening server if IP is available (send PING)
    f_snap->udp_ping("sendmail_udp_signal");
}


/** \brief Register the sendmail action.
 *
 * This function registers this plugin as supporting the "sendmail" action.
 * This is used by the backend to start a sendmail server so users on a
 * website sending emails end up having the email sent when this action
 * is running in the background.
 *
 * At this time we only support one action named "sendmail".
 *
 * \param[in] actions  The list of supported actions where we add ourselves.
 */
void sendmail::on_register_backend_action(snap::server::backend_action_map_t& actions)
{
    actions[get_name(SNAP_NAME_SENDMAIL)] = this;
}


/** \brief Start the sendmail server.
 *
 * When running the backend the user can ask to run the sendmail
 * server (--action sendmail). This function captures those events.
 * It loops until stopped with a STOP message via the UDP address/port.
 * Note that Ctrl-C won't work because it does not support killing
 * both: the parent and child processes (we do a fork() to create
 * this child.)
 *
 * The loop reads all the emails that are ready to be processed then
 * falls asleep until the next UDP PING event received via the
 * sendmail_udp_signal IP:Port information.
 *
 * Note that because the UDP signals are not 100% reliable, the
 * server actually sleeps for 5 minutes and checks for new emails
 * whether a PING signal was received or not.
 *
 * The email data is found in the Cassandra cluster and never
 * sent along the UDP signal. This means the UDP signals do not need
 * to be secure.
 *
 * \note
 * The \p action parameter is here because some plugins may
 * understand multiple actions in which case we need to know
 * which action is waking us up.
 *
 * \param[in] action  The action this function is being called with.
 */
void sendmail::on_backend_action(const QString& action)
{
    QSharedPointer<udp_client_server::udp_server> udp_signals(f_snap->udp_get_server("sendmail_udp_signal"));
    const char *stop(get_name(SNAP_NAME_SENDMAIL_STOP));
    for(;;)
    {
        // immediately process emails that have already arrived
        process_emails();
        run_emails();
        char buf[256];
        int r(udp_signals->timed_recv(buf, sizeof(buf), 5 * 60 * 1000)); // wait for up to 5 minutes (x 60 seconds)
        if(r != -1 || errno != EAGAIN)
        {
            if(r < 1 || r >= static_cast<int>(sizeof(buf) - 1))
            {
                perror("udp_signals->timed_recv():");
                std::cerr << "error: an error occured in the UDP recv() call, returned size: " << r << std::endl;
                exit(1);
            }
            buf[r] = '\0';
            if(strcmp(buf, stop) == 0)
            {
                // clean STOP
                return;
            }
            // should we check that we really received a PING?
            //const char *ping(get_name(SNAP_NAME_SENDMAIL_PING));
            //if(strcmp(buf, ping) != 0)
            //{
            //    continue
            //}
        }
    }
}


/** \brief Process all the emails received in Cassandra.
 *
 * This function goes through the list of "new" emails received
 * in the Cassandra cluster as the post_email() function deposits
 * them there.
 *
 * First, the emails are processed in memory and then saved in
 * each destination user's mailbox in Cassandra (all email
 * addresses exist in our database whether someone wants it or
 * not!) Finally, users who request to receive a copy or
 * notifications have those sent to their usual mailbox. A mailbox
 * can also be marked as a blackhole (destination does not
 * exist) or a "do not contact" mailbox (destination does not
 * want to receive anything from us.)
 *
 * Mailing lists are managed at the bext level. These lists are
 * used to transform a name in an email into a list of email addresses.
 * See the attach_email() function for more details.
 */
void sendmail::process_emails()
{
    QSharedPointer<QtCassandra::QCassandraTable> table(get_emails_table());
    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(get_name(SNAP_NAME_SENDMAIL_NEW)));
    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setCount(100); // should this be a parameter?
    column_predicate.setIndex(); // behave like an index
    for(;;)
    {
        row->clearCache();
        row->readCells(column_predicate);
        const QtCassandra::QCassandraCells& cells(row->cells());
        if(cells.isEmpty())
        {
            break;
        }
        // handle one batch
        for(QtCassandra::QCassandraCells::const_iterator c(cells.begin());
                c != cells.end();
                ++c)
        {
            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            QSharedPointer<QtCassandra::QCassandraCell> cell(*c);
            const QtCassandra::QCassandraValue value(cell->value());
            if(!value.nullValue())
            {
                email e;
                e.unserialize(value.stringValue());
                attach_email(e);
            }
            // we're done with that email, get rid of it
            row->dropCell(cell->columnKey());
        }
    }
}


/** \brief Process one email.
 *
 * This function processes one email. This means changing each destination
 * found in the To: field with the corresponding list of users (in case the
 * name references a mailing list) and then sending the email to the user's
 * account.
 *
 * Note that at this point this process does not actually send any emails.
 * It merely posts them to each user. This allows us to avoid sending the
 * same user multiple times the same email, to group emails, send emails
 * to a given user at most once a day, etc.
 *
 * \param[in] e  The email to attach to a list of users.
 */
void sendmail::attach_email(const email& e)
{
    QString to(e.get_header(get_name(SNAP_NAME_SENDMAIL_TO)));

    // transform To: ... in a list of emails
    tld_email_list list;
    if(list.parse(to.toStdString(), 0) != TLD_RESULT_SUCCESS)
    {
        // Nothing we can do with those!? We should have erred when the
        // user specified this email address a long time ago.
        return;
    }

    QSharedPointer<QtCassandra::QCassandraTable> table(get_emails_table());
    QSharedPointer<QtCassandra::QCassandraRow> lists(table->row(get_name(SNAP_NAME_SENDMAIL_LISTS)));

    // read all the emails
    const QString& site_key(e.get_site_key());
    tld_email_list::tld_email_t m;
    bool is_list(false);
    while(list.next(m))
    {
        std::vector<tld_email_list::tld_email_t> emails;
        if(!m.f_email_only.empty())
        {
            QString list_key(site_key + ": " + m.f_email_only.c_str());
            if(lists->exists(list_key))
            {
                // if the email is a list, we do not directly send to it
                is_list = true;
                QtCassandra::QCassandraValue list_value(lists->cell(list_key)->value());
                tld_email_list user_list;
                if(user_list.parse(to.toStdString(), 0) == TLD_RESULT_SUCCESS)
                {
                    tld_email_list::tld_email_t um;
                    while(user_list.next(um))
                    {
                        // TODO
                        // what if um is the name of a list? We would
                        // have to add that to a list which itself
                        // gets processed
                        emails.push_back(um);
                    }
                }
                // else ignore this error at this point...
            }
        }
        if(!is_list)
        {
            emails.push_back(m);
        }
        else
        {
            is_list = false;
        }
        if(!emails.empty())
        {
            // if the list is not empty, handle it!
            for(std::vector<tld_email_list::tld_email_t>::const_iterator
                    it(emails.begin()); it != emails.end(); ++it)
            {
                // if groups are specified then the email address can be empty
                if(!it->f_email_only.empty())
                {
                    email copy(e);
                    copy.add_header(get_name(SNAP_NAME_SENDMAIL_TO), it->f_canonicalized_email.c_str());
                    attach_user_email(copy);
                }
            }
        }
    }
}


/** \brief Attach the specified email to the specified user.
 *
 * The specified email has an email address which is expected to be the
 * final destination (i.e. a user). This email is added to the user's email
 * account. It is then added to an index of emails that need to actually
 * be sent unless the user frequency parameter says that the email is only
 * to be registered in the system.
 *
 * \param[in] e  The email to attach to a user's account.
 */
void sendmail::attach_user_email(const email& e)
{
    QSharedPointer<QtCassandra::QCassandraTable> table(get_emails_table());
    users::users *users_plugin(users::users::instance());
    const char *email_key(users::get_name(users::SNAP_NAME_USERS_ORIGINAL_EMAIL));
    QSharedPointer<QtCassandra::QCassandraTable> users_table(users_plugin->get_users_table());

    // TBD: would we need to have a lock to test whether the user
    //      exists? since we're not about to add it ourselves, I
    //      do not think it is necessary
    QString to(e.get_header(get_name(SNAP_NAME_SENDMAIL_TO)));
    tld_email_list list;
    if(list.parse(to.toStdString(), 0) != TLD_RESULT_SUCCESS)
    {
        // this should never happen here
        throw sendmail_exception_invalid_argument("To: field is not a valid email");
    }
    tld_email_list::tld_email_t m;
    if(!list.next(m))
    {
        throw sendmail_exception_invalid_argument("To: field does not include at least one email");
    }
    QString key(m.f_email_only.c_str());
    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(key));
    QSharedPointer<QtCassandra::QCassandraCell> cell(row->cell(email_key));
    cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    QtCassandra::QCassandraValue email_data(cell->value());
    if(email_data.nullValue())
    {
        // the user does not yet exist, we only email people who have some
        // sort an account because otherwise we could not easily track
        // people's wishes (i.e. whether they do not want to receive our
        // emails); this system allows us to block all emails
        users_plugin->register_user(m.f_email_only.c_str(), "!");
    }

    // TODO: if the user is a placeholder (i.e. user changed his email
    //       address) then we need to get the new email...

    // save the email for that user
    // (i.e. emails can be read from within the website)
    QString serialized_email(e.serialize());
    QtCassandra::QCassandraValue email_value;
    email_value.setStringValue(serialized_email);
    QString unique_key(e.get_email_key());
    row->cell(unique_key + "::" + get_name(SNAP_NAME_SENDMAIL_EMAIL))->setValue(email_value);
    QtCassandra::QCassandraValue status_value;
    status_value.setStringValue(get_name(SNAP_NAME_SENDMAIL_STATUS_NEW));
    row->cell(unique_key + "::" + get_name(SNAP_NAME_SENDMAIL_STATUS))->setValue(status_value);
    QtCassandra::QCassandraValue sent_value;
    sent_value.setStringValue(get_name(SNAP_NAME_SENDMAIL_STATUS_NEW));
    row->cell(unique_key + "::" + get_name(SNAP_NAME_SENDMAIL_SENDING_STATUS))->setValue(sent_value);

    // try to retrieve the mail frequency the user likes, but first check
    // whether the email has one because if so it overrides the user's choice
    QtCassandra::QCassandraValue freq_value(row->cell(get_name(SNAP_NAME_SENDMAIL_FREQUENCY))->value());
    if(freq_value.nullValue())
    {
        freq_value = users_table->row(key)->cell(get_name(SNAP_NAME_SENDMAIL_FREQUENCY))->value();
    }

    const char *immediate(get_name(SNAP_NAME_SENDMAIL_FREQUENCY_IMMEDIATE));
    QString frequency(immediate);
    if(!freq_value.nullValue())
    {
        frequency = freq_value.stringValue();
    }
    // default date for immediate emails
    time_t unix_date(time(NULL));
    // TODO: add user's timezone adjustment or the following math is wrong
    if(frequency != immediate)
    {
        struct tm t;
        gmtime_r(&unix_date, &t);
        t.tm_sec = 0;
        t.tm_min = 0;
        t.tm_hour = 10;
        if(frequency == get_name(SNAP_NAME_SENDMAIL_FREQUENCY_DAILY))
        {
            // tomorrow at 10am
            ++t.tm_mday;
        }
        else if(frequency == get_name(SNAP_NAME_SENDMAIL_FREQUENCY_WEEKLY))
        {
            // next Sunday at 10am
            t.tm_mday += 7 - t.tm_wday;
            // TODO: allow users to select the day of the week they prefer
        }
        else if(frequency == get_name(SNAP_NAME_SENDMAIL_FREQUENCY_MONTHLY))
        {
            t.tm_mday = 1;
            t.tm_mday = 1;
            t.tm_mon += 1;
        }
        else
        {
            // TODO: warn about invalid value
            SNAP_LOG_WARNING("unknown email frequency \"")(frequency)("\" for user \"")(key)("\"");
            ++t.tm_mday; // as DAILY
        }
        t.tm_isdst = 0; // mkgmtime() ignores DST... (i.e. UTC is not affected)
        unix_date = mkgmtime(&t);
    }

    QString index_key(QString("%1::%2").arg(unix_date, 16, 16, QLatin1Char('0')).arg(key));

    QtCassandra::QCassandraValue index_value;
    const char *index(get_name(SNAP_NAME_SENDMAIL_INDEX));
    if(table->exists(index))
    {
        // the index already exists, check to see whether that cell exists
        if(table->row(index)->exists(index_key))
        {
            // it exists, we need to concatenate the values
            index_value = table->row(index)->cell(index_key);
        }
    }
    if(!index_value.nullValue())
    {
        index_value.setStringValue(index_value.stringValue() + "," + unique_key);
    }
    else
    {
        index_value.setStringValue(unique_key);
    }
    table->row(index)->cell(index_key)->setValue(index_value);

    //sendemail(key, unique_key);
}


/** \brief Go through the list of emails to send.
 *
 * This function goes through the '*index*' of emails that are ready to
 * be sent to end users. When emails are posted to the sendmail plugin,
 * they are added to a list with a date when they should be sent.
 */
void sendmail::run_emails()
{
    QSharedPointer<QtCassandra::QCassandraTable> table(get_emails_table());
    const char *index(get_name(SNAP_NAME_SENDMAIL_INDEX));
    QSharedPointer<QtCassandra::QCassandraRow> row(table->row(index));
    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setStartColumnName("0");
    // we use +1 otherwise immediate emails are sent 5 min. later!
    time_t unix_date(time(NULL) + 1);
    QString end(QString("%1").arg(unix_date, 16, 16, QLatin1Char('0')));
    column_predicate.setEndColumnName(end);
    column_predicate.setCount(100); // should this be a parameter?
    column_predicate.setIndex(); // behave like an index
    for(;;)
    {
        row->clearCache();
        row->readCells(column_predicate);
        const QtCassandra::QCassandraCells& cells(row->cells());
        if(cells.isEmpty())
        {
            break;
        }
        // handle one batch
        for(QtCassandra::QCassandraCells::const_iterator c(cells.begin());
                c != cells.end();
                ++c)
        {
            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            QSharedPointer<QtCassandra::QCassandraCell> cell(*c);
            const QtCassandra::QCassandraValue value(cell->value());
            const QString column_key(cell->columnKey());
            const QString key(column_key.mid(18));
            if(!value.nullValue())
            {
                QString unique_keys(value.stringValue());
                QStringList list(unique_keys.split(","));
                const int max(list.size());
                for(int i(0); i < max; ++i)
                {
                    sendemail(key, list[i]);
                }
            }
            // we're done with that email, get rid of it
            row->dropCell(column_key);
        }
    }
}


/** \brief This function actually sends the email.
 *
 * This function is the one that actually takes the email and sends it to
 * the destination. At this point it makes use of the sendmail tool.
 *
 * \param[in] key  The key of the email being send; i.e. the email in the To:
 *                 header
 * \param[in] unique_key  The email unique key.
 */
void sendmail::sendemail(const QString& key, const QString& unique_key)
{
    QSharedPointer<QtCassandra::QCassandraTable> table(get_emails_table());
    QtCassandra::QCassandraValue sent_value(table->row(key)->cell(unique_key + "::" + get_name(SNAP_NAME_SENDMAIL_SENDING_STATUS))->value());
    //if(sent_value.stringValue() != get_name(SNAP_NAME_SENDMAIL_STATUS_NEW))
    if(sent_value.stringValue() == get_name(SNAP_NAME_SENDMAIL_STATUS_SENT))
    {
        // email was already sent, not too sure why we're being called,
        // just ignore to avoid bothering the destination owner...
        return;
    }
    // mark that the email was sent, if it fails from here, then we do not
    // try again... althought we mark the status can be used to warn the
    // sender that a problem arised before the email was actually sent
    QtCassandra::QCassandraValue sending_value;
    sending_value.setStringValue(get_name(SNAP_NAME_SENDMAIL_STATUS_LOADING));
    table->row(key)->cell(unique_key + "::" + get_name(SNAP_NAME_SENDMAIL_SENDING_STATUS))->setValue(sending_value);

    QtCassandra::QCassandraValue email_data(table->row(key)->cell(unique_key + "::" + get_name(SNAP_NAME_SENDMAIL_EMAIL))->value());
    f_email = email();
    f_email.unserialize(email_data.stringValue());
    f_email.add_header(get_name(SNAP_NAME_SENDMAIL_CONTENT_TYPE), "text/html; charset=\"utf-8\"");

    const QString& path(f_email.get_email_path());
    if(!path.isEmpty())
    {
        // TODO -- we need to get a layout that is for the email, not the
        //         default layout which will include all the theme
        //         everything (title, header, footer...)
        const QString body(layout::layout::instance()->apply_layout(path, this));

        email::email_attachment body_attachment;
        // TBD: do we have to define a "text/plain" type too?
        body_attachment.set_data(body.toUtf8(), "text/html; charset=\"utf-8\"");
        f_email.set_body_attachment(body_attachment);

        // Use the page title as the subject
        // (TBD: should the page title always overwrite the subject?)
        if(f_email.get_header(get_name(SNAP_NAME_SENDMAIL_SUBJECT)).isEmpty())
        {
            // TODO: apply filters on the subject
            content::content *c(content::content::instance());
            f_email.set_subject(c->get_content_parameter(path, get_name(content::SNAP_NAME_CONTENT_TITLE)).stringValue());
        }
    }

    // verify that we have at least one attachment
    const int max(f_email.get_attachment_count());
    if(max < 1)
    {
        // this should never happen since this is tested in the post_email() function
        throw sendmail_exception_invalid_argument("To: email is invalid, email won't get sent");
    }

    // we want to transform the body from HTML to text ahead of time
    email::email_attachment body(f_email.get_attachment(0));
    // TODO: verify that the body is indeed HTML!
    //       although html2text works against plain text but that's a waste
    QString plain_text;
    const QString body_mime_type(body.get_header(get_name(SNAP_NAME_SENDMAIL_CONTENT_TYPE)));
    if(body_mime_type.mid(0, 9) == "text/html")
    {
        process p("html2text");
        p.set_mode(process::PROCESS_MODE_INOUT);
        p.set_command("html2text");
        //p.add_argument("-help");
        p.add_argument("-nobs");
        p.add_argument("-utf8");
        p.add_argument("-style");
        p.add_argument("pretty");
        p.add_argument("-width");
        p.add_argument("70");
        QByteArray data(body.get_data());
        p.set_input(QString::fromUtf8(data.data()));
        int r(p.run());
        if(r == 0)
        {
            plain_text = p.get_output();
        }
    }

    QString to(f_email.get_header(get_name(SNAP_NAME_SENDMAIL_TO)));
    tld_email_list list;
    if(list.parse(to.toStdString(), 0) != TLD_RESULT_SUCCESS)
    {
        // this should never happen here
        throw sendmail_exception_invalid_argument("To: email is invalid, email won't get sent");
    }
    tld_email_list::tld_email_t m;
    if(!list.next(m))
    {
        throw sendmail_exception_invalid_argument("To: email does not return at least one email, email won't get sent");
    }

    // now we're starting to send the email to the system sendmail tool
    sending_value.setStringValue(get_name(SNAP_NAME_SENDMAIL_STATUS_SENDING));
    table->row(key)->cell(unique_key + "::" + get_name(SNAP_NAME_SENDMAIL_SENDING_STATUS))->setValue(sending_value);

    QString cmd("sendmail -f ");
    cmd += f_email.get_header(get_name(SNAP_NAME_SENDMAIL_FROM));
    cmd += " ";
    cmd += m.f_email_only.c_str();

    // TODO: put the FILE pointer in an RAII class!!!
    //       we're looping forever so it would leak quite a bit if not properly
    //       closed with a pclose().
    FILE *f(popen(cmd.toUtf8().data(), "w"));

    // To debug, you may set *f to stdout, but make sure to not pclose(f);
    // once done with it!
    //FILE *f(stdout);

    if(f == NULL)
    {
        // TODO: register the error
        return;
    }
    // convert email data to text and send that to the sendmail command
    email::header_map_t headers(f_email.get_all_headers());
    const bool body_only(max == 1 && plain_text.isEmpty());
    QString boundary;
    if(!body_only)
    {
        // boundary      := 0*69<bchars> bcharsnospace
        // bchars        := bcharsnospace / " "
        // bcharsnospace := DIGIT / ALPHA / "'" / "(" / ")" /
        //                  "+" / "_" / "," / "-" / "." /
        //                  "/" / ":" / "=" / "?"
        // Note: we generate boundaries without special characters
        //       (and especially no spaces or dashes)
        const char allowed[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; //'()+_,./:=?";
        boundary = "=Snap.Websites=";
        for(int i(0); i < 20; ++i)
        {
            int c(rand() % (sizeof(allowed) - 1));
            boundary += allowed[c];
        }
        headers["Content-Type"] = "multipart/alternative; boundary=\"" + boundary + "\"";
    }
    for(email::header_map_t::const_iterator it(headers.begin());
                                            it != headers.end();
                                            ++it)
    {
        fprintf(f, "%s: %s\n", it.key().toUtf8().data(), it.value().toUtf8().data());
    }

    // one empty line before the contents
    fprintf(f, "\n");

    if(body_only)
    {
        // in this case we only have one entry, probably HTML, and thus we
        // can avoid the multi-part headers and attachments
        email::email_attachment attachment(f_email.get_attachment(0));
        fprintf(f, "%s\n", attachment.get_data().data());
    }
    else
    {
        if(!plain_text.isEmpty())
        {
            fprintf(f, "--%s\n", boundary.toUtf8().data());
            fprintf(f, "Content-Type: text/plain; charset=\"utf-8\"\n");
            fprintf(f, "MIME-Version: 1.0\n");
            //fprintf(f, "Content-Transfer-Encoding: quoted-printable\n");
            fprintf(f, "Content-Description: Mail message body\n");
            fprintf(f, "\n");
            // TODO: actually quoted-printable encode this buffer!
            fprintf(f, "%s\n", plain_text.toUtf8().data());
        }
        // note that we send ALL the attachments, including attachment 0 since
        // if we converted the HTML to plain text, we still want to send the
        // HTML to the user
        for(int i(0); i < max; ++i)
        {
            email::email_attachment attachment(f_email.get_attachment(i));
            fprintf(f, "--%s\n", boundary.toUtf8().data());
            email::header_map_t attachment_headers(attachment.get_all_headers());
            for(email::header_map_t::const_iterator it(attachment_headers.begin());
                                                    it != attachment_headers.end();
                                                    ++it)
            {
                fprintf(f, "%s: %s\n", it.key().toUtf8().data(), it.value().toUtf8().data());
            }

            // one empty line before the contents
            fprintf(f, "\n");

            // in this case the data is expected to already be encoded except
            // for the first message (is that true?)
            fprintf(f, "%s\n", attachment.get_data().data());
        }
        fprintf(f, "--%s--\n", boundary.toUtf8().data());
    }

    // end the message
    fprintf(f, "\n");
    fprintf(f, ".\n");
    pclose(f);

    // now it is marked as fully sent
    sending_value.setStringValue(get_name(SNAP_NAME_SENDMAIL_STATUS_SENT));
    table->row(key)->cell(unique_key + "::" + get_name(SNAP_NAME_SENDMAIL_SENDING_STATUS))->setValue(sending_value);
}


/** \brief Add sendmail specific tags to the layout DOM.
 *
 * This function adds different sendmail specific tags to the layout page
 * and body XML documents. Especially, it will add all the parameters that
 * the user added to the email object before calling the post_email()
 * function.
 *
 * \param[in] l  The layout pointer.
 * \param[in] path  The path being managed.
 * \param[in,out] page  The page being generated.
 * \param[in,out] body  The body being generated.
 */
void sendmail::on_generate_main_content(layout::layout *l, const QString& path, QDomElement& page, QDomElement& body)
{
    // by default an email is just like a regular page
    content::content::instance()->on_generate_main_content(l, path, page, body);

    // but we also have email specific parameters we want to add
    QDomDocument doc(page.ownerDocument());

    {
        QDomElement sendmail_tag(doc.createElement("sendmail"));
        body.appendChild(sendmail_tag);

        {
            QDomElement from(doc.createElement("from"));
            sendmail_tag.appendChild(from);
            const QString from_email(f_email.get_header(get_name(SNAP_NAME_SENDMAIL_FROM)));
            QDomText from_text(doc.createTextNode(from_email));
            from.appendChild(from_text);
            // TODO: parse the email address with libtld and offer:
            //         sender-name
            //         sender-email
        }
        {
            QDomElement to(doc.createElement("to"));
            sendmail_tag.appendChild(to);
            const QString to_email(f_email.get_header(get_name(SNAP_NAME_SENDMAIL_TO)));
            QDomText to_text(doc.createTextNode(to_email));
            to.appendChild(to_text);
        }
        {
            QDomElement path_tag(doc.createElement("path"));
            sendmail_tag.appendChild(path_tag);
            QDomText path_text(doc.createTextNode(f_email.get_email_path()));
            path_tag.appendChild(path_text);
        }
        {
            QDomElement key(doc.createElement("key"));
            sendmail_tag.appendChild(key);
            QDomText key_text(doc.createTextNode(f_email.get_email_key()));
            key.appendChild(key_text);
        }
        const QString created(f_snap->date_to_string(f_email.get_time() * 1000000, true));
        {
            QDomElement time_tag(doc.createElement("created"));
            sendmail_tag.appendChild(time_tag);
            QDomText time_text(doc.createTextNode(created));
            time_tag.appendChild(time_text);
        }
        {
            QDomElement time_tag(doc.createElement("date"));
            sendmail_tag.appendChild(time_tag);
            QDomText time_text(doc.createTextNode(created.mid(0, 10)));
            time_tag.appendChild(time_text);
        }
        {
            QDomElement time_tag(doc.createElement("time"));
            sendmail_tag.appendChild(time_tag);
            QDomText time_text(doc.createTextNode(created.mid(11)));
            time_tag.appendChild(time_text);
        }
        {
            QDomElement time_tag(doc.createElement("attachment_count"));
            sendmail_tag.appendChild(time_tag);
            QDomText time_text(doc.createTextNode(QString("%1").arg(f_email.get_attachment_count())));
            time_tag.appendChild(time_text);
        }
        const QString x_priority(f_email.get_header(get_name(SNAP_NAME_SENDMAIL_X_PRIORITY)));
        {
            // save the priority as a name
            QDomElement important(doc.createElement("important"));
            sendmail_tag.appendChild(important);
            const QString important_email(f_email.get_header(get_name(SNAP_NAME_SENDMAIL_IMPORTANT)));
            QDomText important_text(doc.createTextNode(important_email));
            important.appendChild(important_text);
        }
        {
            // save the priority as a value + name between parenthesis
            QDomElement priority(doc.createElement("x-priority"));
            sendmail_tag.appendChild(priority);
            QDomText priority_text(doc.createTextNode(x_priority));
            priority.appendChild(priority_text);
        }
        {
            // save the priority as a value
            QDomElement priority(doc.createElement("priority"));
            sendmail_tag.appendChild(priority);
            QStringList value_name(x_priority.split(" "));
            QDomText priority_text(doc.createTextNode(value_name[0]));
            priority.appendChild(priority_text);
        }
        const email::header_map_t& parameters(f_email.get_all_parameters());
        if(parameters.size() > 0)
        {
            QDomElement parameters_tag(doc.createElement("parameters"));
            sendmail_tag.appendChild(parameters_tag);
            for(email::header_map_t::const_iterator it(parameters.begin());
                                                    it != parameters.end();
                                                    ++it)
            {
                QDomElement param_tag(doc.createElement("param"));
                param_tag.setAttribute("name", it.key());
                param_tag.setAttribute("value", it.value());
                parameters_tag.appendChild(param_tag);
            }
        }
    }
}


/** \brief Replace a token with a corresponding value.
 *
 * This function replaces the sendmail tokens with their value. The values
 * were already computed in the XML document, so all we have to do is query
 * the XML and return the corresponding value.
 *
 * The supported tokens are:
 *
 * \li [sendmail::unsubscribe-link]
 *
 * \param[in] f  The filter object.
 * \param[in] token  The token object, with the token name and optional parameters.
 */
void sendmail::on_replace_token(filter::filter *f, QDomDocument& xml, filter::filter::token_info_t& token)
{
    if(token.f_name.mid(0, 10) == "sendmail::")
    {
        if(token.is_token("sendmail::unsubscribe-link"))
        {
            token.f_replacement = "http://snapwebsites.org/";
            token.f_found = true;
        }
        else
        {
            QString xpath;
            if(token.is_token("sendmail::from"))
            {
                xpath = "/snap/page/body/sendmail/from";
            }
            else if(token.is_token("sendmail::to"))
            {
                xpath = "/snap/page/body/sendmail/to";
            }
            else if(token.is_token("sendmail::path"))
            {
                xpath = "/snap/page/body/sendmail/path";
            }
            else if(token.is_token("sendmail::key"))
            {
                xpath = "/snap/page/body/sendmail/key";
            }
            else if(token.is_token("sendmail::created"))
            {
                xpath = "/snap/page/body/sendmail/created";
            }
            else if(token.is_token("sendmail::date"))
            {
                xpath = "/snap/page/body/sendmail/date";
            }
            else if(token.is_token("sendmail::time"))
            {
                xpath = "/snap/page/body/sendmail/time";
            }
            else if(token.is_token("sendmail::attachment_count"))
            {
                xpath = "/snap/page/body/sendmail/attachment_count";
            }
            else if(token.is_token("sendmail::priority"))
            {
                xpath = "/snap/page/body/sendmail/x-priority";
            }
            else if(token.is_token("sendmail::parameter"))
            {
                if(token.verify_args(1, 1))
                {
                    filter::filter::parameter_t param(token.get_arg("name", 0, filter::filter::TOK_STRING));
                    if(!token.f_error)
                    {
                        xpath = "/snap/page/body/sendmail/parameters/param[@name=\"" + param.f_value + "\"]/@value";
                    }
                }
            }
            if(!xpath.isEmpty())
            {
                QDomXPath dom_xpath;
                dom_xpath.setXPath(xpath);
                QDomXPath::node_vector_t result(dom_xpath.apply(xml));
                if(result.size() > 0)
                {
                    // apply the replacement
                    if(result[0].isElement())
                    {
                        // get the value between the tags
                        QDomDocument document;
                        QDomNode copy(document.importNode(result[0], true));
                        document.appendChild(copy);
                        token.f_replacement = document.toString();
                    }
                    else if(result[0].isAttr())
                    {
                        // get an attribute
                        token.f_replacement = result[0].toAttr().value();
                    }
                }
            }
        }
    }
}


SNAP_PLUGIN_END()

// There is an example of SMTP
// Actually we're under Linux and want to use sendmail instead (much easier!)
// http://stackoverflow.com/questions/9317305/sending-an-email-from-a-c-c-program-in-linux
//
// http://curl.haxx.se/libcurl/c/smtp-tls.html
// telnet mail.m2osw.com 25
// Trying 69.55.233.23...
// Connected to mail.m2osw.com.
// Escape character is '^]'.
// 220 mail.m2osw.com ESMTP Postfix (Made to Order Software Corporation)
// HELO mail.m2osw.com
// 250 mail.m2osw.com
// MAIL FROM: <alexis@m2osw.com>
// 250 2.1.0 Ok
// RCPT TO: <alexis_wilke@yahoo.com>
// 250 2.1.5 Ok
// DATA
// 354 End data with <CR><LF>.<CR><LF>
// From: <alexis@m2osw.com>
// To: <alexis_wilke@yahoo.com>
// Subject: Hello!
// 
// Testing SMTP really quick. We need to understand how to get the necessary info so it works.
// 
// .
// 250 2.0.0 Ok: queued as 9652742A0FC
// QUIT
// 221 2.0.0 Bye
// Connection closed by foreign host.


// vim: ts=4 sw=4 et
