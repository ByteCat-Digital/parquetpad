DUCKDB proposal for ParquetPad (filtering + sorting)

Goal
- Add fast filtering and sorting on Parquet data that may not fit in RAM.
- Keep UI responsive, minimize memory use, and avoid full materialization.

Key idea
- Use DuckDB as the query engine; read Parquet directly and stream/limit results.
- Drive the Qt table model from DuckDB query results in pages.

Can we do LIMIT + OFFSET?
- Yes. DuckDB supports LIMIT and OFFSET in SELECT queries.
- This enables page-based loading as long as ordering is deterministic.

About total filtered rows
- Exact counts are expensive because they require a full scan.
- Options:
  - Show ?unknown? initially and compute COUNT(*) in the background.
  - Show a running estimate based on sampled row groups or partial scan.
  - Provide a ?Compute total rows? action when the user needs it.

High-level architecture
+--------------------+        +-------------------+
| Qt UI + Model      | <----> | DuckDB connection |
| (paging)           |        | (query planner)   |
+--------------------+        +-------------------+
                                      |
                                      v
                               Parquet file(s)

Data flow
1) Open Parquet file -> create DuckDB connection.
2) Create a view/table binding to Parquet:
   - CREATE VIEW data AS SELECT * FROM read_parquet('path');
3) When filter/sort changes, build SQL:
   - SELECT <columns> FROM data
     WHERE <filter_expr>
     ORDER BY <sort_expr>
     LIMIT <page_size> OFFSET <page_start>;
4) Fetch results into Arrow or row vectors; fill Qt model.
5) Cache pages (LRU) to keep scrolling fast.

Sorting and paging
- Sorting must be stable for paging to be deterministic.
- Include a tie-breaker in ORDER BY (e.g., a rowid or all key columns).
- If no natural unique column, create a synthetic row id:
  - SELECT *, row_number() OVER () AS __rowid FROM read_parquet(...)
  - Use ORDER BY <user_sort>, __rowid
- DuckDB will spill to disk for large sorts, keeping memory bounded.

Filtering
- Translate UI filter inputs into SQL predicates.
- For multiple column filters, combine with AND/OR as needed.
- DuckDB can push filters down into Parquet scans when possible.

Handling large results
- Use LIMIT/OFFSET paging to avoid holding all rows in memory.
- Cache recent pages in memory for smooth scrolling.
- Run queries asynchronously to avoid blocking the UI thread.

Proposed Qt model changes
- Replace ParquetFileReader usage with a DuckDB-backed data provider.
- Add states:
  - FilterState (per-column or SQL expression)
  - SortState (column + direction + optional tie-breaker)
  - PagingState (page size, current page, total rows known/unknown)
- data() uses cached page data; cache miss triggers async query.

Threading model
- Worker thread executes SQL and returns rows to UI thread.
- Cancel in-flight queries when filters/sorts change.
- Debounce rapid filter changes to avoid thrashing.

Total row count strategy
Option A: background COUNT(*)
- After filter changes, run:
  - SELECT COUNT(*) FROM data WHERE <filter_expr>;
- Update UI when count arrives.

Option B: estimate first, exact later
- Fast estimate from sample:
  - SELECT COUNT(*) FROM data USING SAMPLE <ratio> WHERE <filter_expr>;
- Scale to estimate total; refine with full count in background.

Option C: no count
- UI shows ?N+? style (?showing 1?100 of many?).

Degree of change
- High.
- New DuckDB dependency and query layer.
- Replace Arrow dataset read path in ParquetTableModel.
- New paging/cache + async query infrastructure.

Pros
- SQL-based filters and sorts are expressive.
- DuckDB handles external sorting and spilling automatically.
- Supports large datasets with bounded memory usage.
- Easy to extend with computed columns and aggregations.

Cons
- New dependency and build integration work.
- Query orchestration and threading complexity.
- Need to maintain SQL translation for filters.

Implementation sketch
1) Add DuckDB dependency (vcpkg or conan).
2) Introduce DuckDbDataSource class:
   - open(file_path)
   - setFilter(filter_expr)
   - setSort(sort_expr)
   - fetchPage(offset, limit) -> rows
   - countRows(optional)
3) Update ParquetTableModel:
   - hold DuckDbDataSource
   - map row index -> page -> row
   - implement async fetch + cache
4) Add UI filter/sort controls and wire them to model.
5) Add background COUNT(*) and update rowCount when finished.

Suggested first milestone
- Minimal SQL filters (single column equals/contains/range).
- Single-column sort with tie-breaker.
- LIMIT/OFFSET paging with 10k rows per page.
- No total count or compute count in background.

Open questions to resolve
- Which columns are unique enough to use as a tie-breaker?
- Is it acceptable to compute row_number() for every query (can be costly)?
- Do we need multi-file datasets or single-file only?
