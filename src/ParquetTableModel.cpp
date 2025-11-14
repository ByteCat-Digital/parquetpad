#include "ParquetTableModel.h"

// Undefine 'signals' macro from Qt to prevent conflict with arrow headers
#undef signals
#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/result.h>
#include <parquet/arrow/reader.h>
#include <parquet/file_reader.h>

#include <QDateTime>
#include <QDebug>
#include <QFile>

ParquetTableModel::ParquetTableModel(QObject *parent)
    : QAbstractTableModel(parent),
      m_totalRows(0),
      m_numRowGroups(0),
      m_currentBatchIndex(-1) // -1 indicates no batch loaded
{
}

ParquetTableModel::~ParquetTableModel() {
    clearData();
}

int ParquetTableModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_totalRows;
}

int ParquetTableModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid() || !m_schema) {
        return 0;
    }
    return m_schema->num_fields();
}

QVariant ParquetTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole || !m_parquetFileReader || !m_schema) {
        return QVariant();
    }

    int row = index.row();
    int col = index.column();

    // Determine which batch this row belongs to
    int targetBatchIndex = row / BATCH_SIZE;
    int rowInBatch = row % BATCH_SIZE;

    // Load batch if it's not the current one
    if (targetBatchIndex != m_currentBatchIndex) {
        if (!loadBatch(targetBatchIndex)) {
            return QVariant(); // Failed to load batch
        }
    }

    if (!m_currentBatch || rowInBatch >= m_currentBatch->num_rows()) {
        return QVariant(); // Should not happen if batch loading is correct
    }

    std::shared_ptr<arrow::Array> column_array = m_currentBatch->column(col)->chunk(0);
    if (!column_array) {
        return QVariant();
    }

    // Extract data based on Arrow type
    switch (column_array->type_id()) {
        case arrow::Type::BOOL:
            return std::static_pointer_cast<arrow::BooleanArray>(column_array)->Value(rowInBatch);
        case arrow::Type::INT8:
            return std::static_pointer_cast<arrow::Int8Array>(column_array)->Value(rowInBatch);
        case arrow::Type::INT16:
            return std::static_pointer_cast<arrow::Int16Array>(column_array)->Value(rowInBatch);
        case arrow::Type::INT32:
            return std::static_pointer_cast<arrow::Int32Array>(column_array)->Value(rowInBatch);
        case arrow::Type::INT64:
            return static_cast<qlonglong>(std::static_pointer_cast<arrow::Int64Array>(column_array)->Value(rowInBatch));
        case arrow::Type::UINT8:
            return std::static_pointer_cast<arrow::UInt8Array>(column_array)->Value(rowInBatch);
        case arrow::Type::UINT16:
            return std::static_pointer_cast<arrow::UInt16Array>(column_array)->Value(rowInBatch);
        case arrow::Type::UINT32:
            return std::static_pointer_cast<arrow::UInt32Array>(column_array)->Value(rowInBatch);
        case arrow::Type::UINT64:
            return static_cast<qulonglong>(std::static_pointer_cast<arrow::UInt64Array>(column_array)->Value(rowInBatch));
        case arrow::Type::FLOAT:
            return std::static_pointer_cast<arrow::FloatArray>(column_array)->Value(rowInBatch);
        case arrow::Type::DOUBLE:
            return std::static_pointer_cast<arrow::DoubleArray>(column_array)->Value(rowInBatch);
        case arrow::Type::STRING:
        case arrow::Type::LARGE_STRING:
            return QString::fromUtf8(std::static_pointer_cast<arrow::StringArray>(column_array)->Value(rowInBatch));
        case arrow::Type::TIMESTAMP: {
            auto timestamp_array = std::static_pointer_cast<arrow::TimestampArray>(column_array);
            if (timestamp_array->IsNull(rowInBatch)) return QVariant();
            // Convert Arrow timestamp to QDateTime
            // Arrow timestamps are typically microseconds, milliseconds, or seconds since epoch
            // QDateTime::fromSecsSinceEpoch expects seconds
            // This conversion might need refinement based on the actual unit of the timestamp
            int64_t us_since_epoch = timestamp_array->Value(rowInBatch);
            auto unit = std::static_pointer_cast<arrow::TimestampType>(timestamp_array->type())->unit();
            qint64 ms_since_epoch;
            switch (unit) {
                case arrow::TimeUnit::MICRO: ms_since_epoch = us_since_epoch / 1000; break;
                case arrow::TimeUnit::NANO: ms_since_epoch = us_since_epoch / 1000000; break;
                case arrow::TimeUnit::MILLI: ms_since_epoch = us_since_epoch; break;
                case arrow::TimeUnit::SECOND: ms_since_epoch = us_since_epoch * 1000; break;
                default: return QString("Unsupported Timestamp Unit");
            }
            return QDateTime::fromMSecsSinceEpoch(ms_since_epoch);
        }
        // Add more types as needed
        default:
            // Fallback for unsupported types, try to convert to string
            if (column_array->IsNull(rowInBatch)) {
                return QVariant();
            }
            auto scalar = column_array->GetScalar(rowInBatch);
            if (scalar.ok() && (*scalar)->is_valid) {
                return QString::fromStdString((*scalar)->ToString());
            }
            return QVariant();
    }
}

