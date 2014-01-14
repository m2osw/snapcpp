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

    f_cassandra.connect( settings.value( "cassandra_host" ).toString()
                       , settings.value( "cassandra_port" ).toInt()
                       );

    qDebug() << "Working on Cassandra Cluster Named"    << f_cassandra.clusterName();
    qDebug() << "Working on Cassandra Protocol Version" << f_cassandra.protocolVersion();

    f_contextCombo->setModel( &f_contextModel );
    f_tables->setModel( &f_tableModel );
    f_rows->setModel( &f_rowModel );
    updateContextList();
    //fillTableList();

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
    f_tableModel.setStringList( QStringList() );

    QSharedPointer<QCassandraContext> qcontext( f_cassandra.findContext(f_context) );

    QStringList list;
    const QCassandraTables& tables = qcontext->tables();
    std::for_each( tables.begin(), tables.end(),
                   [&list]( const QSharedPointer<QCassandraTable>& table )
    {
        qDebug() << "\ttable_name = " << table->tableName();

        list << table->tableName();
#if 0
        QCassandraRowPredicate rowp;
        rowp.setStartRowName("");
        rowp.setEndRowName("");
        rowp.setCount(10); // 100 is the default
        table->readRows( rowp );
        const QCassandraRows& rows = table->rows();
        std::for_each( rows.begin(), rows.end(),
                       [&rows]( const QSharedPointer<QCassandraRow>& row )
        {
            qDebug() << "\t\trow_name = " << row->rowName();
            /*const QCassandraRows& rows = rows[row_name]->rows();
                            QList<QString> row_names = rows.keys();*/
        });
#endif
    });

    f_tableModel.setStringList( list );
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
    f_rowModel.setStringList( QStringList() );

    QStringList tablesList( f_tableModel.stringList() );
    QSharedPointer<QCassandraContext> qcontext( f_cassandra.findContext(f_context) );
    QSharedPointer<QCassandraTable> table( qcontext->table( tablesList[index.row()] ));

    QCassandraRowPredicate rowp;
    //rowp.setStartRowName("");
    //rowp.setEndRowName("");
    //rowp.setCount(10); // 100 is the default
    table->readRows( rowp );
    QStringList list;
    const QCassandraRows& rows = table->rows();
    std::for_each( rows.begin(), rows.end(),
                   [&list]( const QSharedPointer<QCassandraRow>& row )
    {
        qDebug() << "\t\trow_name = " << row->rowName();
        list << row->rowName();
    });

    f_rowModel.setStringList( list );
}


void MainWindow::updateContextList()
{
    f_contextModel.setStringList( QStringList() );

    const QCassandraContexts& context_list = f_cassandra.contexts();
    QList<QString> keys = context_list.keys();

    QStringList context_key_list;
    std::for_each( keys.begin(), keys.end(),
                   [&context_key_list]( const QString& key )
    {
        context_key_list << key;
    });

    f_contextModel.setStringList( context_key_list );

    const int idx = f_contextCombo->findText( f_context );
    if( idx != -1 )
    {
		f_contextCombo->setCurrentIndex( idx );
    }
}


void MainWindow::on_f_contextCombo_currentIndexChanged(const QString &arg1)
{
    if( !arg1.isEmpty() )
	{
		f_context = arg1;
		fillTableList();
	}
}
