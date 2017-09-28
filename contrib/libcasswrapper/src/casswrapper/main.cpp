#include <QSqlDriverPlugin>
#include <QStringList>

#include "qsql_cassandra_p.h"

QT_BEGIN_NAMESPACE

class QCassandraDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "cassandra.json")

public:
    QCassandraDriverPlugin();

    QSqlDriver* create(const QString &) Q_DECL_OVERRIDE;
};

QCassandraDriverPlugin::QCassandraDriverPlugin()
    : QSqlDriverPlugin()
{
}

QSqlDriver* QCassandraDriverPlugin::create(const QString &name)
{
    if( name == QLatin1String("QCassandra") )
	{
        QCassandraDriver* driver = new QCassandraDriver();
		driver->setBlocking(false);
        return driver;
	}
	else if( name == QLatin1String("QCassandraBlocking") )
	{
        QCassandraDriver* driver = new QCassandraDriver();
		driver->setBlocking(true);
        return driver;
    }
    return 0;
}

QT_END_NAMESPACE
