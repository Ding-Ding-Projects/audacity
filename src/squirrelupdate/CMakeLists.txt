#
# Audacity: A Digital Audio Editor
#

muse_create_module(squirrelupdate)

target_sources(squirrelupdate PRIVATE
    squirrelupdatemodule.cpp
    squirrelupdatemodule.h
    squirrelupdatetypes.h
    isquirrelupdateconfiguration.h
    isquirrelupdateservice.h
    updatebannerstate.cpp
    updatebannerstate.h

    internal/packageverifier.cpp
    internal/packageverifier.h
    internal/releasesparser.cpp
    internal/releasesparser.h
    internal/squirrelinstalllayout.cpp
    internal/squirrelinstalllayout.h
    internal/squirrelupdateconfiguration.cpp
    internal/squirrelupdateconfiguration.h
    internal/squirrelupdateservice.cpp
    internal/squirrelupdateservice.h
    internal/squirrelupdateuiactions.cpp
    internal/squirrelupdateuiactions.h
)

target_link_libraries(squirrelupdate PRIVATE muse::network muse::actions muse::ui)

add_subdirectory(qml/Audacity/SquirrelUpdate)

if (AU_BUILD_SQUIRRELUPDATE_TESTS)
    add_subdirectory(tests)
endif()
