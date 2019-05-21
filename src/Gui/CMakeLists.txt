#add_subdirectory(Icons)
add_subdirectory(Stylesheets)

if(WIN32)
add_definitions(-DFCGui -DQIIS_MAKEDLL -DQSINT_MAKEDLL -DOVR_OS_WIN32 -DQUARTER_INTERNAL -DQUARTER_MAKE_DLL -DCOIN_DLL)
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

if (BUILD_QT5)
    include_directories(
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Widgets_INCLUDE_DIRS}
        ${Qt5OpenGL_INCLUDE_DIRS}
        ${Qt5PrintSupport_INCLUDE_DIRS}
        ${Qt5Svg_INCLUDE_DIRS}
        ${Qt5Network_INCLUDE_DIRS}
        ${Qt5UiTools_INCLUDE_DIRS}
    )
    list(APPEND FreeCADGui_LIBS
        ${Qt5Core_LIBRARIES}
        ${Qt5Widgets_LIBRARIES}
        ${Qt5OpenGL_LIBRARIES}
        ${Qt5PrintSupport_LIBRARIES}
        ${Qt5Svg_LIBRARIES}
        ${Qt5Network_LIBRARIES}
        ${Qt5UiTools_LIBRARIES}
    )
    if(${Qt5WinExtras_FOUND})
      include_directories(
        ${Qt5WinExtras_INCLUDE_DIRS}
      )
      list(APPEND FreeCADGui_LIBS
        ${Qt5WinExtras_LIBRARIES}
      )
    endif()
else()
    include_directories(
        ${QT_INCLUDE_DIR}
    )
    list(APPEND FreeCADGui_LIBS
        ${QT_LIBRARIES}
        ${QT_QTUITOOLS_LIBRARY}
    )
endif()

IF(SPNAV_FOUND)
    if(SPNAV_USE_X11)
      add_definitions(-DSPNAV_USE_X11)
      if (BUILD_QT5 AND UNIX AND NOT APPLE)
        find_package(Qt5X11Extras REQUIRED)
        include_directories(${Qt5X11Extras_INCLUDE_DIRS})
        list(APPEND FreeCADGui_LIBS ${Qt5X11Extras_LIBRARIES})
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

if(SHIBOKEN_INCLUDE_DIR)
    if (BUILD_QT5)
        add_definitions(-DHAVE_SHIBOKEN2)
    else()
        add_definitions(-DHAVE_SHIBOKEN)
    endif()
    include_directories(
        ${SHIBOKEN_INCLUDE_DIR}
    )
    list(APPEND FreeCADGui_LIBS
        ${SHIBOKEN_LIBRARY}
    )
endif(SHIBOKEN_INCLUDE_DIR)

if(PYSIDE_INCLUDE_DIR)
    include_directories(
        ${PYSIDE_INCLUDE_DIR}
        ${PYSIDE_INCLUDE_DIR}/QtCore
        ${PYSIDE_INCLUDE_DIR}/QtGui
    )
    list(APPEND FreeCADGui_LIBS
        ${PYSIDE_LIBRARY}
    )
    if (BUILD_QT5)
        include_directories(
            ${PYSIDE_INCLUDE_DIR}/QtWidgets
        )
        add_definitions(-DHAVE_PYSIDE2)
    else()
        add_definitions(-DHAVE_PYSIDE)
    endif()
endif(PYSIDE_INCLUDE_DIR)

generate_from_xml(DocumentPy)
generate_from_xml(PythonWorkbenchPy)
generate_from_xml(ViewProviderPy)
generate_from_xml(ViewProviderDocumentObjectPy)
generate_from_xml(WorkbenchPy)
generate_from_xml(SelectionObjectPy)

generate_from_py(FreeCADGuiInit GuiInitScript.h)

