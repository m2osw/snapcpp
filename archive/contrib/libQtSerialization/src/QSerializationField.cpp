/*
 * Text:
 *      src/QSerializationField.cpp
 *
 * Description:
 *      Handling of the field base class.
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

#include "QtSerialization/QSerializationField.h"
#include "QtSerialization/QSerializationComposite.h"

namespace QtSerialization
{


/** \class QField
 * \brief The base class for fields to be read from serialized data.
 *
 * This class allows you to read data from serialized data streams. The
 * base field class cannot be instantiated because of a pure virtual
 * function.
 *
 * Instead, you want to use one of the following classes:
 *
 * \li QFieldBasicType < T > -- for any basic C++ type
 * \li QFieldString -- for a QString field
 * \li QFieldTag -- for a user defined field, including arrays and
 * sub-composites
 *
 * \sa QFieldBasicType < T >
 * \sa QFieldString
 * \sa QFieldTag
 */


/** \brief Initializes the base field.
 *
 * This function calls the composite addField() function to register this field
 * in the composite.
 *
 * The name must be unique (not already exist) in the composite.
 *
 * It is preferable to have a name limited to ASCII characters, although any
 * valid UTF-8 character is acceptable.
 *
 * \param[in] composite  The composite where this field will be added.
 * \param[in] name  The name of this field.
 */
QField::QField(QComposite& composite, const QString& name)
{
    composite.addField(name, *this);
}


/** \brief Destructor.
 *
 * The virtual destructor of the QField class.
 */
QField::~QField()
{
}


/** \fn void QField::read(QReader& r);
 * \brief Read the field from the stream.
 *
 * This function is a pure virtual function in the QField class. It has to be
 * implemented in the derived classes such as QFieldString.
 *
 * The function is expected to read the field data and save it in the
 * corresponding user's class fields.
 *
 * \param[in] r  The reader used to read the field data.
 */


} // namespace QtSerialization
// vim: ts=4 sw=4 et
