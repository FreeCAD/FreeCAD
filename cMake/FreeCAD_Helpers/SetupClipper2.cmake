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

    # project(Clipper2 VERSION ...) is scoped to the bundled directory. Export the
    # version so LibraryVersions.h.cmake can fill the About line (#32627).
    if(NOT Clipper2_VERSION)
        get_directory_property(_clipper2_version
            DIRECTORY "${CMAKE_SOURCE_DIR}/src/3rdParty/Clipper2"
            DEFINITION Clipper2_VERSION)
        if(_clipper2_version)
            set(Clipper2_VERSION "${_clipper2_version}")
        endif()
        unset(_clipper2_version)
    endif()
endif()

endmacro(SetupClipper2)
