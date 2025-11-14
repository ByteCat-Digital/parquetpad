#include "FileInfoDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <arrow/api.h>

FileInfoDialog::FileInfoDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Parquet File Information");
    setMinimumSize(400, 300);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    m_infoTextEdit = new QTextEdit(this);
    m_infoTextEdit->setReadOnly(true);
    mainLayout->addWidget(m_infoTextEdit);

    QPushButton *closeButton = new QPushButton("Close", this);
    connect(closeButton, &QPushButton::clicked, this, &FileInfoDialog::accept);
    mainLayout->addWidget(closeButton, 0, Qt::AlignRight);
}

FileInfoDialog::~FileInfoDialog() = default;

#include <QLocale>

// Helper function to format bytes into KB, MB, GB, etc.
QString formatSize(qint64 bytes) {
    if (bytes < 1024)
        return QLocale().toString(bytes) + " B";
    double kb = bytes / 1024.0;
    if (kb < 1024.0)
        return QLocale().toString(kb, 'f', 2) + " KB";
    double mb = kb / 1024.0;
    if (mb < 1024.0)
        return QLocale().toString(mb, 'f', 2) + " MB";
    double gb = mb / 1024.0;
    return QLocale().toString(gb, 'f', 2) + " GB";
}

void FileInfoDialog::setFileInfo(const QString &filePath, qint64 fileSize, qint64 uncompressedSize,
                                 int totalRows, int numRowGroups,
                                 std::shared_ptr<arrow::Schema> schema) {
    QString info;
    info += "<b>File Path:</b> " + filePath + "\n";
    info += "<b>File Size:</b> " + formatSize(fileSize) + "\n";
    info += "<b>Uncompressed Size:</b> " + formatSize(uncompressedSize) + "\n";
    info += "<b>Total Rows:</b> " + QString::number(totalRows) + "\n";
    info += "<b>Number of Row Groups:</b> " + QString::number(numRowGroups) + "\n\n";

    if (schema) {
        info += "<b>Schema:</b>\n";
        for (int i = 0; i < schema->num_fields(); ++i) {
            std::shared_ptr<arrow::Field> field = schema->field(i);
            info += QString("  - %1: %2\n")
                        .arg(QString::fromStdString(field->name()))
                        .arg(QString::fromStdString(field->type()->ToString()));
        }
    } else {
        info += "<b>Schema:</b> Not available\n";
    }

    m_infoTextEdit->setHtml(info.replace("\n", "<br>"));
}
