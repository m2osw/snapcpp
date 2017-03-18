/*
 * Header:
 *      src/QSerializationWriter.cpp
 *
 * Description:
 *      Write data to a stream in a serialized manner that can be later read
 *      back to your variables using a QReader object. The QWriter is pretty
 *      much a standalone class.
 *
 * Documentation:
 *      See the corresponding .cpp file.
 *
 * License:
 *      Copyright (c) 2012-2017 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "QtSerialization/QSerializationWriter.h"
#include "QtSerialization/QSerializationExceptions.h"
#include "QtSerialization/QSerialization.h"

namespace QtSerialization
{


/** \class QWriter
 * \brief Ease the creation of serialized data.
 *
 * This class let you create serialized data in a manner that allows the
 * QReader to read it back in memory for further usage.
 *
 * The writer is pretty much a stand alone class that can be used to
 * write fields to the output stream. The writeTag() functions can be
 * used to write any amount of data.
 *
 * The programmer must make sure that the field names used to create
 * the output are the same as those used to read the data later with
 * a QReader object.
 *
 * It is possible to save arrays and other complex composite objects.
 */


/** \class QWriter::QTag
 * \brief A helper class to not forget to close \<v> tags.
 *
 * This class is used whenever you create a write() function that will
 * serialize your data in a specific \<v> tag. The class, when destroyed,
 * ensures that the tag gets closed.
 *
 * The class takes the QWriter and the field name. The tag is written as
 * \<v n="\<name>"> and on destruction, the class closes the tag as
 * \</v>.
 */


/** \brief Initializes the writer for serialization.
 *
 * This function creates a writer that can be used to serialize a set of
 * classes and sub-classes.
 *
 * All the information defined on the constructor cannot be changed later.
 * To save another set of classes and sub-classes, create a new QWriter.
 * It is actually expected that QWriter's will be created on the stack.
 *
 * \param[in] stream  The stream where the serialized data is written.
 * \param[in] name  The name of this file.
 * \param[in] major_version  The major version of the file.
 * \param[in] minor_version  The minor version of the file.
 */
QWriter::QWriter(QIODevice& stream, const QString& name, quint16 major_version, quint16 minor_version)
    : f_initialized(false),
      f_name(name),
      f_major_version(major_version),
      f_minor_version(minor_version),
      f_stream(&stream)
{
}


/** \brief Clean up the writer object.
 *
 * This function cleans up the writer object after it closed the \<r>
 * tag (assuming that it was used.)
 *
 * It is VERY important to destroy the QWriter before you destroy your
 * QIODevice (output stream). In most cases, since you need to create
 * the output stream before the QWriter, it is unlikely that would
 * happen the other way around unless you are using allocated objects.
 */
QWriter::~QWriter()
{
    if(f_initialized) {
        // we have to avoid exceptions in the destructor
        try {
            writeData("</r>");
        }
        catch(QExceptionNullReference const &) {
        }
        catch(QExceptionIOError const &) {
        }
    }
}


/** \brief Open a \<v> tag.
 *
 * This function writes a \<v> tag along with its name.
 *
 * Note that the name is XML encoded before being saved so it remains
 * compatible in any XML document. The quotes are also encoded since
 * the name is saved as an attribute.
 *
 * \param[in] name  Name of this field saved in the \<v> tag.
 */
void QWriter::writeStartTag(const QString& name)
{
    if(!f_initialized) {
        // if it is the very first time we want an <r> tag here
        f_initialized = true;
        writeData(QString("<r f=\"%1\" v=\"%2\" m=\"%3\" n=\"%4\">")
            .arg(FILE_FORMAT_VERSION)
            .arg(f_major_version)
            .arg(f_minor_version)
            .arg(xmlEncode(f_name, ENCODE_DOUBLE_QUOTE | ENCODE_QUOTE))
        );
    }
    writeData("<v n=\"" + xmlEncode(name, ENCODE_DOUBLE_QUOTE | ENCODE_QUOTE) + "\">");
}


/** \brief Close a \<v> tag.
 *
 * This function writes a \</v> closing tag.
 */
