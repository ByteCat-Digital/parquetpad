# Major Decisions for ParquetPad

This document outlines key architectural and implementation decisions made during the development of ParquetPad.

## 1. Core Technologies

*   **UI Framework:** Qt6 (specifically `qtbase` with `widgets` feature) was chosen for its cross-platform capabilities, mature widget set, and C++ native integration, aligning with the C++20 requirement.
*   **Parquet/Arrow Integration:** Apache Arrow C++ library was selected for reading Parquet files due to its high performance, memory efficiency, and direct support for Parquet format. The `arrow` and `parquet` components are used.
*   **Build System:** CMake is used for project configuration and cross-platform build generation.
*   **Dependency Management:** vcpkg in manifest mode (`vcpkg.json`) is used to manage Qt and Arrow dependencies, ensuring consistent builds across different environments and simplifying dependency acquisition.

## 2. Virtual Scrolling / Data Loading Strategy

*   **Requirement:** "only load 10000 parquet rows at a time" and "virtual scroll".
*   **Implementation:** A custom `ParquetTableModel` inheriting from `QAbstractTableModel` was implemented.
    *   It maintains a `BATCH_SIZE` of 10,000 rows.
    *   The `data()` method determines which batch a requested row belongs to.
    *   If the required batch is not currently loaded, `loadBatch()` is called.
    *   `loadBatch()` uses `parquet::arrow::FileReader::ReadTable(offset, num_rows, ...)` to efficiently read only the necessary range of rows from the Parquet file into an `arrow::Table`. This ensures that only a small portion of the potentially large Parquet file is held in memory at any given time, fulfilling the memory efficiency aspect of virtual scrolling.
    *   The `m_currentBatch` and `m_currentBatchIndex` members manage the currently loaded data batch.

## 3. File Opening

*   **Requirement:** "open from command line or through a menu".
*   **Implementation:**
    *   **Menu:** A "File -> Open..." action is provided in the `MainWindow` using `QFileDialog::getOpenFileName`.
    *   **Command Line:** `main.cpp` checks `argc > 1` and passes the first argument to `MainWindow::openFile()`, allowing users to specify a file path directly when launching the application.

## 4. File Information Dialog

*   **Requirement:** "File Information" dialog showing number of rows, schema, and number of row groups.
*   **Implementation:** A `FileInfoDialog` (inheriting from `QDialog`) was created.
    *   It takes the file path, total rows, number of row groups, and the Arrow `Schema` as input.
    *   This information is extracted from the `parquet::arrow::FileReader` in `ParquetTableModel::loadParquetFile()` and passed to the dialog.
    *   The schema is formatted for display, showing field names and their Arrow data types.

## 5. Installation and Distribution

*   **Requirement:** "install in CMake to install the application and all its dependencies in a single directory (e.g. don't want it to install to system path)".
*   **Implementation:**
    *   The `CMakeLists.txt` defines an `INSTALL_DIR` within the build directory.
    *   `install(TARGETS parquetpad ...)` is used to install the executable.
    *   `install(RUNTIME_DEPENDENCY_SET ...)` (CMake 3.21+) is utilized to collect and install runtime dependencies (DLLs/SOs) alongside the executable, creating a self-contained distribution. This is a cross-platform solution.
    *   Platform-specific tools like `windeployqt` (for Windows) and `macdeployqt` (for macOS) are conditionally used via `install(CODE ...)` to ensure all necessary Qt plugins and frameworks are bundled.

## 6. C++ Standard

*   **Requirement:** C++20.
*   **Implementation:** `set(CMAKE_CXX_STANDARD 20)` and `set(CMAKE_CXX_STANDARD_REQUIRED ON)` are set in `CMakeLists.txt`.

## 7. Lean and Mean Principle

*   The design prioritizes minimal dependencies and direct integration with Qt and Arrow.
*   The virtual scrolling mechanism is central to keeping memory footprint low for large files.
*   The UI is kept simple and functional, focusing on the core task of viewing Parquet data.