# The XML files
SET(FreeCADGui_XML_SRCS
    ViewProviderDocumentObjectPy.xml
    ViewProviderPy.xml
    PythonWorkbenchPy.xml
    WorkbenchPy.xml
    SelectionObjectPy.xml
    DocumentPy.xml
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

set(Gui_MOC_HDRS
    Action.h
    ActionFunction.h
    Assistant.h
    AutoSaver.h
    CallTips.h
    CombiView.h
    Control.h
    Clipping.h
    DemoMode.h
    DownloadItem.h
    DownloadManager.h
    DlgActionsImp.h
    DlgActivateWindowImp.h
    DlgUnitsCalculatorImp.h
    DlgCommandsImp.h
    DlgCustomizeImp.h
    DlgCustomizeSpaceball.h
    DlgCustomizeSpNavSettings.h
    DlgDisplayPropertiesImp.h
    DlgEditorImp.h
    DlgGeneralImp.h
    DlgInputDialogImp.h
    DlgKeyboardImp.h
    DlgMacroExecuteImp.h
    DlgMacroRecordImp.h
    DlgRunExternal.h
    DlgEditFileIncludeProptertyExternal.h
    DlgMaterialPropertiesImp.h
    DlgOnlineHelpImp.h
    DlgParameterImp.h
    DlgParameterFind.h
    DlgPreferencesImp.h
    DlgProjectInformationImp.h
    DlgProjectUtility.h
    DlgPropertyLink.h
    DlgReportViewImp.h
    DlgSettings3DViewImp.h
    DlgSettingsViewColor.h
    DlgSettingsColorGradientImp.h
    DlgSettingsDocumentImp.h
    DlgSettingsImageImp.h
    DlgSettingsMacroImp.h
    DlgSettingsUnitsImp.h
    DlgCheckableMessageBox.h
    DlgToolbarsImp.h
    DlgWorkbenchesImp.h
    TaskDlgRelocation.h
    TaskCSysDragger.h
    DlgUndoRedo.h
    DockWindow.h
    DockWindowManager.h
    DocumentRecovery.h
    EditorView.h
    TextDocumentEditorView.h
    ExpressionCompleter.h
    DlgExpressionInput.h
    FileDialog.h
    Flag.h
    GraphicsViewZoom.h
    GraphvizView.h
    GuiApplication.h
    GuiApplicationNativeEventAware.h
    InputVector.h
    MainWindow.h
    ManualAlignment.h
    MDIView.h
    NetworkRetriever.h
    OnlineDocumentation.h
    Placement.h
    InputField.h
    PrefWidgets.h
    ProgressBar.h
    PropertyPage.h
    PropertyView.h
    PythonConsole.h
    PythonDebugger.h
    PythonEditor.h
    QuantitySpinBox.h
    QListWidgetCustom.h
    QuantitySpinBox_p.h
    ReportView.h
    SceneInspector.h
    SelectionView.h
    SpinBox.h
    Splashscreen.h
    TextEdit.h
    TextureMapping.h
    ToolBox.h
    Transform.h
    Tree.h
    TreeView.h
    ProjectView.h
    View3DInventor.h
    WidgetFactory.h
    Widgets.h
    Language/Translator.h
    propertyeditor/PropertyEditor.h
    propertyeditor/PropertyItem.h
    propertyeditor/PropertyItemDelegate.h
    propertyeditor/PropertyModel.h
    TaskView/TaskAppearance.h
    TaskView/TaskSelectLinkProperty.h
    TaskView/TaskDialog.h
    TaskView/TaskWatcher.h
    TaskView/TaskEditControl.h
    TaskView/TaskView.h
    DAGView/DAGView.h
    DAGView/DAGModel.h
    ${FreeCADGui_SDK_MOC_HDRS}
)

fc_wrap_cpp(Gui_MOC_SRCS ${Gui_MOC_HDRS})
#SOURCE_GROUP("Moc" FILES ${Gui_MOC_SRCS})

if (BUILD_QT5)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView-internal.cpp
        COMMAND ${Qt5Core_MOC_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView-internal.cpp ${CMAKE_CURRENT_SOURCE_DIR}/GraphvizView.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GraphvizView.cpp)

    set_property(SOURCE GraphvizView.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView-internal.cpp)
else()
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView-internal.cpp
        COMMAND ${QT_MOC_EXECUTABLE} -o ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView-internal.cpp ${CMAKE_CURRENT_SOURCE_DIR}/GraphvizView.cpp
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GraphvizView.cpp)

    set_property(SOURCE GraphvizView.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/moc_GraphvizView-internal.cpp)
