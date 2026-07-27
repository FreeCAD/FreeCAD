# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif()

set(MbDFEM_Scripts
    Init.py
    TestMbDFEMApp.py
)

set(MbDFEM_Examples
    Examples/CreateMbDFEMModel.py
)

if(BUILD_GUI)
    list(APPEND MbDFEM_Scripts InitGui.py)
endif()

add_custom_target(MbDFEMScripts ALL
    SOURCES ${MbDFEM_Scripts} ${MbDFEM_Examples}
)

fc_target_copy_resource_flat(MbDFEMScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/MbDFEM
    ${MbDFEM_Scripts}
)

fc_target_copy_resource(MbDFEMScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/MbDFEM
    ${MbDFEM_Examples}
)

install(
    FILES ${MbDFEM_Scripts}
    DESTINATION Mod/MbDFEM
)

install(
    FILES ${MbDFEM_Examples}
    DESTINATION Mod/MbDFEM/Examples
)
