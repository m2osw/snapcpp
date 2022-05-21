/*
 * Header:
 *      include/QtSerialization/QSerializationComposite.h
 *
 * Description:
 *      Implementation of the composite class. The composite class is a
 *      set of named fields used to read the data from a serialized stream.
 *      Fields named in the composite can be loaded, others are ignored.
 *
 * Documentation:
 *      See the corresponding .cpp file.
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
#ifndef QT_SERIALIZATION_COMPOSITE_H
#define QT_SERIALIZATION_COMPOSITE_H

#include <QMap>
#include <QString>

namespace QtSerialization
{

class QReader;
class QField;

class QComposite
{
public:
    void addField(const QString& name, QField& field);

    void readField(QReader& s, const QString& name);

private:
    typedef QMap<QString, QField *> fields_map_t;

    fields_map_t    f_fields = fields_map_t();
};


} // namespace QtSerialization
#endif
// #ifndef QT_SERIALIZATION_COMPOSITE_H
// vim: ts=4 sw=4 et
