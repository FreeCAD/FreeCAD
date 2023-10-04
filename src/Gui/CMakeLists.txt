#add_subdirectory(Icons)
add_subdirectory(Stylesheets)
add_subdirectory(PreferencePacks)
add_subdirectory(PreferencePackTemplates)

if(WIN32)
add_definitions(-DFCGui -DQSINT_MAKEDLL -DOVR_OS_WIN32 -DQUARTER_INTERNAL -DQUARTER_MAKE_DLL -DCOIN_DLL)
endif(WIN32)

IF(CMAKE_BUILD_TYPE)
    add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
ENDIF(CMAKE_BUILD_TYPE)

if (FREECAD_USE_3DCONNEXION)
    add_definitions(-D_USE_3DCONNEXION_SDK)
    if(APPLE)
        set(3DCONNEXION_LINKFLAGS "-F/Library/Frameworks -weak_framework 3DconnexionClient")
        set(3DCONNEXION_INCLUDE_DIR ${3DCONNEXIONCLIENT_FRAMEWORK}/Headers
                                    ${3DCONNEXIONCLIENT_FRAMEWORK}/Headers/3DconnexionClient )
    endif(APPLE)
endif(FREECAD_USE_3DCONNEXION)

if (BUILD_VR)
    add_definitions(-DBUILD_VR )
endif(BUILD_VR)

if (BUILD_ADDONMGR)
    add_definitions(-DBUILD_ADDONMGR )
endif(BUILD_ADDONMGR)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Quarter
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/Language
    ${CMAKE_CURRENT_BINARY_DIR}/propertyeditor
    ${CMAKE_CURRENT_BINARY_DIR}/TaskView
    ${CMAKE_CURRENT_BINARY_DIR}/Quarter
    ${CMAKE_CURRENT_BINARY_DIR}/DAGView
    ${Boost_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${3DCONNEXION_INCLUDE_DIR}
)

if(MSVC)
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/3rdParty/OpenGL/api
    )
endif(MSVC)

if(MSVC)
    set(FreeCADGui_LIBS
         FreeCADApp
         ${COIN3D_LIBRARIES}
         ${OPENGL_gl_LIBRARY}
    )

    if(FREECAD_USE_3DCONNEXION)
        list(APPEND FreeCADGui_LIBS
            hid
        )
    endif()

else(MSVC)
    set(FreeCADGui_LIBS
        FreeCADApp
        ${COIN3D_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OPENGL_gl_LIBRARY}
        ${3DCONNEXION_LINKFLAGS}
    )
endif(MSVC)

if (QtWebEngineWidgets_FOUND AND BUILD_WITH_CONDA)
    add_definitions(-DQTWEBENGINE)
endif()

include_directories(
    ${QtCore_INCLUDE_DIRS}
    ${QtWidgets_INCLUDE_DIRS}
    ${QtOpenGL_INCLUDE_DIRS}
    ${QtPrintSupport_INCLUDE_DIRS}
    ${QtSvg_INCLUDE_DIRS}
    ${QtSvgWidgets_INCLUDE_DIRS}
    ${QtNetwork_INCLUDE_DIRS}
    ${QtUiTools_INCLUDE_DIRS}
)
list(APPEND FreeCADGui_LIBS
    ${QtCore_LIBRARIES}
    ${QtWidgets_LIBRARIES}
    ${QtOpenGL_LIBRARIES}
    ${QtPrintSupport_LIBRARIES}
    ${QtSvg_LIBRARIES}
    ${QtSvgWidgets_LIBRARIES}
    ${QtNetwork_LIBRARIES}
    ${QtUiTools_LIBRARIES}
)

if(${Qt5WinExtras_FOUND})
    include_directories(
        ${Qt5WinExtras_INCLUDE_DIRS}
    )
    list(APPEND FreeCADGui_LIBS
        ${Qt5WinExtras_LIBRARIES}
    )
endif()

IF(SPNAV_FOUND)
    if(SPNAV_USE_X11)
        add_definitions(-DSPNAV_USE_X11)
        if (FREECAD_QT_MAJOR_VERSION EQUAL 5 AND UNIX AND NOT APPLE)
            find_package(Qt5X11Extras REQUIRED)
            include_directories(${Qt5X11Extras_INCLUDE_DIRS})
            list(APPEND FreeCADGui_LIBS ${Qt5X11Extras_LIBRARIES})
            # Note that Qt6 has removed the QtX11Extras files
        endif()
        find_package(X11 QUIET)
        if (X11_FOUND)
            list(APPEND FreeCADGui_LIBS
                ${X11_X11_LIB}
            )
        endif(X11_FOUND)
        SET(FreeCADGui_SDK_SRCS
            3Dconnexion/GuiAbstractNativeEvent.cpp
            3Dconnexion/GuiNativeEventLinuxX11.cpp
            )
        SET(FreeCADGui_SDK_MOC_HDRS
            3Dconnexion/GuiAbstractNativeEvent.h
            3Dconnexion/GuiNativeEventLinuxX11.h
            )
    else(SPNAV_USE_X11)
        SET(FreeCADGui_SDK_SRCS
            3Dconnexion/GuiAbstractNativeEvent.cpp
            3Dconnexion/GuiNativeEventLinux.cpp
            )
        SET(FreeCADGui_SDK_MOC_HDRS
            3Dconnexion/GuiAbstractNativeEvent.h
            3Dconnexion/GuiNativeEventLinux.h
            )
    endif(SPNAV_USE_X11)
    SOURCE_GROUP("3D connexion SDK" FILES ${FreeCADGui_SDK_SRCS})

    add_definitions(-DSPNAV_FOUND)
    include_directories(
        ${SPNAV_INCLUDE_DIR}
    )
    list(APPEND FreeCADGui_LIBS
        ${SPNAV_LIBRARIES}
    )
ENDIF(SPNAV_FOUND)

IF(OCULUS_FOUND)
    add_definitions(-DOCULUS_FOUND)
    include_directories(
        ${OCULUS_INCLUDE_DIRS}
    )
    list(APPEND FreeCADGui_LIBS
        ${OCULUS_LIBRARIES}
    )
ENDIF(OCULUS_FOUND)

