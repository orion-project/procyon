cmake_minimum_required(VERSION 3.16)

# Enable vcpkg manifest mode only when Qt is supplied by vcpkg. Configure with:
# cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DUSE_VCPKG_QT=ON
set(USE_VCPKG_QT OFF CACHE BOOL "Build with Qt6 and external dependencies installed by vcpkg")

if(USE_VCPKG_QT)
    set(VCPKG_MANIFEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_qt")
endif()

# Let static vcpkg builds use the matching MSVC runtime.
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()
if(VCPKG_TARGET_TRIPLET STREQUAL "x64-windows-static")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Prepare version number
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version/version.txt" APP_VER_FULL)
string(STRIP "${APP_VER_FULL}" APP_VER_FULL)
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" APP_VER_NUMERIC "${APP_VER_FULL}")
if(NOT APP_VER_NUMERIC)
    message(FATAL_ERROR "version/version.txt must begin with a semantic version")
endif()

# History: The original project file procyon.pro
# had been created by QtCreator 2017-08-17T13:11:44
# and ported to CMakeLists.txt by ChatGPT 2026-07-28 🤖
project(procyon VERSION "${APP_VER_NUMERIC}" LANGUAGES C CXX)

set(APP_VER_SHORT ${PROJECT_VERSION})
set(APP_VER_MAJOR ${PROJECT_VERSION_MAJOR})
set(APP_VER_MINOR ${PROJECT_VERSION_MINOR})
set(APP_VER_PATCH ${PROJECT_VERSION_PATCH})
string(TIMESTAMP APP_VER_DATE "%Y-%m-%d")
string(TIMESTAMP APP_VER_YEAR "%Y")

if (WIN32)
    configure_file(version/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT USE_VCPKG_QT)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_CURRENT_SOURCE_DIR}/bin>")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network PrintSupport Sql Svg Widgets)

# [hunspell](http://hunspell.github.io/) used for spell checking.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Hunspell REQUIRED)

# [hoedown](https://github.com/hoedown/hoedown) renders Markdown memos to HTML.
# Its source is fetched and built through the project's local vcpkg overlay port.
# It doesn't provice an official vcpkg port.
find_package(hoedown CONFIG REQUIRED)

# The CMake configuration downloads a checksum-pinned revision of the current
# [LibreOffice dictionaries](https://github.com/LibreOffice/dictionaries) source,
# extracts the English dictionary, and converts the Russian dictionary to UTF-8
# only when it is not already UTF-8. The dictionaries are then copied beside the built executable.
# Dictionary preparation is enabled by default during CMake configuration.
# To use an existing dictionary directory or configure without network access, pass `-DPREPARE_DICTIONARIES=OFF`.
option(PREPARE_DICTIONARIES "Download and prepare Hunspell dictionaries during configuration" ON)
if(PREPARE_DICTIONARIES)
    # Searches for an already installed Python interpreter and fails configuration if none is found
    # CMake needs python/python3 available on PATH (or supplied via Python3_EXECUTABLE)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    set(HUNSPELL_DICT_REVISION "f2ff99058268502bdcf4cad25c1ca2935ad8aa7d")
    set(HUNSPELL_DICT_ARCHIVE "${CMAKE_BINARY_DIR}/downloads/libreoffice-dictionaries-${HUNSPELL_DICT_REVISION}.tar.gz")
    set(HUNSPELL_DICT_DIR "${CMAKE_CURRENT_BINARY_DIR}/dicts")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/downloads")
    message(STATUS "Downloading hunspell dictionaries...")
    file(DOWNLOAD
        "https://github.com/LibreOffice/dictionaries/archive/${HUNSPELL_DICT_REVISION}.tar.gz"
        "${HUNSPELL_DICT_ARCHIVE}"
        EXPECTED_HASH SHA512=c3935525719a75077d7fa05378aabc0be2f8486c98fafb259043b831e16f8e939c46dceef9077458149a515bc0bfb962fd7f31da66773eb8d87083e733fee76f
        TLS_VERIFY ON
        STATUS HUNSPELL_DICT_DOWNLOAD_STATUS
        SHOW_PROGRESS
    )
    list(GET HUNSPELL_DICT_DOWNLOAD_STATUS 0 HUNSPELL_DICT_DOWNLOAD_CODE)
    if(NOT HUNSPELL_DICT_DOWNLOAD_CODE EQUAL 0)
        list(GET HUNSPELL_DICT_DOWNLOAD_STATUS 1 HUNSPELL_DICT_DOWNLOAD_ERROR)
        message(FATAL_ERROR "Unable to download Hunspell dictionaries: ${HUNSPELL_DICT_DOWNLOAD_ERROR}")
    endif()

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/deps/prepare_dict.py"
            --archive "${HUNSPELL_DICT_ARCHIVE}"
            --output-dir "${HUNSPELL_DICT_DIR}"
        RESULT_VARIABLE HUNSPELL_DICT_PREPARE_RESULT
        OUTPUT_VARIABLE HUNSPELL_DICT_PREPARE_OUTPUT
        ERROR_VARIABLE HUNSPELL_DICT_PREPARE_ERROR
    )
    if(NOT HUNSPELL_DICT_PREPARE_RESULT EQUAL 0)
        message(FATAL_ERROR "Unable to prepare Hunspell dictionaries:\n${HUNSPELL_DICT_PREPARE_OUTPUT}${HUNSPELL_DICT_PREPARE_ERROR}")
    endif()
