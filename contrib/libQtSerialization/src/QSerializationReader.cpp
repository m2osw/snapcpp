/*
 * Header:
 *      src/QSerializationReader.cpp
 *
 * Description:
 *      Read serialized data from a stream and save it in fields and
 *      sub-objects as defined by QComposite objects.
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

#include "QtSerialization/QSerializationReader.h"
#include "QtSerialization/QSerializationComposite.h"
#include "QtSerialization/QSerializationExceptions.h"

namespace QtSerialization
{

/** \class QReader
 * \brief This class is used to read data from a serialized buffer.
 *
 * This class accepts an input stream (read-only or read/write) and
 * converts it to data that fields can parse back the way it was when
 * saving this data.
 *
 * The reader can parse older and newer as well as current data. It
 * knows how to skip data that isn't known by the current implementation
 * making it a perfect choice for any data changes a lot between versions
 * and that needs to be serialized.
 *
 * As a user you are expected to call the read() function. Once the header
 * was read, you can then use the different version functions to check
 * what version the file is defined as.
 *
 * If an error occurs while reading, then the class throws an exception.
 * Including if a read from the input stream fails. If the read() function
 * returns as it then the read was 100% successful.
 */


/** \fn QReader::~QReader();
 * \brief The destructor ensures everything is cleaned up as expected.
 *
 * At this time the destructor of the reader does nothing. It is there
 * to make sure that virtual functions work as expected.
 */


/** \fn int QReader::tag() const;
 * \brief Retrieve the name of the last tag loaded.
 *
 * This function returns the name (1 character) of the last tag loaded.
 * The library only supports tags of 1 character (a-z). At this point
 * we support the \<r> tag as the root tag, and the \<v> tag inside,
 * defining all the serialized data.
 *
 * \return The name of the tag last read (r or v).
 */


/** \fn bool QReader::closingTag() const;
 * \brief Check whether the last tag was a closing tag.
 *
 * When a tag is read, and the name starts with a slash (/) then this
 * flag is set to true. This means \</v> or \</r> was found.
 *
 * If text is read, then this flag always returns false.
 *
 * \return true if the last tag read started with a slash (/).
 */


/** \fn QString QReader::text() const;
 * \brief Retrieve the text last loaded.
 *
 * This function is an overload on the attribute() function with the
 * 'n' (name) attribute. This is a convenience to make it clear that
 * we wanted to load the text last read with readText() rather than
 * checking out the name attribute.
 *
 * The text will be empty if there was no text or readTag() was last
 * called.
 *
 * \return The last text read.
 */


/** \fn void QReader::setClosingTag(bool closing = true);
 * \brief Mark the tag as a closing tag.
 *
 * This function is called internally when reading the tag and it is
 * discovered to be a closing tag.
 *
 * \param[in] closing  Whether the tag is a closing tag (true) or an opening tag (false).
 */


/** \fn void QReader::append(int c);
 * \brief Append one character to the text buffer.
 *
 * This function is an overload. It adds one character at the end of the
 * 'n' attribute which is considered to be the text buffer when
 * readText() is called.
 *
 * \param[in] c  The character to append to the text.
 */


/** \fn void QReader::setText(const QString& text);
 * \brief Set the text to the \p text parameter.
 *
 * This function is an overload. It sets the text of the QReader to
 * the specified \p text string. It really sets the 'n' attribute
 * which is considered to be the text buffer when readText() is
 * called.
 *
 * \param[in] text  The new text for QReader.
 */


/** \brief The stream to read the data from.
 *
 * This constructor initializes the reader. The reader object maintains
 * the stream and its buffers. It is capable of reading the next tag
 * or text block.
 *
 * The file format and major/minor version of the streams being read are
 * saved in this object. Until the file is read, these values are set to
 * -1. The name of the file is an empty string ("") until read from the
 * input file.
 *
 * One reader can be used to read one stream. To read another stream,
 * create a new reader.
 *
 * To start reading the data in the stream, call the read() function.
 *
 * \param[in] stream  The stream to read from.
 *
 * \sa read()
 * \sa file_format_version()
 * \sa major_version()
 * \sa minor_version()
 */
