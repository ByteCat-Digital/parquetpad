#include <parquet/arrow/reader.h>

#include "MainWindow.h"
#include <QMenuBar>
#include <QFileDialog>
#include <QTableView>
#include <QVBoxLayout>

#include <QApplication>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_tableView(new QTableView(this)),
      m_parquetTableModel(new ParquetTableModel(this)),
      m_fileInfoDialog(new FileInfoDialog(this)),
      m_aboutDialog(new AboutDialog(this))
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

    m_helpMenu = menuBar()->addMenu("&Help");
    m_aboutAction = new QAction("&About", this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
    m_helpMenu->addAction(m_aboutAction);
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
        QFileInfo fileInfo(m_parquetTableModel->filePath());
        qint64 fileSize = fileInfo.size();
        qint64 uncompressedSize = 0;

        auto fileReader = m_parquetTableModel->getFileReader();
        if (fileReader) {
            auto fileMetadata = fileReader->parquet_reader()->metadata();
            for (int i = 0; i < fileMetadata->num_row_groups(); ++i) {
                uncompressedSize += fileMetadata->RowGroup(i)->total_byte_size();
            }
        }

        m_fileInfoDialog->setFileInfo(m_parquetTableModel->filePath(),
                                      fileSize,
                                      uncompressedSize,
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

void MainWindow::showAboutDialog() {
    m_aboutDialog->exec();
}