endif()

set(PROJECT_SOURCES
    src/app.qrc
    src/main.cpp
    src/AppSettings.cpp
    src/core/Db.cpp src/core/Db.h
    src/core/FolderStore.cpp src/core/FolderStore.h
    src/core/MemoStore.cpp src/core/MemoStore.h
    src/core/MemoType.cpp src/core/MemoType.h
    src/core/SettingsManager.cpp src/core/SettingsManager.h
    src/core/SqlHelper.cpp src/core/SqlHelper.h
    src/editors/MarkdownMemoEditor.cpp
    src/editors/MemoEditor.cpp
    src/highlighter/EnotStorage.cpp
    src/highlighter/PhlManager.cpp
    src/MainWindow.cpp
    src/markdown/MarkdownHelper.cpp
    src/markdown/ori_html.c
    src/spellcheck/LangCodeAndNames.cpp
    src/spellcheck/Spellchecker.cpp
    src/spellcheck/TextEditSpellcheck.cpp
    src/tabs/AppSettingsTab.cpp
    src/tabs/CmdConsoleTab.cpp
    src/tabs/CssEditorTab.cpp
    src/tabs/GridViewMemoTab.cpp
    src/tabs/HelpTab.cpp
    src/tabs/MemoTab.cpp
    src/tabs/PhlEditorTab.cpp
    src/tabs/QssEditorTab.cpp
    src/tabs/SqlConsoleTab.cpp
    src/tabs/TextMemoTab.cpp
    src/tabs/TabHelpers.cpp
    src/TextEditHelpers.cpp
    src/widgets/MemoTextBrowser.cpp
    src/widgets/MemoTextEdit.cpp
    src/widgets/OpenTabsWidget.cpp src/widgets/OpenTabsWidget.h
    src/widgets/TreeWidget.cpp src/widgets/TreeWidget.h
)

if(WIN32)
    list(APPEND PROJECT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app.rc
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
    )
endif()

if(APPLE)
    list(APPEND PROJECT_SOURCES img/icon/main.icns)
    set_source_files_properties(img/icon/main.icns PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
endif()

add_subdirectory(orion)
set(LIB_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/orion/resources.qrc
)

qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${LIB_RESOURCES}
)

# Copy hunspell dictionaries beside the built executable.
if(PREPARE_DICTIONARIES)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${HUNSPELL_DICT_DIR}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/dicts"
        COMMENT "Copying Hunspell dictionaries next to ${PROJECT_NAME}"
    )
endif()