if(FREECAD_USE_SHIBOKEN)
    add_definitions(-DHAVE_SHIBOKEN${PYSIDE_MAJOR_VERSION})
    include_directories(
        ${SHIBOKEN_INCLUDE_DIR}
    )
    if (SHIBOKEN_LIBRARY)
        list(APPEND FreeCADGui_LIBS
            ${SHIBOKEN_LIBRARY}
        )
    else (SHIBOKEN_LIBRARY)
        if (TARGET Shiboken2::libshiboken)
            list(APPEND FreeCADGui_LIBS
                Shiboken2::libshiboken
            )
        elseif (TARGET Shiboken6::libshiboken)
            list(APPEND FreeCADGui_LIBS
                Shiboken6::libshiboken
            )
        endif (TARGET Shiboken2::libshiboken)
    endif (SHIBOKEN_LIBRARY)
endif(FREECAD_USE_SHIBOKEN)

if(FREECAD_USE_PYSIDE)
    include_directories(
        ${PYSIDE_INCLUDE_DIR}
        ${PYSIDE_INCLUDE_DIR}/QtCore
        ${PYSIDE_INCLUDE_DIR}/QtGui
    )
    if (PYSIDE_LIBRARY)
        list(APPEND FreeCADGui_LIBS
            ${PYSIDE_LIBRARY}
        )
    else (PYSIDE_LIBRARY)
        if (TARGET PySide2::pyside2)
            list(APPEND FreeCADGui_LIBS
                PySide2::pyside2
            )
        elseif (TARGET PySide6::pyside6)
            list(APPEND FreeCADGui_LIBS
                PySide6::pyside6
            )
        endif ()
    endif (PYSIDE_LIBRARY)
    include_directories(
        ${PYSIDE_INCLUDE_DIR}/QtWidgets
    )
    add_definitions(-DHAVE_PYSIDE${PYSIDE_MAJOR_VERSION})
endif(FREECAD_USE_PYSIDE)

generate_from_xml(DocumentPy)
generate_from_xml(PythonWorkbenchPy)
generate_from_xml(ViewProviderPy)
generate_from_xml(ViewProviderDocumentObjectPy)
generate_from_xml(ViewProviderExtensionPy)
generate_from_xml(WorkbenchPy)
generate_from_xml(SelectionObjectPy)
generate_from_xml(LinkViewPy)
generate_from_xml(ViewProviderLinkPy)
generate_from_xml(AxisOriginPy)
generate_from_xml(CommandPy)

generate_from_py(FreeCADGuiInit GuiInitScript.h)

# The XML files
SET(FreeCADGui_XML_SRCS
    ViewProviderDocumentObjectPy.xml
    ViewProviderPy.xml
    ViewProviderExtensionPy.xml
    PythonWorkbenchPy.xml
    WorkbenchPy.xml
    SelectionObjectPy.xml
    DocumentPy.xml
    LinkViewPy.xml
    ViewProviderLinkPy.xml
    AxisOriginPy.xml
    CommandPy.xml
)
SOURCE_GROUP("XML" FILES ${FreeCADApp_XML_SRCS})

# The 3D Connexion SDK files
if(FREECAD_USE_3DCONNEXION AND MSVC)
SET(FreeCADGui_SDK_SRCS
    3Dconnexion/I3dMouseParams.h
    3Dconnexion/MouseParameters.cpp
    3Dconnexion/MouseParameters.h
    3Dconnexion/GuiAbstractNativeEvent.cpp
    3Dconnexion/GuiNativeEventWin32.cpp
)
SOURCE_GROUP("3D connexion SDK" FILES ${FreeCADGui_SDK_SRCS})
SET(FreeCADGui_SDK_MOC_HDRS
  3Dconnexion/GuiAbstractNativeEvent.h
  3Dconnexion/GuiNativeEventWin32.h
)
endif(FREECAD_USE_3DCONNEXION AND MSVC)

if(FREECAD_USE_3DCONNEXION AND APPLE)
SET(FreeCADGui_SDK_SRCS
    3Dconnexion/GuiAbstractNativeEvent.cpp
    3Dconnexion/GuiNativeEventMac.cpp
)
SOURCE_GROUP("3D connexion SDK" FILES ${FreeCADGui_SDK_SRCS})
SET(FreeCADGui_SDK_MOC_HDRS
  3Dconnexion/GuiAbstractNativeEvent.h
  3Dconnexion/GuiNativeEventMac.h
)
endif(FREECAD_USE_3DCONNEXION AND APPLE)

set_property(SOURCE GraphvizView.h GraphvizView.cpp PROPERTY SKIP_AUTOMOC ON)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView-internal.cpp
    COMMAND ${QtCore_MOC_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView-internal.cpp ${CMAKE_CURRENT_SOURCE_DIR}/GraphvizView.cpp
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GraphvizView.cpp)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView.cpp
    COMMAND ${QtCore_MOC_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView.cpp ${CMAKE_CURRENT_SOURCE_DIR}/GraphvizView.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GraphvizView.h)

set_property(SOURCE GraphvizView.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView-internal.cpp)
set_property(SOURCE GraphvizView.h APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView.cpp)