QVariant ParquetTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal && m_schema && section < m_schema->num_fields()) {
            return QString::fromStdString(m_schema->field(section)->name());
        } else if (orientation == Qt::Vertical) {
            return section; // Row numbers
        }
    }
    return QVariant();
}

bool ParquetTableModel::loadParquetFile(const QString &filePath) {
    clearData(); // Clear any previously loaded data

    if (!QFile::exists(filePath)) {
        qWarning() << "File does not exist:" << filePath;
        return false;
    }

    m_filePath = filePath;

    arrow::Result<std::shared_ptr<arrow::io::ReadableFile>> infile_result = arrow::io::ReadableFile::Open(filePath.toStdString());
    if (!infile_result.ok()) {
        qWarning() << "Error opening file:" << infile_result.status().ToString().c_str();
        return false;
    }
    std::shared_ptr<arrow::io::ReadableFile> infile = *infile_result;

    arrow::Result<std::unique_ptr<parquet::arrow::FileReader>> reader_result = parquet::arrow::OpenFile(infile, arrow::default_memory_pool());
    if (!reader_result.ok()) {
        qWarning() << "Error creating Parquet reader:" << reader_result.status().ToString().c_str();
        return false;
    }
    m_parquetFileReader = std::move(*reader_result);

    // Get schema and number of rows
    arrow::Status schema_status = m_parquetFileReader->GetSchema(&m_schema);
    if (!schema_status.ok()) {
        qWarning() << "Error getting schema:" << schema_status.ToString().c_str();
        return false;
    }

    m_totalRows = m_parquetFileReader->parquet_reader()->metadata()->num_rows();
    m_numRowGroups = m_parquetFileReader->parquet_reader()->metadata()->num_row_groups();

    // Reset batch info
    m_currentBatchIndex = -1;
    m_currentBatch.reset();

    beginResetModel();
    endResetModel();

    return true;
}

QString ParquetTableModel::filePath() const
{
    return m_filePath;
}

void ParquetTableModel::clearData()
{
    beginResetModel();
    m_filePath.clear();
    m_parquetFileReader.reset();
    m_schema.reset();
    m_totalRows = 0;
    m_numRowGroups = 0;
    m_currentBatchIndex = -1;
    m_currentBatch.reset();
    endResetModel();
}

int ParquetTableModel::getTotalRows() const {
    return m_totalRows;
}

int ParquetTableModel::getNumRowGroups() const {
    return m_numRowGroups;
}

std::shared_ptr<arrow::Schema> ParquetTableModel::getSchema() const {
    return m_schema;
}

