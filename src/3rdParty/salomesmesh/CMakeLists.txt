# SMESH cmake build file, using only by Fem module
cmake_minimum_required(VERSION 3.3)
project(SMESH C CXX)

# --------------------------------------------------------------------------- #
# OPTIONS, this should be replaced by freecad's options
# --------------------------------------------------------------------------- #
# option(ENABLE_NETGEN "Enable NETGEN" ON)
# option(ENABLE_MED "Enable MED" OFF)
# option(ENABLE_BLSURF "Enable BLSURF" OFF)
# option(ENABLE_LIB_NAMING "Enable additional library naming" OFF)
# set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "Installation directory.")
# set(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

# for netgen it is bydefault turn off, as only patched netgen can works with OCCT 7.x
set(ENABLE_NETGEN ${BUILD_FEM_NETGEN})
set(ENABLE_MED OFF)
set(ENABLE_BLSURF OFF)

# --------------------------------------------------------------------------- #
# SETTINGS
# --------------------------------------------------------------------------- #
set(SMESH_VERSION_MAJOR 8)
set(SMESH_VERSION_MINOR 3)
set(SMESH_VERSION_PATCH 0)
set(SMESH_VERSION_TWEAK 3)

# Build shared libraries
set(BUILD_SHARED_LIBS ON)

# --------------------------------------------------------------------------- #
# copied from FreeCAD salomemesh 7.7
# --------------------------------------------------------------------------- #
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wno-sign-compare -Wno-reorder -Wno-switch -Wno-unused-variable -Wno-unused-but-set-variable -Wno-comment -Wno-unused-parameter -Wno-empty-body -Wno-pedantic")
elseif(CMAKE_COMPILER_IS_CLANGXX)
    set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wno-sign-compare -Wno-reorder -Wno-switch -Wno-unused-variable -Wno-unused-private-field -Wno-unused-function -Wno-sometimes-uninitialized -Wno-overloaded-virtual -Wno-dynamic-class-memaccess -Wno-comment -Wno-unused-parameter -Wno-extra-semi")
endif()

if(CMAKE_COMPILER_IS_CLANGXX)
    set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wno-self-assign -Wno-reorder -Wno-switch-enum -Wno-unknown-pragmas -Wno-logical-op-parentheses -Wno-unused-variable -Wno-unused-function -Wno-overloaded-virtual")
endif()

if (VTK_OPTIONS)
    add_definitions(${VTK_OPTIONS})
endif()

# rename some variable
set(OpenCASCADE_INCLUDE_DIR ${OCC_INCLUDE_DIR})
set(OpenCASCADE_LIBRARY_DIR ${OCC_LIBRARY_DIR})
# MED_* or MEDFILE_*, there may be  some bugs in smesh repo, not consistent
# Trace shared object reliesed on dl library, add ${CMAKE_DL_LIBS}



# some lib detected by smesh is commented out
# some other should not be used by smesh, zlib, hdf5
include_directories(
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_BINARY_DIR}
#    ${Boost_INCLUDE_DIRS}
#    ${OCC_INCLUDE_DIR}
    ${NGLIB_INCLUDE_DIR}
#    ${NETGEN_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
#    ${VTK_INCLUDE_DIRS}
    ${HDF5_INCLUDE_DIR}
#    ${MEDFILE_INCLUDE_DIRS}
#    ${BOOST_INCLUDE_DIR}
)
# --------------------------------------------------------------------------- #
# end of section copied from FreeCAD salomemesh 7.7
# --------------------------------------------------------------------------- #

# --------------------------------------------------------------------------- #
# OpenCASCADE
# --------------------------------------------------------------------------- #
if(NOT DEFINED OpenCASCADE_INCLUDE_DIR OR NOT DEFINED OpenCASCADE_LIBRARY_DIR)
    message(STATUS "Searching for OpenCASCADE...")
    find_package(OpenCASCADE REQUIRED)
endif()

if(NOT EXISTS ${OpenCASCADE_INCLUDE_DIR})
    message(FATAL_ERROR "Failed to find OpenCASCADE include directory.")
endif()

if(NOT EXISTS ${OpenCASCADE_LIBRARY_DIR})
    message(FATAL_ERROR "Failed to find OpenCASCADE library directory.")
endif()

