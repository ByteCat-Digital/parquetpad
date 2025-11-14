cmake_minimum_required(VERSION 3.21)
project(parquetpad VERSION 1.0.0 LANGUAGES CXX)

# Configure version.h
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
    @ONLY
)

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

add_executable(parquetpad WIN32)

if(WIN32)
    target_sources(parquetpad PRIVATE src/parquetpad.rc)
endif()

target_sources(parquetpad PRIVATE
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ParquetTableModel.h
    src/ParquetTableModel.cpp
    src/FileInfoDialog.h
    src/FileInfoDialog.cpp
    src/AboutDialog.h
    src/AboutDialog.cpp
    src/resources.qrc
)

target_include_directories(parquetpad PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}")

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
# This section sets up the installation rules for the project.
# CPack will use these rules to create packages.

# The executable and its dependencies will be installed in the same directory.
set(INSTALL_BIN_DIR ".")

install(TARGETS parquetpad
    RUNTIME DESTINATION "${INSTALL_BIN_DIR}"
    BUNDLE DESTINATION "."
)

# Manually install Arrow and Parquet DLLs
if(WIN32 AND TARGET Arrow::arrow_shared)
    install(FILES
        "$<TARGET_FILE:Arrow::arrow_shared>"
        "$<TARGET_FILE:Parquet::parquet_shared>"
        DESTINATION "${INSTALL_BIN_DIR}"
    )
endif()

# Platform-specific deployment steps
if(WIN32)
    # On Windows, use windeployqt to gather Qt plugins, translations, etc.
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt PATHS "${Qt6_INSTALL_DIR}/bin" NO_DEFAULT_PATH)
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        install(CODE "
            message(STATUS \"Running windeployqt...\")
            execute_process(
                COMMAND \"${WINDEPLOYQT_EXECUTABLE}\"
                    --dir \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${INSTALL_BIN_DIR}\"
                    --plugindir \"plugins\"
                    \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${INSTALL_BIN_DIR}/${PROJECT_NAME}.exe\"
                WORKING_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${INSTALL_BIN_DIR}\"
                RESULT_VARIABLE result
            )
            if(NOT result EQUAL 0)
                message(FATAL_ERROR \"windeployqt failed with exit code ${result}\")
            endif()
        ")
    else()
        message(WARNING "windeployqt not found. The Windows installation might be incomplete.")
    endif()
elseif(APPLE)
    # On macOS, macdeployqt is used to create a proper .app bundle.
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt PATHS "${Qt6_INSTALL_DIR}/bin" NO_DEFAULT_PATH)
    if(MACDEPLOYQT_EXECUTABLE)
        message(STATUS "Found macdeployqt: ${MACDEPLOYQT_EXECUTABLE}")
        install(CODE "
            message(STATUS \"Running macdeployqt...\")
            execute_process(
                COMMAND \"${MACDEPLOYQT_EXECUTABLE}\" \"${CMAKE_INSTALL_PREFIX}/parquetpad.app\"
                RESULT_VARIABLE result
            )
            if(NOT result EQUAL 0)
                message(FATAL_ERROR \"macdeployqt failed with exit code ${result}\")
            endif()
        " COMPONENT Applications)
    else()
        message(WARNING "macdeployqt not found. The macOS installation might be incomplete.")
    endif()
endif()


# --- CPack Configuration ---
# This section configures CPack for creating installers and packages.

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple Parquet file viewer.")
set(CPACK_PACKAGE_VENDOR "ByteCat Digital (Pty) Ltd")
set(CPACK_PACKAGE_CONTACT "johan@bytecat.co.za")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")

    # NSIS specific settings
    set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME} ${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_INSTALLER_ICON "${CMAKE_SOURCE_DIR}/assets/icons/cat_windows_icon.ico")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/icons/cat_windows_icon.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/assets/icons/cat_windows_icon.ico")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut \\\"$DESKTOP\\\\${CPACK_PACKAGE_NAME}.lnk\\\" \\\"$INSTDIR\\\\${INSTALL_BIN_DIR}\\\\parquetpad.exe\\\"")
    set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME}")
    set(CPACK_NSIS_MUI_FINISHPAGE_RUN "$INSTDIR\\\\${INSTALL_BIN_DIR}\\\\parquetpad.exe")

elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    # For a professional look on macOS, you might want to create a background image for the DMG.
    # set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/assets/dmg_background.png")
    set(CPACK_DMG_WINDOW_POSITION "100 100")
    set(CPACK_DMG_WINDOW_SIZE "500 320")
    set(CPACK_DMG_ICON_SIZE "80")

else() # Linux
    set(CPACK_GENERATOR "TGZ")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${PROJECT_NAME}")
endif()

include(CPack)