SET(Gui_UIC_SRCS
    AboutApplication.ui
    Clipping.ui
    DemoMode.ui
    DlgActions.ui
    DlgActivateWindow.ui
    DlgUnitsCalculator.ui
    DlgAuthorization.ui
    DlgChooseIcon.ui
    DlgCreateNewPreferencePack.ui
    DlgCustomizeSpNavSettings.ui
    DlgDisplayProperties.ui
    DlgInputDialog.ui
    DlgKeyboard.ui
    DlgMacroExecute.ui
    DlgRunExternal.ui
    DlgMacroRecord.ui
    DlgMaterialProperties.ui
    DlgOnlineHelp.ui
    DlgParameter.ui
    DlgParameterFind.ui
    DlgPreferencePackManagement.ui
    DlgPreferences.ui
    DlgProjectInformation.ui
    DlgProjectUtility.ui
    DlgPropertyLink.ui
    DlgRevertToBackupConfig.ui
    PreferencePages/DlgSettings3DView.ui
    PreferencePages/DlgSettingsCacheDirectory.ui
    DlgSettingsColorGradient.ui
    PreferencePages/DlgSettingsDocument.ui
    PreferencePages/DlgSettingsEditor.ui
    PreferencePages/DlgSettingsGeneral.ui
    DlgSettingsImage.ui
    PreferencePages/DlgSettingsMacro.ui
    PreferencePages/DlgSettingsNavigation.ui
    PreferencePages/DlgSettingsNotificationArea.ui
    PreferencePages/DlgSettingsPythonConsole.ui
    PreferencePages/DlgSettingsReportView.ui
    PreferencePages/DlgSettingsSelection.ui
    PreferencePages/DlgSettingsTheme.ui
    PreferencePages/DlgSettingsViewColor.ui
    PreferencePages/DlgSettingsWorkbenches.ui
    DlgCheckableMessageBox.ui
    DlgToolbars.ui
    DlgTreeWidget.ui
    DlgLocationAngle.ui
    DlgLocationPos.ui
    DocumentRecovery.ui
    DownloadManager.ui
    DownloadItem.ui
    DlgExpressionInput.ui
    MouseButtons.ui
    SceneInspector.ui
    InputVector.ui
    Placement.ui
    TextureMapping.ui
    TaskView/TaskAppearance.ui
    TaskView/TaskOrientation.ui
    TaskView/TaskImage.ui
    TaskView/TaskSelectLinkProperty.ui
    TaskElementColors.ui
    DlgObjectSelection.ui
    DlgAddProperty.ui
    VectorListEditor.ui
)

set (FreeCAD_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Language/FreeCAD_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Language/FreeCAD_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Language)
qt_create_resource_file_prefix(${FreeCAD_TR_QRC} ${QM_SRCS})
set(Gui_RES_SRCS
    Icons/resource.qrc
    Language/translation.qrc
    ${FreeCAD_TR_QRC}
    QSint/actionpanel/schemes.qrc
)

qt_add_resources(Gui_QRC_SRCS ${Gui_RES_SRCS})

SOURCE_GROUP("Uic" FILES ${Gui_UIC_HDRS})

# The command sources
SET(Command_CPP_SRCS
    Action.cpp
    ActionFunction.cpp
    Command.cpp
    CommandDoc.cpp
    CommandFeat.cpp
    CommandMacro.cpp
    CommandStd.cpp
    CommandWindow.cpp
    CommandTest.cpp
    CommandView.cpp
    CommandStructure.cpp
    CommandLink.cpp
    CommandPyImp.cpp
    ShortcutManager.cpp
    CommandCompleter.cpp
    CommandActionPy.cpp
)
SET(Command_SRCS
    ${Command_CPP_SRCS}
    Action.h
    ActionFunction.h
    Command.h
    CommandT.h
    ShortcutManager.h
    CommandCompleter.h
    CommandActionPy.h
)
SOURCE_GROUP("Command" FILES ${Command_SRCS})

# The dialog sources
SET(Dialog_CPP_SRCS
    Clipping.cpp
    DemoMode.cpp
    DlgActivateWindowImp.cpp
    DlgCreateNewPreferencePackImp.cpp
    DlgUnitsCalculatorImp.cpp
    DlgDisplayPropertiesImp.cpp
    DlgInputDialogImp.cpp
    DlgMacroExecuteImp.cpp
    DlgRunExternal.cpp
    DlgEditFileIncludePropertyExternal.cpp
    DlgMacroRecordImp.cpp
    DlgMaterialPropertiesImp.cpp
    DlgParameterImp.cpp
    DlgParameterFind.cpp
    DlgPreferencePackManagementImp.cpp
    DlgProjectInformationImp.cpp
    DlgProjectUtility.cpp
    DlgPropertyLink.cpp
    DlgRevertToBackupConfigImp.cpp
    DlgExpressionInput.cpp
    TaskDlgRelocation.cpp
    DlgCheckableMessageBox.cpp
    TaskCSysDragger.cpp
    DlgUndoRedo.cpp
    InputVector.cpp
    Placement.cpp
    PropertyPage.cpp
    SceneInspector.cpp
    TextureMapping.cpp
    Transform.cpp
    DownloadItem.cpp
    DownloadManager.cpp
    DocumentRecovery.cpp
    TaskElementColors.cpp
    DlgObjectSelection.cpp
    DlgAddProperty.cpp
    VectorListEditor.cpp
)

SET(Dialog_HPP_SRCS
    Clipping.h
    DemoMode.h
    DlgActivateWindowImp.h
    DlgCreateNewPreferencePackImp.h
    DlgUnitsCalculatorImp.h
    DlgDisplayPropertiesImp.h
    DlgInputDialogImp.h
    DlgMacroExecuteImp.h
    DlgRunExternal.h
    DlgEditFileIncludePropertyExternal.h
    DlgMacroRecordImp.h
    DlgMaterialPropertiesImp.h
    DlgParameterImp.h
    DlgParameterFind.h
    DlgPreferencePackManagementImp.h
    DlgProjectInformationImp.h
    DlgProjectUtility.h
    DlgPropertyLink.h
    DlgRevertToBackupConfigImp.h
    DlgCheckableMessageBox.h
    DlgExpressionInput.h
    TaskDlgRelocation.h
    TaskCSysDragger.h
    DlgUndoRedo.h
    InputVector.h
    Placement.h
    PropertyPage.h
    SceneInspector.h
    TextureMapping.h
    Transform.h
    DownloadItem.h
    DownloadManager.h
    DocumentRecovery.h
    TaskElementColors.h
    DlgObjectSelection.h
    DlgAddProperty.h
    VectorListEditor.h
)

SET(Dialog_SRCS
    ${Dialog_CPP_SRCS}
    ${Dialog_HPP_SRCS}
    AboutApplication.ui
    Clipping.ui
    DemoMode.ui
    DlgActivateWindow.ui
    DlgUnitsCalculator.ui
    DlgAuthorization.ui
    DlgDisplayProperties.ui
    DlgInputDialog.ui
    DlgAddProperty.ui
    DlgLocationAngle.ui
    DlgLocationPos.ui
    DlgMacroExecute.ui
    DlgRunExternal.ui
    DlgMacroRecord.ui
    DlgMaterialProperties.ui
    DlgParameter.ui
    DlgParameterFind.ui
    DlgPreferencePackManagement.ui
    DlgProjectInformation.ui
    DlgProjectUtility.ui
    DlgPropertyLink.ui
    DlgRevertToBackupConfig.ui
    DlgCheckableMessageBox.ui
    DlgTreeWidget.ui
    DlgExpressionInput.ui
    DlgCreateNewPreferencePack.ui
    DownloadManager.ui
    DownloadItem.ui
    DocumentRecovery.ui
    MouseButtons.ui
    InputVector.ui
    Placement.ui
    SceneInspector.ui
    TextureMapping.ui
    TaskElementColors.ui
    DlgObjectSelection.ui
    VectorListEditor.ui
)
SOURCE_GROUP("Dialog" FILES ${Dialog_SRCS})