message(STATUS "OpenCASCADE include directory: ${OpenCASCADE_INCLUDE_DIR}")
message(STATUS "OpenCASCADE library directory: ${OpenCASCADE_LIBRARY_DIR}")
include_directories(${OpenCASCADE_INCLUDE_DIR})
link_directories(${OpenCASCADE_LIBRARY_DIR})


# --------------------------------------------------------------------------- #
# VTK
# --------------------------------------------------------------------------- #
if(NOT DEFINED VTK_INCLUDE_DIRS OR NOT DEFINED VTK_LIBRARIES)
    message(STATUS "Searching for VTK...")
    find_package(VTK REQUIRED)
endif()

if(NOT DEFINED VTK_INCLUDE_DIRS)
    message(FATAL_ERROR "Failed to define VTK include directories.")
endif()

if(NOT DEFINED VTK_LIBRARIES)
    message(FATAL_ERROR "Failed to define VTK libraries.")
endif()

message(STATUS "VTK include directories: ${VTK_INCLUDE_DIRS}")
include_directories(${VTK_INCLUDE_DIRS})


# --------------------------------------------------------------------------- #
# BOOST
# --------------------------------------------------------------------------- #
if(NOT DEFINED Boost_INCLUDE_DIR)
    message(STATUS "Searching for Boost...")
    find_package(Boost REQUIRED COMPONENTS filesystem thread)
endif()

if(NOT EXISTS ${Boost_INCLUDE_DIR})
    message(FATAL_ERROR "Failed to find Boost include directory.")
endif()

message(STATUS "Boost include directory: ${Boost_INCLUDE_DIR}")
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

if (MSVC)
    # find the shared boost libs
    add_definitions(-DBOOST_ALL_DYN_LINK)
    # set postfix for debug libs
    if(NOT CMAKE_DEBUG_POSTFIX)
        set(CMAKE_DEBUG_POSTFIX d)
    endif()
endif()

# --------------------------------------------------------------------------- #
# PTHREAD
# --------------------------------------------------------------------------- #
if(UNIX)
    find_package(Threads REQUIRED)
else(UNIX)
  set(PTHREAD_INCLUDE_DIRS "" CACHE PATH "pthread include directory.")
  if(NOT EXISTS ${PTHREAD_INCLUDE_DIRS})
      message(FATAL_ERROR "pthread include directory is required.")
  endif()
  message(STATUS "pthread include directory: ${PTHREAD_INCLUDE_DIRS}")
  include_directories(${PTHREAD_INCLUDE_DIRS})

  set(PTHREAD_LIB_DIRS "" CACHE PATH "pthread library directory.")
  if(NOT EXISTS ${PTHREAD_LIB_DIRS})
      message(FATAL_ERROR "pthread library directory is required.")
  endif()
  message(STATUS "pthread library directory: ${PTHREAD_LIB_DIRS}")
  link_directories(${PTHREAD_LIB_DIRS})
endif(UNIX)