endif()

SET(Gui_UIC_SRCS
    AboutApplication.ui
    Clipping.ui
    DemoMode.ui
    DlgActions.ui
    DlgActivateWindow.ui
    DlgUnitsCalculator.ui
    DlgAuthorization.ui
    DlgChooseIcon.ui
    DlgCommands.ui
    DlgCustomizeSpNavSettings.ui
    DlgDisplayProperties.ui
    DlgEditor.ui
    DlgGeneral.ui
    DlgInputDialog.ui
    DlgKeyboard.ui
    DlgMacroExecute.ui
    DlgRunExternal.ui
    DlgMacroRecord.ui
    DlgMaterialProperties.ui
    DlgOnlineHelp.ui
    DlgParameter.ui
    DlgParameterFind.ui
    DlgPreferences.ui
    DlgProjectInformation.ui
    DlgProjectUtility.ui
    DlgPropertyLink.ui
    DlgReportView.ui
    DlgSettings3DView.ui
    DlgSettingsUnits.ui
    DlgSettingsViewColor.ui
    DlgSettingsColorGradient.ui
    DlgSettingsDocument.ui
    DlgSettingsImage.ui
    DlgSettingsMacro.ui
    DlgCheckableMessageBox.ui
    DlgToolbars.ui
    DlgWorkbenches.ui
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
    TaskView/TaskSelectLinkProperty.ui
)

SET(Gui_RES_SRCS
    Icons/resource.qrc
    Language/translation.qrc
    iisTaskPanel/src/iisTaskPanel.qrc
    QSint/actionpanel/schemes.qrc
)

if (BUILD_QT5)
    QT5_ADD_RESOURCES(Gui_QRC_SRCS ${Gui_RES_SRCS})
    QT5_WRAP_UI(Gui_UIC_HDRS ${Gui_UIC_SRCS})
else()
    QT4_ADD_RESOURCES(Gui_QRC_SRCS ${Gui_RES_SRCS})
    QT4_WRAP_UI(Gui_UIC_HDRS ${Gui_UIC_SRCS})
endif()
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
)
SET(Command_SRCS
    ${Command_CPP_SRCS}
    Action.h
    ActionFunction.h
    Command.h
)
SOURCE_GROUP("Command" FILES ${Command_SRCS})

# The dialog sources
SET(Dialog_CPP_SRCS
    Clipping.cpp
    DemoMode.cpp
    DlgActivateWindowImp.cpp
    DlgUnitsCalculatorImp.cpp
    DlgDisplayPropertiesImp.cpp
    DlgInputDialogImp.cpp
    DlgMacroExecuteImp.cpp
    DlgRunExternal.cpp
    DlgEditFileIncludeProptertyExternal.cpp
    DlgMacroRecordImp.cpp
    DlgMaterialPropertiesImp.cpp
    DlgParameterImp.cpp
    DlgParameterFind.cpp
    DlgProjectInformationImp.cpp
    DlgProjectUtility.cpp
    DlgPropertyLink.cpp
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
)