# The customize dialog sources
SET(Dialog_Customize_CPP_SRCS
    DlgActionsImp.cpp
    DlgCustomizeImp.cpp
    DlgCustomizeSpaceball.cpp
    DlgCustomizeSpNavSettings.cpp
    DlgKeyboardImp.cpp
    DlgToolbarsImp.cpp
    QListWidgetCustom.cpp
    ListWidgetDragBugFix.cpp
)
SET(Dialog_Customize_HPP_SRCS
    DlgActionsImp.h
    DlgCustomizeImp.h
    DlgCustomizeSpaceball.h
    DlgCustomizeSpNavSettings.h
    DlgKeyboardImp.h
    DlgToolbarsImp.h
    QListWidgetCustom.h
    ListWidgetDragBugFix.h
)
SET(Dialog_Customize_SRCS
    ${Dialog_Customize_CPP_SRCS}
    ${Dialog_Customize_HPP_SRCS}
    DlgActions.ui
    DlgChooseIcon.ui
    DlgCustomizeSpNavSettings.ui
    DlgKeyboard.ui
    DlgToolbars.ui
)
SOURCE_GROUP("Dialog\\Customize" FILES ${Dialog_Customize_SRCS})

# The settings dialog sources
SET(Dialog_Settings_CPP_SRCS
    DlgOnlineHelpImp.cpp
    DlgPreferencesImp.cpp
    PreferencePages/DlgSettings3DViewImp.cpp
    PreferencePages/DlgSettingsCacheDirectory.cpp
    DlgSettingsColorGradientImp.cpp
    PreferencePages/DlgSettingsDocumentImp.cpp
    PreferencePages/DlgSettingsEditor.cpp
    PreferencePages/DlgSettingsGeneral.cpp
    DlgSettingsImageImp.cpp
    PreferencePages/DlgSettingsMacroImp.cpp
    PreferencePages/DlgSettingsNavigation.cpp
    PreferencePages/DlgSettingsNotificationArea.cpp
    PreferencePages/DlgSettingsPythonConsole.cpp
    PreferencePages/DlgSettingsReportView.cpp
    PreferencePages/DlgSettingsSelection.cpp
    PreferencePages/DlgSettingsTheme.cpp
    PreferencePages/DlgSettingsViewColor.cpp
    PreferencePages/DlgSettingsWorkbenchesImp.cpp
)
SET(Dialog_Settings_HPP_SRCS
    DlgOnlineHelpImp.h
    DlgPreferencesImp.h
    PreferencePages/DlgSettings3DViewImp.h
    PreferencePages/DlgSettingsCacheDirectory.h
    DlgSettingsColorGradientImp.h
    PreferencePages/DlgSettingsDocumentImp.h
    PreferencePages/DlgSettingsEditor.h
    PreferencePages/DlgSettingsGeneral.h
    DlgSettingsImageImp.h
    PreferencePages/DlgSettingsMacroImp.h
    PreferencePages/DlgSettingsNavigation.h
    PreferencePages/DlgSettingsNotificationArea.h
    PreferencePages/DlgSettingsPythonConsole.h
    PreferencePages/DlgSettingsReportView.h
    PreferencePages/DlgSettingsSelection.h
    PreferencePages/DlgSettingsTheme.h
    PreferencePages/DlgSettingsViewColor.h
    PreferencePages/DlgSettingsWorkbenchesImp.h
)
SET(Dialog_Settings_SRCS
    ${Dialog_Settings_CPP_SRCS}
    ${Dialog_Settings_HPP_SRCS}
    DlgOnlineHelp.ui
    DlgPreferences.ui
    PreferencePages/DlgSettings3DView.ui
    PreferencePages/DlgSettingsCacheDirectory.ui
    DlgSettingsColorGradient.ui
    PreferencePages/DlgSettingsDocument.ui
    PreferencePages/DlgSettingsEditor.ui
    PreferencePages/DlgSettingsGeneral.ui
    DlgSettingsImage.ui
    PreferencePages/DlgSettingsMacro.ui
    PreferencePages/DlgSettingsNavigation.ui
    PreferencePages/DlgSettingsNotificationArea.ui
    PreferencePages/DlgSettingsPythonConsole.ui
    PreferencePages/DlgSettingsReportView.ui
    PreferencePages/DlgSettingsSelection.ui
    PreferencePages/DlgSettingsTheme.ui
    PreferencePages/DlgSettingsViewColor.ui
    PreferencePages/DlgSettingsWorkbenches.ui
)
SOURCE_GROUP("Dialog\\Settings" FILES ${Dialog_Settings_SRCS})

# The dock windows sources
SET(Dock_Windows_CPP_SRCS
    ComboView.cpp
    DockWindow.cpp
    PropertyView.cpp
    ReportView.cpp
    SelectionView.cpp
    ToolBox.cpp
    Tree.cpp
    TreeView.cpp
    ProjectView.cpp
    DAGView/DAGView.cpp
    DAGView/DAGModel.cpp
    DAGView/DAGRectItem.cpp
    DAGView/DAGModelGraph.cpp
    DAGView/DAGFilter.cpp
)
SET(Dock_Windows_HPP_SRCS
    ComboView.h
    DockWindow.h
    PropertyView.h
    ReportView.h
    SelectionView.h
    ToolBox.h
    Tree.h
    TreeView.h
    ProjectView.h
    DAGView/DAGView.h
    DAGView/DAGModel.h
    DAGView/DAGRectItem.h
    DAGView/DAGModelGraph.h
    DAGView/DAGFilter.h
)
SET(Dock_Windows_SRCS
    ${Dock_Windows_CPP_SRCS}
    ${Dock_Windows_HPP_SRCS}
)
SOURCE_GROUP("Dock Windows" FILES ${Dock_Windows_SRCS})

