cmake_minimum_required(VERSION 3.21)
project(audacity_converter_core_tests LANGUAGES CXX)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui)
qt_standard_project_setup()

add_executable(converter_core_smoke
    ../converter_smoke.cpp
    ../../convertercatalog.cpp
    ../../conversionengine.cpp
    ../../conversionqueue.cpp
)
target_include_directories(converter_core_smoke PRIVATE ../..)
target_link_libraries(converter_core_smoke PRIVATE Qt6::Core Qt6::Gui)
set_target_properties(converter_core_smoke PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED YES)

enable_testing()
add_test(NAME converter_core_smoke COMMAND converter_core_smoke)
