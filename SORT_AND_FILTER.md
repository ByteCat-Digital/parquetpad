SORT_AND_FILTER proposals for ParquetPad

Context
- Current model uses ParquetFileReader + ReadRowGroups into an Arrow Table per batch.
- UI reads values row-by-row via QAbstractTableModel data() for display.
- Goal: fast filtering and sorting on large, compressed files without loading all data.

Key constraints
- Must remain responsive in QT.
- Must avoid full-table materialization.
- Must keep I/O sequential and leverage Parquet row group/column pruning.

Option A: Arrow Dataset + Scanner (+ optional Acero)
Summary
- Replace direct ParquetFileReader calls with Arrow Dataset (file or directory).
- Use Scanner with filter expressions and projection to push down to Parquet.
- Optionally execute with Acero for in-memory, vectorized filtering and sorting.

How it works
- Build a Dataset from file path.
- Create a ScanOptions with filter expression and requested columns.
- Use a RecordBatchReader / ScanBatches to stream batches.
- Maintain a "visible row index" mapping for filtered results and a cache of batches.

Filtering efficiency
- Dataset can push filters to Parquet row groups using statistics.
- Only required columns are read (projection pruning).
- Works well for equality/range filters on columns with stats.

Sorting efficiency
- Full sort of entire dataset is expensive; prefer:
  - Top-K sort for limited views.
  - External sort: read batches, sort chunks, spill to disk, merge.
  - If user sorts a column, you can do a two-phase plan:
    1) Scan only sort column + row_id to build an index (chunk sort/merge).
    2) Use row_id to fetch display columns on demand.
- Acero has sort support, but may require materializing into memory; use cautiously.

Degree of change
- Medium.
- Replace the current loadBatch path with Dataset scanning and a batch cache.
- Add a filter/sort controller that rebuilds scan options and invalidates cache.
- UI stays similar; QAbstractTableModel remains.

Pros
- Keeps Arrow-native path; minimal dependencies.
- Leverages Parquet row group stats and column pruning.
- Streaming model fits large files.

Cons
- Sorting at scale requires more engineering (external sort or index build).
- Filter results change row mapping; need an index/map layer.
- Acero sorting may still require significant RAM for large datasets.

Recommendation
- Adopt Dataset + Scanner for filtering and projection now.
- Implement sorting as "index-only" first, then external sort if needed.

Option B: Arrow Dataset + Acero compute plan (full query engine)
Summary
- Build a compute plan that scans, filters, projects, then sorts/limits.
- Use Acero for expression evaluation.

Degree of change
- Medium to high.
- Adds compute plan orchestration and output handling for incremental UI.

Pros
- Unified query pipeline; less custom code for filters.
- Good CPU efficiency from vectorized kernels.

Cons
- Sorting can still be memory heavy.
- More complex to integrate with Qt paging and random access.
- Risk of initial latency for large scans unless carefully chunked.

Recommendation
- Consider for more complex expression support (AND/OR, functions).
- Use only if you want SQL-like filters and are okay with more integration.

Option C: Embedded DuckDB backend (Arrow/Parquet scan)
Summary
- Use DuckDB as a local query engine to do filtering/sorting.
- DuckDB can read Parquet directly and stream results.

Degree of change
- High.
- Add DuckDB dependency, query layer, and result adapter to Qt model.

Pros
- SQL is a natural interface for filters/sorts.
- Fast external sorting and spill-to-disk are built-in.
- Mature optimizer; good performance on large datasets.

Cons
- Larger dependency footprint and different API surface.
- Results are not Arrow-native unless you use DuckDB Arrow API.
- Requires careful threading/integration to keep UI responsive.

Recommendation
- Best if you want SQL and robust large-scale sorting quickly.
- Overkill if you only need simple column filters and basic sort.

Option D: Minimal-change approach using current ParquetFileReader
Summary
- Keep ParquetFileReader; add filter predicates and a row index list.
- Build a bitmap/row index by scanning only filter columns.
- Use row index to render current view, fetching values on demand.

Degree of change
- Low to medium.

Pros
- Minimal refactor; easiest to implement.
- Can be fast for simple filters.

Cons
- Hard to push filters to row groups without Dataset.
- Sorting still expensive without external sort or index build.
- More custom code to maintain.

Recommendation
- Good stopgap; not the best long-term path.

Other ideas (worth considering)
1) Column stats + row-group skipping
   - Use row group stats to quickly skip reading irrelevant groups.
   - Works best for range filters on min/max-friendly columns.
   - Can be added on top of Dataset or current reader.

2) Background scan + progressive UI
   - Run scans in a background thread.
   - Provide progressive results in UI, update as more batches arrive.
   - Essential for long-running sorts or filters.

3) Persistent lightweight index
   - Build optional on-disk indices per file (e.g., sorted key -> row ids).
   - Speeds repeated filters/sorts on large datasets.
   - Cost: index build time and extra disk space.

4) Limit and preview-first
   - For sorting, default to "top N rows" with a cap.
   - Provide "sort all" as a slower, explicit action.
   - Keeps UX snappy and reduces full scans.

Implementation sketch (Dataset-first)
1) Replace loadBatch with Dataset scanner that yields RecordBatches.
2) Add a FilterState and SortState to the model.
3) Create a RowIndex structure:
   - For filtering: vector of row ids or batch+offset pairs.
   - For sorting: store sorted row ids built from sort key columns.
4) Add a batch cache (LRU) for display columns.
5) Update data() to translate row index -> batch -> value.

Degree of change summary
- Dataset + Scanner: medium.
- Dataset + Acero: medium-high.
- DuckDB: high.
- Minimal ParquetFileReader changes: low-medium.

Suggested path
1) Short-term: Dataset + Scanner for filtering, with batch cache.
2) Mid-term: index-only sorting using sort key columns, plus top-N.
3) Long-term: external sort (Arrow compute) or DuckDB if SQL is desired.
