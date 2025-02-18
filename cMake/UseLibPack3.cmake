set(ENV{PATH} "${FREECAD_LIBPACK_DIR};$ENV{PATH}")
list(PREPEND CMAKE_PREFIX_PATH "${FREECAD_LIBPACK_DIR}")

set (Python3_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/python.exe)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Make sure we are using the shared versions of Boost here: the LibPack includes both
set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost COMPONENTS filesystem program_options regex system thread date_time REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/lib/cmake NO_DEFAULT_PATH)

set(OCE_DIR ${FREECAD_LIBPACK_DIR}/lib/cmake CACHE PATH "" FORCE)

set(SWIG_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/swig.exe CACHE FILEPATH "Swig" FORCE)

find_package(Qt6 REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/lib/cmake NO_DEFAULT_PATH)
message(STATUS "Found LibPack 3 Qt ${Qt6_VERSION}")

if(FREECAD_LIBPACK_VERSION VERSION_GREATER_EQUAL "3.1.0")
    find_package(pybind11 REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/share/cmake/pybind11 NO_DEFAULT_PATH)
    message(STATUS "Found LibPack 3 pybind11 ${pybind11_VERSION}")
    set(FREECAD_USE_PYBIND11 ON)
endif()

find_package(XercesC REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/cmake NO_DEFAULT_PATH)
message(STATUS "Found LibPack 3 XercesC ${XercesC_VERSION}")

find_package(yaml-cpp REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/lib/cmake NO_DEFAULT_PATH)
message(STATUS "Found LibPack 3 yaml-cpp ${yaml-cpp_VERSION}")

find_package(Coin REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/lib/cmake NO_DEFAULT_PATH)
message(STATUS "Found LibPack 3 Coin ${Coin_VERSION}")
# For compatibility with the rest of the cMake scripts:
set (COIN3D_FOUND TRUE)

set (NETGENDATA ${FREECAD_LIBPACK_DIR}/include/netgen)

if (FREECAD_USE_FREETYPE)
    find_package(freetype REQUIRED PATHS ${FREECAD_LIBPACK_DIR}/lib/cmake NO_DEFAULT_PATH)
    message(STATUS "Found LibPack 3 Freetype ${freetype_VERSION} library from ${freetype_LIBRARY}")
endif (FREECAD_USE_FREETYPE)

set (HDF5_DIR ${FREECAD_LIBPACK_DIR}/share/cmake)

link_directories (${FREECAD_LIBPACK_DIR}/lib)
