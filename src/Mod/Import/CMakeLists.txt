# SPDX-License-Identifier: LGPL-2.1-or-later

add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

#IF (BUILD_GUI)
#    PYSIDE_WRAP_RC(Import_QRC_SRCS Resources/Import.qrc)
#ENDIF (BUILD_GUI)

#ADD_CUSTOM_TARGET(ImportRC ALL
#    SOURCES ${Import_QRC_SRCS}
#)


#IF (BUILD_GUI)
#    fc_target_copy_resource(ImportRC
#        ${CMAKE_CURRENT_BINARY_DIR}
#        ${CMAKE_BINARY_DIR}/Mod/Import
#        Import_rc.py)
#ENDIF (BUILD_GUI)

SET(DXF_Plate
    DxfPlate/blocks10.rub
    DxfPlate/blocks112.rub
    DxfPlate/blocks114.rub
    DxfPlate/classes14.rub
    DxfPlate/entities0.rub
    DxfPlate/entities12.rub
    DxfPlate/entities14.rub
    DxfPlate/header0.rub
    DxfPlate/header12.rub
    DxfPlate/header14.rub
    DxfPlate/objects14.rub
    DxfPlate/tables10.rub
    DxfPlate/tables112.rub
    DxfPlate/tables114.rub
    DxfPlate/tables20.rub
    DxfPlate/tables212.rub
    DxfPlate/tables214.rub
)

set(Import_Scripts
    Init.py
    stepZ.py
)

if(BUILD_GUI)
    list (APPEND Import_Scripts InitGui.py)
    list (APPEND Import_Scripts TestImportGui.py)
endif(BUILD_GUI)

# Add importtests package files so they are available in the build/install tree.
# Non-GUI test shims and fixtures
list(APPEND Import_Scripts
    TestImport.py
    importtests/__init__.py
    importtests/TestImportBase.py
)

if(BUILD_GUI)
    # GUI-only test shims
    list(APPEND Import_Scripts
        importtests/TestImportBaseGui.py
        importtests/TestImportStepGui.py
    )
endif(BUILD_GUI)

# Fixtures used by importtests (copied into build tree and installed)
set(importtests_FIXTURES
    importtests/fixtures/__init__.py
    importtests/fixtures/README.md
)


add_custom_target(ImportScripts ALL
    SOURCES ${Import_Scripts} ${DXF_Plate}
)

fc_target_copy_resource(ImportScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Import
    ${Import_Scripts}
)

# Copy fixtures into the build tree so tests can access them
fc_copy_sources(ImportScripts "${CMAKE_BINARY_DIR}/Mod/Import" ${importtests_FIXTURES})

fc_target_copy_resource(ImportScripts
    ${CMAKE_SOURCE_DIR}/src/Mod/Import
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Import
    ${DXF_Plate}
)

INSTALL(
    FILES
        ${Import_Scripts}
        #${Import_QRC_SRCS}
    DESTINATION
        Mod/Import
)

INSTALL(
    FILES
        ${importtests_FIXTURES}
    DESTINATION
        Mod/Import/importtests/fixtures
)

INSTALL(
    DIRECTORY
        DxfPlate
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/Mod/Import
    FILES_MATCHING
        PATTERN "*.rub*"
)
