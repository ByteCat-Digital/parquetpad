#include "ArrowBackend.h"

// Undefine 'signals' macro from Qt to prevent conflict with arrow headers
#undef signals
#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>
#include <parquet/file_reader.h>

constexpr int PAGE_SIZE = 10000;

ArrowBackend::ArrowBackend() = default;
ArrowBackend::~ArrowBackend() = default;

void ArrowBackend::open(const std::string& path) {
    arrow::Result<std::shared_ptr<arrow::io::ReadableFile>> infile_result = arrow::io::ReadableFile::Open(path);
    if (!infile_result.ok()) {
        // In a real app, we'd propagate this error
        throw std::runtime_error("Failed to open file: " + infile_result.status().ToString());
    }
    std::shared_ptr<arrow::io::ReadableFile> infile = *infile_result;

    arrow::Result<std::unique_ptr<parquet::arrow::FileReader>> reader_result = parquet::arrow::OpenFile(infile, arrow::default_memory_pool());
    if (!reader_result.ok()) {
        throw std::runtime_error("Failed to create Parquet reader: " + reader_result.status().ToString());
    }
    m_reader = std::move(*reader_result);

    arrow::Status schema_status = m_reader->GetSchema(&m_schema);
    if (!schema_status.ok()) {
        throw std::runtime_error("Failed to get schema: " + schema_status.ToString());
    }

    if (m_reader->parquet_reader() && m_reader->parquet_reader()->metadata()) {
        m_numRowGroups = m_reader->parquet_reader()->metadata()->num_row_groups();
    } else {
        m_numRowGroups = 0;
    }
}

void ArrowBackend::setQuery(const Query& query) {
    // ArrowBackend does not support filtering or sorting.
    // This method is a no-op but required by the interface.
}

std::shared_ptr<arrow::Schema> ArrowBackend::getSchema() {
    return m_schema;
}

std::shared_ptr<arrow::Table> ArrowBackend::fetchPage(int page) {
    if (!m_reader || page < 0) {
        return nullptr;
    }

    int64_t start_row = static_cast<int64_t>(page) * PAGE_SIZE;
    if (start_row >= getRowCount()) {
        return nullptr;
    }

    int64_t num_rows_to_read = std::min(static_cast<int64_t>(PAGE_SIZE), getRowCount() - start_row);

    std::vector<int> row_group_indices;
    int64_t current_row_count = 0;
    int64_t target_end_row = start_row + num_rows_to_read;
    auto metadata = m_reader->parquet_reader()->metadata();

    for (int i = 0; i < m_numRowGroups; ++i) {
        int64_t rg_num_rows = metadata->RowGroup(i)->num_rows();
        if (current_row_count + rg_num_rows > start_row && current_row_count < target_end_row) {
            row_group_indices.push_back(i);
        }
        current_row_count += rg_num_rows;
    }

    if (row_group_indices.empty()) {
        return nullptr;
    }

    std::shared_ptr<arrow::Table> table;
    arrow::Status read_status = m_reader->ReadRowGroups(row_group_indices, &table);
    if (!read_status.ok()) {
        // In a real app, propagate this error
        return nullptr;
    }
    
    // The previous logic for slicing was incorrect.
    // We need to calculate the offset within the first row group.
    int64_t offset_in_table = 0;
    current_row_count = 0;
    for(int i = 0; i < row_group_indices[0]; ++i) {
        current_row_count += metadata->RowGroup(i)->num_rows();
    }
    offset_in_table = start_row - current_row_count;

    return table->Slice(offset_in_table, num_rows_to_read);
}

int64_t ArrowBackend::getRowCount() {
    if (!m_reader || !m_reader->parquet_reader()->metadata()) {
        return 0;
    }
    return m_reader->parquet_reader()->metadata()->num_rows();
}

bool ArrowBackend::isRowCountApproximate() {
    return false; // For ArrowBackend, the count is always exact.
}

void ArrowBackend::cancel() {
    // No-op for this synchronous backend.
}
