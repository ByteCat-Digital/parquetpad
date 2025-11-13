#include "MainWindow.h"
#include <QMenuBar>
#include <QFileDialog>
#include <QTableView>
#include <QVBoxLayout>

#include <QApplication>
#include <QMessageBox>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_tableView(new QTableView(this)),
      m_parquetTableModel(new ParquetTableModel(this)),
      m_fileInfoDialog(new FileInfoDialog(this))
{
    setWindowTitle("ParquetPad");
    setMinimumSize(800, 600);

    setCentralWidget(m_tableView);
    m_tableView->setModel(m_parquetTableModel);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tableView, &QTableView::customContextMenuRequested, this, &MainWindow::showContextMenu);

    createMenus();
}

MainWindow::~MainWindow() = default;

void MainWindow::createMenus() {
    m_fileMenu = menuBar()->addMenu("&File");

    m_openAction = new QAction("&Open...", this);
    m_openAction->setShortcut(QKeySequence::Open);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFileAction);
    m_fileMenu->addAction(m_openAction);

    m_fileInfoAction = new QAction("&File Information...", this);
    m_fileInfoAction->setDisabled(true); // Disabled until a file is loaded
    connect(m_fileInfoAction, &QAction::triggered, this, &MainWindow::showFileInfo);
    m_fileMenu->addAction(m_fileInfoAction);

    m_fileMenu->addSeparator();

    m_exitAction = new QAction("E&xit", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    m_fileMenu->addAction(m_exitAction);
}

void MainWindow::openFileAction() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open Parquet File", QString(), "Parquet Files (*.parquet)");
    if (!filePath.isEmpty()) {
        openFile(filePath);
    }
}

void MainWindow::openFile(const QString &filePath) {
    if (m_parquetTableModel->loadParquetFile(filePath)) {
        setWindowTitle("ParquetPad - " + QFileInfo(filePath).fileName());
        m_fileInfoAction->setEnabled(true);
    } else {
        QMessageBox::critical(this, "Error", "Could not open Parquet file: " + filePath);
        setWindowTitle("ParquetPad");
        m_fileInfoAction->setDisabled(true);
    }
}

void MainWindow::showFileInfo() {
    if (m_parquetTableModel->getTotalRows() > 0) {
        m_fileInfoDialog->setFileInfo(m_parquetTableModel->filePath(),
                                      m_parquetTableModel->getTotalRows(),
                                      m_parquetTableModel->getNumRowGroups(),
                                      m_parquetTableModel->getSchema());
        m_fileInfoDialog->exec();
    } else {
        QMessageBox::information(this, "Information", "No Parquet file loaded.");
    }
}

void MainWindow::showContextMenu(const QPoint &pos) {
    if (!m_fileInfoAction->isEnabled()) {
        return;
    }

    QMenu contextMenu(this);
    contextMenu.addAction(m_fileInfoAction);
    contextMenu.exec(m_tableView->viewport()->mapToGlobal(pos));
}
