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

    f_cassandra.connect( settings.value( "cassandra_host" ).toString()
                       , settings.value( "cassandra_port" ).toInt()
                       );

    qDebug() << "Working on Cassandra Cluster Named"    << f_cassandra.clusterName();
    qDebug() << "Working on Cassandra Protocol Version" << f_cassandra.protocolVersion();

    f_tables->setModel( &f_tableModel );
    f_rows->setModel( &f_rowModel );
    FillTableList();

    connect( qApp, SIGNAL(aboutToQuit()), this, SLOT(OnAboutToQuit()) );
}


MainWindow::~MainWindow()
{
}


void MainWindow::OnAboutToQuit()
{
    QSettings settings( this );
    settings.setValue( "geometry", saveGeometry() );
    settings.setValue( "state",    saveState()    );
}


void MainWindow::FillTableList()
{
    QSettings settings( this );
    const QString context( settings.value( "context" ).toString() );

    f_contextEdit->setText( context );
    f_tableModel.setStringList( QStringList() );

    QSharedPointer<QCassandraContext> qcontext( f_cassandra.findContext(context) );

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
        FillTableList();
    }
}

void MainWindow::on_f_tables_clicked(const QModelIndex &index)
{
    f_rowModel.setStringList( QStringList() );

    const QString context( f_contextEdit->text() );
    QStringList tablesList( f_tableModel.stringList() );
    QSharedPointer<QCassandraContext> qcontext( f_cassandra.findContext(context) );
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