QReader::QReader(QIODevice& stream)
    : f_initialized(false),
      //f_name() -- auto-init
      f_file_format_version(-1),
      f_major_version(-1),
      f_minor_version(-1),
      f_stream(&stream),
      //f_buffer[] -- considered empty since f_pos >= f_buf_size
      f_pos(0),
      f_buf_size(0),
      f_unget('\0'),
      f_tag('\0'),
      f_closing(false)
      //f_attr_f -- auto-init (empty)
      //f_attr_v -- auto-init (empty)
      //f_attr_m -- auto-init (empty)
      //f_attr_n -- auto-init (empty)
{
}


/** \brief Retrieve the file format as it was saved in the stream.
 *
 * This function returns the file format ('f' attribute of the \<r> tag)
 * that was used when creating the serialized data. This represents the
 * FILE_FORMAT_VERSION variable of the library used to save the serialized
 * data. It may be different from the current library version or the version
 * of the library you used to compile your software.
 *
 * \return The file format version as it was saved in your serialized data.
 */
quint16 QReader::file_format_version() const
{
    if(f_file_format_version == -1)
    {
        throw QExceptionNotDefined("file format version is not defined");
    }
    return static_cast<quint16>(f_file_format_version);
}


/** \brief Retrieve the major file version.
 *
 * This function returns the 'v' parameter of the \<r> tag. This is the
 * version you used to call the QWriter constructor. You may use that
 * information to tweak your data forward as required.
 *
 * \return The major version as it was saved in your serialized data.
 */
quint16 QReader::major_version() const
{
    if(f_major_version == -1)
    {
        throw QExceptionNotDefined("major version is not defined");
    }
    return f_major_version;
}


/** \brief Retrieve the minor file version.
 *
 * This function returns the 'm' parameter of the \<r> tag. This is the
 * version you used to call the QWriter constructor. You may use that
 * information to tweak your data forward as required.
 *
 * \return The minor version as it was saved in your serialized data.
 */
quint16 QReader::minor_version() const
{
    if(f_minor_version == -1)
    {
        throw QExceptionNotDefined("minor version is not defined");
    }
    return f_minor_version;
}


/** \brief Read this component and any sub-component.
 *
 * This function reads all the tags of a serialization file and saves the
 * data in the structures as specified in the composite class.
 *
 * \param[in] composite  The declaration of your class fields and sub-components.
 */
void QReader::read(QComposite& composite)
{
    if(!f_initialized) {
        // if not initialized we expect to find the <r>...</r> tag
        f_initialized = true;
        readTag();
        if(f_tag != 'r' || f_closing) {
            throw QExceptionInvalidTag("serialization only supports <r> XML files");
        }

        // attributes in the r tag has information about the serialization in general
        f_name = attribute('n');
        f_file_format_version = attribute('f').toUShort();
        f_major_version = attribute('v').toUShort();
        f_minor_version = attribute('m').toUShort();

        // now read all the <v> tags
        read(composite);

        // end file with </r>
        if(f_tag != 'r') {
            throw QExceptionInvalidTag("serialization last closing tag was expected to be </r>");
        }

        // here we could also check whether this is the end of the file...
        // but the truth is that it is not required to know such!
    }
    else {
        // parse all the tags at this level until we find a closing tag (</v>)
        for(readTag(); !f_closing; readTag()) {
            composite.readField(*this, attribute('n'));
        }
    }
}


/** \brief Read the next chunk of data.
 *
 * This function reads the next tag. The result is saved in the
 * reader buffers:
 *
 * \li Opening or Closing tag
 * \li Tag name (r or v at this time)
 * \li Tag attributes (f, v, m, and n)
 *
 * Note that the tag and attribute names are just one letter.
 *
 * When the end of the file is found, the buffer is reset and
 * it will have the tag named '\0'. However, if the input stream
 * is valid, this function should not find the end of file since
 * the last tag will be closed first.
 *
 * \exception QExceptionInvalidRead
 * If the input does not represent a valid tag (valid for this
 * implementation of QSerializationField) then this exception
 * is raised.
 *
 * \sa readText()
 * \sa tag()
 * \sa closingTag()
 * \sa attribute()
 */
