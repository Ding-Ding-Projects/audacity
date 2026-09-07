cmake_minimum_required(VERSION 3.24)
project(FrontBuildProvenanceTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Core)
add_executable(front_build_provenance_tests
    buildprovenance_tests.cpp
    ../../qml/Audacity/AppShell/buildprovenance.cpp)
target_include_directories(front_build_provenance_tests PRIVATE ../../qml/Audacity/AppShell)
target_link_libraries(front_build_provenance_tests PRIVATE Qt6::Core)
enable_testing()
add_test(NAME front_build_provenance COMMAND front_build_provenance_tests)
