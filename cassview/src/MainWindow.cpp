#include "MainWindow.h"
#include "SettingsDialog.h"

using namespace QtCassandra;

MainWindow::MainWindow(QWidget *p)
    : QMainWindow(p)
{
    setupUi(this);

    QSettings settings( this );
    restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    restoreState   ( settings.value( "state"   , saveState()    ).toByteArray() );
    f_context = settings.value("context", "snap_websites").toString();

    f_cassandra = static_cast<cassandra_t>(new QCassandra);
    f_cassandra->connect( settings.value( "cassandra_host" ).toString()
                        , settings.value( "cassandra_port" ).toInt()
                        );

    qDebug() << "Working on Cassandra Cluster Named"    << f_cassandra->clusterName();
    qDebug() << "Working on Cassandra Protocol Version" << f_cassandra->protocolVersion();

    f_contextCombo->setModel( &f_cassandraModel );
    f_tables->setModel( &f_contextModel );
    f_rows->setModel( &f_tableModel );

    f_cassandraModel.setCassandra( f_cassandra );
    const int idx = f_contextCombo->findText( f_context );
    if( idx != -1 )
    {
        f_contextCombo->setCurrentIndex( idx );
    }

    connect( qApp, SIGNAL(aboutToQuit()), this, SLOT(OnAboutToQuit()) );
}


MainWindow::~MainWindow()
{
}


void MainWindow::OnAboutToQuit()
{
    QSettings settings( this );
    settings.setValue( "geometry", saveGeometry()                );
    settings.setValue( "state",    saveState()                   );
    settings.setValue( "context",  f_contextCombo->currentText() );
}


void MainWindow::fillTableList()
{
    QSharedPointer<QCassandraContext> qcontext( f_cassandra->findContext(f_context) );
	f_contextModel.setContext( qcontext );
}


void MainWindow::on_action_Settings_triggered()
{
    SettingsDialog dlg(this);
    if( dlg.exec() == QDialog::Accepted )
    {
        fillTableList();
    }
}

void MainWindow::on_f_tables_clicked(const QModelIndex &index)
{
    QString table_name( f_contextModel.data(index).toString() );
    QSharedPointer<QCassandraContext> qcontext( f_cassandra->findContext(f_context) );
    QSharedPointer<QCassandraTable> table( qcontext->findTable(table_name) );

    f_tableModel.setTable( table );
}


void MainWindow::on_f_contextCombo_currentIndexChanged(const QString &arg1)
{
    if( !arg1.isEmpty() )
	{
		f_context = arg1;
		fillTableList();
	}
}

void MainWindow::on_f_rows_doubleClicked(const QModelIndex &/*index*/)
{

}
