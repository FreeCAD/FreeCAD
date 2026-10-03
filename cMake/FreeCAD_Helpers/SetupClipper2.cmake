macro(SetupClipper2)

if(FREECAD_USE_EXTERNAL_CLIPPER2)
    find_package(Clipper2 2.0 REQUIRED)
    if(NOT TARGET Clipper2::Clipper2Z)
        message(FATAL_ERROR "Clipper2 must be built with USINGZ")
    endif()
else()
    # Configure Clipper2 options
    set(CLIPPER2_UTILS OFF CACHE BOOL "Disable Clipper2 utilities" FORCE)
    set(CLIPPER2_EXAMPLES OFF CACHE BOOL "Disable Clipper2 examples" FORCE)
    set(CLIPPER2_TESTS OFF CACHE BOOL "Disable Clipper2 tests" FORCE)
    set(CLIPPER2_USINGZ ONLY CACHE STRING "Build Clipper2Z with Z-coordinate support" FORCE)

    add_subdirectory(src/3rdParty/Clipper2)
    add_library(Clipper2::Clipper2Z ALIAS Clipper2Z)

    # project(Clipper2 ...) defines Clipper2_VERSION in the scope of the subdirectory
    # only, so it does not reach src/LibraryVersions.h.cmake or
    # src/Doc/ThirdPartyLibraries.html.cmake and the reported version stays empty.
    # Recover it from the version header that the subdirectory generates from the very
    # same source of truth, instead of hardcoding a version that would go stale.
    if(NOT Clipper2_VERSION)
        set(_clipper2_version_header "${CMAKE_SOURCE_DIR}/src/3rdParty/Clipper2/Clipper2Lib/include/clipper2/clipper.version.h")
        if(EXISTS "${_clipper2_version_header}")
            file(STRINGS "${_clipper2_version_header}" _clipper2_version_line REGEX "CLIPPER2_VERSION[ \t]*=")
            string(REGEX MATCH "=[ \t]*\"([^\"]+)\"" _clipper2_version "${_clipper2_version_line}")
            set(Clipper2_VERSION "${CMAKE_MATCH_1}")
        endif()
    endif()
    if(NOT Clipper2_VERSION)
        message(WARNING "Bundled Clipper2 did not define Clipper2_VERSION, the reported version will be empty.")
    endif()
endif()

endmacro(SetupClipper2)
