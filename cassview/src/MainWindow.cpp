#include "MainWindow.h"

#include "AboutDialog.h"
#include "DisplayException.h"
#include "InputDialog.h"
#include "SettingsDialog.h"

#include <QMessageBox>

#include <iostream>

using namespace QtCassandra;

MainWindow::MainWindow(QWidget *p)
    : QMainWindow(p)
{
    setupUi(this);

    QSettings const settings( this );
    restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    restoreState   ( settings.value( "state"   , saveState()    ).toByteArray() );

    f_mainSplitter->restoreState( settings.value( "splitterState", f_mainSplitter->saveState() ).toByteArray() );

    f_context = settings.value("context", "snap_websites").toString();

    f_session = QCassandraSession::create();
    connectCassandra();

    f_contextCombo->setModel( &f_cassandraModel );
    f_tables->setModel( &f_contextModel );
    f_rowsView->setModel( &f_tableModel );
    f_cellsView->setModel( &f_rowModel );

    f_cassandraModel.setCassandra( f_session );
    int const idx = f_contextCombo->findText( f_context );
    if( idx != -1 )
    {
        f_contextCombo->setCurrentIndex( idx );
    }

    f_tables->setCurrentIndex( 0 );

    f_mainSplitter->setStretchFactor( 0, 0 );
    f_mainSplitter->setStretchFactor( 1, 1 );

    f_cellsView->setContextMenuPolicy( Qt::CustomContextMenu );
    f_cellsView->setWordWrap( true ); // this is true by default anyway, and it does not help when we have a column with a super long string...

    action_InsertColumn->setEnabled( false );
    action_DeleteColumns->setEnabled( false );

    connect( f_cellsView, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(onShowCellsContextMenu(const QPoint&)) );
    connect( f_cellsView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(onCellsCurrentChanged(QModelIndex,QModelIndex)) );
    connect( f_rowsView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(onRowsCurrentChanged(QModelIndex,QModelIndex)) );
    connect( &f_rowModel, SIGNAL(modelReset()),
             this, SLOT(onCellsModelReset()) );
    connect( qApp, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()) );
    //connect( f_cellsView->horizontalHeader(), SIGNAL(sectionClicked(int)),
             //this, SLOT(onSectionClicked(int)) );
    connect( f_filterEdit, SIGNAL(returnPressed()), this, SLOT(on_f_applyFilter_clicked()) );
}


MainWindow::~MainWindow()
{
}


namespace
{
    void displayError( const std::exception& except, const QString& caption, const QString& message )
    {
        DisplayException de( except.what(), caption, message );
        de.displayError();
    }
}


void MainWindow::connectCassandra()
{
    QSettings const settings;
    QString const host( settings.value( "cassandra_host" ).toString() );
    int     const port( settings.value( "cassandra_port" ).toInt()    );
    try
    {
        f_session->connect( host, port );
        //
        //qDebug() << "Working on Cassandra Cluster Named"    << f_session->clusterName();
        //qDebug() << "Working on Cassandra Protocol Version" << f_session->protocolVersion();

        QString const hostname( tr("%1:%2").arg(host).arg(port) );
        setWindowTitle( tr("Cassandra View [%1]").arg(hostname) );
        f_connectionBtn->setText( hostname );
    }
    catch( std::exception const & except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Cannot connect to Cassandra server at '%1:%2'! Please check your connection information in the settings.")
                      .arg(host)
                      .arg(port)
                    );
        on_action_Settings_triggered();
    }
}


void MainWindow::onAboutToQuit()
{
    saveValue();

    QSettings settings( this );
    settings.setValue( "geometry",      saveGeometry()                );
    settings.setValue( "state",         saveState()                   );
    settings.setValue( "splitterState", f_mainSplitter->saveState()   );
    settings.setValue( "context",       f_contextCombo->currentText() );
}


void MainWindow::fillTableList()
{
    f_tableModel.clear();
    f_rowModel.clear();

    f_contextModel.setCassandra( f_session, f_context );

    const int idx = f_contextCombo->findText( f_context );
    if( idx != -1 )
    {
        f_contextCombo->setCurrentIndex( idx );
    }
}


void MainWindow::onShowCellsContextMenu( const QPoint& mouse_pos )
{
    if( !f_rowsView->selectionModel()->hasSelection() )
    {
        // Do nothing, as something must be selected in the rows!
        return;
    }

    QPoint global_pos( f_cellsView->mapToGlobal(mouse_pos) );

    QMenu menu( this );
    menu.addAction( action_InsertColumn );
    menu.addAction( action_DeleteColumns );
    menu.exec(global_pos);
}


void MainWindow::onCellsModelReset()
{
    //f_cellsView->resizeColumnsToContents();
}


void MainWindow::on_action_Settings_triggered()
{
    saveValue();

    try
    {
        SettingsDialog dlg(this);
        if( dlg.exec() == QDialog::Accepted )
        {
            connectCassandra();
            fillTableList();
        }
    }
    catch( const std::exception& except )
    {
        displayError( except
                      , tr("Connection Error")
                      , tr("Error connecting to the server!")
                      );
    }
}


