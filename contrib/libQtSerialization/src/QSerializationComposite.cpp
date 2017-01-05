/*
 * Text:
 *      src/QSerializationComposite.cpp
 *
 * Description:
 *      Handling of the composite field class.
 *
 * Documentation:
 *      See each function below.
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

#include "QtSerialization/QSerializationComposite.h"
#include "QtSerialization/QSerializationField.h"
#include "QtSerialization/QSerializationExceptions.h"
#include <QDebug>

namespace QtSerialization
{

/** \class QComposite
 * \brief Initialize the composite type.
 *
 * A composite type is a set of fields that are to be read
 * from serialized data. It is used as the parent of the
 * fields defined in that composite.
 */


/** \brief Add a field to this composite.
 *
 * Call this function whenever you create a field in a
 * composite.
 *
 * Fields are added to a map which has the side effect of
 * sorting the fields in "alphabetical" order (the order
 * is really using the binary code of each character and
 * the locale is totally ignored.)
 *
 * Note that the composite keeps a reference to your field
 * so you may change it with time, however, you should
 * not delete the field before the composite it was added
 * into.
 *
 * \exception QExceptionAlreadyDefined
 * This exception is raised when you call this function
 * twice with the same name. Two fields cannot make use of the
 * same name. Arrays make use the QFieldTag which runs one of
 * your functions over and over again so you can create as many
 * children as required.
 *
 * \param[in] name  The name of this field as it appears in the serialized file.
 * \param[in] field  The field to add to the composite.
 *
 * \sa readField()
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
void QComposite::addField(const QString& name, QField& field)
{
    // we prevent users from adding more than one field
    // with a given name
    if(f_fields.contains(name)) {
        throw QExceptionAlreadyDefined("two fields inside one composite cannot be named the same.");
    }
    f_fields[name] = &field;
}
#pragma GCC diagnostic pop


/** \brief Read the named field of this composite.
 *
 * This function parses the XML data and reads the named field.
 * When the function returns all the data is saved in your class variable
 * members.
 *
 * This function is smart enough to know how to skip the field if it
 * is not defined in this software. This means you can add and remove
 * fields from your classes as required by your application without
 * using the capability to read your serialized data from older and
 * newer versions.
 *
 * \param[in,out] r  The reader the data is read from.
 * \param[in] name  The name of the field being read.
 *
 * \sa addField()
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
void QComposite::readField(QReader& r, const QString& name)
{
    if(f_fields.contains(name)) {
        f_fields[name]->read(r);
    }
    else {
        // TODO: skip unknown field
printf("Could not find field named \"%s\".\n", name.toUtf8().data());
        throw QExceptionNotSupported("reading of unknown fields is not yet supported");
    }
}
#pragma GCC diagnostic pop


} // namespace QtSerialization
// vim: ts=4 sw=4 et
