#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QMenu>
#include <QAction>
#include <QFileDialog>

#include "ParquetTableModel.h"
#include "FileInfoDialog.h"
#include "AboutDialog.h"

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
    void showAboutDialog();

private:
    void createMenus();

    QTableView *m_tableView;
    ParquetTableModel *m_parquetTableModel;
    FileInfoDialog *m_fileInfoDialog;
    AboutDialog *m_aboutDialog;

    QMenu *m_fileMenu;
    QMenu *m_helpMenu;
    QAction *m_openAction;
    QAction *m_fileInfoAction;
    QAction *m_exitAction;
    QAction *m_aboutAction;
};

#endif // MAINWINDOW_H
