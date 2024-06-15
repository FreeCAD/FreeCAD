set(ENV{PATH} "${FREECAD_LIBPACK_DIR};$ENV{PATH}")
list(PREPEND CMAKE_PREFIX_PATH "${FREECAD_LIBPACK_DIR}")

# Make sure we are using the static versions of Boost here: the LibPack includes both
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost COMPONENTS filesystem program_options regex system thread date_time REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/lib/cmake NO_DEFAULT_PATH)

set(OCE_DIR ${FREECAD_LIBPACK_DIR}/lib/cmake CACHE PATH "" FORCE)

set(SWIG_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/swig.exe CACHE FILEPATH "Swig" FORCE)

find_package(Qt6 REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/lib/cmake NO_DEFAULT_PATH)
find_package(XercesC REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/cmake NO_DEFAULT_PATH)
find_package(Coin REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/lib/cmake NO_DEFAULT_PATH)

# For compatibility with the rest of the cMake scripts:
set (COIN3D_FOUND TRUE)

set (NETGENDATA ${FREECAD_LIBPACK_DIR}/include/netgen)

if (FREECAD_USE_FREETYPE)
    set(FREETYPE_INCLUDE_DIR_freetype2 ${FREECAD_LIBPACK_DIR}/include/freetype2)
endif (FREECAD_USE_FREETYPE)

set (HDF5_DIR ${FREECAD_LIBPACK_DIR}/share/cmake)

link_directories (${FREECAD_LIBPACK_DIR}/lib)
