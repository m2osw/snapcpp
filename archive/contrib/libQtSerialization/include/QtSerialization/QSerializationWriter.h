/*
 * Header:
 *      include/QtSerialization/QSerializationWriter.h
 *
 * Description:
 *      Handling of serialization in a forward and backward manner. This module
 *      is used to save data to a serialization stream.
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
#ifndef QT_SERIALIZATION_WRITER_H
#define QT_SERIALIZATION_WRITER_H

#include <QIODevice>
#include <QPointer>
#include <QString>

namespace QtSerialization
{

class QWriter
{
public:
    class QTag
    {
    public:
        QTag(QWriter& w, const QString& name);
        virtual ~QTag();

    private:
        QWriter&    f_writer;
    };

    static const int ENCODE_QUOTE = 0x00000001;
    static const int ENCODE_DOUBLE_QUOTE = 0x00000002;

    QWriter(QIODevice& stream, const QString& name, quint16 major_version, quint16 minor_version);
    virtual ~QWriter();

    void writeStartTag(const QString& name);
    void writeEndTag();
    void writeTag(const QString& name, const QString& data);

    static QString xmlEncode(const QString& string, const int encode = 0);

private:
    void writeData(const QString& name);

    bool                f_initialized;
    QString             f_name;
    qint32              f_major_version;
    qint32              f_minor_version;
    QPointer<QIODevice> f_stream;
};


void writeTag(QWriter& w, const QString& name, const bool data);
void writeTag(QWriter& w, const QString& name, const qint8 data);
void writeTag(QWriter& w, const QString& name, const quint8 data);
void writeTag(QWriter& w, const QString& name, const qint16 data);
void writeTag(QWriter& w, const QString& name, const quint16 data);
void writeTag(QWriter& w, const QString& name, const qint32 data);
void writeTag(QWriter& w, const QString& name, const quint32 data);
void writeTag(QWriter& w, const QString& name, const qint64 data);
void writeTag(QWriter& w, const QString& name, const quint64 data);
void writeTag(QWriter& w, const QString& name, const float data);
void writeTag(QWriter& w, const QString& name, const double data);
void writeTag(QWriter& w, const QString& name, const QString& data);
void writeTag(QWriter& w, const QString& name, const char *data);


} // namespace QtSerialization
#endif
//#ifndef QT_SERIALIZATION_WRITER_H
// vim: ts=4 sw=4 et
