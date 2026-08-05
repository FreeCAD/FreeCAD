# SPDX-License-Identifier: LGPL-2.1-or-later

set(MbDFEM_LIBS
    FreeCADApp
    Part
)

generate_from_py(MbDAssembly)
generate_from_py(MbDItemIJ)
generate_from_py(MbDPart)

set(MbDFEM_SRCS
    ../MbDFEMGlobal.h
    AppMbDFEM.cpp
    MbDAction.cpp
    MbDAction.h
    MbDAssembly.cpp
    MbDAssembly.h
    MbDAssembly.pyi
    MbDAssemblyPyImp.cpp
    MbDFolders.cpp
    MbDFolders.h
    MbDGroupUtils.cpp
    MbDGroupUtils.h
    MbDItemIJ.cpp
    MbDItemIJ.h
    MbDItemIJ.pyi
    MbDItemIJPyImp.cpp
    MbDJoint.cpp
    MbDJoint.h
    MbDMarker.cpp
    MbDMarker.h
    MbDMotion.cpp
    MbDMotion.h
    MbDParameters.cpp
    MbDParameters.h
    MbDPart.cpp
    MbDPart.h
    MbDPart.pyi
    MbDPartPyImp.cpp
)

add_library(MbDFEM SHARED ${MbDFEM_SRCS})

target_include_directories(
    MbDFEM
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(MbDFEM ${MbDFEM_LIBS})

if(FREECAD_WARN_ERROR)
    target_compile_warn_error(MbDFEM)
endif()

SET_BIN_DIR(MbDFEM MbDFEM /Mod/MbDFEM)
SET_PYTHON_PREFIX_SUFFIX(MbDFEM)

install(TARGETS MbDFEM DESTINATION ${CMAKE_INSTALL_LIBDIR})