# The editor sources
SET(Editor_CPP_SRCS
    CallTips.cpp
    EditorView.cpp
    TextDocumentEditorView.cpp
    PythonConsole.cpp
    PythonConsolePy.cpp
    PythonDebugger.cpp
    PythonTracing.cpp
    PythonEditor.cpp
    SyntaxHighlighter.cpp
    TextEdit.cpp
)
SET(Editor_HPP_SRCS
    CallTips.h
    EditorView.h
    TextDocumentEditorView.h
    PythonConsole.h
    PythonConsolePy.h
    PythonDebugger.h
    PythonTracing.h
    PythonEditor.h
    SyntaxHighlighter.h
    TextEdit.h
)
SET(Editor_SRCS
    ${Editor_CPP_SRCS}
    ${Editor_HPP_SRCS}
)
SOURCE_GROUP("Editor" FILES ${Editor_SRCS})

# The help system
SET(Help_CPP_SRCS
    Assistant.cpp
    NetworkRetriever.cpp
    OnlineDocumentation.cpp
    WhatsThis.cpp
)
SET(Help_SRCS
    ${Help_CPP_SRCS}
    Assistant.h
    NetworkRetriever.h
    OnlineDocumentation.h
    WhatsThis.h
)
SOURCE_GROUP("Help" FILES ${Help_SRCS})


# The i18n sources
SET(Language_SRCS
    Language/Translator.cpp
    Language/Translator.h
)
SOURCE_GROUP("Language" FILES ${Language_SRCS})

# The property editor
SET(Propertyeditor_SRCS
    propertyeditor/PropertyEditor.cpp
    propertyeditor/PropertyEditor.h
    propertyeditor/PropertyItem.cpp
    propertyeditor/PropertyItem.h
    propertyeditor/PropertyItemDelegate.cpp
    propertyeditor/PropertyItemDelegate.h
    propertyeditor/PropertyModel.cpp
    propertyeditor/PropertyModel.h
)
SOURCE_GROUP("Propertyeditor" FILES ${Propertyeditor_SRCS})

# The task view
SET(Task_View_SRCS
    TaskView/TaskAppearance.cpp
    TaskView/TaskAppearance.h
    TaskView/TaskAppearance.ui
    TaskView/TaskOrientation.cpp
    TaskView/TaskOrientation.h
    TaskView/TaskOrientation.ui
    TaskView/TaskImage.cpp
    TaskView/TaskImage.h
    TaskView/TaskImage.ui
    TaskView/TaskSelectLinkProperty.cpp
    TaskView/TaskSelectLinkProperty.h
    TaskView/TaskSelectLinkProperty.ui
    TaskView/TaskEditControl.cpp
    TaskView/TaskEditControl.h
    TaskView/TaskEditControl.ui
    TaskView/TaskView.cpp
    TaskView/TaskView.h
    TaskView/TaskDialog.cpp
    TaskView/TaskDialog.h
    TaskView/TaskDialogPython.cpp
    TaskView/TaskDialogPython.h
    TaskView/TaskWatcher.cpp
    TaskView/TaskWatcher.h
)
SOURCE_GROUP("Task View" FILES ${Task_View_SRCS})

SET(qsintActionPanel_SRCS
    QSint/actionpanel/actionbox.cpp
    QSint/actionpanel/actionbox.h
    QSint/actionpanel/actionlabel.cpp
    QSint/actionpanel/actionlabel.h
    QSint/actionpanel/actiongroup.cpp
    QSint/actionpanel/actiongroup.h
    QSint/actionpanel/actionpanel.cpp
    QSint/actionpanel/actionpanel.h
    QSint/actionpanel/actionpanelscheme.cpp
    QSint/actionpanel/actionpanelscheme.h
    QSint/actionpanel/androidpanelscheme.cpp
    QSint/actionpanel/androidpanelscheme.h
    QSint/actionpanel/macpanelscheme.cpp
    QSint/actionpanel/macpanelscheme.h
    QSint/actionpanel/taskgroup_p.cpp
    QSint/actionpanel/taskgroup_p.h
    QSint/actionpanel/taskheader_p.cpp
    QSint/actionpanel/taskheader_p.h
    QSint/actionpanel/winvistapanelscheme.cpp
    QSint/actionpanel/winvistapanelscheme.h
    QSint/actionpanel/winxppanelscheme.cpp
    QSint/actionpanel/winxppanelscheme.h
    QSint/actionpanel/freecadscheme.cpp
    QSint/actionpanel/freecadscheme.h
)
SOURCE_GROUP("Widget\\QSintActionPanel" FILES ${qsintActionPanel_SRCS})
set(qsint_MOC_HDRS
    QSint/actionpanel/actionbox.h
    QSint/actionpanel/actionlabel.h
    QSint/actionpanel/actiongroup.h
    QSint/actionpanel/actionpanel.h
    QSint/actionpanel/taskheader_p.h
)

qt_wrap_cpp(qsint_MOC_SRCS ${qsint_MOC_HDRS})
SOURCE_GROUP("Widget\\QSintActionPanel\\Mocs" FILES ${qsint_MOC_SRCS})

#SET(Resource_RCS
#   Icons/resource.qrc
#   Language/translation.qrc)
#qt_add_resources(Resource_SRCS ${Resource_RCS})
#SET(Resource_SRCS
#   ${Resource_SRCS}
#   Icons/resource.qrc
#   Language/translation.qrc
#)
#SOURCE_GROUP("Resource" FILES ${Resource_SRCS})

