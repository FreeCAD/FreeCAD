# Find the spnav library and header.
#
# Sets the usual variables expected for find_package scripts:
#
# SPNAV_INCLUDE_DIR - header location SPNAV_LIBRARIES - library to link against SPNAV_FOUND - true
# if pugixml was found.

if(UNIX)

    find_path(SPNAV_INCLUDE_DIR spnav.h)

    find_library(SPNAV_LIBRARY NAMES spnav libspnav)

    # Support the REQUIRED and QUIET arguments, and set SPNAV_FOUND if found.
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Spnav DEFAULT_MSG SPNAV_LIBRARY SPNAV_INCLUDE_DIR)

    if(SPNAV_FOUND)
        set(SPNAV_LIBRARIES ${SPNAV_LIBRARY})
    endif()

    mark_as_advanced(SPNAV_LIBRARY SPNAV_INCLUDE_DIR)

endif(UNIX)
