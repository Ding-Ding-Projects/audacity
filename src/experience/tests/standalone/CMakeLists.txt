cmake_minimum_required(VERSION 3.21)
project(experience_standalone_tests LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS Core)

add_executable(experience_standalone_tests
    main.cpp
    ../../internal/schoolmode.cpp
    ../../internal/narratorqueue.cpp
)

target_include_directories(experience_standalone_tests PRIVATE ../..)
target_link_libraries(experience_standalone_tests PRIVATE Qt6::Core)
