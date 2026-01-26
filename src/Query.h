#pragma once

#include <string>
#include <vector>
#include <variant>

// Basic filter predicate
struct FilterPredicate {
    std::string column;
    std::string operation; // e.g., "=", ">", "contains"
    std::variant<std::string, double, int64_t> value;
};

// Sort description
struct SortKey {
    std::string column;
    bool ascending = true;
};

// Represents the complete query from the UI
struct Query {
    std::vector<FilterPredicate> filters;
    std::vector<SortKey> sort_keys;
    std::vector<std::string> projection; // columns to select

    bool isFiltered() const { return !filters.empty(); }
    bool isSorted() const { return !sort_keys.empty(); }
};
