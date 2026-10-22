cmake_minimum_required(VERSION 3.21)
project(MotoTool VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(MOTOTOOL_QT_MAJOR "5" CACHE STRING "Qt major version for diagnostics (5 or 6)")
set_property(CACHE MOTOTOOL_QT_MAJOR PROPERTY STRINGS 5 6)
option(MOTOTOOL_BUILD_LEGACY "Compile the unvalidated legacy servicing client" OFF)
include(CTest)

if(NOT MOTOTOOL_QT_MAJOR MATCHES "^[56]$")
    message(FATAL_ERROR "MOTOTOOL_QT_MAJOR must be 5 or 6.")
endif()
if(MOTOTOOL_BUILD_LEGACY AND NOT MOTOTOOL_QT_MAJOR STREQUAL "5")
    message(FATAL_ERROR "The legacy client requires Qt 5. Use a separate Qt 5 build directory.")
endif()

set(MOTOTOOL_QT_PACKAGE "Qt${MOTOTOOL_QT_MAJOR}")
if(MOTOTOOL_QT_MAJOR STREQUAL "5")
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets)
else()
    find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets)
endif()

function(mototool_compile_options target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /we4715 /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror=return-type)
    endif()
endfunction()

add_library(diagnostics_core STATIC
    diagnostics/diagnosticscore.cpp
    diagnostics/diagnosticscore.h
)
target_include_directories(diagnostics_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/diagnostics")
target_link_libraries(diagnostics_core PUBLIC ${MOTOTOOL_QT_PACKAGE}::Core)
mototool_compile_options(diagnostics_core)

add_library(diagnostics_ui STATIC
    diagnostics/diagnosticswindow.cpp
    diagnostics/diagnosticswindow.h
)
target_link_libraries(diagnostics_ui PUBLIC diagnostics_core ${MOTOTOOL_QT_PACKAGE}::Widgets)
target_compile_definitions(diagnostics_ui PUBLIC MOTOTOOL_VERSION="${PROJECT_VERSION}-dev")
mototool_compile_options(diagnostics_ui)

add_executable(MotoTool_Diagnostics WIN32 diagnostics/main.cpp)
target_link_libraries(MotoTool_Diagnostics PRIVATE diagnostics_ui)
mototool_compile_options(MotoTool_Diagnostics)

# The checked-in legacy .rc contains a local developer path and a missing manifest.
# Generate a portable icon resource without silently reusing that broken file.
if(WIN32)
    enable_language(RC)
    configure_file(cmake/app-icon.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app-icon.rc" @ONLY)
    target_sources(MotoTool_Diagnostics PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app-icon.rc")
endif()

if(MOTOTOOL_BUILD_LEGACY OR BUILD_TESTING)
    add_library(mototool_safety STATIC securityhelpers.cpp securityhelpers.h portselection.cpp portselection.h)
    target_include_directories(mototool_safety PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(mototool_safety PUBLIC ${MOTOTOOL_QT_PACKAGE}::Core ${MOTOTOOL_QT_PACKAGE}::Widgets)
    mototool_compile_options(mototool_safety)
endif()

if(MOTOTOOL_BUILD_LEGACY)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Network SerialPort)
    add_executable(MotoTool_MTK WIN32
        main.cpp mainwindow.cpp mainwindow.h mainwindow.ui
        backendclass.cpp backendclass.h metamodethread.cpp metamodethread.h
        globalvars.cpp globalvars.h class_logfile.cpp class_logfile.h resources.qrc
    )
    target_link_libraries(MotoTool_MTK PRIVATE mototool_safety Qt5::Network Qt5::SerialPort)
    # Containment remains enabled in Debug and Release; legacy debug output leaks data.
    target_compile_definitions(MotoTool_MTK PRIVATE QT_NO_DEBUG_OUTPUT)
    mototool_compile_options(MotoTool_MTK)
    if(WIN32)
        target_sources(MotoTool_MTK PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app-icon.rc")
    endif()
endif()

if(BUILD_TESTING)
    find_package(${MOTOTOOL_QT_PACKAGE} REQUIRED COMPONENTS Test)
    find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)

    add_executable(security_tests tests/security_tests.cpp)
    target_link_libraries(security_tests PRIVATE mototool_safety ${MOTOTOOL_QT_PACKAGE}::Test)
    mototool_compile_options(security_tests)
    add_test(NAME security_helpers COMMAND security_tests -txt)
    set_tests_properties(security_helpers PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

    add_executable(fake_adb tests/fake_adb.cpp)
    target_link_libraries(fake_adb PRIVATE ${MOTOTOOL_QT_PACKAGE}::Core)
    set_target_properties(fake_adb PROPERTIES
        OUTPUT_NAME adb
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests/fixture"
    )
    mototool_compile_options(fake_adb)

    add_executable(diagnostics_tests tests/diagnostics_tests.cpp)
    target_include_directories(diagnostics_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(diagnostics_tests PRIVATE diagnostics_core ${MOTOTOOL_QT_PACKAGE}::Test)
    add_dependencies(diagnostics_tests fake_adb)
    mototool_compile_options(diagnostics_tests)
    add_test(NAME diagnostics_core COMMAND diagnostics_tests -txt)
    set_tests_properties(diagnostics_core PROPERTIES
        ENVIRONMENT "MOTOTOOL_TEST_ADB=$<TARGET_FILE:fake_adb>;QT_QPA_PLATFORM=offscreen"
        TIMEOUT 90
    )

    add_executable(diagnostics_ui_tests tests/diagnostics_ui_tests.cpp)
    target_link_libraries(diagnostics_ui_tests PRIVATE diagnostics_ui ${MOTOTOOL_QT_PACKAGE}::Test)
    add_dependencies(diagnostics_ui_tests fake_adb)
    mototool_compile_options(diagnostics_ui_tests)
    add_test(NAME diagnostics_ui COMMAND diagnostics_ui_tests -txt)
    set_tests_properties(diagnostics_ui PROPERTIES
        ENVIRONMENT "MOTOTOOL_TEST_ADB=$<TARGET_FILE:fake_adb>;QT_QPA_PLATFORM=offscreen"
        TIMEOUT 90
    )

    add_test(NAME source_safety
        COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests -p "test_*.py" -v
    )
    set_tests_properties(source_safety PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" TIMEOUT 30)
endif()
