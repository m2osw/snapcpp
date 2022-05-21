/*
 * Text:
 *      src/QSerializationFieldString.cpp
 *
 * Description:
 *      Handling of QString's loading with QReader.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
 *
 *      https://snapwebsites.org/
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
#include "QtSerialization/QSerializationFieldString.h"
#include "QtSerialization/QSerializationExceptions.h"
#include "QtSerialization/QSerializationReader.h"
#include <QDebug>

namespace QtSerialization
{

/** \class QFieldString
 * \brief Handle the reading of a string based field.
 *
 * This class is used to read a string from the reader and save it in
 * the specified field. It makes use of a direct reference to a
 * QString.
 *
 * Remember that field names have to be unique. To create an
 * array of fields with the same name, you have to use the
 * QFieldTag instead.
 */


/** \brief Initializes the String field capability.
 *
 * This constructor initializes a field so a string can be read from
 * a serialized buffer.
 *
 * The \p field is expected to remain valid for the life time of the
 * QFieldString object.
 *
 * \exception QExceptionNullReference
 * The reference to the QString (\p field) cannot represent a NULL pointer.
 *
 * \param[in] composite  The composite that will hold this field.
 * \param[in] name  The name of the field in the serialized data.
 * \param[in,out] field  A reference to the string that will be loaded from
 *                       the serialized data.
 */
QFieldString::QFieldString(QComposite& composite, const QString& name, QString& field)
    : QField(composite, name),
      f_field(field)  // keep a reference
{
    if(&f_field == NULL) {
        throw QExceptionNullReference("a QString reference cannot be a NULL pointer");
    }
}


/** \brief Read the string from the input reader.
 *
 * This function reads the string from the \p r reader and saves
 * the string in the user supplied string (as defined in the \p field
 * reference of the constructor.)
 *
 * \param[in] r  The reader the string is expected to be read from.
 */
void QFieldString::read(QReader& r)
{
    r.readText();
    f_field = r.text();
    // also read the closing tag so it's equivalent to the array/composite fields
    r.readTag();
}


} // namespace QtSerialization
// vim: ts=4 sw=4 et