void MainWindow::on_f_tables_currentIndexChanged(QString const & table_name)
{
    saveValue();

    f_rowModel.clear();

    if( table_name.isEmpty() )
    {
        return;
    }

    QString filter_text( f_filterEdit->text( ) );
    QRegExp filter( filter_text );
    if(!filter.isValid() && !filter_text.isEmpty())
    {
        // reset the filter
        filter = QRegExp();

        QMessageBox::warning( QApplication::activeWindow()
            , tr("Warning!")
            , tr("Warning!\nThe filter regular expression is not valid. It will not be used.")
            , QMessageBox::Ok
            );
    }

    try
    {
        f_tableModel.setSession
                ( f_session
                , f_context
                , table_name
                , filter
                );
    }
    catch( const std::exception& except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Error connecting to the server!")
                    );
    }
}


void MainWindow::on_f_applyFilter_clicked()
{
    saveValue();

    QString const table_name( f_tables->currentText( ) );
    on_f_tables_currentIndexChanged( table_name );
}


void MainWindow::on_f_refreshView_clicked()
{
    saveValue();

    QString const table_name( f_tables->currentText( ) );
    on_f_tables_currentIndexChanged( table_name );
}


void MainWindow::on_f_contextCombo_currentIndexChanged(const QString &arg1)
{
    saveValue();

    if( arg1.isEmpty() )
    {
        return;
    }

    try
    {
        f_context = arg1;
        fillTableList();
    }
    catch( const std::exception& except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Error connecting to the server!")
                    );
    }
}


void MainWindow::changeRow(const QModelIndex &index)
{
    saveValue();

    const QByteArray row_key( f_tableModel.data(index).toByteArray() );

    f_rowModel.setSession
        ( f_session
        , f_tableModel.keyspaceName()
        , f_tableModel.tableName()
        , row_key
        );

    action_InsertColumn->setEnabled( true );
    action_DeleteColumns->setEnabled( true );
}


void MainWindow::changeCell(const QModelIndex &index)
{
    saveValue();

    const QByteArray row_key( f_tableModel.data(index).toByteArray() );

    f_rowModel.setSession
        ( f_session
        , f_tableModel.keyspaceName()
        , f_tableModel.tableName()
        , row_key
        );

    action_InsertColumn->setEnabled( true );
    action_DeleteColumns->setEnabled( true );
}


void MainWindow::saveValue()
{
    auto doc( f_valueEdit->document() );
    if( doc->isModified() )
    {
        auto selected_rows( f_cellsView->selectionModel()->selectedRows() );
        if( selected_rows.size() == 1 )
        {
            const QByteArray key( f_rowModel.data( *(selected_rows.begin()) ).toByteArray() );
            const QString q_str(
                QString("UPDATE %1.%2 SET value = ?")
                    .arg(f_rowModel.keyspaceName())
                    .arg(f_rowModel.tableName())
                );
            QCassandraQuery query( f_session );
            query.query( q_str );
            query.bindByteArray( 0, doc->toPlainText().toUtf8() );
            query.start();
            query.end();

            // TODO: handle error... This probably should be run asynchronously;
            // i.e. query.start( false /*block*/ );
        }
    }
}


void MainWindow::onRowsCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ )
{
    try
    {
        changeRow( current );
    }
    catch( const std::exception& except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Error connecting to the server!")
                    );
    }
}


void MainWindow::onCellsCurrentChanged( const QModelIndex & current, const QModelIndex & /*previous*/ )
{
    try
    {
        changeCell( current );
    }
    catch( const std::exception& except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Error connecting to the server!")
                    );
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


void MainWindow::onSectionClicked( int /*section*/ )
{
    //f_cellsView->resizeColumnToContents( section );
}


void MainWindow::on_action_InsertColumn_triggered()
{
    f_rowModel.insertRows( 0, 0 );
}


void MainWindow::on_action_DeleteColumns_triggered()
{
    try
    {
        const QModelIndexList selectedItems( f_cellsView->selectionModel()->selectedRows() );
        if( !selectedItems.isEmpty() )
        {
            QMessageBox::StandardButton result
                    = QMessageBox::warning( QApplication::activeWindow()
                    , tr("Warning!")
                    , tr("Warning!\nYou are about to remove %1 columns from row '%2', in table '%3'.\nThis cannot be undone!")
                      .arg(selectedItems.size())
                      .arg(QString::fromUtf8(f_rowModel.rowKey().data()))
                      .arg(f_rowModel.tableName())
                    , QMessageBox::Ok | QMessageBox::Cancel
                    );
            if( result == QMessageBox::Ok )
            {
                f_rowModel.removeRows( selectedItems[0].row(), selectedItems.size() );
            }
        }
    }
    catch( const std::exception& except )
    {
        displayError( except
                    , tr("Connection Error")
                    , tr("Error connecting to the server!")
                    );
    }
}


void MainWindow::on_f_connectionBtn_clicked()
{
    on_action_Settings_triggered();
}


// vim: ts=4 sw=4 et