# The 3d view
SET(View3D_CPP_SRCS
    Camera.cpp
    Flag.cpp
    GLBuffer.cpp
    GLPainter.cpp
    MouseSelection.cpp
    NavigationStyle.cpp
    InventorNavigationStyle.cpp
    CADNavigationStyle.cpp
    RevitNavigationStyle.cpp
    BlenderNavigationStyle.cpp
    MayaGestureNavigationStyle.cpp
    OpenCascadeNavigationStyle.cpp
    OpenSCADNavigationStyle.cpp
    TinkerCADNavigationStyle.cpp
    TouchpadNavigationStyle.cpp
    GestureNavigationStyle.cpp
    SplitView3DInventor.cpp
    View.cpp
    View3DInventor.cpp
    View3DInventorExamples.cpp
    View3DInventorSelection.cpp
    View3DInventorViewer.cpp
    View3DInventorRiftViewer.cpp
    View3DSettings.cpp
    CoinRiftWidget.cpp
    View3DPy.cpp
    View3DViewerPy.cpp
    NaviCube.cpp
)
SET(View3D_SRCS
    ${View3D_CPP_SRCS}
    Camera.h
    Flag.h
    GLBuffer.h
    GLPainter.h
    MouseSelection.h
    NavigationStyle.h
    GestureNavigationStyle.h
    SplitView3DInventor.h
    View.h
    View3DInventor.h
    View3DInventorExamples.h
    View3DInventorSelection.h
    View3DInventorViewer.h
    View3DPy.h
    View3DInventorRiftViewer.h
    View3DSettings.h
    CoinRiftWidget.h
    View3DViewerPy.h
    NaviCube.h
)
SOURCE_GROUP("View3D" FILES ${View3D_SRCS})

