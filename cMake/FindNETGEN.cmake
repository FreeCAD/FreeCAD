# Try to find nglib/netgen
# 
# Optional input NETGENDATA is path to the netgen libsrc source tree - this is
# required due to some headers not being installed by netgen.
#
# Once done this will define
#
# NGLIB_INCLUDE_DIR   - where the nglib include directory can be found
# NGLIB_LIBRARIES     - Link this to use nglib
# NETGEN_INCLUDE_DIRS - where the netgen include directories can be found
# NETGEN_DEFINITIONS  - C++ compiler defines required to use netgen/nglib
#
# See also: http://git.salome-platform.org/gitweb/?p=NETGENPLUGIN_SRC.git;a=summary

SET(NETGEN_DEFINITIONS -DNO_PARALLEL_THREADS -DOCCGEOMETRY)

IF(DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)
    # We haven't supported Netgen prior to 5.3.1 on MacOS, and the current
    # plan is for the next Netgen version to be 6.1 (currently unreleased).
    LIST( APPEND NETGEN_DEFINITIONS -DNETGEN_V5 )

    IF(DEFINED HOMEBREW_PREFIX)
        EXEC_PROGRAM(brew ARGS --prefix nglib OUTPUT_VARIABLE NGLIB_PREFIX)
    ELSE(DEFINED HOMEBREW_PREFIX)
        SET(NGLIB_PREFIX ${MACPORTS_PREFIX})
    ENDIF(DEFINED HOMEBREW_PREFIX)

    FIND_PATH(NGLIB_INCLUDE_DIR nglib.h ${NGLIB_PREFIX}/include)

    FIND_LIBRARY(NGLIB_LIBNGLIB nglib ${NGLIB_PREFIX}/lib)
    FIND_LIBRARY(NGLIB_LIBMESH mesh ${NGLIB_PREFIX}/lib)
    FIND_LIBRARY(NGLIB_LIBOCC occ ${NGLIB_PREFIX}/lib)
    FIND_LIBRARY(NGLIB_LIBINTERFACE interface ${NGLIB_PREFIX}/lib)
    SET(NGLIB_LIBRARIES ${NGLIB_LIBNGLIB} ${NGLIB_LIBMESH}
                        ${NGLIB_LIBOCC} ${NGLIB_LIBINTERFACE})

    IF(NOT NETGENDATA)
        SET(NETGENDATA ${NGLIB_PREFIX}/include/netgen)
    ENDIF(NOT NETGENDATA)

ELSE(DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)
    FIND_PATH(NGLIB_INCLUDE_DIR nglib.h /usr/include)
    FIND_LIBRARY(NGLIB_LIBRARIES nglib /usr/lib /usr/local/lib)

    IF(NOT NETGENDATA)
        SET(NETGENDATA /usr/share/netgen/libsrc)
    ENDIF(NOT NETGENDATA)

ENDIF(DEFINED MACPORTS_PREFIX OR DEFINED HOMEBREW_PREFIX)

FIND_PATH(NETGEN_DIR_csg csg.hpp PATHS ${NETGENDATA}/csg)
FIND_PATH(NETGEN_DIR_gen array.hpp PATHS ${NETGENDATA}/general)
FIND_PATH(NETGEN_DIR_geom2d geom2dmesh.hpp PATHS ${NETGENDATA}/geom2d)
FIND_PATH(NETGEN_DIR_gprim gprim.hpp PATHS ${NETGENDATA}/gprim)
FIND_PATH(NETGEN_DIR_la linalg.hpp PATHS ${NETGENDATA}/linalg)
FIND_PATH(NETGEN_DIR_mesh meshing.hpp PATHS ${NETGENDATA}/meshing)
FIND_PATH(NETGEN_DIR_occ occgeom.hpp PATHS ${NETGENDATA}/occ)
FIND_PATH(NETGEN_DIR_stlgeom stlgeom.hpp PATHS ${NETGENDATA}/stlgeom)

LIST( APPEND NETGEN_INCLUDE_DIRS
      ${NETGEN_DIR_csg} ${NETGEN_DIR_gen} ${NETGEN_DIR_geom2d} 
      ${NETGEN_DIR_gprim} ${NETGEN_DIR_la} ${NETGEN_DIR_mesh}
      ${NETGEN_DIR_occ} ${NETGEN_DIR_stlgeom} )

