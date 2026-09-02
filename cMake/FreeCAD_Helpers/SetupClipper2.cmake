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
endif()

endmacro(SetupClipper2)
