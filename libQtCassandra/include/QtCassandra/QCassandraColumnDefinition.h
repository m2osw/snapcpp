/*
 * Header:
 *      QCassandraColumnDefinitions.h
 *
 * Description:
 *      Handling of the cassandra::ColumnDef.
 *
 * Documentation:
 *      See the corresponding .cpp file.
 *
 * License:
 *      Copyright (c) 2011-2013 Made to Order Software Corp.
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
#ifndef QCASSANDRA_COLUMN_DEFINITION_H
#define QCASSANDRA_COLUMN_DEFINITION_H

#include <QObject>
#include <QString>
#include <QMap>
#include <memory>

namespace QtCassandra
{

class QCassandraTable;
class QCassandraColumnDefinitionPrivate;


// Cassandra ColumnDef
class QCassandraColumnDefinition : public QObject
{
public:
    typedef std::shared_ptr<QCassandraColumnDefinition> pointer_t;
    typedef QMap<QString, QString>                      QCassandraIndexOptions;

    enum index_type_t {
        INDEX_TYPE_UNKNOWN = -2,
        INDEX_TYPE_UNDEFINED = -1,
        INDEX_TYPE_KEYS = 0
    };

    virtual ~QCassandraColumnDefinition();

    QString columnName() const;

    void setValidationClass(const QString& name);
    QString validationClass() const;

    void setIndexType(index_type_t index_type);
    void unsetIndexType();
    bool hasIndexType() const;
    index_type_t indexType() const;

    void setIndexName(const QString& name);
    void unsetIndexName();
    bool hasIndexName() const;
    QString indexName() const;

    void setIndexOptions(const QCassandraIndexOptions& options); // since 1.0
    const QCassandraIndexOptions& indexOptions() const;
    void setIndexOption(const QString& option, const QString& value);
    QString indexOption(const QString& option) const;
    void eraseIndexOption(const QString& option);

private:
    QCassandraColumnDefinition(std::shared_ptr<QCassandraTable> table, const QString& name);

    void parseColumnDefinition(const void *data);
    void prepareColumnDefinition(void *data) const;

    friend class QCassandraTable;
    friend class QCassandraColumnDefinitionPrivate;

    std::auto_ptr<QCassandraColumnDefinitionPrivate>    f_private;
    // f_table is a parent that has a strong shared pointer over us so it
    // cannot disappear before we do, thus only a bare pointer is enough here
    // (there isn't a need to use a QWeakPointer or QPointer either)
    std::shared_ptr<QCassandraTable>                    f_table;
    QCassandraIndexOptions                              f_index_options;
};

// array of column definitions
typedef QMap<QString, QCassandraColumnDefinition::pointer_t > QCassandraColumnDefinitions;



} // namespace QtCassandra
#endif
//#ifndef QCASSANDRA_COLUMN_DEFINITION_H
// vim: ts=4 sw=4 et
