if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

if(BUILD_MATERIAL_EXTERNAL)
    add_definitions(-DBUILD_MATERIAL_EXTERNAL)
endif(BUILD_MATERIAL_EXTERNAL)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(MatGui_LIBS
    Materials
    FreeCADGui
)

include_directories(
    SYSTEM
    ${QtConcurrent_INCLUDE_DIRS}
)
list(APPEND MatGui_LIBS
    ${QtConcurrent_LIBRARIES}
)

set(Material_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Material_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Material_TR_QRC} ${QM_SRCS})
qt_add_resources(MatGui_QRC_SRCS Resources/Material.qrc ${Material_TR_QRC})

generate_from_py(MaterialTreeWidget)

SET(Python_SRCS
    MaterialTreeWidget.pyi
    MaterialTreeWidgetPyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

set(MatGui_UIC_SRCS
    Array2D.ui
    Array3D.ui
    DlgDisplayProperties.ui
    DlgInspectAppearance.ui
    DlgInspectMaterial.ui
    DlgMaterial.ui
    DlgSettingsDefaultMaterial.ui
    DlgSettingsMaterial.ui
    ImageEdit.ui
    ListEdit.ui
    MaterialSave.ui
    MaterialsEditor.ui
    ModelSelect.ui
    TextEdit.ui
)
if(BUILD_MATERIAL_EXTERNAL)
    list(APPEND MatGui_UIC_SRCS
        DlgSettingsExternal.ui
        TaskMigrateExternal.ui
    )
endif(BUILD_MATERIAL_EXTERNAL)

SET(MatGui_SRCS
    ${Python_SRCS}
    ${MatGui_QRC_SRCS}
    ${MatGui_UIC_HDRS}
    AppearancePreview.h
    AppearancePreview.cpp
    AppMatGui.cpp
    Array2D.cpp
    Array2D.h
    Array2D.ui
    Array3D.cpp
    Array3D.h
    Array3D.ui
    ArrayDelegate.cpp
    ArrayDelegate.h
    ArrayModel.cpp
    ArrayModel.h
    BaseDelegate.cpp
    BaseDelegate.h
    Command.cpp
    DlgDisplayPropertiesImp.cpp
    DlgDisplayPropertiesImp.h
    DlgDisplayProperties.ui
    DlgInspectAppearance.cpp
    DlgInspectAppearance.h
    DlgInspectAppearance.ui
    DlgInspectMaterial.cpp
    DlgInspectMaterial.h
    DlgInspectMaterial.ui
    DlgMaterialImp.cpp
    DlgMaterialImp.h
    DlgMaterial.ui
    DlgSettingsDefaultMaterial.cpp
    DlgSettingsDefaultMaterial.h
    DlgSettingsDefaultMaterial.ui
    DlgSettingsMaterial.cpp
    DlgSettingsMaterial.h
    DlgSettingsMaterial.ui
    ImageEdit.cpp
    ImageEdit.h
    ImageEdit.ui
    ListDelegate.cpp
    ListDelegate.h
    ListEdit.cpp
    ListEdit.h
    ListEdit.ui
    ListModel.cpp
    ListModel.h
    MaterialDelegate.cpp
    MaterialDelegate.h
    MaterialSave.cpp
    MaterialSave.h
    MaterialSave.ui
    MaterialsEditor.cpp
    MaterialsEditor.h
    MaterialsEditor.ui
    MaterialTreeWidget.cpp
    MaterialTreeWidget.h
    ModelSelect.cpp
    ModelSelect.h
    ModelSelect.ui
    PreCompiled.cpp
    PreCompiled.h
    TextEdit.cpp
    TextEdit.h
    TextEdit.ui
    Workbench.cpp
    Workbench.h
    WorkbenchManipulator.cpp
    WorkbenchManipulator.h
)
if(BUILD_MATERIAL_EXTERNAL)
    list(APPEND MatGui_SRCS
        DlgSettingsExternal.cpp
        DlgSettingsExternal.h
        DlgSettingsExternal.ui
        TaskMigrateExternal.cpp
        TaskMigrateExternal.h
        TaskMigrateExternal.ui
    )
endif(BUILD_MATERIAL_EXTERNAL)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${MatGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(MatGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

SET(MatGuiIcon_SVG
    Resources/icons/list.svg
    Resources/icons/Material_Edit.svg
    Resources/icons/MaterialWorkbench.svg
    Resources/icons/multiline.svg
    Resources/icons/preferences-material.svg
    Resources/icons/preview-rendered.svg
    Resources/icons/preview-vector.svg
    Resources/icons/table.svg
)

SET(MatGuiImages
    Resources/images/default_image.png
)

SET(Material_Ui_Files
    Resources/ui/materials-editor.ui
)

add_library(MatGui SHARED ${MatGui_SRCS} ${MatGuiIcon_SVG} ${MatGuiImages} ${Material_Ui_Files})
target_link_libraries(MatGui ${MatGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(MatGui)
endif()

SET_BIN_DIR(MatGui MatGui /Mod/Material)
SET_PYTHON_PREFIX_SUFFIX(MatGui)

fc_copy_sources(MatGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material" ${MatGuiIcon_SVG})
fc_copy_sources(MatGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material" ${MatGuiImages})
fc_copy_sources(MatGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Material" ${Material_Ui_Files})

INSTALL(TARGETS MatGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${MatGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Material/Resources/icons")
INSTALL(FILES ${MatGuiImages} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Material/Resources/images")
INSTALL(FILES ${Material_Ui_Files} DESTINATION "Mod/Material/Resources/ui")
