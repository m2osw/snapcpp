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

    f_cassandra = QCassandra::create();
    f_cassandra->connect( settings.value( "cassandra_host" ).toString()
                        , settings.value( "cassandra_port" ).toInt()
                        );

    qDebug() << "Working on Cassandra Cluster Named"    << f_cassandra->clusterName();
    qDebug() << "Working on Cassandra Protocol Version" << f_cassandra->protocolVersion();

    f_contextCombo->setModel( &f_cassandraModel );
    f_tables->setModel( &f_contextModel );
    f_rows->setModel( &f_tableModel );
	f_cells->setModel( &f_rowModel );

    f_cassandraModel.setCassandra( f_cassandra );
    const int idx = f_contextCombo->findText( f_context );
    if( idx != -1 )
    {
        f_contextCombo->setCurrentIndex( idx );
    }

    f_tables->setCurrentIndex( 0 );

    connect( f_rows->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(onCurrentChanged(QModelIndex,QModelIndex)) );
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
    f_tableModel.setTable( QCassandraTable::pointer_t() );
    f_rowModel.setRow( QCassandraRow::pointer_t() );

    QCassandraContext::pointer_t qcontext( f_cassandra->findContext(f_context) );
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

void MainWindow::on_f_tables_currentIndexChanged(const QString &table_name)
{
    f_rowModel.setRow( QCassandraRow::pointer_t() );

    QCassandraContext::pointer_t qcontext( f_cassandra->findContext(f_context) );
    QCassandraTable::pointer_t table( qcontext->findTable(table_name) );
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


void MainWindow::changeRow(const QModelIndex &index)
{
    QString row_name( f_tableModel.data(index).toString() );

    QCassandraContext::pointer_t qcontext( f_cassandra->findContext(f_context) );
    QCassandraRow::pointer_t row( f_tableModel.getTable()->findRow(row_name) );

    f_rowModel.setRow( row );
}


void MainWindow::onCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ )
{
    changeRow( current );
}