# --------------------------------------------------------------------------- #
# NETGEN
# --------------------------------------------------------------------------- #
if(ENABLE_NETGEN)
   if(NOT DEFINED NETGEN_INCLUDE_DIR OR NOT DEFINED NETGEN_LIBRARY_DIR)
       message(STATUS "Searching for Netgen...")
       find_package(Netgen REQUIRED)
   endif()

    if(NOT EXISTS ${NETGEN_INCLUDE_DIR})
        message(FATAL_ERROR "Failed to find Netgen include directory.")
    endif()

    if(NOT EXISTS ${NETGEN_LIBRARY_DIR})
        message(FATAL_ERROR "Failed to find Netgen library directory.")
    endif()

    message(STATUS "Netgen include directory: ${NETGEN_INCLUDE_DIR}")
    message(STATUS "Netgen library directory: ${NETGEN_LIBRARY_DIR}")

    link_directories(${NETGEN_LIBRARY_DIR})
    include_directories(${NETGEN_INCLUDE_DIR})
    include_directories(${NETGEN_INCLUDE_DIR}/occ)
    include_directories(${NETGEN_INCLUDE_DIR}/meshing)
    include_directories(${NETGEN_INCLUDE_DIR}/general)
    include_directories(${NETGEN_INCLUDE_DIR}/linalg)
    include_directories(${NETGEN_INCLUDE_DIR}/csg)
    include_directories(${NETGEN_INCLUDE_DIR}/geom2d)
    include_directories(${NETGEN_INCLUDE_DIR}/gprim)
    include_directories(${NETGEN_INCLUDE_DIR}/include)
    include_directories(${NETGEN_INCLUDE_DIR}/stlgeom)

    # Set NETGEN_VERSION
    file(STRINGS ${NETGEN_INCLUDE_DIR}/mydefs.hpp NETGEN_VERSION
        REGEX "#define PACKAGE_VERSION.*"
        )

    if(NETGEN_VERSION)
        string(REGEX MATCHALL "[0-9]+" NETGEN_VERSION ${NETGEN_VERSION})
        list(LENGTH NETGEN_VERSION NETGEN_VERSION_COUNT)
        list(GET NETGEN_VERSION 0 NETGEN_VERSION_MAJOR)
        if(NETGEN_VERSION_COUNT GREATER 1)
            list(GET NETGEN_VERSION 1 NETGEN_VERSION_MINOR)
        else()
            set(NETGEN_VERSION_MINOR 0)
        endif()
    else()
        # Assume version 6.2.
        message(STATUS "WARNING: Assuming Netgen 6.2.")
        set(NETGEN_VERSION_MAJOR 6)
        set(NETGEN_VERSION_MINOR 2)
    endif()

    MATH(EXPR NETGEN_VERSION "(${NETGEN_VERSION_MAJOR} << 16) + (${NETGEN_VERSION_MINOR} << 8)")
    message(STATUS "Using Netgen version ${NETGEN_VERSION_MAJOR}.${NETGEN_VERSION_MINOR}, calculated: ${NETGEN_VERSION}")

endif()


# --------------------------------------------------------------------------- #
# BLSURF
# --------------------------------------------------------------------------- #
if(ENABLE_BLSURF)
    set(MESHGEMS_INCLUDE "" CACHE PATH "MeshGems include directory.")
    if(NOT EXISTS ${MESHGEMS_INCLUDE})
        message(FATAL_ERROR "MeshGems include directory is required.")
    endif()

    set(MESHGEMS_LIB "" CACHE FILE "MeshGems library.")
    if(NOT EXISTS ${MESHGEMS_LIB})
        message(FATAL_ERROR "MeshGems library is required.")
    endif()

    set(MESHGEMS_CADSURF_LIB "" CACHE FILE "MeshGems MG-CADSurf library.")
    if(NOT EXISTS ${MESHGEMS_CADSURF_LIB})
        message(FATAL_ERROR "MeshGems MG-CADSurf library is required.")
    endif()

endif()


# --------------------------------------------------------------------------- #
# MED
# --------------------------------------------------------------------------- #
if(ENABLE_MED)
    if(NOT DEFINED MED_INCLUDE_DIR OR NOT DEFINED MED_LIBRARY_DIR)
        message(STATUS "Searching for MED...")
        find_package(MEDFile CONFIG REQUIRED)
    endif()

    if(NOT EXISTS ${MEDFILE_INCLUDE_DIRS})
        message(FATAL_ERROR "Failed to find MED include directories.")
    endif()

    if(NOT EXISTS ${MEDFILE_INCLUDE_DIRS})
        message(FATAL_ERROR "Failed to find MED librar directory.")
    endif()

    message(STATUS "MED include directory: ${MED_INCLUDE_DIR}")
    message(STATUS "MED library directory: ${MED_LIBRARY_DIR}")
    include_directories(${MEDFILE_INCLUDE_DIRS})
endif()


# --------------------------------------------------------------------------- #
# SMESH
# --------------------------------------------------------------------------- #
# Includes
include_directories(inc)

# Build settings according to the platform
if(UNIX)
    # Same settings are used for both MacOSX and Unix/Linux
    add_definitions(-DHAVE_CONFIG_H -DHAVE_LIMITS_H -DCSFDB -DLIN -DOCC_CONVERT_SIGNALS)
