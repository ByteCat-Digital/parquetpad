HYBRID proposal for ParquetPad (Arrow + Acero + DuckDB)

Goal
- Keep the fast, low-dependency Arrow path for simple viewing.
- Use Arrow+Acero for filtering without full SQL engine overhead.
- Use DuckDB only when sorting is requested (with or without filtering).
- Minimize user-visible latency, memory use, and code duplication.

Design summary
- Introduce a BackendRouter that selects one of three backends:
  1) ArrowBackend: raw scan + paging (no filter, no sort)
  2) ArrowAceroBackend: filter-only queries
  3) DuckDbBackend: any query with sort
- All backends expose a common DataProvider interface.

Routing rules
- If filter is empty AND sort is empty -> ArrowBackend
- If filter is non-empty AND sort is empty -> ArrowAceroBackend
- If sort is non-empty (regardless of filter) -> DuckDbBackend

Why this split
- ArrowBackend is already fast for sequential paging.
- Acero offers efficient, vectorized filtering and Parquet pushdown.
- Sorting at scale is hard without external sort; DuckDB handles it.

Common DataProvider interface
- open(file_path)
- setSchema(selected_columns)
- setFilter(filter_expr)
- setSort(sort_expr)
- fetchPage(offset, limit) -> rows
- rowCount() -> known/unknown + value
- cancel()

Backend behaviors

1) ArrowBackend
- Reads row groups via ParquetFileReader or Arrow Dataset scanner.
- No filtering or sorting.
- Paging via batch size; minimal memory.
- rowCount() returns total rows from metadata.

2) ArrowAceroBackend
- Build Arrow Dataset Scanner + Acero filter expression.
- Projection pruning for selected columns.
- Streaming RecordBatchReader feeding the cache.
- No global sort; preserves natural file order.
- rowCount() is unknown until full scan completes; update asynchronously.

3) DuckDbBackend
- Load Parquet with read_parquet().
- Apply filters + ORDER BY + LIMIT/OFFSET for paging.
- Use tie-breaker in ORDER BY for deterministic paging.
- rowCount() via COUNT(*) in background.

Switching and caching strategy
- On query change, BackendRouter chooses backend and resets caches.
- Maintain per-backend cache instances to allow quick switching back.
- Cancel in-flight scans when route changes.

UI implications
- Sorting triggers a visible ?querying with DuckDB? status.
- Filter-only shows ?Arrow/Acero? status.
- Provide user option to force DuckDB if they want SQL filters.

Pros
- Minimal changes for simple usage.
- Best performance path for each query type.
- Keeps dependencies lean for common scenarios.

Cons
- More complex architecture and testing matrix.
- Need to keep consistent semantics across backends.
- Different row order semantics between Arrow and DuckDB.

Degree of change
- High overall, but isolated by the backend interface.
- ArrowBackend can reuse existing code with light refactoring.
- ArrowAceroBackend adds Dataset + filter expression translation.
- DuckDbBackend adds a new dependency and query path.

Implementation sketch
1) Extract ParquetTableModel data access into IDataProvider.
2) Implement ArrowBackend using existing reader/batch logic.
3) Implement ArrowAceroBackend (Dataset + Acero filter pipeline).
4) Implement DuckDbBackend with SQL query paging.
5) Add BackendRouter to pick provider by query type.
6) Add unified filter/sort expression builder and validator.
7) Add async query execution with cancellation tokens.

Testing strategy
- Golden file tests with identical filter results across backends.
- Sorting correctness tests (stable ordering with tie-breaker).
- Performance tests on large files (filter-only vs sort+filter).

Suggested rollout
- Phase 1: Interface + ArrowBackend refactor.
- Phase 2: ArrowAceroBackend (filter-only).
- Phase 3: DuckDbBackend (sorting).
- Phase 4: UX polish (progress, cancel, status, count estimation).

Open questions
- Do you want SQL filters when in DuckDB, or keep the same UI filter builder?
- Are there unique columns available for deterministic ORDER BY tie-breakers?
- Should users be able to force a backend (advanced settings)?
