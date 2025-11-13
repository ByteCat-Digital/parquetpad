cmake_minimum_required(VERSION 3.21)
project(parquetpad LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(DEFINED ENV{QT_DIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Arrow REQUIRED)
find_package(Parquet REQUIRED)

add_executable(parquetpad)

target_sources(parquetpad PRIVATE
    src/main.cpp
    src/MainWindow.cpp
    src/ParquetTableModel.cpp
    src/FileInfoDialog.cpp
)

target_include_directories(parquetpad PRIVATE src)

# Detect which Arrow/Parquet variant is available (shared or static)
if(TARGET Arrow::arrow_shared)
    message(STATUS "Found shared Arrow/Parquet libraries.")
    set(ARROW_TARGET Arrow::arrow_shared)
    set(PARQUET_TARGET Parquet::parquet_shared)
elseif(TARGET Arrow::arrow_static)
    message(STATUS "Found static Arrow/Parquet libraries.")
    set(ARROW_TARGET Arrow::arrow_static)
    set(PARQUET_TARGET Parquet::parquet_static)
else()
    message(FATAL_ERROR "Could not find Arrow/Parquet library targets (shared or static). Please check your vcpkg installation of Arrow with the Parquet feature.")
endif()

target_link_libraries(parquetpad PRIVATE
    Qt6::Widgets
    ${ARROW_TARGET}
    ${PARQUET_TARGET}
)

# --- Installation ---
# This ensures the application and its dependencies are installed into a single directory.
set(INSTALL_DIR "${CMAKE_BINARY_DIR}/install")
message(STATUS "Install directory: ${INSTALL_DIR}")

install(TARGETS parquetpad
    RUNTIME DESTINATION "${INSTALL_DIR}/bin"
    LIBRARY DESTINATION "${INSTALL_DIR}/lib"
    ARCHIVE DESTINATION "${INSTALL_DIR}/lib/static"
)

# This is the modern CMake way to bundle dependencies on all platforms.
# It requires CMake 3.21+
install(RUNTIME_DEPENDENCY_SET app_dependencies
    POST_EXCLUDE_REGEXES "^(api-ms-win-.*|lib[b-z].*\.so\.[0-9]+|libgcc_s\.so\.1|libstdc\+\+\.so\.6|libm\.so\.6|libc\.so\.6|libdl\.so\.2|libpthread\.so\.0|ld-linux-x86-64\.so\.2)$"
    DIRECTORIES
        "${Qt6_INSTALL_DIR}"
    DESTINATION "${INSTALL_DIR}/bin"
)

# For Windows, we need to run windeployqt to get Qt plugins and QML files
if(WIN32)
    # A more robust way is to find windeployqt and run it.
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt PATHS "${Qt6_INSTALL_DIR}/bin" NO_DEFAULT_PATH)
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        install(CODE "execute_process(COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" --dir \"${INSTALL_DIR}/bin\" --plugindir plugins)")
    else()
        message(WARNING "windeployqt not found. The installation might be incomplete.")
    endif()
endif()

# For macOS, we can use macdeployqt
if(APPLE)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt PATHS "${Qt6_INSTALL_DIR}/bin" NO_DEFAULT_PATH)
    if(MACDEPLOYQT_EXECUTABLE)
        message(STATUS "Found macdeployqt: ${MACDEPLOYQT_EXECUTABLE}")
        install(CODE "execute_process(COMMAND \"${MACDEPLOYQT_EXECUTABLE}\" \"${INSTALL_DIR}/bin/parquetpad.app\")")
    else()
        message(WARNING "macdeployqt not found. The installation might be incomplete.")
    endif()
endif()