#quarter sources
FILE(GLOB_RECURSE Quarter_CPP_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Quarter/*.cpp)
FILE(GLOB_RECURSE Quarter_H_SRC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} Quarter/*.h)

SET(Quarter_MOC_HDR
    Quarter/SignalThread.h
    Quarter/InteractionMode.h
    Quarter/SensorManager.h
    Quarter/ContextMenu.h
    Quarter/eventhandlers/FocusHandler.h
    Quarter/eventhandlers/DragDropHandler.h
    Quarter/eventhandlers/EventFilter.h
    Quarter/QuarterWidget.h
)

qt_wrap_cpp(Quarter_MOC_SRCS ${Quarter_MOC_HDR})

SET(Quarter_SRCS
    ${Quarter_CPP_SRC}
    ${Quarter_H_SRC}
    ${Quarter_MOC_SRCS}
)
SOURCE_GROUP("Quarter" FILES ${Quarter_SRCS})

# The view provider sources
SET(Viewprovider_CPP_SRCS
    ViewProvider.cpp
    ViewProviderExtension.cpp
    ViewProviderExtensionPyImp.cpp
    ViewProviderGroupExtension.cpp
    ViewProviderGeoFeatureGroupExtension.cpp
    ViewProviderOriginGroupExtension.cpp
    ViewProviderAnnotation.cpp
    ViewProviderDocumentObject.cpp
    ViewProviderDocumentObjectGroup.cpp
    ViewProviderDocumentObjectPyImp.cpp
    ViewProviderDragger.cpp
    ViewProviderExtern.cpp
    ViewProviderFeature.cpp
    ViewProviderGeometryObject.cpp
    ViewProviderImagePlane.cpp
    ViewProviderInventorObject.cpp
    ViewProviderMeasureDistance.cpp
    ViewProviderPyImp.cpp
    ViewProviderPythonFeature.cpp
    ViewProviderVRMLObject.cpp
    ViewProviderBuilder.cpp
    ViewProviderPlacement.cpp
    ViewProviderOriginFeature.cpp
    ViewProviderPlane.cpp
    ViewProviderLine.cpp
    ViewProviderGeoFeatureGroup.cpp
    ViewProviderOriginGroup.cpp
    ViewProviderPart.cpp
    ViewProviderOrigin.cpp
    ViewProviderMaterialObject.cpp
    ViewProviderTextDocument.cpp
    ViewProviderLink.cpp
    LinkViewPyImp.cpp
    ViewProviderLinkPyImp.cpp
    AxisOriginPyImp.cpp
    AxisOrigin.cpp
)
SET(Viewprovider_SRCS
    ${Viewprovider_CPP_SRCS}
    ViewProvider.h
    ViewProviderExtension.h
    ViewProviderGroupExtension.h
    ViewProviderGeoFeatureGroupExtension.h
    ViewProviderOriginGroupExtension.h
    ViewProviderAnnotation.h
    ViewProviderDocumentObject.h
    ViewProviderDocumentObjectGroup.h
    ViewProviderDragger.h
    ViewProviderExtern.h
    ViewProviderFeature.h
    ViewProviderGeometryObject.h
    ViewProviderImagePlane.h
    ViewProviderInventorObject.h
    ViewProviderMeasureDistance.h
    ViewProviderPythonFeature.h
    ViewProviderVRMLObject.h
    ViewProviderBuilder.h
    ViewProviderPlacement.h
    ViewProviderOriginFeature.h
    ViewProviderPlane.h
    ViewProviderLine.h
    ViewProviderGeoFeatureGroup.h
    ViewProviderOriginGroup.h
    ViewProviderPart.h
    ViewProviderOrigin.h
    ViewProviderMaterialObject.h
    ViewProviderTextDocument.h
    ViewProviderLink.h
    AxisOrigin.h
)
SOURCE_GROUP("View3D\\Viewprovider" FILES ${Viewprovider_SRCS})

# The Inventor sources
SET(Inventor_CPP_SRCS
    Inventor/SoDrawingGrid.cpp
    Inventor/SoAutoZoomTranslation.cpp
    Inventor/MarkerBitmaps.cpp
    Inventor/SmSwitchboard.cpp
    SoFCBackgroundGradient.cpp
    SoFCBoundingBox.cpp
    SoFCColorBar.cpp
    SoFCColorGradient.cpp
    SoFCColorLegend.cpp
    SoFCDB.cpp
    SoFCInteractiveElement.cpp
    SoFCOffscreenRenderer.cpp
    SoQtOffscreenRendererPy.cpp
    SoFCSelection.cpp
    SoFCUnifiedSelection.cpp
    SoFCSelectionContext.cpp
    SoFCSelectionAction.cpp
    SoFCVectorizeSVGAction.cpp
    SoFCVectorizeU3DAction.cpp
    SoNavigationDragger.cpp
    SoAxisCrossKit.cpp
    SoTextLabel.cpp
    SoDatumLabel.cpp
    SoTouchEvents.cpp
    SoMouseWheelEvent.cpp
    SoFCCSysDragger.cpp
)
SET(Inventor_SRCS
    ${Inventor_CPP_SRCS}
    Inventor/SoDrawingGrid.h
    Inventor/SoAutoZoomTranslation.h
    Inventor/MarkerBitmaps.h
    Inventor/SmSwitchboard.h
    SoFCBackgroundGradient.h
    SoFCBoundingBox.h
    SoFCColorBar.h
    SoFCColorGradient.h
    SoFCColorLegend.h
    SoFCDB.h
    SoFCInteractiveElement.h
    SoFCOffscreenRenderer.h
    SoQtOffscreenRendererPy.h
    SoFCSelection.h
    SoFCUnifiedSelection.h
    SoFCSelectionContext.h
    SoFCSelectionAction.h
    SoFCVectorizeSVGAction.h
    SoFCVectorizeU3DAction.h
    SoNavigationDragger.h
    SoAxisCrossKit.h
    SoTextLabel.h
    SoDatumLabel.h
    SoTouchEvents.h
    SoMouseWheelEvent.h
    SoFCCSysDragger.h
)
SOURCE_GROUP("View3D\\Inventor" FILES ${Inventor_SRCS})

# The widget sources
SET(Widget_CPP_SRCS
    FileDialog.cpp
    MainWindow.cpp
    MainWindowPy.cpp
    NotificationBox.cpp
    NotificationArea.cpp
    PrefWidgets.cpp
    InputField.cpp
    ProgressBar.cpp
    ProgressDialog.cpp
    QuantitySpinBox.cpp
    SpinBox.cpp
    Splashscreen.cpp
    PythonWrapper.cpp
    UiLoader.cpp
    WidgetFactory.cpp
    Widgets.cpp
    Window.cpp
    ViewParams.cpp
)
SET(Widget_HPP_SRCS
    FileDialog.h
    MainWindow.h
    MainWindowPy.h
    NotificationBox.h
    NotificationArea.h
    PrefWidgets.h
    InputField.h
    ProgressBar.h
    ProgressDialog.h
    QuantitySpinBox.h
    QuantitySpinBox_p.h
    SpinBox.h
    Splashscreen.h
    PythonWrapper.h
    UiLoader.h
    WidgetFactory.h
    Widgets.h
    Window.h
    ViewParams.h
)
SET(Widget_SRCS
    ${Widget_CPP_SRCS}
    ${Widget_HPP_SRCS}
)
SOURCE_GROUP("Widget" FILES ${Widget_SRCS})

SET(Params_CPP_SRCS
    TreeParams.cpp
)

SET(Params_HPP_SRCS
    TreeParams.h
)

SET(Params_SRCS
    ${Params_CPP_SRCS}
    ${Params_HPP_SRCS}
)
SOURCE_GROUP("Params" FILES ${Params_SRCS})
# The view sources
SET(View_CPP_SRCS
    MDIView.cpp
    MDIViewPy.cpp
    MDIViewPyWrap.cpp
    GraphvizView.cpp
    ImageView.cpp
    ActiveObjectList.cpp
)
SET(View_HPP_SRCS
    MDIView.h
    MDIViewPy.h
    MDIViewPyWrap.h
    GraphvizView.h
    ImageView.h
    ActiveObjectList.h
)
SET(View_SRCS
    ${View_CPP_SRCS}
    ${View_HPP_SRCS}
)
SOURCE_GROUP("View" FILES ${View_SRCS})

# The workbench sources
SET(Workbench_CPP_SRCS
    DockWindowManager.cpp
    MenuManager.cpp
    PythonWorkbenchPyImp.cpp
    ToolBarManager.cpp
    ToolBoxManager.cpp
    Workbench.cpp
    WorkbenchFactory.cpp
    WorkbenchManager.cpp
    WorkbenchManipulator.cpp
    WorkbenchPyImp.cpp
)
SET(Workbench_SRCS
    ${Workbench_CPP_SRCS}
    DockWindowManager.h
    MenuManager.h
    ToolBarManager.h
    ToolBoxManager.h
    Workbench.h
    WorkbenchFactory.h
    WorkbenchManager.h
    WorkbenchManipulator.h
)
SOURCE_GROUP("Workbench" FILES ${Workbench_SRCS})

SET(Selection_SRCS
    SelectionObjectPyImp.cpp
    SelectionObject.h
    SelectionObject.cpp
    Selection.h
    Selection.cpp
    SelectionFilter.h
    SelectionFilter.cpp
    SelectionFilterPy.h
    SelectionFilterPy.cpp
    SelectionFilter.y
    SelectionFilter.l
    SelectionObserverPython.cpp
    SelectionObserverPython.h
)
SOURCE_GROUP("Selection" FILES ${Selection_SRCS})

# The FreeCADGui sources
SET(FreeCADGui_CPP_SRCS
    Application.cpp
    ApplicationPy.cpp
    AutoSaver.cpp
    BitmapFactory.cpp
    Document.cpp
    DocumentModel.cpp
    DocumentPyImp.cpp
    DocumentObserver.cpp
    DocumentObserverPython.cpp
    EditableDatumLabel.cpp
    ExpressionBinding.cpp
    ExpressionBindingPy.cpp
    GraphicsViewZoom.cpp
    ExpressionCompleter.cpp
    FileHandler.cpp
    FileHandler.h
    GuiApplication.cpp
    GuiApplicationNativeEventAware.cpp
    GuiConsole.cpp
    Macro.cpp
    MergeDocuments.cpp
    Namespace.h
    resource.cpp
    Control.cpp
    SpaceballEvent.cpp
    PreferencePackManager.cpp
    Thumbnail.cpp
    Utilities.cpp
    WaitCursor.cpp
    ManualAlignment.cpp
    TransactionObject.cpp
    UserSettings.cpp
)
SET(FreeCADGui_SRCS
    Application.h
    AutoSaver.h
    BitmapFactory.h
    Document.h
    DocumentModel.h
    DocumentObserver.h
    DocumentObserverPython.h
    EditableDatumLabel.h
    ExpressionBinding.h
    ExpressionBindingPy.h
    ExpressionCompleter.h
    FreeCADGuiInit.py
    GraphicsViewZoom.h
    GuiApplication.h
    GuiApplicationNativeEventAware.h
    3Dconnexion/GuiAbstractNativeEvent.h
    GuiConsole.h
    InventorAll.h
    Macro.h
    MergeDocuments.h
    MetaTypes.h
    Notifications.h
    PreCompiled.cpp
    PreCompiled.h
    QtAll.h
    Control.h
    SpaceballEvent.h
    PreferencePackManager.h
    Thumbnail.h
    Tools.h
    Utilities.h
    WaitCursor.h
    ManualAlignment.h
    TransactionObject.h
    UserSettings.h
    ${FreeCADGui_SDK_MOC_HDRS}
)

SET(FreeCADGui_SRCS
    ${FreeCADGui_SDK_SRCS}
    ${FreeCADGui_CPP_SRCS}
    ${FreeCADGui_XML_SRCS}
    ${qsint_MOC_SRCS}
    ${Gui_QRC_SRCS}
    ${Gui_UIC_HDRS}
    ${Command_SRCS}
    ${Dialog_SRCS}
    ${Dialog_Customize_SRCS}
    ${Dialog_Settings_SRCS}
    ${Dock_Windows_SRCS}
    ${Editor_SRCS}
    ${Help_SRCS}
    ${Inventor_SRCS}
    ${Language_SRCS}
    ${Propertyeditor_SRCS}
    ${Task_View_SRCS}
    ${qsintActionPanel_SRCS}
    ${Resource_SRCS}
    ${Quarter_SRCS}
    ${View3D_SRCS}
    ${Viewprovider_SRCS}
    ${Widget_SRCS}
    ${Params_SRCS}
    ${View_SRCS}
    ${Workbench_SRCS}
    ${Selection_SRCS}
    ${FreeCADGui_SRCS}
)

if(MSVC)
    SET(FreeCADGui_CPP_SRCS
        Language/Translator.cpp
        propertyeditor/PropertyEditor.cpp
        propertyeditor/PropertyItem.cpp
        propertyeditor/PropertyItemDelegate.cpp
        propertyeditor/PropertyModel.cpp
        TaskView/TaskAppearance.cpp
        TaskView/TaskOrientation.cpp
        TaskView/TaskImage.cpp
        TaskView/TaskSelectLinkProperty.cpp
        TaskView/TaskEditControl.cpp
        TaskView/TaskView.cpp
        ${Command_CPP_SRCS}
        ${Dialog_CPP_SRCS}
        ${Dialog_Customize_CPP_SRCS}
        ${Dialog_Settings_CPP_SRCS}
        ${Dock_Windows_CPP_SRCS}
        ${Editor_CPP_SRCS}
        ${Help_CPP_SRCS}
        ${Inventor_CPP_SRCS}
        ${View3D_CPP_SRCS}
        ${Viewprovider_CPP_SRCS}
        ${Widget_CPP_SRCS}
        ${Workbench_CPP_SRCS}
        ${FreeCADGui_CPP_SRCS}
    )
endif(MSVC)

if (FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    ADD_MSVC_PRECOMPILED_HEADER(FreeCADGui PreCompiled.h PreCompiled.cpp FreeCADGui_CPP_SRCS)
endif(FREECAD_USE_PCH)

if (FREECAD_USE_QT_FILEDIALOG)
    set_source_files_properties(FileDialog.cpp PROPERTIES COMPILE_FLAGS -DUSE_QT_FILEDIALOG)
endif()

# Suppress some very long Eigen3 warnings of older versions and
# suppress this warning for the SbMatrix class
if (EIGEN3_NO_DEPRECATED_COPY)
    set_source_files_properties(
        NaviCube.cpp
        Inventor/SoAutoZoomTranslation.cpp
        SoFCOffscreenRenderer.cpp
        SoFCSelectionAction.cpp
        SoNavigationDragger.cpp
        SoFCCSysDragger.cpp
        Quarter/QuarterWidget.cpp
        View3DInventorViewer.cpp
        PROPERTIES COMPILE_FLAGS ${EIGEN3_NO_DEPRECATED_COPY})
endif ()

add_library(FreeCADGui SHARED ${FreeCADGui_SRCS})
target_link_libraries(FreeCADGui ${FreeCADGui_LIBS})

SET_BIN_DIR(FreeCADGui FreeCADGui)

if(WIN32)
    INSTALL(TARGETS FreeCADGui
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADGui
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    #INSTALL(FILES Icons/freecad.xpm
    #    Icons/freecad-icon-16.png
    #    Icons/freecad-icon-32.png
    #    Icons/freecad-icon-48.png
    #    Icons/freecad-icon-64.png
    #    Icons/freecad.svg
    #    Icons/freecad-doc.png
    #    DESTINATION ${CMAKE_INSTALL_DATADIR}
    #)
    INSTALL(FILES Icons/freecad-icon-16.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps RENAME org.freecad.FreeCAD.png)
    INSTALL(FILES Icons/freecad-icon-32.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps RENAME org.freecad.FreeCAD.png)
    INSTALL(FILES Icons/freecad-icon-48.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps RENAME org.freecad.FreeCAD.png)
    INSTALL(FILES Icons/freecad-icon-64.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps RENAME org.freecad.FreeCAD.png)
    INSTALL(FILES Icons/freecad.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps RENAME org.freecad.FreeCAD.svg)
    INSTALL(FILES Icons/freecad.xpm DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps)
    INSTALL(FILES Icons/freecad-doc.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/mimetypes RENAME application-x-extension-fcstd.svg)
endif(WIN32)

set(FreeCADGui_Scripts
    RemoteDebugger.ui
    RemoteDebugger.py
)

set(FreeCADGui_Configs
    3Dconnexion/3DConnexion.xml
)

add_custom_target(FreeCADGui_Resources ALL
    SOURCES ${FreeCADGui_Scripts} ${FreeCADGui_Configs}
)

fc_copy_sources(FreeCADGui_Resources
    ${CMAKE_BINARY_DIR}/Ext/freecad/gui
    ${FreeCADGui_Scripts}
)

fc_target_copy_resource(FreeCADGui_Resources
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}
    ${FreeCADGui_Configs}
)

INSTALL(
    FILES
        ${FreeCADGui_Scripts}
    DESTINATION
        Ext/freecad/gui
)

INSTALL(
    FILES
        ${FreeCADGui_Configs}
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}/3Dconnexion
)