void QWriter::writeEndTag()
{
    writeData("</v>");
}


/** \brief Write a complete tag.
 *
 * This function is used whenever a full tag can be written to the output
 * stream at once. This is ideal for all the basic C++ types.
 *
 * The data and name will be XML encoded as required.
 *
 * In order to serialize all kinds of different basic C++ types, helper
 * functions named writeTag() are offered. These are used as:
 *
 * \code
 * writeTag(w, "field-name", this->field_name);
 * \endcode
 *
 * Where w is the writer. Those helper functions know how to convert the
 * field data to a string and then it calls this writeTag() function member
 * to save the result in the output stream. For example, integers are
 * converted using the following:
 *
 * \code
 * w.writeTag(name, QString("%1").arg(data));
 * \endcode
 *
 * \param[in] name  The name of the field to save in the \<v> tag.
 * \param[in] data  The data saved inside this tag.
 *
 * \sa writeStartTag()
 * \sa writeEndTag()
 * \sa xmlEncode()
 */
void QWriter::writeTag(const QString& name, const QString& data)
{
    writeStartTag(name);
    writeData(xmlEncode(data));
    writeEndTag();
}


/** \brief Write data to the stream.
 *
 * This function writes data to the output stream and verifies that
 * the data was properly written. If not, the function raises an
 * exception.
 *
 * \exception QExceptionIOError
 * If the write to the stream does not write all the characters
 * specified in the input data then this exception is raised.
 *
 * \param[in] data  The data to write to the output stream.
 */
void QWriter::writeData(const QString& data)
{
    if(f_stream.isNull()) {
        throw QExceptionNullReference("the QWriter stream pointer is NULL");
    }

    QByteArray utf8(data.toUtf8());
    if(f_stream->write(utf8.data(), utf8.size()) != utf8.size()) {
        throw QExceptionIOError("I/O error--write failed");
    }
}


/** \brief Encode a string so it can be used in an XML document.
 *
 * This function encodes special XML characters using the
 * standard XML entities.
 *
 * The following describes what gets encoded:
 *
 * \li \& -- The ampersand character is always changed to \&amp;
 * \li \< -- The less than character is always changed to &lt;
 * \li \> -- The less than character is always changed to &gt;
 * \li " -- The double quote character is changed to &quot; when \p encode includes the ENCODE_DOUBLE_QUOTE flag
 * \li ' -- The single quote character is changed to &apos; when \p encode includes the ENCODE_QUOTE flag
 *
 * Other characters are kept verbatim.
 *
 * To transform a string that is to be used as the value of an attribute,
 * call this function with ENCODE_DOUBLE_QUOTE or ENCODE_QUOTE, or use
 * both to make sure that it always works.
 *
 * To transform a string to be used as text in a body (i.e. not within
 * the start tag) then the quote flags are not required.
 *
 * \par Supported Encode Flags
 *
 * \li ENCODE_QUOTE -- encode single quotes (') as \&amp;apos;
 * \li ENCODE_DOUBLE_QUOTE -- encode double quotes (") as \&amp;quot;
 *
 * \param[in] string  The XML string to encode.
 * \param[in] encode  The encode flags (see description.)
 *
 * \return A copy of the input string with its special characters encoded.
 *
 * \sa xmlDecode()
 */
QString QWriter::xmlEncode(const QString& string, const int encode)
{
    QString result;

    for(QString::const_iterator it(string.begin());
                    it != string.end();
                    ++it)
    {
        switch(it->unicode()) {
        case '&':
            result += "&amp;";
            break;

        case '<':
            result += "&lt;";
            break;

        case '>':
            result += "&gt;";
            break;

        case '"':
            if(encode & ENCODE_DOUBLE_QUOTE) {
                result += "&quot;";
            }
            else {
                result += '"';
            }
            break;

        case '\'':
            if(encode & ENCODE_QUOTE) {
                // This is not compatible with older IE browsers
                // You'd have to use &#x27; or &#39; instead
                result += "&apos;";
            }
            else {
                result += "'";
            }
            break;

        default:
            result += *it;
            break;

        }
    }

    return result;
}


