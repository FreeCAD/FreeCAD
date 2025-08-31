# Turn compiler warnings on for gcc
if (CMAKE_BUILD_TOOL MATCHES "make")
    MESSAGE(STATUS "setting gcc options: -Wall -Werror -Wno-deprecated -pedantic-errors")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif (CMAKE_BUILD_TOOL MATCHES "make")

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS /wd4244)
endif(MSVC)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/Mod/Import/App/dxf
)

link_directories(${OCC_LIBRARY_DIR})


include_directories(${pybind11_INCLUDE_DIR})


# this defines the source-files for library
set(AREA_SRC_COMMON
    Arc.cpp
    Area.cpp
    AreaDxf.cpp
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
    Adaptive.cpp
    clipper.cpp
)

# this defines the additional source-files for python module (wrapper to libarea)
set(PYAREA_SRC
    pyarea.cpp
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
  area-native
  SHARED
  ${AREA_SRC_COMMON}
  ${AREA_SRC_CLIPPER}
)

add_library(
    area
    SHARED
    ${PYAREA_SRC}
)


if(MSVC)
    set(area_native_LIBS
        debug MSVCRTD.LIB
        debug MSVCPRTD.LIB
        optimized MSVCRT.LIB
        optimized MSVCPRT.LIB
    )

    #Universal C runtime introduced in VS 2015 (cl version 19)
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19"))
        list(APPEND area_native_LIBS
            debug vcruntimed.lib
            debug ucrtd.lib
            optimized vcruntime.lib
            optimized ucrt.lib
        )
    endif()

    set(area_LIBS
        ${Boost_LIBRARIES}
        ${area_native_LIBS}
    )
    if(BUILD_DYNAMIC_LINK_PYTHON)
        list(APPEND area_LIBS ${Python3_LIBRARIES})
    endif(BUILD_DYNAMIC_LINK_PYTHON)
elseif(MINGW)
    set(area_native_LIBS
        Rpcrt4.lib
    )
    set(area_LIBS
        ${Boost_LIBRARIES}
        ${area_native_LIBS}
    )
    if(BUILD_DYNAMIC_LINK_PYTHON)
        list(APPEND area_LIBS ${Python3_LIBRARIES})
    endif(BUILD_DYNAMIC_LINK_PYTHON)
else(MSVC)
    set(area_native_LIBS
        )
    set(area_LIBS
        ${Boost_LIBRARIES}
    )
    if(BUILD_DYNAMIC_LINK_PYTHON)
        list(APPEND area_LIBS ${Python3_LIBRARIES})
    endif(BUILD_DYNAMIC_LINK_PYTHON)
endif(MSVC)

target_link_libraries(area-native ${area_native_LIBS} Import)
SET_BIN_DIR(area-native area-native /Mod/CAM)

target_link_libraries(area area-native ${area_LIBS} ${area_native_LIBS})

# TODO why CMAKE_SHARED_LINKER_FLAGS is not used here?
# This is a dirty workaround!
if(NOT BUILD_DYNAMIC_LINK_PYTHON AND CMAKE_COMPILER_IS_CLANGXX)
    if(APPLE)
        target_link_libraries(area "-Wl,-undefined,dynamic_lookup")
    else(UNIX)
        target_link_libraries(area "-Wl,--undefined,dynamic_lookup")
    endif()
endif()

SET_BIN_DIR(area area /Mod/CAM)
SET_PYTHON_PREFIX_SUFFIX(area)

EXECUTE_PROCESS(COMMAND ${Python3_EXECUTABLE} -c
    "from sysconfig import get_path; print(get_path('platlib'))"
    OUTPUT_VARIABLE Python_site_packages OUTPUT_STRIP_TRAILING_WHITESPACE )

message(STATUS "area module (for Path Workbench) will be installed to: " ${CMAKE_INSTALL_LIBDIR})

if(WIN32)
    set_target_properties(area-native PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    INSTALL(TARGETS area-native
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS area-native
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif(WIN32)

# this installs the python library
install(
    TARGETS area
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
