/*
 * Text:
 *      QCassandraQuery.h
 *
 * Description:
 *      Handling of the cassandra::CfDef (Column Family Definition).
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

#include <map>
#include <memory>
#include <string>

#include <QString>
#include <QByteArray>

typedef struct CassCluster_      CassCluster;
typedef struct CassCollection_   CassCollection;
typedef struct CassColumnMeta_   CassColumnMeta;
typedef struct CassFuture_       CassFuture;
typedef struct CassIterator_     CassIterator;
typedef struct CassKeyspaceMeta_ CassKeyspaceMeta;
typedef struct CassResult_       CassResult;
typedef struct CassSchemaMeta_   CassSchemaMeta;
typedef struct CassSession_      CassSession;
typedef struct CassStatement_    CassStatement;
typedef struct CassValue_        CassValue;

namespace QtCassandra
{

namespace CassTools
{
    typedef std::shared_ptr<CassCluster>            cluster_pointer_t;
    typedef std::shared_ptr<CassCollection>         collection_pointer_t;
    typedef std::shared_ptr<const CassColumnMeta>   column_meta_pointer_t;
    typedef std::shared_ptr<CassFuture>             future_pointer_t;
    typedef std::shared_ptr<CassIterator>           iterator_pointer_t;
    typedef std::shared_ptr<const CassKeyspaceMeta> keyspace_meta_pointer_t;
    typedef std::shared_ptr<const CassResult>       result_pointer_t;
    typedef std::shared_ptr<const CassSchemaMeta>   schema_meta_pointer_t;
    typedef std::shared_ptr<const CassTableMeta>    table_meta_pointer_t;
    typedef std::shared_ptr<CassSession>            session_pointer_t;
    typedef std::shared_ptr<CassStatement>          statement_pointer_t;
}


class QCassandraSession
        : public std::enable_shared_from_this<QCassandraSession>
{
public:
    typedef std::shared_ptr<QCassandraSession> pointer_t;

    static pointer_t create();
    ~QCassandraSession();

    void connect( const QString& host = "localhost", const int port = 9042 );
    void connect( const QStringList& host_list     , const int port = 9042 );
    void disconnect();
    bool isConnected() const;

    CassTools::cluster_pointer_t cluster()    const;
    CassTools::session_pointer_t session()    const;
    CassTools::future_pointer_t  connection() const;

private:
    QCassandraSession();

    CassTools::cluster_pointer_t       f_cluster;
    CassTools::session_pointer_t       f_session;
    CassTools::future_pointer_t        f_connection;
};


class QCassandraSessionMeta
{
public:
    typedef std::shared_ptr<QCassandraSessionMeta>  pointer_t;
    typedef std::map<QString,QString>               qstring_map_t;

    QCassandraSessionMeta( QCassandraSession::pointer_t session );
    ~QCassandraSessionMeta();

    QCassandraSession::pointer_t    session() const;

    cass_uint32_t   snapshotVersion() const;

    class KeyspaceMeta
    {
    public:
        typedef std::shared_ptr<KeyspaceMeta> pointer_t;
        typedef std::map<QString,pointer_t>   map_t;

        KeyspaceMeta( QCassandraSessionMeta::pointer_t session_meta );

        QCassandraSession::pointer_t session() const;

        QString         getName() const;
        qstring_map_t   getFields() const;

        class TableMeta
        {
        public:
            typedef std::shared_ptr<TableMeta>  pointer_t;
            typedef std::map<QString,pointer_t> map_t;

            TableMeta( KeyspaceMeta::pointer_t kysp );

            QString     getName() const;

            class ColumnMeta
            {
            public:
                typedef std::shared_ptr<ColumnMeta> pointer_t;
                typedef std::map<QString,pointer_t> map_t;

                typedef enum
                {
                    TypeRegular, TypePartitionKey, TypeClusteringKey, TypeStatic, TypeCompactValue;
                }
                type_t;

                ColumnMeta( TableMeta::pointer_t tbl );

                QString         getName() const;
                qstring_map_t   getFields() const;
                type_t          getType() const;

            private:
                qstring_map_t   f_fields;
            };

        private:
            KeyspaceMeta::pointer_t f_keyspace;
            ColumnMeta::map_t       f_columns;
        };

        const TableMeta::map_t& getTableMetaMap() const;

    private:
        QCassandraSessionMeta::pointer_t    f_session;
        qstring_map_t                       f_fields;
        TableMeta::map_t                    f_tables;
    };

    KeyspaceMeta::map_t   getKeyspaceMetaMap() const;

private:
    QCassandraSession::pointer_t    f_session;
    qstring_list_t                  f_keyspaceNames;
};


class QCassandraQuery
{
public:
    typedef std::shared_ptr<QCassandraQuery>  pointer_t;
    typedef std::map<std::string,std::string> string_map_t;

    QCassandraQuery( QCassandraSession::pointer_t session );
    ~QCassandraQuery();

    void       query         ( const QString& query_string, const int bind_count = 0 );
    void       setPagingSize ( const int size );

    void       bindBool      ( const size_t num, const bool          value );
    void       bindInt32     ( const size_t num, const int32_t       value );
    void       bindInt64     ( const size_t num, const int64_t       value );
    void       bindFloat     ( const size_t num, const float         value );
    void       bindDouble    ( const size_t num, const double        value );
    void       bindString    ( const size_t num, const QString&      value );
    void       bindByteArray ( const size_t num, const QByteArray&   value );
    void       bindJsonMap   ( const size_t num, const string_map_t& value );
    void       bindMap       ( const size_t num, const string_map_t& value );

    void       start();
    bool       nextRow();
    bool       nextPage();
    void       end();

    bool       getBoolColumn      ( const QString& name  ) const;
    bool       getBoolColumn      ( const int      num   ) const;
    int32_t    getInt32Column     ( const QString& name  ) const;
    int32_t    getInt32Column     ( const int      num   ) const;
    int64_t    getInt64Column     ( const QString& name  ) const;
    int64_t    getInt64Column     ( const int      num   ) const;
    float      getFloatColumn     ( const QString& name  ) const;
    float      getFloatColumn     ( const int      num   ) const;
    double     getDoubleColumn    ( const QString& name  ) const;
    double     getDoubleColumn    ( const int      num   ) const;
    QString    getStringColumn    ( const QString& name  ) const;
    QString    getStringColumn    ( const int      num   ) const;
    QByteArray getByteArrayColumn ( const QString& name  ) const;
    QByteArray getByteArrayColumn ( const int      num   ) const;

    string_map_t getJsonMapColumn ( const QString& name ) const;
    string_map_t getJsonMapColumn ( const int num ) const;
    string_map_t getMapColumn     ( const QString& name ) const;
    string_map_t getMapColumn     ( const int num ) const;

private:
    // Current query
    //
    QCassandraSession::pointer_t   f_session;
    QString                        f_queryString;
    CassTools::statement_pointer_t f_queryStmt;
    CassTools::future_pointer_t    f_sessionFuture;
    CassTools::result_pointer_t    f_queryResult;
    CassTools::iterator_pointer_t  f_rowsIterator;

    bool 		    getBoolFromValue      ( const CassValue* value ) const;
    QByteArray      getByteArrayFromValue ( const CassValue* value ) const;
    string_map_t    getMapFromValue       ( const CassValue* value ) const;
    void            throwIfError          ( const QString& msg     );
};


}
// namespace QtCassandra

    
// vim: ts=4 sw=4 et
