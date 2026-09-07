# The workflow records one explicit decision after probing its optional cache.
# An unset decision preserves ordinary local CMake cache discovery.
function(audacity_ci_compiler_cache_options output)
    set(options "")
    if(DEFINED ENV{MUSE_CI_COMPILER_CACHE} AND NOT "$ENV{MUSE_CI_COMPILER_CACHE}" STREQUAL "")
        set(mode "$ENV{MUSE_CI_COMPILER_CACHE}")
        if(NOT mode STREQUAL "ON" AND NOT mode STREQUAL "OFF")
            message(FATAL_ERROR "Invalid compiler cache decision: expected ON or OFF")
        endif()
        list(APPEND options "-DMUSE_COMPILE_USE_CCACHE:BOOL=${mode}"
            "-DCMAKE_C_COMPILER_LAUNCHER:STRING=" "-DCMAKE_CXX_COMPILER_LAUNCHER:STRING=")
        if(mode STREQUAL "ON")
            set(program "$ENV{MUSE_CI_CCACHE_PROGRAM}")
            if(NOT IS_ABSOLUTE "${program}" OR NOT EXISTS "${program}" OR IS_DIRECTORY "${program}" OR program MATCHES "[;\r\n]")
                message(FATAL_ERROR "The selected ccache executable disappeared or has an unsafe path")
            endif()
            list(APPEND options "-DCOMPILER_CACHE_PROGRAM:FILEPATH=${program}")
        else()
            # Clear stale launchers and discovery results, including alternatives
            # such as sccache, so OFF really means an uncached compiler invocation.
            list(APPEND options "-DCOMPILER_CACHE_PROGRAM:FILEPATH=")
        endif()
    endif()
    set(${output} "${options}" PARENT_SCOPE)
endfunction()
