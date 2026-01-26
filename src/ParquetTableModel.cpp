#include "ParquetTableModel.h"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QtTypes>

#include "ArrowBackend.h" // For now, we hardcode this backend

// Undefine 'signals' macro from Qt to prevent conflict with arrow headers
#undef signals
#include <arrow/table.h>
#include <arrow/schema.h>
#include <arrow/array.h>
#include <arrow/type.h>
#include <arrow/array/array_binary.h>
#include <arrow/array/array_primitive.h>
#include <arrow/array/array_timestamp.h>
#include <arrow/scalar.h>

ParquetTableModel::ParquetTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      m_currentPageIndex(-1)
{
}

ParquetTableModel::~ParquetTableModel() {
    clearData();
}

int ParquetTableModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid() || !m_dataProvider) {
        return 0;
    }
    return m_dataProvider->getRowCount();
}

int ParquetTableModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid() || !m_dataProvider || !m_dataProvider->getSchema()) {
        return 0;
    }
    return m_dataProvider->getSchema()->num_fields();
}

QString ParquetTableModel::getColumnString(const std::shared_ptr<arrow::Array>& array, int64_t index) const  {
    if (array->IsNull(index)) {
        return "<NULL>";
    }
    auto str_array = std::static_pointer_cast<arrow::StringArray>(array);
    arrow::StringArray::offset_type length;
    const uint8_t *str = str_array->GetValue(index, &length);
    return QString::fromUtf8(reinterpret_cast<const char *>(str), static_cast<qsizetype>(length));
}

QString ParquetTableModel::getColumnLargeString(const std::shared_ptr<arrow::Array>& array, int64_t index) const {
    if (array->IsNull(index)) {
        return "<NULL>";
    }
    auto str_array = std::static_pointer_cast<arrow::LargeStringArray>(array);
    arrow::LargeStringArray::offset_type length;
    const uint8_t *str = str_array->GetValue(index, &length);
    return QString::fromUtf8(reinterpret_cast<const char *>(str), static_cast<qsizetype>(length));
}

QVariant ParquetTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole || !m_dataProvider) {
        return QVariant();
    }

    int row = index.row();
    int col = index.column();

    int targetPageIndex = row / PAGE_SIZE;
    int rowInPage = row % PAGE_SIZE;

    if (targetPageIndex != m_currentPageIndex) {
        if (!loadPage(targetPageIndex)) {
            return QVariant(); // Failed to load page
        }
    }

    if (!m_currentPage || rowInPage >= m_currentPage->num_rows()) {
        return QVariant();
    }

    std::shared_ptr<arrow::Array> column_array = m_currentPage->column(col)->chunk(0);
    if (!column_array || column_array->IsNull(rowInPage)) {
        return QVariant();
    }

    // Extract data based on Arrow type
    switch (column_array->type_id()) {
        case arrow::Type::BOOL:
            return std::static_pointer_cast<arrow::BooleanArray>(column_array)->Value(rowInPage);
        case arrow::Type::INT8:
            return std::static_pointer_cast<arrow::Int8Array>(column_array)->Value(rowInPage);
        case arrow::Type::INT16:
            return std::static_pointer_cast<arrow::Int16Array>(column_array)->Value(rowInPage);
        case arrow::Type::INT32:
            return std::static_pointer_cast<arrow::Int32Array>(column_array)->Value(rowInPage);
        case arrow::Type::INT64:
            return static_cast<qlonglong>(std::static_pointer_cast<arrow::Int64Array>(column_array)->Value(rowInPage));
        case arrow::Type::UINT8:
            return std::static_pointer_cast<arrow::UInt8Array>(column_array)->Value(rowInPage);
        case arrow::Type::UINT16:
            return std::static_pointer_cast<arrow::UInt16Array>(column_array)->Value(rowInPage);
        case arrow::Type::UINT32:
            return std::static_pointer_cast<arrow::UInt32Array>(column_array)->Value(rowInPage);
        case arrow::Type::UINT64:
            return static_cast<qulonglong>(std::static_pointer_cast<arrow::UInt64Array>(column_array)->Value(rowInPage));
        case arrow::Type::FLOAT:
            return std::static_pointer_cast<arrow::FloatArray>(column_array)->Value(rowInPage);
        case arrow::Type::DOUBLE:
            return std::static_pointer_cast<arrow::DoubleArray>(column_array)->Value(rowInPage);
        case arrow::Type::STRING:
            return getColumnString(column_array, rowInPage);
        case arrow::Type::LARGE_STRING:
            return getColumnLargeString(column_array, rowInPage);
        case arrow::Type::TIMESTAMP: {
            auto timestamp_array = std::static_pointer_cast<arrow::TimestampArray>(column_array);
            if (timestamp_array->IsNull(rowInPage)) return QVariant();
            int64_t value = timestamp_array->Value(rowInPage);
            auto unit = std::static_pointer_cast<arrow::TimestampType>(timestamp_array->type())->unit();
            qint64 ms_since_epoch;
            switch (unit) {
                case arrow::TimeUnit::MICRO: ms_since_epoch = value / 1000; break;
                case arrow::TimeUnit::NANO: ms_since_epoch = value / 1000000; break;
                case arrow::TimeUnit::MILLI: ms_since_epoch = value; break;
                case arrow::TimeUnit::SECOND: ms_since_epoch = value * 1000; break;
                default: return QString("Unsupported Timestamp Unit");
            }
            return QDateTime::fromMSecsSinceEpoch(ms_since_epoch);
        }
        default: {
            auto scalar = column_array->GetScalar(rowInPage);
            if (scalar.ok() && (*scalar)->is_valid) {
                return QString::fromStdString((*scalar)->ToString());
            }
            return QVariant();
        }
    }
}

