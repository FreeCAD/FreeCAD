project(MyPlugin)
cmake_minimum_required(VERSION 3.5)

# Qt5
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Gui REQUIRED)

# Boost
find_package(Boost COMPONENTS signals REQUIRED)

# Coin3d
#find_package(Coin3D REQUIRED)

# Python
find_package(PythonInterp REQUIRED)
set(Python_ADDITIONAL_VERSIONS ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
IF (NOT DEFINED PYTHON_VERSION_STRING)
    find_package(PythonLibs REQUIRED)
ELSE (NOT DEFINED PYTHON_VERSION_STRING)
    find_package(PythonLibs ${PYTHON_VERSION_STRING} EXACT)
ENDIF(NOT DEFINED PYTHON_VERSION_STRING)

# FreeCAD
set(FREECAD_SRC_DIR ${CMAKE_SOURCE_DIR}/../../../.. CACHE PATH  "FreeCAD sources")
set(FREECAD_BIN_DIR ${CMAKE_SOURCE_DIR}/../../../.. CACHE PATH  "FreeCAD binaries")
set(FREECAD_LIBPACK_DIR ${CMAKE_SOURCE_DIR} CACHE PATH  "Directory of the FreeCAD LibPack")

# -----------------------------------------------------------------------------

add_definitions(-DBOOST_ALL_DYN_LINK)

include_directories(
    ${Boost_INCLUDE_DIRS}
    #${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${FREECAD_SRC_DIR}
    "${FREECAD_LIBPACK_DIR}/include"
)

link_directories(${FREECAD_LIBPACK_DIR}/lib)
link_directories(${FREECAD_BIN_DIR})

# -----------------------------------------------------------------------------

set(MyPlugin_SRCS
    main.cpp
)

set(MyPlugin_LIBS
    ${Boost_LIBRARIES}
    #${COIN3D_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
)

add_executable(MyPlugin WIN32 ${MyPlugin_SRCS})
target_link_libraries(MyPlugin ${MyPlugin_LIBS})

# -----------------------------------------------------------------------------

set(FreeCADPlugin_SRCS
    plugin.cpp
)

set(FreeCADPlugin_LIBS
    ${Boost_LIBRARIES}
    #${COIN3D_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Gui_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
)

if (MSVC)
    set(FreeCADPlugin_LIBS
        ${FreeCADPlugin_LIBS}
        debug FreeCADApp_d
        debug FreeCADBase_d
        debug FreeCADGui_d
        #debug Mesh_d
        optimized FreeCADApp
        optimized FreeCADBase
        optimized FreeCADGui
        #optimized Mesh
    )
else()
    set(FreeCADPlugin_LIBS
        ${FreeCADPlugin_LIBS}
        FreeCADApp
        FreeCADBase
        FreeCADGui
        #Mesh
    )
endif()

add_library(FreeCADPlugin SHARED ${FreeCADPlugin_SRCS})
target_link_libraries(FreeCADPlugin ${FreeCADPlugin_LIBS})
