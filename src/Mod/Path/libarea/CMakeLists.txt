# Turn compiler warnings on for gcc 
if (CMAKE_BUILD_TOOL MATCHES "make")
    MESSAGE(STATUS "setting gcc options: -Wall -Werror -Wno-deprecated -pedantic-errors")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif (CMAKE_BUILD_TOOL MATCHES "make")

include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(NOT FREECAD_LIBPACK_USE OR FREECAD_LIBPACK_CHECKFILE_CLBUNDLER)
    find_package( Boost COMPONENTS python REQUIRED)  # find BOOST and boost-python
    if(Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIRS})
        MESSAGE(STATUS "found Boost: " ${Boost_LIB_VERSION})
        MESSAGE(STATUS "boost-incude dirs are: " ${Boost_INCLUDE_DIRS})
        MESSAGE(STATUS "boost-python lib is: " ${Boost_PYTHON_LIBRARY})
        MESSAGE(STATUS "boost_LIBRARY_DIRS is: " ${Boost_LIBRARY_DIRS})
        MESSAGE(STATUS "Boost_LIBRARIES is: " ${Boost_LIBRARIES})
    endif()
else()
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# this defines the source-files for library
set(AREA_SRC_COMMON
    Arc.cpp
    Area.cpp
    AreaOrderer.cpp
    AreaPocket.cpp
    Circle.cpp
    Curve.cpp
    kurve/Construction.cpp
    kurve/Finite.cpp
    kurve/kurve.cpp
    kurve/Matrix.cpp
    kurve/offset.cpp
)

set(AREA_SRC_CLIPPER
    AreaClipper.cpp
    clipper.cpp
)

# this defines the additional source-files for python module (wrapper to libarea)
set(PYAREA_SRC
    PythonStuff.cpp
)

# this defines the headers
if(DEFINED INCLUDE_INSTALL_DIR)
    set(includedir ${INCLUDE_INSTALL_DIR})
else(DEFINED INCLUDE_INSTALL_DIR)
    set(INCLUDE_INSTALL_DIR include)
    set(includedir ${CMAKE_INSTALL_PREFIX}/${INCLUDE_INSTALL_DIR})
endif(DEFINED INCLUDE_INSTALL_DIR)

file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/kurve/*.h")
file(GLOB headers "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

# this makes the Python module
add_library(
  area
  MODULE
  ${AREA_SRC_COMMON}
  ${AREA_SRC_CLIPPER}
  ${PYAREA_SRC}
)

if(MSVC)
    set(area_LIBS
        ${Boost_LIBRARIES}
        ${PYTHON_LIBRARIES}
        debug MSVCRTD.LIB
        debug MSVCPRTD.LIB
        optimized MSVCRT.LIB
        optimized MSVCPRT.LIB
    )
elseif(MINGW)
    set(area_LIBS
        ${Boost_LIBRARIES}
        ${PYTHON_LIBRARIES}
        Rpcrt4.lib
    )
else(MSVC)
    set(area_LIBS
        ${Boost_LIBRARIES}
        ${PYTHON_LIBRARIES}
    )
endif(MSVC)

target_link_libraries(area ${area_LIBS})

SET_BIN_DIR(area area)
SET_PYTHON_PREFIX_SUFFIX(area)

# this figures out where to install the Python modules
execute_process(
    COMMAND python -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"
    OUTPUT_VARIABLE Python_site_packages
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "area module (for Path Workbench) will be installed to: " ${CMAKE_INSTALL_LIBDIR})

# this installs the python library
install(
    TARGETS area
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
