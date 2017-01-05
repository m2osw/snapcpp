/*
 * Header:
 *      include/QtSerialization/QSerializationFieldTag.h
 *
 * Description:
 *      Implementation of the field tag, which is here to allow you to
 *      dynamically handle the data of complex fields through a function
 *      you define in your class.
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
#ifndef QT_SERIALIZATION_FIELD_TAG_H
#define QT_SERIALIZATION_FIELD_TAG_H

#include "QSerializationField.h"

namespace QtSerialization
{


class QSerializationObject
{
public:
    virtual ~QSerializationObject() {}

    virtual void readTag(const QString& name, QReader& r) = 0;
};


class QFieldTag : public QField
{
public:
    QFieldTag(QComposite& composite, const QString& name, QSerializationObject *obj);

    virtual void read(QReader& r);

private:
    QString                 f_name;
    QSerializationObject *  f_obj;
};


} // namespace QtSerialization
#endif
// #ifndef QT_SERIALIZATION_FIELD_TAG_H
// vim: ts=4 sw=4 et