void QReader::readTag()
{
    QByteArray input;

    reset();

    // read one byte
    int c = get();
    if(c == STREAM_EOF) {
        // End of data reached
        return;
    }
    if(c != '<') {
        invalidRead("a tag was expected");
        /*NOTREACHED*/
    }
    // get the tag name
    c = get();
    if(c == '/') {
        setClosingTag();
        c = get();
    }
    switch(c) {
    case 'r':
    case 'v':
        setTag(c);
        break;

    default:
        invalidRead("a tag was expected");
        /*NOTREACHED*/

    }
    c = get();
    if(c == ' ') {
        // we have attributes
        for(;;) {
            do {
                c = get();
            } while(c == ' ');
            if(c == '>') {
                // done
                return;
            }
            switch(c) {
            case STREAM_EOF:
                invalidRead("unexpected end of input while reading a tag.");
                /*NOTREACHED*/

            case '/':
                invalidRead("empty tags are not currently supported.");
                /*NOTREACHED*/

            default:
                if(c >= 'a' && c <= 'z') {
                    // we accept all of those, skipping unknown names silently
                    break;
                }
                invalidRead("unexpected character for an attribute name.");
                /*NOTREACHED*/

            }
            int attr(c);
            c = get();
            if(c != '=') {
                invalidRead("all attributes must be followed by a value.");
                /*NOTREACHED*/
            }
            c = get();
            if(c != '"') {
                invalidRead("all attributes must be defined between double quotes.");
                /*NOTREACHED*/
            }
            c = get();
            while(c != '"' && c != STREAM_EOF) {
                // <, >, and ' are forbidden in attributes (must be &...; instead)
                if(c == '<' || c == '>' || c == '\'') {
                    invalidRead("unexpected character found in an attribute");
                    /*NOTREACHED*/
                }
                appendAttributeChar(attr, c);
                c = get();
            }
            if(c == STREAM_EOF) {
                invalidRead("unexpected end of an attribute and thus of a tag");
                /*NOTREACHED*/
            }
            setAttribute(attr, xmlDecode(attribute(attr)));
        }
    }
    else if(c != '>') {
        invalidRead("a tag definition must end with >");
        /*NOTREACHED*/
    }
}


/** \brief Read text data inside a tag.
 *
 * This function is used to read the data between the opening
 * and closing of a tag that does not include sub-tags (which
 * is pretty much all the fields except the composite field.)
 *
 * The result is saved in the name attribute. It can be retrieved
 * using the text() function. It will have been XML decoded
 * already.
 *
 * \sa readTag()
 * \sa text()
 */
void QReader::readText()
{
    int c;
    reset();
    for(c = get(); c != '<' && c != STREAM_EOF; c = get()) {
        append(c);
    }
    unget(c);
    setText(xmlDecode(text()));
}


/** \brief Decode a string read from the XML file.
 *
 * This function decodes the XML characters that were encoded when
 * creating a serialized file. Note that only the characters encoded
 * with the xmlEncode() function are currently accepted. Others generate
 * an error by calling invalidRead().
 *
 * \todo
 * A faster implementation would probably use indexOf() and string
 * comparison of sub-strings.
 *
 * \param[in] string  The XML string to encode.
 *
 * \return A copy of the input string with its encoded characters decoded.
 *
 * \sa xmlEncode()
 * \sa invalidRead()
 */
