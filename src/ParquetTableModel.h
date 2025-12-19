#ifndef PARQUETTABLEMODEL_H
#define PARQUETTABLEMODEL_H


#include <QAbstractTableModel>
#include <QVector>
#include <QVariant>
#include <memory>

// Forward declarations for Arrow types
namespace arrow {
    class Table;
    class Schema;
    class Array;
}
namespace parquet {
    namespace arrow {
        class FileReader;
    }
}

class ParquetTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    explicit ParquetTableModel(QObject *parent = nullptr);
    ~ParquetTableModel() override;

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Custom methods
    bool loadParquetFile(const QString &filePath);
    void clearData();

    // Getters for file info
    QString filePath() const;
    int getTotalRows() const;
    int getNumRowGroups() const;
    std::shared_ptr<arrow::Schema> getSchema() const;
    std::shared_ptr<parquet::arrow::FileReader> getFileReader() const;

private:
    QString m_filePath;
    std::shared_ptr<parquet::arrow::FileReader> m_parquetFileReader;
    std::shared_ptr<arrow::Schema> m_schema;
    int m_totalRows;
    int m_numRowGroups;

    // Virtual scrolling / paging
    static constexpr int BATCH_SIZE = 10000; // Load 10,000 rows at a time
    mutable int m_currentBatchIndex; // Which batch is currently loaded (0-based)
    mutable std::shared_ptr<arrow::Table> m_currentBatch; // The currently loaded batch of data

    // Helper to load a specific batch
    bool loadBatch(int batchIndex) const;

    QString getColumnString(const std::shared_ptr<arrow::Array>& array, int64_t index) const;
    QString getColumnLargeString(const std::shared_ptr<arrow::Array>& array, int64_t index) const;
};

#endif // PARQUETTABLEMODEL_H