QVariant ParquetTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (m_dataProvider && m_dataProvider->getSchema() && section < m_dataProvider->getSchema()->num_fields()) {
            return QString::fromStdString(m_dataProvider->getSchema()->field(section)->name());
        }
    } else if (role == Qt::DisplayRole && orientation == Qt::Vertical) {
        return section; // Row numbers
    }
    return QVariant();
}

bool ParquetTableModel::loadParquetFile(const QString &filePath) {
    clearData();

    if (!QFile::exists(filePath)) {
        qWarning() << "File does not exist:" << filePath;
        return false;
    }

    m_filePath = filePath;

    try {
        // For now, hardcode the ArrowBackend. This will be replaced by the BackendRouter.
        m_dataProvider = std::make_unique<ArrowBackend>();
        m_dataProvider->open(filePath.toStdString());
    } catch (const std::exception& e) {
        qWarning() << "Error loading file:" << e.what();
        m_dataProvider.reset();
        return false;
    }

    beginResetModel();
    endResetModel();

    return true;
}

QString ParquetTableModel::filePath() const {
    return m_filePath;
}

void ParquetTableModel::clearData() {
    beginResetModel();
    m_filePath.clear();
    m_dataProvider.reset();
    m_currentPageIndex = -1;
    m_currentPage.reset();
    endResetModel();
}

int ParquetTableModel::getTotalRows() const {
    return m_dataProvider ? m_dataProvider->getRowCount() : 0;
}

int ParquetTableModel::getNumRowGroups() const {
    // This is now backend-specific and might not be relevant to expose here.
    // We could add it to the IDataProvider interface if needed.
    return 0;
}

std::shared_ptr<arrow::Schema> ParquetTableModel::getSchema() const {
    return m_dataProvider ? m_dataProvider->getSchema() : nullptr;
}

bool ParquetTableModel::loadPage(int pageIndex) const {
    if (!m_dataProvider || pageIndex < 0) {
        return false;
    }
    
    m_currentPage = m_dataProvider->fetchPage(pageIndex);
    if (m_currentPage) {
        m_currentPageIndex = pageIndex;
        return true;
    }
    
    m_currentPageIndex = -1;
    return false;
}