QString QReader::xmlDecode(const QString& string)
{
    QString result;

    for(QString::const_iterator it(string.begin()); it != string.end(); ++it)
    {
        if(it->unicode() == '&') {
            ++it;
            QChar buf[8];
            bool valid(false);
            size_t p(0);
            for(; p < sizeof(buf) / sizeof(buf[0]) && it != string.end(); ++it, ++p) {
                buf[p] = it->unicode();
                if(buf[p] == ';') {
                    valid = true;
                    break;
                }
            }
            if(!valid) {
                invalidRead("invalid entity found in input buffer");
            }
            if(p == 3 && buf[0] == 'a' && buf[1] == 'm' && buf[2] == 'p') {
                result += '&';
            }
            else if(p == 2 && buf[0] == 'l' && buf[1] == 't') {
                result += '<';
            }
            else if(p == 2 && buf[0] == 'g' && buf[1] == 't') {
                result += '>';
            }
            else if(p == 4 && buf[0] == 'q' && buf[1] == 'u' && buf[2] == 'o' && buf[3] == 't') {
                result += '"';
            }
            else if(p == 4 && buf[0] == 'a' && buf[1] == 'p' && buf[2] == 'o' && buf[3] == 's') {
                result += '\'';
            }
            else {
                invalidRead("unknown entity found in input buffer");
            }
        }
        else {
            result += *it;
        }
    }

    return result;
}


/** \brief Throw the QExceptionInvalidRead exception.
 *
 * This function is used by the different read functions to generate
 * an error when the input is not compatible with this implementation
 * of QtSerialization.
 *
 * The function raises an exception so it never returns.
 *
 * \exception QExceptionInvalidRead
 * This function raises the invalid read exception with the specified
 * message.
 *
 * \param[in] errmsg  The what string for the exception.
 */
void QReader::invalidRead(const char *errmsg)
{
    throw QExceptionInvalidRead(errmsg);
}


/** \brief Reset the buffer.
 *
 * This function resets the buffer so one can read the next
 * tag or text with a clean slate.
 *
 * This function does not reset the buffered data from the
 * stream or the unget buffer.
 */
void QReader::reset()
{
    f_tag = '\0';
    f_closing = false;
    f_attr_f.clear();
    f_attr_v.clear();
    f_attr_m.clear();
    f_attr_n.clear();
}


/** \brief Read the next character from the input stream.
 *
 * This function reads the next character and returns it value. Note that
 * it really reads 1 byte ignoring the UTF-8 encoding because we actually
 * do not care much about characters other than the ASCII characters
 * (all others are taken as graphics characters whatever they are.)
 *
 * Once the end of the input file is reached, then this function returns
 * STREAM_EOF.
 *
 * \return The following character or STREAM_EOF (-1).
 */
int QReader::get()
{
    if(f_unget != '\0') {
        int c(f_unget);
        f_unget = '\0';
        return c;
    }

    if(f_pos >= f_buf_size) {
        // TBD: some stream may return an error when reading past the end of
        //      the file; if we encounter such, we may want to use a flag to
        //      know once we reached the EOF and always return that,
        //      automatically (we may want to do that anyway to avoid this
        //      IO Device call!)
        f_buf_size = f_stream->read(f_buffer, sizeof(f_buffer));
        if(f_buf_size == -1) {
            throw QExceptionIOError("an I/O error occured while reading the input stream");
        }
        f_pos = 0;
        if(f_buf_size == 0) {
            // we reached the end of the file
            return STREAM_EOF;
        }
    }

    return f_buffer[f_pos++];
}


/** \brief Unget one character.
 *
 * This function is used to unget one character. Not more than one
 * character can be unget at a time.
 *
 * If the input character is EOF, then the function returns immediately
 * and no character is unget.
 *
 * At this point, this is used in the readText() function to unget the
 * '<' character.
 *
 * \exception QExceptionBufferIsFull
 * This exception is raised if the unget buffer is already in use
 * (i.e. you can call this function only once before calling get()
 * again.)
 *
 * \exception QExceptionNotSupported
 * This exception is raised whenever the character being unget is
 * not a value between 0 and 255 (i.e. a byte.) This is really not
 * a character per se since the input buffer is expected to be
 * UTF-8.
 *
 * \param[in] c  The character to unget.
 */
