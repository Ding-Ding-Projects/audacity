cmake_minimum_required(VERSION 3.21)
project(audacity_converter_core_tests LANGUAGES CXX)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui)
qt_standard_project_setup()

add_executable(converter_core_smoke
    ../converter_smoke.cpp
    ../../convertercatalog.cpp
    ../../conversionengine.cpp
    ../../conversionqueue.cpp
    ../../pdfprocessor.cpp
)
target_include_directories(converter_core_smoke PRIVATE ../..)
target_link_libraries(converter_core_smoke PRIVATE Qt6::Core Qt6::Gui)
set_target_properties(converter_core_smoke PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)

enable_testing()
add_test(NAME converter_core_smoke COMMAND converter_core_smoke)
set_tests_properties(converter_core_smoke PROPERTIES TIMEOUT 90)

if(WIN32)
    add_executable(converter_pdf_operations ../pdf_operations.cpp ../../pdfprocessor.cpp)
    target_include_directories(converter_pdf_operations PRIVATE ../..)
    target_compile_definitions(converter_pdf_operations PRIVATE AU_CONVERTER_TEST_HOOKS
        AU_QPDF_LOCK_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../../../../buildscripts/converter-tools/qpdf.lock.json")
    target_link_libraries(converter_pdf_operations PRIVATE Qt6::Core)
    set_target_properties(converter_pdf_operations PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
    add_test(NAME converter_pdf_operations COMMAND converter_pdf_operations)
    set_tests_properties(converter_pdf_operations PROPERTIES TIMEOUT 180)

    add_executable(converter_native_transactions
        ../native_transactions.cpp
        ../../convertercatalog.cpp
        ../../conversionengine.cpp
        ../../pdfprocessor.cpp
    )
    target_include_directories(converter_native_transactions PRIVATE ../..)
    target_compile_definitions(converter_native_transactions PRIVATE AU_CONVERTER_TEST_HOOKS)
    target_link_libraries(converter_native_transactions PRIVATE Qt6::Core Qt6::Gui)
    set_target_properties(converter_native_transactions PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)
    add_test(NAME converter_native_transactions COMMAND converter_native_transactions)
    set_tests_properties(converter_native_transactions PROPERTIES TIMEOUT 90)
endif()
