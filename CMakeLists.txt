cmake_minimum_required(VERSION 3.21)
project(LegattoDeviceStudio VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LEGATTO_QT_MAJOR "5" CACHE STRING "Qt major version for diagnostics (5 or 6)")
set_property(CACHE LEGATTO_QT_MAJOR PROPERTY STRINGS 5 6)
option(LEGATTO_BUILD_LEGACY "Compile the unvalidated legacy servicing client" OFF)
option(LEGATTO_BUILD_EMULATOR_TESTS "Compile opt-in tests for two disposable Android emulators" OFF)
include(CTest)

if(NOT LEGATTO_QT_MAJOR MATCHES "^[56]$")
    message(FATAL_ERROR "LEGATTO_QT_MAJOR must be 5 or 6.")
endif()
if(LEGATTO_BUILD_LEGACY AND NOT LEGATTO_QT_MAJOR STREQUAL "5")
    message(FATAL_ERROR "The legacy client requires Qt 5. Use a separate Qt 5 build directory.")
endif()
if(LEGATTO_BUILD_EMULATOR_TESTS AND NOT BUILD_TESTING)
    message(FATAL_ERROR "LEGATTO_BUILD_EMULATOR_TESTS requires BUILD_TESTING=ON.")
endif()

set(LEGATTO_QT_PACKAGE "Qt${LEGATTO_QT_MAJOR}")
if(LEGATTO_QT_MAJOR STREQUAL "5")
    find_package(Qt5 5.15 REQUIRED COMPONENTS Core Widgets)
else()
    find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets)
endif()

function(legatto_compile_options target)
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
target_link_libraries(diagnostics_core PUBLIC ${LEGATTO_QT_PACKAGE}::Core)
legatto_compile_options(diagnostics_core)

add_library(diagnostics_ui STATIC
    diagnostics/diagnosticswindow.cpp
    diagnostics/diagnosticswindow.h
)
target_link_libraries(diagnostics_ui PUBLIC diagnostics_core ${LEGATTO_QT_PACKAGE}::Widgets)
target_compile_definitions(diagnostics_ui PUBLIC LEGATTO_VERSION="${PROJECT_VERSION}-dev")
legatto_compile_options(diagnostics_ui)

add_executable(LegattoDeviceStudio WIN32 diagnostics/main.cpp)
target_link_libraries(LegattoDeviceStudio PRIVATE diagnostics_ui)
legatto_compile_options(LegattoDeviceStudio)

# The inherited icon belongs only to the explicitly enabled legacy client.
# New diagnostics do not embed legacy artwork with unresolved distribution terms.
if(WIN32 AND LEGATTO_BUILD_LEGACY)
    enable_language(RC)
    configure_file(cmake/app-icon.rc.in "${CMAKE_CURRENT_BINARY_DIR}/app-icon.rc" @ONLY)
endif()

if(LEGATTO_BUILD_LEGACY OR BUILD_TESTING)
    add_library(legatto_safety STATIC securityhelpers.cpp securityhelpers.h portselection.cpp portselection.h)
    target_include_directories(legatto_safety PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(legatto_safety PUBLIC ${LEGATTO_QT_PACKAGE}::Core ${LEGATTO_QT_PACKAGE}::Widgets)
    legatto_compile_options(legatto_safety)
endif()

if(LEGATTO_BUILD_LEGACY)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Network SerialPort)
    add_executable(LegacyServiceClient WIN32
        main.cpp mainwindow.cpp mainwindow.h mainwindow.ui
        backendclass.cpp backendclass.h metamodethread.cpp metamodethread.h
        globalvars.cpp globalvars.h class_logfile.cpp class_logfile.h resources.qrc
    )
    target_link_libraries(LegacyServiceClient PRIVATE legatto_safety Qt5::Network Qt5::SerialPort)
    # Containment remains enabled in Debug and Release; legacy debug output leaks data.
    target_compile_definitions(LegacyServiceClient PRIVATE QT_NO_DEBUG_OUTPUT)
    legatto_compile_options(LegacyServiceClient)
    if(WIN32)
        target_sources(LegacyServiceClient PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/app-icon.rc")
    endif()
endif()

if(BUILD_TESTING)
    find_package(${LEGATTO_QT_PACKAGE} REQUIRED COMPONENTS Test)
    find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)

    add_executable(security_tests tests/security_tests.cpp)
    target_link_libraries(security_tests PRIVATE legatto_safety ${LEGATTO_QT_PACKAGE}::Test)
    legatto_compile_options(security_tests)
    add_test(NAME security_helpers COMMAND security_tests -txt)
    set_tests_properties(security_helpers PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

    add_executable(fake_adb tests/fake_adb.cpp)
    target_link_libraries(fake_adb PRIVATE ${LEGATTO_QT_PACKAGE}::Core)
    set_target_properties(fake_adb PROPERTIES
        OUTPUT_NAME adb
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tests/fixture"
    )
    legatto_compile_options(fake_adb)

    add_executable(diagnostics_tests tests/diagnostics_tests.cpp)
    target_include_directories(diagnostics_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(diagnostics_tests PRIVATE diagnostics_core ${LEGATTO_QT_PACKAGE}::Test)
    add_dependencies(diagnostics_tests fake_adb)
    legatto_compile_options(diagnostics_tests)
    add_test(NAME diagnostics_core COMMAND diagnostics_tests -txt)
    set_tests_properties(diagnostics_core PROPERTIES
        ENVIRONMENT "LEGATTO_TEST_ADB=$<TARGET_FILE:fake_adb>;QT_QPA_PLATFORM=offscreen"
        TIMEOUT 90
    )

    add_executable(diagnostics_ui_tests tests/diagnostics_ui_tests.cpp)
    target_link_libraries(diagnostics_ui_tests PRIVATE diagnostics_ui ${LEGATTO_QT_PACKAGE}::Test)
    add_dependencies(diagnostics_ui_tests fake_adb)
    legatto_compile_options(diagnostics_ui_tests)
    add_test(NAME diagnostics_ui COMMAND diagnostics_ui_tests -txt)
    set_tests_properties(diagnostics_ui PROPERTIES
        ENVIRONMENT "LEGATTO_TEST_ADB=$<TARGET_FILE:fake_adb>;QT_QPA_PLATFORM=offscreen"
        TIMEOUT 90
    )

    if(LEGATTO_BUILD_EMULATOR_TESTS)
        add_executable(emulator_integration_tests tests/emulator_integration_tests.cpp)
        target_link_libraries(emulator_integration_tests PRIVATE diagnostics_ui ${LEGATTO_QT_PACKAGE}::Test)
        legatto_compile_options(emulator_integration_tests)
        # Deliberately no add_test: only the explicit disposable-emulator runner
        # may execute this target. Ordinary CTest always remains hardware-free.
    endif()

    add_test(NAME source_safety
        COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tests -p "test_*.py" -v
    )
    set_tests_properties(source_safety PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" TIMEOUT 30)
endif()