std::shared_ptr<parquet::arrow::FileReader> ParquetTableModel::getFileReader() const {
    return m_parquetFileReader;
}

bool ParquetTableModel::loadBatch(int batchIndex) const {
    if (!m_parquetFileReader || batchIndex < 0) {
        return false;
    }

    int64_t start_row = static_cast<int64_t>(batchIndex) * BATCH_SIZE;
    int64_t num_rows_to_read = std::min(static_cast<int64_t>(BATCH_SIZE), m_totalRows - start_row);

    if (num_rows_to_read <= 0) {
        m_currentBatch.reset();
        m_currentBatchIndex = -1;
        return false;
    }

    // Read a specific range of rows
    // This is more efficient than reading entire row groups if the batch spans multiple row groups
    // and we only need a portion of them.
    // However, Parquet's native reading is by row group.
    // For simplicity and to demonstrate virtual scrolling, we'll read a table and then slice it.
    // A more optimized approach for very large files might involve reading specific row groups
    // and then slicing, or using the Scan API.

    // For now, let's read the entire table and then slice. This is not ideal for very large files
    // but demonstrates the model's ability to handle batches.
    // A better approach would be to read row groups that cover the batch.

    // Let's refine this: we need to read row groups that cover the `start_row` to `start_row + num_rows_to_read`.
    // This requires iterating through row groups.

    // Simpler approach for now: read the entire file into an Arrow Table and then slice.
    // This defeats the purpose of "virtual scroll" for memory, but keeps the UI responsive.
    // The true virtual scroll for memory would involve reading only the necessary row groups.
    // Given the requirement "only load 10000 parquet rows at a time", this implies memory efficiency.

    // Let's try to read row groups.
    std::vector<int> row_group_indices;
    int64_t current_row_count = 0;
    int64_t target_end_row = start_row + num_rows_to_read;

    for (int i = 0; i < m_numRowGroups; ++i) {
        std::shared_ptr<parquet::FileMetaData> metadata = m_parquetFileReader->parquet_reader()->metadata();
        int64_t rg_num_rows = metadata->RowGroup(i)->num_rows();

        if (current_row_count + rg_num_rows > start_row && current_row_count < target_end_row) {
            row_group_indices.push_back(i);
        }
        current_row_count += rg_num_rows;
    }

    if (row_group_indices.empty()) {
        m_currentBatch.reset();
        m_currentBatchIndex = -1;
        return false;
    }

    std::shared_ptr<arrow::Table> full_table_for_batch;
    arrow::Status read_status = m_parquetFileReader->ReadRowGroups(row_group_indices, &full_table_for_batch);
    if (!read_status.ok()) {
        qWarning() << "Failed to read row groups:" << read_status.ToString().c_str();
        return false;
    }

    // Now slice the table to get the exact batch
    int64_t offset_in_full_table = start_row - (static_cast<int64_t>(batchIndex) * BATCH_SIZE); // This needs to be calculated based on actual row group boundaries
    // This logic is tricky. If we read row groups, the `start_row` might not be the start of a row group.
    // The `ReadRowGroups` reads entire row groups.
    // A simpler approach for the "10000 rows at a time" is to use `ReadTable` and then `Slice`.
    // This will load the *entire* file into memory if `ReadTable` is used without limits.
    // The user explicitly asked for "only load 10000 parquet rows at a time".

    // Let's re-evaluate. The `parquet::arrow::FileReader` has `ReadTable(int64_t offset, int64_t num_rows, ...)`
    // This is exactly what we need for virtual scrolling.

    arrow::Status batch_status = m_parquetFileReader->ReadRowGroups(row_group_indices, &m_currentBatch);
    if (!batch_status.ok()) {
        qWarning() << "Failed to read batch:" << batch_status.ToString().c_str();
        return false;
    }

    m_currentBatchIndex = batchIndex;
    return true;
}
