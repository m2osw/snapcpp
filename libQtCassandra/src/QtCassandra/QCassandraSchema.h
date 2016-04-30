/*
 * Text:
 *      QCassandraSchema.cpp
 *
 * Description:
 *      Database schema metadata.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2016 Made to Order Software Corp.
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

#pragma once

#include "QtCassandra/QCassandraQuery.h"
#include "QtCassandra/QCassandraSchemaValue.h"

#include <QString>


namespace QtCassandra
{
namespace QCassandraSchema
{


class SessionMeta
    : public std::enable_shared_from_this<SessionMeta>
{
public:
    typedef std::shared_ptr<SessionMeta>        pointer_t;
    typedef std::map<QString, pointer_t>        map_t;

    class KeyspaceMeta
        : public std::enable_shared_from_this<KeyspaceMeta>
    {
    public:
        typedef std::shared_ptr<KeyspaceMeta>       pointer_t;
        typedef std::map<QString, pointer_t>        map_t;

        class TableMeta
            : public std::enable_shared_from_this<TableMeta>
        {
        public:
            typedef std::shared_ptr<TableMeta>          pointer_t;
            typedef std::map<QString, pointer_t>        map_t;

            class ColumnMeta
                : public std::enable_shared_from_this<ColumnMeta>
            {
            public:
                typedef std::shared_ptr<ColumnMeta>         pointer_t;
                typedef std::map<QString, pointer_t>        map_t;

                enum type_t {
                    TypeRegular,
                    TypePartitionKey,
                    TypeClusteringKey,
                    TypeStatic,
                    TypeCompactValue
                };

                                                ColumnMeta( TableMeta::pointer_t tbl = TableMeta::pointer_t() );

                const QString&                  getName() const;
                type_t                          getType() const;
                const Value::map_t&             getFields() const;
                Value::map_t&                   getFields();

                Value&                          operator [] ( const QString& name );

                void                            encodeColumnMeta(QCassandraEncoder& encoded) const;
                void                            decodeColumnMeta(const QCassandraDecoder& decoder);

            private:
                friend class SessionMeta;

                TableMeta::pointer_t            f_table;
                QString                         f_name;
                Value::map_t                    f_fields;
                type_t                          f_type;
            };

                                            TableMeta( KeyspaceMeta::pointer_t kysp = KeyspaceMeta::pointer_t() );

            const QString&                  getName()   const;
            const Value::map_t&             getFields() const;
            Value::map_t&                   getFields();

            Value&                          operator [] ( const QString& name );

            const ColumnMeta::map_t&        getColumns() const;

            void                            encodeTableMeta(QCassandraEncoder& encoded) const;
            void                            decodeTableMeta(const QCassandraDecoder& decoder);

        private:
            friend class SessionMeta;

            KeyspaceMeta::pointer_t         f_keyspace;
            QString                         f_name;
            Value::map_t                    f_fields;
            ColumnMeta::map_t               f_columns;
        };

                                        KeyspaceMeta( SessionMeta::pointer_t session_meta = SessionMeta::pointer_t() );

        QCassandraSession::pointer_t    session() const;

        const QString&                  getName()   const;
        const Value::map_t&             getFields() const;
        Value::map_t&                   getFields();

        Value&                          operator [] ( const QString& name );

        const TableMeta::map_t&         getTables() const;

        void                            encodeKeyspaceMeta(QCassandraEncoder& encoded) const;
        void                            decodeKeyspaceMeta(const QCassandraDecoder& decoder);

    private:
        friend class SessionMeta;

        SessionMeta::pointer_t          f_session;
        QString                         f_name;
        Value::map_t                    f_fields;
        TableMeta::map_t                f_tables;
    };

                                    SessionMeta( QCassandraSession::pointer_t session = QCassandraSession::pointer_t() );
                                    ~SessionMeta();

    static pointer_t                create( QCassandraSession::pointer_t session );
    void                            loadSchema();
    QCassandraSession::pointer_t    session() const;
    uint32_t                        snapshotVersion() const;
    const KeyspaceMeta::map_t &     getKeyspaces();
    QByteArray                      encodeSessionMeta() const;
    void                            decodeSessionMeta(const QByteArray& encoded);

private:
    QCassandraSession::pointer_t    f_session;
    KeyspaceMeta::map_t             f_keyspaces;
};


} //namespace QtCassandra
} //namespace QCassandraSchema
// vim: ts=4 sw=4 et
