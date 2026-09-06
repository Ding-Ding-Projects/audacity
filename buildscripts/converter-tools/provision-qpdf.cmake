# Shared production step used by both Windows CI build and package entry points.
if(NOT DEFINED QPDF_INSTALL_ROOT OR QPDF_INSTALL_ROOT STREQUAL "")
    message(FATAL_ERROR "QPDF_INSTALL_ROOT must name the existing install tree")
endif()
get_filename_component(QPDF_INSTALL_ROOT "${QPDF_INSTALL_ROOT}" ABSOLUTE)
if(NOT IS_DIRECTORY "${QPDF_INSTALL_ROOT}/bin")
    message(FATAL_ERROR "Installed binary directory is absent: ${QPDF_INSTALL_ROOT}/bin")
endif()
get_filename_component(_qpdf_source_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
find_program(_qpdf_powershell NAMES powershell pwsh REQUIRED)
execute_process(
    COMMAND "${_qpdf_powershell}" -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_CURRENT_LIST_DIR}/bootstrap-qpdf.ps1"
        -DestinationRoot "${QPDF_INSTALL_ROOT}/bin"
        -CacheRoot "${_qpdf_source_root}/build.tools/downloads"
    RESULT_VARIABLE _qpdf_result
    TIMEOUT 200
)
if(NOT _qpdf_result STREQUAL "0")
    message(FATAL_ERROR "Verified qpdf provisioning failed before packaging: ${_qpdf_result}")
endif()
