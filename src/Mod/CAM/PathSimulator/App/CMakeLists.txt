include_directories (
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS})

link_directories (${OCC_LIBRARY_DIR})

set (PathSimulator_LIBS Path Part Mesh FreeCADApp)

set (Python_SRCS PathSimPy.xml PathSimPyImp.cpp)

set (
    PathSimulator_SRCS
    AppPathSimulator.cpp
    PathSim.cpp
    PathSim.h
    VolSim.cpp
    VolSim.h
    PreCompiled.cpp
    PreCompiled.h
    ${Python_SRCS})

generate_from_xml (PathSimPy)

source_group ("Python" FILES ${Python_SRCS})

add_library (PathSimulator SHARED ${PathSimulator_SRCS})
target_link_libraries (PathSimulator ${PathSimulator_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (PathSimulator)
endif ()

set_bin_dir (PathSimulator PathSimulator /Mod/CAM)
set_python_prefix_suffix (PathSimulator)

install (TARGETS PathSimulator DESTINATION ${CMAKE_INSTALL_LIBDIR})
