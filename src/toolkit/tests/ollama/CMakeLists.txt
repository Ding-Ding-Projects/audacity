cmake_minimum_required(VERSION 3.21)
project(audacity_ollama_catalog_tests LANGUAGES CXX)
find_package(Qt6 6.10 REQUIRED COMPONENTS Core Network)
qt_standard_project_setup()
add_executable(ollama_catalog_tests
    ../ollamacatalog_main.cpp
    ../../internal/ollamaclient.cpp
    ../../internal/ollamaclient.h
    ../../internal/ollamaclientcatalog.h
)
target_include_directories(ollama_catalog_tests PRIVATE ../../internal)
target_link_libraries(ollama_catalog_tests PRIVATE Qt6::Core Qt6::Network)
target_compile_features(ollama_catalog_tests PRIVATE cxx_std_17)
enable_testing()
add_test(NAME ollama_catalog_tests COMMAND ollama_catalog_tests)
set_tests_properties(ollama_catalog_tests PROPERTIES TIMEOUT 120)
