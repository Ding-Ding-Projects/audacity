#
# Audacity: A Digital Audio Editor
#

muse_create_module(experience)

target_sources(experience PRIVATE
    experiencemodule.cpp
    experiencemodule.h
    experiencetypes.cpp
    experiencetypes.h
    iexperienceconfiguration.h
    iexperienceservice.h
    imessagestyler.h
    inotificationcenter.h

    internal/experienceconfiguration.cpp
    internal/experienceconfiguration.h
    internal/experienceservice.cpp
    internal/experienceservice.h
    internal/experiencetranslator.cpp
    internal/experiencetranslator.h
    internal/messagestyler.cpp
    internal/messagestyler.h
    internal/notificationcenter.cpp
    internal/notificationcenter.h
    internal/personalvocabulary.cpp
    internal/personalvocabulary.h
    internal/settingsscheduler.cpp
    internal/settingsscheduler.h
)

find_package(Qt6 REQUIRED COMPONENTS Network)

target_link_libraries(experience PRIVATE muse::languages Qt6::Network)

# Several translation units in this module declare the same names in anonymous
# namespaces (moduleName, clampLevel, M3_REDUCED_MOTION), which a unity build
# merges into one file and rejects. Keeping this module out of the unity build
# lets it compile unchanged; renaming those symbols would let this line go.
set_target_properties(experience PROPERTIES UNITY_BUILD OFF)

add_subdirectory(qml/Audacity/Experience)

if (AU_BUILD_EXPERIENCE_TESTS)
    add_subdirectory(tests)
endif()