void QReader::unget(int c)
{
    if(c == STREAM_EOF) {
        return;
    }

    if(c < 0 || c > 255) {
        throw QExceptionNotSupported("character out of range");
    }
    if(f_unget != '\0') {
        throw QExceptionBufferIsFull("unget buffer is full");
    }

    f_unget = static_cast<char>(c);
}


/** \brief Save the tag name.
 *
 * This function saves the tag name. It limits the name to currently
 * understood tags. Trying to set any other tag name results in an
 * exception being raised.
 *
 * Unknown tag names are expected to be ignored (skipped) by the reader
 * and thus never appear here.
 *
 * \exception QExceptionNotSupported
 * The tag name (i.e. the \p c parameter) does not represent a valid
 * tag name for the serialization environment.
 *
 * \param[in] c  The character to unget.
 */
void QReader::setTag(int c)
{
    switch(c) {
    case 'r':
    case 'v':
        break;

    default:
        throw QExceptionNotSupported("unknown tag name");

    }

    f_tag = static_cast<char>(c);
}


/** \brief Retrieve the content of a tag attribute.
 *
 * This function returns the content of the specified attribute.
 * The attribute name is one letter. Only the following are supported
 * at this time:
 *
 * \li 'f' -- the file version (FILE_FORMAT_VERSION)
 * \li 'v' -- the user defined file major version
 * \li 'm' -- the user defined file minor version
 * \li 'n' -- the name of this file or field
 *
 * Other attribute names are all considered empty and an empty attribute
 * is considered undefined.
 *
 * The attribute names are currently limited to 1 character.
 *
 * Attribute values have already been run through the xmlDecode() function
 * and this they do not include any entities (although if the original
 * included an entity, it will still be there.)
 *
 * \param[in] attr  The name of the attribute.
 */
QString QReader::attribute(int attr) const
{
    switch(attr) {
    case 'f':
        return QString::fromUtf8(f_attr_f.data());

    case 'v':
        return QString::fromUtf8(f_attr_v.data());

    case 'm':
        return QString::fromUtf8(f_attr_m.data());

    case 'n':
        return QString::fromUtf8(f_attr_n.data());

    // catch invalid names?
    }

    // all others are considered empty
    return QString();
}


/** \brief Set the value of an attribute.
 *
 * This function is used to set the value of an attribute
 * at once. The string is converted to the internal array
 * buffer.
 *
 * Note that if the name of an unknown attribute is used,
 * then the function generates an exception.
 *
 * The value of attributes is expected to represent the
 * raw value of the attribute. This means XML entities are
 * not encoded.
 *
 * The currently understood attributes are:
 *
 * \li 'f' -- the file version (FILE_FORMAT_VERSION)
 * \li 'v' -- the user defined file major version
 * \li 'm' -- the user defined file minor version
 * \li 'n' -- the name of this file or field
 *
 * \param[in] attr  The attribute to be set.
 * \param[in] value  The new value for this attribute.
 */
void QReader::setAttribute(int attr, const QString& value)
{
    switch(attr) {
    case 'f':
        f_attr_f = value.toUtf8();
        break;

    case 'v':
        f_attr_v = value.toUtf8();
        break;

    case 'm':
        f_attr_m = value.toUtf8();
        break;

    case 'n':
        f_attr_n = value.toUtf8();
        break;

    default:
        throw QExceptionNotSupported("unknown attribute");

    }
}


/** \brief Append a character to an attribute buffer.
 *
 * This function is used to append one more character to the
 * specified attribute. The input data is expected to be UTF-8
 * and thus each call may not represent a full character (i.e.
 * the parameter \p c is not expected to represent a UCS-4
 * character, instead, it is viewed as a byte.)
 */
void QReader::appendAttributeChar(int attr, int c)
{
    switch(attr) {
    case 'f':
        f_attr_f.append(c);
        break;

    case 'v':
        f_attr_v.append(c);
        break;

    case 'm':
        f_attr_m.append(c);
        break;

    case 'n':
        f_attr_n.append(c);
        break;

    default:
        throw QExceptionNotSupported("unknown attribute");

    }
}


} // namespace QtSerialization
// vim: ts=4 sw=4 et
