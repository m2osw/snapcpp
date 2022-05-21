/*
 * Header:
 *      include/QtSerialization/QSerializationReader.h
 *
 * Description:
 *      Handling the reading of a serialized buffer in a forward and backward
 *      manner (i.e. the reader can skip unknown/unsupported tags.)
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
#ifndef QT_SERIALIZATION_READER_H
#define QT_SERIALIZATION_READER_H

#include <QIODevice>
#include <QPointer>

namespace QtSerialization
{

class QComposite;

class QReader
{
public:
    QReader(QIODevice& stream);
    virtual ~QReader() {}

    void read(QComposite& composite);

    quint16 file_format_version() const;
    quint16 major_version() const;
    quint16 minor_version() const;

    int tag() const { return f_tag; }
    bool closingTag() const { return f_closing; }

    QString text() const { return attribute('n'); }

    QString attribute(int attr) const;

    void readTag();
    void readText();
    static void invalidRead(const char *errmsg);
    static QString xmlDecode(const QString& string);

private:
    static const int BUFFER_SIZE = 4096;
    static const int STREAM_EOF = -1;

    void reset();

    int get();
    void unget(int c);

    void setTag(int c);
    void setClosingTag(bool closing = true) { f_closing = closing; }

    // share attribute 'n' buffer for the text data
    void append(int c) { appendAttributeChar('n', c); }
    void setText(const QString& t) { return setAttribute('n', t); }

    void setAttribute(int attr, const QString& value);
    void appendAttributeChar(int attr, int c);

    // information about the file being read
    bool                f_initialized = false;
    QString             f_name = QString();
    qint32              f_file_format_version = 0;
    qint32              f_major_version = 0;
    qint32              f_minor_version = 0;

    // stream we're reading from
    QPointer<QIODevice> f_stream;

    // stream buffer
    char                f_buffer[BUFFER_SIZE] = {};
    qint64              f_pos = 0;
    qint64              f_buf_size = 0;
    char                f_unget = 0;

    // last data read
    char                f_tag = '\0';
    bool                f_closing = false;
    QByteArray          f_attr_f = QByteArray();
    QByteArray          f_attr_v = QByteArray();
    QByteArray          f_attr_m = QByteArray();
    QByteArray          f_attr_n = QByteArray(); // shared with the set/get text
};


} // namespace QtSerialization
#endif
//#ifndef QT_SERIALIZATION_READER_H
// vim: ts=4 sw=4 et