SET(Dialog_HPP_SRCS
    Clipping.h
    DemoMode.h
    DlgActivateWindowImp.h
    DlgUnitsCalculatorImp.h
    DlgDisplayPropertiesImp.h
    DlgInputDialogImp.h
    DlgMacroExecuteImp.h
    DlgRunExternal.h
    DlgEditFileIncludeProptertyExternal.h
    DlgMacroRecordImp.h
    DlgMaterialPropertiesImp.h
    DlgParameterImp.h
    DlgParameterFind.h
    DlgProjectInformationImp.h
    DlgProjectUtility.h
    DlgPropertyLink.h
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
    DlgLocationAngle.ui
    DlgLocationPos.ui
    DlgMacroExecute.ui
    DlgRunExternal.ui
    DlgMacroRecord.ui
    DlgMaterialProperties.ui
    DlgParameter.ui
    DlgParameterFind.ui
    DlgProjectInformation.ui
    DlgProjectUtility.ui
    DlgPropertyLink.ui
    DlgCheckableMessageBox.ui
    DlgTreeWidget.ui
    DlgExpressionInput.ui
    DownloadManager.ui
    DownloadItem.ui
    DocumentRecovery.ui
    MouseButtons.ui
    InputVector.ui
    Placement.ui
    SceneInspector.ui
    TextureMapping.ui
)
SOURCE_GROUP("Dialog" FILES ${Dialog_SRCS})

# The customize dialog sources
SET(Dialog_Customize_CPP_SRCS
    DlgActionsImp.cpp
    DlgCommandsImp.cpp
    DlgCustomizeImp.cpp
    DlgCustomizeSpaceball.cpp
    DlgCustomizeSpNavSettings.cpp
    DlgKeyboardImp.cpp
    DlgToolbarsImp.cpp
    DlgWorkbenchesImp.cpp
    QListWidgetCustom.cpp
)
SET(Dialog_Customize_HPP_SRCS
    DlgActionsImp.h
    DlgCommandsImp.h
    DlgCustomizeImp.h
    DlgCustomizeSpaceball.h
    DlgCustomizeSpNavSettings.h
    DlgKeyboardImp.h
    DlgToolbarsImp.h
    DlgWorkbenchesImp.h
    QListWidgetCustom.h
)
SET(Dialog_Customize_SRCS
    ${Dialog_Customize_CPP_SRCS}
    ${Dialog_Customize_HPP_SRCS}
    DlgActions.ui
    DlgChooseIcon.ui
    DlgCommands.ui
    DlgCustomizeSpNavSettings.ui
    DlgKeyboard.ui
    DlgToolbars.ui
    DlgWorkbenches.ui
)
SOURCE_GROUP("Dialog\\Customize" FILES ${Dialog_Customize_SRCS})

# The settings dialog sources
SET(Dialog_Settings_CPP_SRCS
    DlgEditorImp.cpp
    DlgGeneralImp.cpp
    DlgOnlineHelpImp.cpp
    DlgPreferencesImp.cpp
    DlgReportViewImp.cpp
    DlgSettings3DViewImp.cpp
    DlgSettingsUnitsImp.cpp
    DlgSettingsViewColor.cpp
    DlgSettingsColorGradientImp.cpp
    DlgSettingsDocumentImp.cpp
    DlgSettingsImageImp.cpp
    DlgSettingsMacroImp.cpp
)
SET(Dialog_Settings_HPP_SRCS
    DlgEditorImp.h
    DlgGeneralImp.h
    DlgOnlineHelpImp.h
    DlgPreferencesImp.h
    DlgReportViewImp.h
    DlgSettings3DViewImp.h
    DlgSettingsUnitsImp.h
    DlgSettingsViewColor.h
    DlgSettingsColorGradientImp.h
    DlgSettingsDocumentImp.h
    DlgSettingsImageImp.h
    DlgSettingsMacroImp.h
)
SET(Dialog_Settings_SRCS
    ${Dialog_Settings_CPP_SRCS}
    ${Dialog_Settings_HPP_SRCS}
    DlgEditor.ui
    DlgGeneral.ui
    DlgOnlineHelp.ui
    DlgPreferences.ui
    DlgReportView.ui
    DlgSettings3DView.ui
    DlgSettingsUnits.ui
    DlgSettingsViewColor.ui
    DlgSettingsColorGradient.ui
    DlgSettingsDocument.ui
    DlgSettingsImage.ui
    DlgSettingsMacro.ui
)
SOURCE_GROUP("Dialog\\Settings" FILES ${Dialog_Settings_SRCS})

