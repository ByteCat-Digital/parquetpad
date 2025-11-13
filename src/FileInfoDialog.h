#ifndef FILEINFODIALOG_H
#define FILEINFODIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <memory>

// Forward declaration for Arrow types
namespace arrow {
    class Schema;
}

class FileInfoDialog : public QDialog {
    Q_OBJECT

public:
    explicit FileInfoDialog(QWidget *parent = nullptr);
    ~FileInfoDialog() override;

    void setFileInfo(const QString &filePath, int totalRows, int numRowGroups,
                     std::shared_ptr<arrow::Schema> schema);

private:
    QTextEdit *m_infoTextEdit;
};

#endif // FILEINFODIALOG_H
