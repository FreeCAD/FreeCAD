#
# libPRC top-level CMakeLists.txt
# https://github.com/XenonofArcticus/libPRC
#

cmake_minimum_required( VERSION 2.8.5 )
project( libPRC )

# Define project-specific macros.
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules ${CMAKE_MODULE_PATH} )
include( CMakeMacros )


# Installation directories
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib )


#
# Required ZLIB dependency.
# Set ZLIB_ROOT (env or cmake) as hint, if necessary.
find_package( ZLIB REQUIRED )

#
# Optional libharu. If not found, prctopdf will not be built
# Set LIBHARU_ROOT (env or cmake) as hint, if necessary.
find_package( LIBHARU )
if( LIBHARU_FOUND AND LIBHARU_STATIC )
    # If using static libHaru, then we must find and link with PNG.
    find_package( PNG REQUIRED )
    # Just append the PNG lib to the libHaru libraries.
    list( APPEND LIBHARU_LIBRARIES ${PNG_LIBRARY} )
endif()

#
# Optional OpenSceneGraph. If not found, the osgdb_prc plugin
# library will not be built.
# Set OSG_DIR (env or cmake) as hint, if necessary.
set( _osgComponents osgGA osgText osgViewer osgSim osgDB osgUtil osg OpenThreads )
find_package( OpenSceneGraph 2.8.5 COMPONENTS ${_osgComponents} )

#
# Optional Doxygen, for documentation.
# add a custom target to build documentation.
option( LIBPRC_DOCUMENTATION "Enable to create build target for Doxygen documentation." OFF )
mark_as_advanced( CLEAR LIBPRC_DOCUMENTATION )
if( LIBPRC_DOCUMENTATION )
    find_package( Doxygen REQUIRED )
    set( HAVE_DOT "NO" )
    if( DOXYGEN_DOT_PATH )
        set( HAVE_DOT "YES" )
    endif( DOXYGEN_DOT_PATH )

    configure_file( ${PROJECT_SOURCE_DIR}/doc/doxyfile.cmake
        ${PROJECT_BINARY_DIR}/doc/doxyfile
    )
    add_custom_target( Documentation ${DOXYGEN_EXECUTABLE}
        ${PROJECT_BINARY_DIR}/doc/doxyfile
    )
endif()

#
# Select asymptote (for dev and verification) or native libPRC.
option( PRC_USE_ASYMPTOTE "Set to ON to use Asymptote instead of libPRC (testing/dev only)." ON )
if( PRC_USE_ASYMPTOTE )
    add_definitions( -DPRC_USE_ASYMPTOTE )
    set( PRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/asymptote )
    set( PRC_LIBRARY asymptote )
else()
    set( PRC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/src/libPRC )
    set( PRC_LIBRARY libPRC )
endif()

#
# By default, build shared libraries.
set( BUILD_SHARED_LIBS ON CACHE BOOL "Build shared or static libs" )

if( WIN32 )
    # Common debug postfix for Windows.
    set( CMAKE_DEBUG_POSTFIX d )
endif()



add_subdirectory( src )



#
# Install pdb files for Debug and RelWithDebInfo builds
if( MSVC )
    install(
        DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/\${CMAKE_INSTALL_CONFIG_NAME}/
        DESTINATION lib
        USE_SOURCE_PERMISSIONS
        COMPONENT libprc-dev
        FILES_MATCHING PATTERN "*.pdb"
    )
    install(
        DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/\${CMAKE_INSTALL_CONFIG_NAME}/
        DESTINATION lib
        USE_SOURCE_PERMISSIONS
        COMPONENT libprc-dev
        FILES_MATCHING PATTERN "*.pdb"
    )
endif()
