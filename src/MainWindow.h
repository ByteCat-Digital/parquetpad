#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QMenu>
#include <QAction>
#include <QFileDialog>

#include "ParquetTableModel.h"
#include "FileInfoDialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void openFile(const QString &filePath);

private slots:
    void openFileAction();
    void showFileInfo();
    void showContextMenu(const QPoint &pos);

private:
    void createMenus();

    QTableView *m_tableView;
    ParquetTableModel *m_parquetTableModel;
    FileInfoDialog *m_fileInfoDialog;

    QMenu *m_fileMenu;
    QAction *m_openAction;
    QAction *m_fileInfoAction;
    QAction *m_exitAction;
};

#endif // MAINWINDOW_H
