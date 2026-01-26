#ifndef PARQUETTABLEMODEL_H
#define PARQUETTABLEMODEL_H

#include "IDataProvider.h"
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
    int getNumRowGroups() const; // This might be deprecated or changed
    std::shared_ptr<arrow::Schema> getSchema() const;

private:
    QString m_filePath;
    std::unique_ptr<IDataProvider> m_dataProvider;

    // Caching
    static constexpr int PAGE_SIZE = 10000;
    mutable int m_currentPageIndex;
    mutable std::shared_ptr<arrow::Table> m_currentPage;

    // Helper to load a specific page
    bool loadPage(int pageIndex) const;

    QString getColumnString(const std::shared_ptr<arrow::Array>& array, int64_t index) const;
    QString getColumnLargeString(const std::shared_ptr<arrow::Array>& array, int64_t index) const;
};

#endif // PARQUETTABLEMODEL_H