else(UNIX)
  if(WIN32)
    if(MSVC)
        add_definitions(-DWNT -DWIN32 -D_WINDOWS -DCSFDB -DUSE_CLOCK -DMSDOS -DNO_ONEXIT -DNO_My_ctype -DNO_ISATTY -DNO_FPINIT /wd4290 /wd4251 /wd4018 /wd4800 /wd4996 /wd4244 /wd4805 /wd4806 /wd4275 /wd4005 /wd4099 /wd4101 /wd4146 /wd4267 /wd4390 /wd4503 /wd4436)
    else(MSVC)
        add_definitions(-DWNT -DWIN32 -D_WINDOWS -DCSFDB)
    endif(MSVC)
  else(WIN32)
    message(FATAL_ERROR "Unknown platform")
  endif(WIN32)
endif(UNIX)

# Basics
file(GLOB Basics_SRC src/Basics/*.cxx)
add_library(SMESHBasics ${Basics_SRC})
if(UNIX)
    target_link_libraries(SMESHBasics Threads::Threads)
else(UNIX)
  target_link_libraries(SMESHBasics pthreads)
endif(UNIX)
if(WIN32)
    set_target_properties(SMESHBasics PROPERTIES COMPILE_FLAGS "-DBASICS_EXPORTS")
endif()

# Controls
file(GLOB Controls_SRC src/Controls/*.cxx)
add_library(SMESHControls ${Controls_SRC})
target_link_libraries(SMESHControls SMDS SMESHDS SMESHUtils TKernel TKBRep TKG3d TKTopAlgo TKGeomBase TKGeomAlgo ${VTK_LIBRARIES})
if(WIN32)
    set_target_properties(SMESHControls PROPERTIES COMPILE_FLAGS "-DSMESHCONTROLS_EXPORTS")
endif()

# Driver
file(GLOB Driver_SRC src/Driver/*.cxx)
add_library(Driver ${Driver_SRC})
target_link_libraries(Driver SMESHMisc SMESHDS SMESHUtils TKernel TKTopAlgo)
if(WIN32)
    set_target_properties(Driver PROPERTIES COMPILE_FLAGS "-DMESHDRIVER_EXPORTS")
endif()

# DriverDAT
file(GLOB DriverDAT_SRC src/DriverDAT/*.cxx)
add_library(DriverDAT ${DriverDAT_SRC})
target_link_libraries(DriverDAT SMESHMisc SMESHTrace SMESHBasics Driver SMDS)
if(WIN32)
    set_target_properties(DriverDAT PROPERTIES COMPILE_FLAGS "-DMESHDRIVERDAT_EXPORTS")
endif()

# DriverGMF
file(GLOB DriverGMF_SRC src/DriverGMF/*.cxx src/DriverGMF/libmesh5.c)
add_library(DriverGMF ${DriverGMF_SRC})
target_link_libraries(DriverGMF SMESHBasics SMESHUtils Driver)
if(WIN32)
    set_target_properties(DriverGMF PROPERTIES COMPILE_FLAGS "-DMESHDriverGMF_EXPORTS")
endif()

# DriverSTL
file(GLOB DriverSTL_SRC src/DriverSTL/*.cxx)
add_library(DriverSTL ${DriverSTL_SRC})
target_link_libraries(DriverSTL Driver SMDS SMESHUtils TKernel TKSTL TKTopAlgo TKMesh)
if(WIN32)
    set_target_properties(DriverSTL PROPERTIES COMPILE_FLAGS "-DMESHDRIVERSTL_EXPORTS")
endif()

# DriverUNV
file(GLOB DriverUNV_SRC src/DriverUNV/*.cxx)
add_library(DriverUNV ${DriverUNV_SRC})
target_link_libraries(DriverUNV SMDS Driver SMESHMisc SMESHTrace SMESHBasics)
if(WIN32)
    set_target_properties(DriverUNV PROPERTIES COMPILE_FLAGS "-DMESHDRIVERUNV_EXPORTS")
endif()

# DriverMED
if(ENABLE_MED)
    file(GLOB DriverMED_SRC src/DriverMED/*.cxx)
    add_library(DriverMED ${DriverMED_SRC})
    target_link_libraries(DriverMED SMDS Driver SMESHMisc SMESHTrace SMESHBasics ${MEDFILE_LIBRARIES})
    if(WIN32)
        set_target_properties(DriverMED PROPERTIES COMPILE_FLAGS "-MESHDRIVERMED_EXPORT")
    endif()
endif()

# MEFISTO2
file(GLOB MEFISTO2_SRC src/MEFISTO2/aptrte.cxx src/MEFISTO2/trte.c)
add_library(MEFISTO2 ${MEFISTO2_SRC})
if (WIN32)
    set_target_properties(MEFISTO2 PROPERTIES COMPILE_FLAGS "-DMEFISTO2D_EXPORTS")
endif()
target_link_libraries(MEFISTO2 SMESHTrace TKernel)

# Misc
file(GLOB Misc_SRC src/Misc/*.cxx)
add_library(SMESHMisc ${Misc_SRC})
target_link_libraries(SMESHMisc SMESHTrace TKBRep TKShHealing TKMesh TKGeomAlgo TKTopAlgo TKG2d TKG3d TKV3d TKGeomBase TKBO)
if(WIN32)
    set_target_properties(SMESHMisc PROPERTIES COMPILE_FLAGS "-DUTILS_EXPORTS")
endif()

# NETGENPlugin
if(ENABLE_NETGEN)
    file(GLOB NETGENPlugin_SRC src/NETGENPlugin/*.cxx)
    add_library(NETGENPlugin ${NETGENPlugin_SRC})
    if (NETGEN_LIBRARY)
        target_link_libraries(NETGENPlugin StdMeshers TKIGES TKXDEIGES ${NETGEN_LIBRARY})
    else (NETGEN_LIBRARY)
        target_link_libraries(NETGENPlugin StdMeshers nglib)
    endif (NETGEN_LIBRARY)
    if(WIN32)
        set_target_properties(NETGENPlugin PROPERTIES COMPILE_FLAGS "-DNETGENPLUGIN_EXPORTS -DNO_PARALLEL_THREADS -DOCCGEOMETRY -DNETGEN_VERSION=${NETGEN_VERSION}")
    else()
        set_target_properties(NETGENPlugin PROPERTIES COMPILE_FLAGS "${NETGEN_CXX_FLAGS} -DNETGEN_VERSION=${NETGEN_VERSION}")
    endif()
endif()

# SMDS
file(GLOB SMDS_SRC src/SMDS/*.cxx)
add_library(SMDS ${SMDS_SRC})
target_link_libraries(SMDS SMESHMisc SMESHTrace ${VTK_LIBRARIES})
if(WIN32)
    set_target_properties(SMDS PROPERTIES COMPILE_FLAGS "-DSMDS_EXPORTS")
endif()

# SMESH
file(GLOB SMESH_SRC src/SMESH/*.cxx)
add_library(SMESH ${SMESH_SRC})
target_link_libraries(SMESH SMESHControls SMESHMisc SMESHDS DriverDAT DriverSTL DriverUNV DriverGMF TKShHealing TKPrim TKG2d TKCDF TKMeshVS ${Boost_LIBRARIES})
if(WIN32)
    set_target_properties(SMESH PROPERTIES COMPILE_FLAGS "-DSMESHimpl_EXPORTS")
endif()

# SMESHDS
file(GLOB SMESHDS_SRC src/SMESHDS/*.cxx)
add_library(SMESHDS ${SMESHDS_SRC})
target_link_libraries(SMESHDS SMDS SMESHTrace TKBRep)
if(WIN32)
    set_target_properties(SMESHDS PROPERTIES COMPILE_FLAGS "-DSMESHDS_EXPORTS")
endif()

# StdMeshers
file(GLOB StdMeshers_SRC src/StdMeshers/*.cxx)
add_library(StdMeshers ${StdMeshers_SRC})
target_link_libraries(StdMeshers SMESH MEFISTO2 TKOffset)
if(WIN32)
    set_target_properties(StdMeshers PROPERTIES COMPILE_FLAGS "-DSTDMESHERS_EXPORTS")
else()
    set_target_properties(StdMeshers PROPERTIES COMPILE_FLAGS "${StdMeshers_CFLAGS}")
endif()

# Trace
file(GLOB Trace_SRC src/Trace/*.cxx)
add_library(SMESHTrace ${Trace_SRC})
if(UNIX)
  target_link_libraries(SMESHTrace Threads::Threads SMESHBasics ${CMAKE_DL_LIBS})
else(UNIX)
  target_link_libraries(SMESHTrace pthreads SMESHBasics)
endif(UNIX)
if(WIN32)
    set_target_properties(SMESHTrace PROPERTIES COMPILE_FLAGS "-DSALOMELOCALTRACE_EXPORTS")
endif()

# Utils
file(GLOB Utils_SRC src/Utils/*.cxx)
add_library(SMESHUtils ${Utils_SRC})
target_link_libraries(SMESHUtils SMDS TKShHealing TKPrim TKernel TKBRep TKG2d TKG3d TKGeomBase TKGeomAlgo TKTopAlgo TKMesh ${Boost_LIBRARIES})
if(WIN32)
    set_target_properties(SMESHUtils PROPERTIES COMPILE_FLAGS "-DSMESHUtils_EXPORTS")
endif()

# BLSURFPlugin
if(ENABLE_BLSURF)
    include_directories(${MESHGEMS_INCLUDE})
    file(GLOB BLSURFPlugin_SRC src/BLSURFPlugin/*.cxx)
    add_library(BLSURFPlugin ${BLSURFPlugin_SRC})
    target_link_libraries(BLSURFPlugin TKBRep TKG2d TKG3d TKGeomBase TKGeomAlgo TKShHealing SMDS SMESHDS SMESH StdMeshers ${MESHGEMS_CADSURF_LIB} ${MESHGEMS_LIB})
    if(WIN32)
        set_target_properties(BLSURFPlugin PROPERTIES COMPILE_FLAGS "-DBLSURFPLUGIN_EXPORTS")
    endif()
endif()

#########################################################################
# Install
set(SMESH_LIBRARIES Driver DriverDAT DriverGMF DriverSTL DriverUNV MEFISTO2 SMDS SMESH SMESHBasics SMESHControls SMESHDS SMESHMisc SMESHTrace SMESHUtils StdMeshers)

if(ENABLE_NETGEN)
    set(SMESH_LIBRARIES ${SMESH_LIBRARIES} NETGENPlugin)
endif()

if(ENABLE_BLSURF)
    set(SMESH_LIBRARIES ${SMESH_LIBRARIES} BLSURFPlugin)
endif()

if(ENABLE_MED)
    set(SMESH_LIBRARIES ${SMESH_LIBRARIES} DriverMED)
endif()

if (MSVC AND ENABLE_LIB_NAMING)
    foreach(it ${SMESH_LIBRARIES})
        LIST(APPEND SMESH_LIBS "optimized ${it}")
        LIST(APPEND SMESH_LIBS "debug ${it}${CMAKE_DEBUG_POSTFIX}")
    endforeach()
else(MSVC AND ENABLE_LIB_NAMING)
    set (SMESH_LIBS ${SMESH_LIBRARIES})
endif(MSVC AND ENABLE_LIB_NAMING)

if(BUILD_SHARED_LIBS)
set_target_properties(${SMESH_LIBRARIES} PROPERTIES VERSION
                      ${SMESH_VERSION_MAJOR}.${SMESH_VERSION_MINOR}.${SMESH_VERSION_PATCH}.${SMESH_VERSION_TWEAK})
set_target_properties(${SMESH_LIBRARIES} PROPERTIES SOVERSION
                      ${SMESH_VERSION_MAJOR}.${SMESH_VERSION_MINOR})
endif()

# --------------------------------------------------------------------------- #
# smesh install, comment out
# --------------------------------------------------------------------------- #
# install(TARGETS ${SMESH_LIBRARIES}
#         ARCHIVE DESTINATION "lib"
#         RUNTIME DESTINATION "bin"
#         LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# install(DIRECTORY ${CMAKE_SOURCE_DIR}/inc/ DESTINATION "include/smesh")

# # Configuration file
# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/SMESHConfig.cmake.in
# 	           ${CMAKE_CURRENT_BINARY_DIR}/SMESHConfig.cmake @ONLY)

# install(FILES ${CMAKE_CURRENT_BINARY_DIR}/SMESHConfig.cmake DESTINATION cmake)

# --------------------------------------------------------------------------- #
# copied from FreeCAD salomemesh 7.7
# --------------------------------------------------------------------------- #
if(WIN32)
    INSTALL(TARGETS ${SMESH_LIBRARIES}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS ${SMESH_LIBRARIES}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(WIN32)

#netgen plugin has been added into ${SMESH_LIBRARIES}
