cmake_minimum_required(VERSION 3.24)
project(AudacityProfileTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 6.10 REQUIRED COMPONENTS Core Gui)
# Export/import mechanics test only. The Windows product links muse_global
# statically (BUILD_SHARED_LIBS=OFF); this DLL target is not product-binary proof.
add_library(profile_provider SHARED ../profilepaths.cpp)
target_compile_definitions(profile_provider PRIVATE AUDACITY_PROFILEPATHS_BUILD PUBLIC AUDACITY_PROFILEPATHS_SHARED)
target_link_libraries(profile_provider PUBLIC Qt6::Core)
target_include_directories(profile_provider PUBLIC ..)
add_library(profile_consumer SHARED consumer.cpp)
target_link_libraries(profile_consumer PRIVATE profile_provider)
add_executable(profile_tests tests.cpp)
target_link_libraries(profile_tests PRIVATE profile_provider profile_consumer Qt6::Core)
enable_testing()
add_test(NAME profile_isolation COMMAND profile_tests)

# Match the product's static provider linkage without building the whole product.
add_library(profile_provider_static STATIC ../profilepaths.cpp)
target_link_libraries(profile_provider_static PUBLIC Qt6::Core)
target_include_directories(profile_provider_static PUBLIC ..)
add_library(profile_consumer_static STATIC staticconsumer.cpp)
target_link_libraries(profile_consumer_static PRIVATE profile_provider_static)
set(PROFILE_IPC_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../muse/framework/multiwindows/internal/multiprocess/ipc)
add_executable(profile_resource_tests resource_tests.cpp ${PROFILE_IPC_DIR}/ipclock.cpp)
target_include_directories(profile_resource_tests PRIVATE ${PROFILE_IPC_DIR})
target_link_libraries(profile_resource_tests PRIVATE profile_provider_static profile_consumer_static Qt6::Core)
add_test(NAME profile_resource_isolation COMMAND profile_resource_tests)

set(PROFILE_TOOLKIT_DIR ${CMAKE_CURRENT_LIST_DIR}/../../toolkit/internal)
add_executable(profile_toolkit_tests toolkit_tests.cpp
    ${PROFILE_TOOLKIT_DIR}/bookmarkmodel.cpp ${PROFILE_TOOLKIT_DIR}/bookmarkmodel.h
    ${PROFILE_TOOLKIT_DIR}/hardwarefitservice.cpp ${PROFILE_TOOLKIT_DIR}/hardwarefitservice.h
    ${PROFILE_TOOLKIT_DIR}/hardwareprobe.cpp ${PROFILE_TOOLKIT_DIR}/hardwarefit.cpp
    ${PROFILE_TOOLKIT_DIR}/externaleditorservice.cpp ${PROFILE_TOOLKIT_DIR}/externaleditorservice.h)
set_target_properties(profile_toolkit_tests PROPERTIES AUTOMOC ON)
target_include_directories(profile_toolkit_tests PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../.. ${PROFILE_TOOLKIT_DIR})
target_link_libraries(profile_toolkit_tests PRIVATE profile_provider_static Qt6::Core)
add_test(NAME profile_toolkit_isolation COMMAND profile_toolkit_tests)

# CPU image processing and QObject models only, no QGuiApplication or window.
set(PROFILE_BRANDING_DIR ${CMAKE_CURRENT_LIST_DIR}/../../branding)
set(PROFILE_PERSONALIZE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../personalize/internal)
add_executable(profile_branding_tests branding_tests.cpp
    ${PROFILE_BRANDING_DIR}/brandingstore.cpp ${PROFILE_BRANDING_DIR}/brandingstore.h
    ${PROFILE_PERSONALIZE_DIR}/brandingmodel.cpp ${PROFILE_PERSONALIZE_DIR}/brandingmodel.h)
set_target_properties(profile_branding_tests PROPERTIES AUTOMOC ON)
target_include_directories(profile_branding_tests PRIVATE ${PROFILE_BRANDING_DIR} ${PROFILE_PERSONALIZE_DIR})
target_link_libraries(profile_branding_tests PRIVATE profile_provider_static Qt6::Core Qt6::Gui)
add_test(NAME profile_branding_isolation COMMAND profile_branding_tests)
