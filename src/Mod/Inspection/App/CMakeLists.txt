include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS})

link_directories(${OCC_LIBRARY_DIR})

set(Inspection_LIBS FreeCADApp Mesh Points Part)

set(Inspection_SRCS AppInspection.cpp InspectionFeature.cpp InspectionFeature.h PreCompiled.cpp
                    PreCompiled.h)

set(Inspection_Scripts ../Init.py)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    get_msvc_precompiled_source("PreCompiled.cpp" PCH_SRCS ${Inspection_SRCS})
    add_msvc_precompiled_header(Inspection PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(Inspection SHARED ${Inspection_SRCS} ${Inspection_Scripts})
target_link_libraries(Inspection ${Inspection_LIBS})
if(FREECAD_WARN_ERROR)
    target_compile_warn_error(Inspection)
endif()

fc_target_copy_resource_flat(Inspection ${CMAKE_CURRENT_SOURCE_DIR}
                             ${CMAKE_BINARY_DIR}/Mod/Inspection ${Inspection_Scripts})

set_bin_dir(Inspection Inspection /Mod/Inspection)
set_python_prefix_suffix(Inspection)

install(TARGETS Inspection DESTINATION ${CMAKE_INSTALL_LIBDIR})
