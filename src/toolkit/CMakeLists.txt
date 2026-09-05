#
# Audacity: A Digital Audio Editor
#

declare_module(toolkit)

set(MODULE_QML_IMPORT ${CMAKE_CURRENT_LIST_DIR}/qml)

# Bundle every documentation article under docs/features into this module's
# Qt resources, so the in-app documentation browser works fully offline.
# The list is discovered by globbing at configure time (re-run whenever a
# file is added or removed, thanks to CONFIGURE_DEPENDS) rather than by a
# hand maintained manifest, so an article on disk cannot silently fail to
# reach the bundle.
set(AU_TOOLKIT_DOCS_DIR ${CMAKE_CURRENT_LIST_DIR}/../../docs/features)
file(GLOB_RECURSE AU_TOOLKIT_DOC_FILES CONFIGURE_DEPENDS ${AU_TOOLKIT_DOCS_DIR}/*.md)

if (NOT AU_TOOLKIT_DOC_FILES)
    message(FATAL_ERROR "toolkit: no documentation articles found under ${AU_TOOLKIT_DOCS_DIR}. "
                         "The in-app documentation browser has nothing to bundle.")
endif()

set(AU_TOOLKIT_DOCS_QRC_ENTRIES "")
foreach(AU_TOOLKIT_DOC_FILE ${AU_TOOLKIT_DOC_FILES})
    get_filename_component(AU_TOOLKIT_DOC_NAME ${AU_TOOLKIT_DOC_FILE} NAME)
    string(APPEND AU_TOOLKIT_DOCS_QRC_ENTRIES
        "        <file alias=\"${AU_TOOLKIT_DOC_NAME}\">${AU_TOOLKIT_DOC_FILE}</file>\n")
endforeach()

set(AU_TOOLKIT_GENERATED_DOCS_QRC ${CMAKE_CURRENT_BINARY_DIR}/au_toolkit_docs.qrc)
file(WRITE ${AU_TOOLKIT_GENERATED_DOCS_QRC}
    "<RCC>\n    <qresource prefix=\"/docs/features\">\n${AU_TOOLKIT_DOCS_QRC_ENTRIES}    </qresource>\n</RCC>\n")

# Completeness check: every article the glob just found must appear, once,
# in the generated resource file we just wrote. This fails configuration
# loudly rather than letting a bundling mistake ship silently.
file(READ ${AU_TOOLKIT_GENERATED_DOCS_QRC} AU_TOOLKIT_GENERATED_DOCS_QRC_CONTENT)
foreach(AU_TOOLKIT_DOC_FILE ${AU_TOOLKIT_DOC_FILES})
    get_filename_component(AU_TOOLKIT_DOC_NAME ${AU_TOOLKIT_DOC_FILE} NAME)
    string(FIND "${AU_TOOLKIT_GENERATED_DOCS_QRC_CONTENT}" "alias=\"${AU_TOOLKIT_DOC_NAME}\"" AU_TOOLKIT_DOC_FOUND_AT)
    if (AU_TOOLKIT_DOC_FOUND_AT EQUAL -1)
        message(FATAL_ERROR "toolkit: documentation article ${AU_TOOLKIT_DOC_NAME} is on disk "
                             "but missing from the generated documentation bundle.")
    endif()
endforeach()

set(MODULE_QRC
    ${CMAKE_CURRENT_LIST_DIR}/au_toolkit.qrc
    ${AU_TOOLKIT_GENERATED_DOCS_QRC}
)

set(MODULE_SRC
    ${CMAKE_CURRENT_LIST_DIR}/toolkitmodule.cpp
    ${CMAKE_CURRENT_LIST_DIR}/toolkitmodule.h

    ${CMAKE_CURRENT_LIST_DIR}/internal/hardwarefit.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/hardwarefit.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/hardwareprobe.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/hardwareprobe.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/harnessprofile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/harnessprofile.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/exportservice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/exportservice.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/exportservicewrapper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/exportservicewrapper.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/hardwarefitservice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/hardwarefitservice.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/bulkselectionmodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/bulkselectionmodel.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/pullcartmodel.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/pullcartmodel.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/docsindex.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/docsindex.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/externaleditorservice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/externaleditorservice.h
    ${CMAKE_CURRENT_LIST_DIR}/internal/ollamaclient.cpp
    ${CMAKE_CURRENT_LIST_DIR}/internal/ollamaclient.h
)

set(MODULE_LINK
    muse::uicomponents_qml
    muse::ui
    Qt::Network
)

setup_module()

if (AU_BUILD_TOOLKIT_TESTS)
    add_subdirectory(tests)
endif()