# The dock windows sources
SET(Dock_Windows_CPP_SRCS
    CombiView.cpp
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
    CombiView.h
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

SET(iisTaskPanel_SRCS
    iisTaskPanel/src/iisfreecadscheme.cpp
    iisTaskPanel/src/iisfreecadscheme.h
    iisTaskPanel/src/iisiconlabel.cpp
    iisTaskPanel/src/iisiconlabel.h
    iisTaskPanel/src/iistaskbox.cpp
    iisTaskPanel/src/iistaskbox.h
    iisTaskPanel/src/iistaskgroup.cpp
    iisTaskPanel/src/iistaskgroup.h
    iisTaskPanel/src/iistaskheader.cpp
    iisTaskPanel/src/iistaskheader.h
    iisTaskPanel/src/iistaskpanel.cpp
    iisTaskPanel/src/iistaskpanel.h
    iisTaskPanel/src/iistaskpanelscheme.cpp
    iisTaskPanel/src/iistaskpanelscheme.h
    iisTaskPanel/src/iiswinxptaskpanelscheme.cpp
    iisTaskPanel/src/iiswinxptaskpanelscheme.h
    iisTaskPanel/src/iistaskpanel_global.h
)
SOURCE_GROUP("Widget\\iisTaskPanel" FILES ${iisTaskPanel_SRCS})
set(iis_MOC_HDRS
    iisTaskPanel/src/iisiconlabel.h
    iisTaskPanel/src/iistaskbox.h
    iisTaskPanel/src/iistaskgroup.h
    iisTaskPanel/src/iistaskheader.h
)

if (BUILD_QT5)
    qt5_wrap_cpp(iis_MOC_SRCS ${iis_MOC_HDRS})
else()
    qt4_wrap_cpp(iis_MOC_SRCS ${iis_MOC_HDRS})
endif()
SOURCE_GROUP("Widget\\iisTaskPanel\\Mocs" FILES ${iis_MOC_SRCS})

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

if(BUILD_QT5)
    qt5_wrap_cpp(qsint_MOC_SRCS ${qsint_MOC_HDRS})
else()
    qt4_wrap_cpp(qsint_MOC_SRCS ${qsint_MOC_HDRS})
endif()
SOURCE_GROUP("Widget\\QSintActionPanel\\Mocs" FILES ${qsint_MOC_SRCS})

#SET(Resource_RCS
#   Icons/resource.qrc
#   Language/translation.qrc)
#qt4_add_resources(Resource_SRCS ${Resource_RCS})
#SET(Resource_SRCS
#   ${Resource_SRCS}
#   Icons/resource.qrc
#   Language/translation.qrc
#)
#SOURCE_GROUP("Resource" FILES ${Resource_SRCS})

# The 3d view
SET(View3D_CPP_SRCS
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
    TouchpadNavigationStyle.cpp
    GestureNavigationStyle.cpp
    SplitView3DInventor.cpp
    View.cpp
    View3DInventor.cpp
    View3DInventorExamples.cpp
    View3DInventorViewer.cpp
    View3DInventorRiftViewer.cpp
    CoinRiftWidget.cpp
    View3DPy.cpp
    View3DViewerPy.cpp
    NaviCube.cpp
)
SET(View3D_SRCS
    ${View3D_CPP_SRCS}
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
    View3DInventorViewer.h
    View3DPy.h
    View3DInventorRiftViewer.h
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
if(BUILD_QT5)
    qt5_wrap_cpp(Quarter_MOC_SRCS ${Quarter_MOC_HDR})
else()
    qt4_wrap_cpp(Quarter_MOC_SRCS ${Quarter_MOC_HDR})
endif()

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
    SoFCSelection.cpp
    SoFCUnifiedSelection.cpp
    SoFCSelectionAction.cpp
    SoFCVectorizeSVGAction.cpp
    SoFCVectorizeU3DAction.cpp
    SoNavigationDragger.cpp
    SoAxisCrossKit.cpp
    SoTextLabel.cpp
    SoTouchEvents.cpp
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
    SoFCSelection.h
    SoFCUnifiedSelection.h
    SoFCSelectionAction.h
    SoFCVectorizeSVGAction.h
    SoFCVectorizeU3DAction.h
    SoNavigationDragger.h
    SoAxisCrossKit.h
    SoTextLabel.h
    SoTouchEvents.h
    SoFCCSysDragger.h
)
SOURCE_GROUP("View3D\\Inventor" FILES ${Inventor_SRCS})

# The widget sources
SET(Widget_CPP_SRCS
    FileDialog.cpp
    MainWindow.cpp
    PrefWidgets.cpp
    InputField.cpp
    ProgressBar.cpp
    QuantitySpinBox.cpp
    SpinBox.cpp
    Splashscreen.cpp
    WidgetFactory.cpp
    Widgets.cpp
    Window.cpp
)
SET(Widget_HPP_SRCS
    FileDialog.h
    MainWindow.h
    PrefWidgets.h
    InputField.h
    ProgressBar.h
    QuantitySpinBox.h
    QuantitySpinBox_p.h
    SpinBox.h
    Splashscreen.h
    WidgetFactory.h
    Widgets.h
    Window.h
)
SET(Widget_SRCS
    ${Widget_CPP_SRCS}
    ${Widget_HPP_SRCS}
)
SOURCE_GROUP("Widget" FILES ${Widget_SRCS})

# The view sources
SET(View_CPP_SRCS
    MDIView.cpp
    GraphvizView.cpp
    ActiveObjectList.cpp
)
SET(View_HPP_SRCS
    MDIView.h
    GraphvizView.h
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
    SelectionFilter.y
    SelectionFilter.l
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
    ExpressionBinding.cpp
    GraphicsViewZoom.cpp
    ExpressionCompleter.cpp
    GuiApplication.cpp
    GuiApplicationNativeEventAware.cpp
    GuiConsole.cpp
    Macro.cpp
    MergeDocuments.cpp
    Namespace.h
    resource.cpp
    Control.cpp
    SpaceballEvent.cpp
    Thumbnail.cpp
    Utilities.cpp
    WaitCursor.cpp
    ManualAlignment.cpp
    TransactionObject.cpp
    WinNativeGestureRecognizers.cpp
)
SET(FreeCADGui_SRCS 
    Application.h
    AutoSaver.h
    BitmapFactory.h
    Document.h
    DocumentModel.h
    DocumentObserver.h
    DocumentObserverPython.h
    ExpressionBinding.cpp
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
    PreCompiled.cpp
    PreCompiled.h
    Qt4All.h
    Control.h
    SpaceballEvent.h
    Thumbnail.h
    Utilities.h
    WaitCursor.h
    ManualAlignment.h
    TransactionObject.h
    WinNativeGestureRecognizers.h
    ${FreeCADGui_SDK_MOC_HDRS}
)

SET(FreeCADGui_SRCS
    ${FreeCADGui_SDK_SRCS}
    ${FreeCADGui_CPP_SRCS}
    ${FreeCADGui_XML_SRCS}
    ${iis_MOC_SRCS}
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
    ${iisTaskPanel_SRCS}
    ${qsintActionPanel_SRCS}
    ${Resource_SRCS}
    ${Quarter_SRCS}
    ${View3D_SRCS}
    ${Viewprovider_SRCS}
    ${Widget_SRCS}
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

add_library(FreeCADGui SHARED ${FreeCADGui_SRCS})
target_link_libraries(FreeCADGui ${FreeCADGui_LIBS})

SET_BIN_DIR(FreeCADGui FreeCADGui)

if(WIN32)
    INSTALL(TARGETS FreeCADGui
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
else(WIN32)
    INSTALL(TARGETS FreeCADGui
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    INSTALL(FILES Icons/freecad.xpm
        Icons/freecad-icon-16.png
        Icons/freecad-icon-32.png
        Icons/freecad-icon-48.png
        Icons/freecad-icon-64.png
        Icons/freecad.svg
        Icons/freecad-doc.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}
    )
endif(WIN32)

