#pragma once

#include "Query.h"
#include <arrow/table.h>
#include <memory>

class IDataProvider {
public:
    virtual ~IDataProvider() = default;

    virtual void open(const std::string& path) = 0;
    virtual void setQuery(const Query& query) = 0;
    virtual std::shared_ptr<arrow::Schema> getSchema() = 0;
    virtual std::shared_ptr<arrow::Table> fetchPage(int page) = 0;
    virtual int64_t getRowCount() = 0;
    virtual bool isRowCountApproximate() = 0;
    virtual void cancel() = 0;
};
