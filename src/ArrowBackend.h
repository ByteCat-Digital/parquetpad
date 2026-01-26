#pragma once

#include "IDataProvider.h"
#include <parquet/arrow/reader.h>

class ArrowBackend : public IDataProvider {
public:
    ArrowBackend();
    ~ArrowBackend() override;

    void open(const std::string& path) override;
    void setQuery(const Query& query) override;
    std::shared_ptr<arrow::Schema> getSchema() override;
    std::shared_ptr<arrow::Table> fetchPage(int page) override;
    int64_t getRowCount() override;
    bool isRowCountApproximate() override;
    void cancel() override;

private:
    std::unique_ptr<parquet::arrow::FileReader> m_reader;
    std::shared_ptr<arrow::Schema> m_schema;
    int m_numRowGroups = 0;
};