# Copy some DLL libraries beside the built executable.
if(WIN32 AND USE_VCPKG_QT)
    # Copy a dynamically linked library that CMake cannot discover through the
    # executable's ordinary link dependencies (for example, a Qt plugin dependency).
    function(deploy_runtime_target runtime_target description)
        if(NOT TARGET "${runtime_target}")
            message(FATAL_ERROR "Unknown runtime target: ${runtime_target}")
        endif()

        get_target_property(runtime_target_type "${runtime_target}" TYPE)
        if(runtime_target_type STREQUAL "SHARED_LIBRARY")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "$<TARGET_FILE:${runtime_target}>" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                COMMENT "Copying ${description} next to ${PROJECT_NAME}"
            )
        endif()
    endfunction()

    # Look up a vcpkg package and deploy the DLL represented by one of its targets.
    function(find_and_deploy_runtime_dependency package_name runtime_target description)
        find_package(${package_name} CONFIG REQUIRED)
        deploy_runtime_target("${runtime_target}" "${description}")
    endfunction()

    # Qt's CMake targets link the SVG module but do not deploy its DLL beside the executable.
    # Copy it for dynamically linked Windows builds so QSvgRenderer can load SVG resources at runtime.
    deploy_runtime_target(Qt6::Svg "Qt6Svg runtime library")

    # The vcpkg qsqlite driver depends on sqlite3.dll, but the driver is loaded as
    # a Qt plugin so CMake's dependency deployment does not discover that DLL.
    find_and_deploy_runtime_dependency(
        unofficial-sqlite3 unofficial::sqlite3::sqlite3 "SQLite runtime library")

    # The JPEG image plugin is loaded at runtime and, in turn, needs jpeg62.dll.
    # That indirect dependency is not found by CMake's executable dependency deployment.
    find_and_deploy_runtime_dependency(
        libjpeg-turbo libjpeg-turbo::jpeg "JPEG runtime library")

    # Deploy the Qt plugins loaded at runtime by Procyon.
    # Qt's normal dependency scanner only follows the executable's direct DLL dependencies,
    # so it misses these dynamically loaded modules and their qt.conf search path.
    set(QT_CONF "${CMAKE_CURRENT_BINARY_DIR}/qt.conf")
    file(GENERATE OUTPUT "${QT_CONF}" CONTENT "[Paths]\nPrefix = .\nPlugins = plugins\n")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${QT_CONF}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/qt.conf"
        COMMENT "Writing Qt plugin search paths for ${PROJECT_NAME}"
    )

    function(deploy_qt_plugin plugin_target plugin_dir)
        if(TARGET ${plugin_target})
            get_target_property(plugin_target_type ${plugin_target} TYPE)
            if(plugin_target_type STREQUAL "MODULE_LIBRARY")
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND "${CMAKE_COMMAND}" -E make_directory
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/${plugin_dir}"
                    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                        "$<TARGET_FILE:${plugin_target}>"
                        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/${plugin_dir}"
                    COMMENT "Deploying ${plugin_target}"
                )
            endif()
        else()
            message(WARNING "Unknown Qt plugin target ${plugin_target}")
        endif()
    endfunction()

    deploy_qt_plugin(Qt6::QWindowsIntegrationPlugin platforms)
    deploy_qt_plugin(Qt6::QSQLiteDriverPlugin sqldrivers)
    deploy_qt_plugin(Qt6::QSvgPlugin imageformats)
    deploy_qt_plugin(Qt6::QJpegPlugin imageformats)
    deploy_qt_plugin(Qt6::QGifPlugin imageformats)
    deploy_qt_plugin(Qt6::QSvgIconPlugin iconengines)
    deploy_qt_plugin(Qt6::QSchannelBackendPlugin tls)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    orion
    Hunspell::Hunspell
    hoedown::hoedown
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::PrintSupport
    Qt6::Sql
    Qt6::Svg
    Qt6::Widgets
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ENABLE_SPELLCHECK
    QT_DEPRECATED_WARNINGS
    QT_USE_QSTRINGBUILDER
    APP_VER="${APP_VER_FULL}"
    APP_VER_MAJOR=${APP_VER_MAJOR}
    APP_VER_MINOR=${APP_VER_MINOR}
    APP_VER_PATCH=${APP_VER_PATCH}
    APP_VER_YEAR=${APP_VER_YEAR}
    APP_VER_DATE="${APP_VER_DATE}"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.orion-project.procyon
    MACOSX_BUNDLE_BUNDLE_VERSION ${APP_VER_FULL}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VER_SHORT}
    MACOSX_BUNDLE_ICON_FILE main.icns
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(${PROJECT_NAME})