/** \brief Initializes the QTag object.
 *
 * The constructor of the QTag class initializes the object by
 * saving a reference to the writer and saving the start tag
 * in the writer output stream.
 *
 * This behavior is called RAII.
 *
 * \param[in] writer  The writer defining the output stream.
 * \param[in] name  The name of the field being saved.
 */
QWriter::QTag::QTag(QWriter& writer, const QString& name)
    : f_writer(writer)
{
    f_writer.writeStartTag(name);
}


/** \brief Destroys a QTag object.
 *
 * This function destroys the QTag object by sending a closing
 * \</v> tag to the output stream. (RAII!)
 */
QWriter::QTag::~QTag()
{
    f_writer.writeEndTag();
}


/** \brief Help function used to serialize a bool field.
 *
 * This function is a helper function used to serialize a
 * field of type bool.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const bool data)
{
    w.writeTag(name, QString("%1").arg(data ? 1 : 0));
}


/** \brief Help function used to serialize a qint8 field.
 *
 * This function is a helper function used to serialize a
 * field of type qint8.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const qint8 data)
{
    w.writeTag(name, QString("%1").arg(data));
}


/** \brief Help function used to serialize a quint8 field.
 *
 * This function is a helper function used to serialize a
 * field of type quint8.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const quint8 data)
{
#pragma GCC push
#pragma GCC diagnostic ignored "-Wsign-promo"
    w.writeTag(name, QString("%1").arg(data));
#pragma GCC pop
}


/** \brief Help function used to serialize a qint16 field.
 *
 * This function is a helper function used to serialize a
 * field of type qint16.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const qint16 data)
{
    w.writeTag(name, QString("%1").arg(data));
}


/** \brief Help function used to serialize a quint16 field.
 *
 * This function is a helper function used to serialize a
 * field of type quint16.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const quint16 data)
{
    w.writeTag(name, QString("%1").arg(data));
}


/** \brief Help function used to serialize a qint32 field.
 *
 * This function is a helper function used to serialize a
 * field of type qint32.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const qint32 data)
{
    w.writeTag(name, QString("%1").arg(data));
}


/** \brief Help function used to serialize a quint32 field.
 *
 * This function is a helper function used to serialize a
 * field of type quint32.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const quint32 data)
{
    w.writeTag(name, QString("%1").arg(data));
}


/** \brief Help function used to serialize a qint64 field.
 *
 * This function is a helper function used to serialize a
 * field of type qint64.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const qint64 data)
{
    w.writeTag(name, QString("%1").arg(data));
}


/** \brief Help function used to serialize a quint64 field.
 *
 * This function is a helper function used to serialize a
 * field of type quint64.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const quint64 data)
{
    w.writeTag(name, QString("%1").arg(data));
}


/** \brief Help function used to serialize a float field.
 *
 * This function is a helper function used to serialize a
 * field of type float.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const float data)
{
    w.writeTag(name, QString("%1").arg(data));
}


/** \brief Help function used to serialize a double field.
 *
 * This function is a helper function used to serialize a
 * field of type double.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const double data)
{
    w.writeTag(name, QString("%1").arg(data));
}


/** \brief Help function used to serialize a string field.
 *
 * This function is a helper function used to serialize a
 * field of type string.
 *
 * Note that this is equivalent to w.writeTag(name, data).
 * It just makes sense to have this helper function so
 * all writes in a serialization look alike.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const QString& data)
{
    w.writeTag(name, data);
}


/** \brief Help function used to serialize a string field.
 *
 * This function is a helper function used to serialize a
 * field of type string.
 *
 * Note that this is equivalent to w.writeTag(name, data).
 * It just makes sense to have this helper function so
 * all writes in a serialization look alike.
 *
 * \param[in] w  The QWriter where the field is written.
 * \param[in] name  The name of the field saved in the 'n' attribute
 *                  of the \<v> tag.
 * \param[in] data  The field data.
 */
void writeTag(QWriter& w, const QString& name, const char *data)
{
    w.writeTag(name, QString::fromUtf8(data));
}


} // namespace QtSerialization
// vim: ts=4 sw=4 et
