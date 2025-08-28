# - Find libHaru
# Find the native libHaru includes and libraries
# This module defines
#  LIBHARU_INCLUDE_DIRS, where to find hpdf.h, etc.
#  LIBHARU_LIBRARIES, libraries to link against to use libHaru.
#  LIBHARU_FOUND, If false, do not try to use libHaru.
#
# Search hint (in CMake or as an environment variable):
#  LIBHARU_ROOT, the libHaru install directory root.
#
# Find libHaru static or dynamic libs? Set as a CMake veriable:
#  LIBHARU_STATIC, find the static libs if ON, dynamic by default



# Find the main libHaru header.
set( LIBHARU_INCLUDE_DIRS )
find_path( LIBHARU_INCLUDE_DIRS hpdf.h
    PATHS ${LIBHARU_ROOT} ENV LIBHARU_ROOT
    PATH_SUFFIXES include
)


# Get a list of libraries, with static 's' suffix if necessary.
set( _requestedComponents libhpdf )
set( _components )
foreach( lib ${_requestedComponents} )
    if( LIBHARU_STATIC )
        list( APPEND _components ${lib}s )
    else()
        list( APPEND _components ${lib} )
    endif()
endforeach()

# Find each library.
set( LIBHARU_LIBRARIES )
foreach( lib ${_components} )
    find_library( LIBHARU_${lib}_LIBRARY
        NAMES ${lib}
        PATHS ${LIBHARU_ROOT} ENV LIBHARU_ROOT
        PATH_SUFFIXES lib
    )
    find_library( LIBHARU_${lib}_LIBRARY_DEBUG
        NAMES ${lib}d
        PATHS ${LIBHARU_ROOT} ENV LIBHARU_ROOT
        PATH_SUFFIXES lib
    )

    if( NOT LIBHARU_${lib}_LIBRARY )
        message( WARNING "Could not find LIBHARU component library ${lib}" )
    else()
        if( LIBHARU_${lib}_LIBRARY_DEBUG AND
                ( NOT LIBHARU_${lib}_LIBRARY_DEBUG STREQUAL LIBHARU_${lib}_LIBRARY ) )
            list( APPEND LIBHARU_LIBRARIES "optimized" ${LIBHARU_${lib}_LIBRARY} )
            list( APPEND LIBHARU_LIBRARIES "debug" ${LIBHARU_${lib}_LIBRARY_DEBUG} )
        else()
            list( APPEND LIBHARU_LIBRARIES ${LIBHARU_${lib}_LIBRARY} )
        endif()
        mark_as_advanced( LIBHARU_${lib}_LIBRARY )
        mark_as_advanced( LIBHARU_${lib}_LIBRARY_DEBUG )
    endif()
endforeach()


# handle the QUIETLY and REQUIRED arguments and set LIBHARU_FOUND to TRUE if 
# all listed variables are TRUE
include( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( LIBHARU
    REQUIRED_VARS LIBHARU_INCLUDE_DIRS LIBHARU_LIBRARIES
)


mark_as_advanced(
    ASSIMP_INCLUDE_DIR
    ASSIMP_LIBRARIES
)
