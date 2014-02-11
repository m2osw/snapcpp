#include "MainWindow.h"
#include "SettingsDialog.h"
#include "AboutDialog.h"

#include <QMessageBox>

#include <iostream>

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

    f_mainSplitter->setStretchFactor( 0, 0 );
    f_mainSplitter->setStretchFactor( 1, 1 );

    f_cells->setContextMenuPolicy( Qt::CustomContextMenu );

    action_InsertColumn->setEnabled( false );
    action_DeleteColumns->setEnabled( false );

    connect( f_cells, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(onShowContextMenu(const QPoint&)) );
    connect( f_rows->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(onRowsCurrentChanged(QModelIndex,QModelIndex)) );
    connect( &f_rowModel, SIGNAL(modelReset()),
             this, SLOT(onCellsModelReset()) );
    connect( qApp, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()) );
    connect( f_cells->horizontalHeader(), SIGNAL(sectionClicked(int)),
             this, SLOT(onSectionClicked(int)));
}


MainWindow::~MainWindow()
{
}


void MainWindow::onAboutToQuit()
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


void MainWindow::onShowContextMenu( const QPoint& mouse_pos )
{
    if( !f_rows->selectionModel()->hasSelection() )
    {
        // Do nothing, as something must be selected in the rows!
        return;
    }

    QPoint global_pos( f_cells->mapToGlobal(mouse_pos) );

    QMenu menu( this );
    menu.addAction( action_InsertColumn );
    menu.addAction( action_DeleteColumns );
    menu.exec(global_pos);
}


void MainWindow::onCellsModelReset()
{
    f_cells->resizeColumnsToContents();
}


void MainWindow::on_action_Settings_triggered()
{
    try
    {
        SettingsDialog dlg(this);
        if( dlg.exec() == QDialog::Accepted )
        {
            fillTableList();
        }
    }
    catch( const std::exception& p_x )
    {
        std::cerr << "Exception caught! [" << p_x.what() << "]" << std::endl;
    }
}


void MainWindow::on_f_tables_currentIndexChanged(const QString &table_name)
{
    try
    {
        f_rowModel.setRow( QCassandraRow::pointer_t() );
        QCassandraContext::pointer_t qcontext( f_cassandra->findContext(f_context) );
        QCassandraTable::pointer_t table( qcontext->findTable(table_name) );
        f_tableModel.setTable( table );
    }
    catch( const std::exception& p_x )
    {
        std::cerr << "Exception caught! [" << p_x.what() << "]" << std::endl;
    }
}


void MainWindow::on_f_contextCombo_currentIndexChanged(const QString &arg1)
{
    if( arg1.isEmpty() )
    {
        return;
    }

    try
    {
        f_context = arg1;
        fillTableList();
    }
    catch( const std::exception& p_x )
    {
        std::cerr << "Exception caught! [" << p_x.what() << "]" << std::endl;
    }
}


void MainWindow::changeRow(const QModelIndex &index)
{
    const QByteArray row_key( f_tableModel.data(index, Qt::UserRole).toByteArray() );
    QCassandraRow::pointer_t row( f_tableModel.getTable()->findRow(row_key) );

    f_rowModel.setRow( row );

    action_InsertColumn->setEnabled( true );
    action_DeleteColumns->setEnabled( true );
}


void MainWindow::onRowsCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ )
{
    try
    {
        changeRow( current );
    }
    catch( const std::exception& p_x )
    {
        std::cerr << "Exception caught! [" << p_x.what() << "]" << std::endl;
    }
}


void MainWindow::on_action_About_triggered()
{
    AboutDialog dlg( this );
    dlg.exec();
}

void MainWindow::on_action_AboutQt_triggered()
{
    QMessageBox::aboutQt( this );
}


void MainWindow::onSectionClicked( int section )
{
    f_cells->resizeColumnToContents( section );
}


void MainWindow::on_action_InsertColumn_triggered()
{
    f_rowModel.insertRows( 0, 0 );  // Always inserts just one row
}


void MainWindow::on_action_DeleteColumns_triggered()
{
    try
    {
        const QModelIndexList selectedItems( f_cells->selectionModel()->selectedRows() );
        f_rowModel.removeRows( selectedItems[0].row(), selectedItems.size() );
    }
    catch( const std::exception& p_x )
    {
        std::cerr << "Exception caught! [" << p_x.what() << "]" << std::endl;
    }
}

// vim: ts=4 sw=4 et syntax=cpp.doxygen
