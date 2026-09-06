# Standalone consumption is deliberate until the owning personalization UI wires this backend.
add_library(audacity_branding STATIC brandingstore.cpp brandingstore.h)
target_link_libraries(audacity_branding PUBLIC Qt6::Core Qt6::Gui)
target_include_directories(audacity_branding PUBLIC ${CMAKE_CURRENT_LIST_DIR})
