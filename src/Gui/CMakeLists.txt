#add_subdirectory(Icons)

if(WIN32)
add_definitions(-DFCGui -DQIIS_MAKEDLL)
endif(WIN32)

if (FREECAD_USE_3DCONNEXION)
add_definitions(-D_USE_3DCONNEXION_SDK)
endif(FREECAD_USE_3DCONNEXION)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/Language
    ${CMAKE_CURRENT_BINARY_DIR}/propertyeditor
    ${CMAKE_CURRENT_BINARY_DIR}/TaskView
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${SOQT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
)

if(MSVC)
    set(FreeCADGui_LIBS
         FreeCADApp
         debug ${COIN3D_LIBRARY_DEBUG}
         optimized ${COIN3D_LIBRARY_RELEASE}
         debug ${SOQT_LIBRARY_DEBUG}
         optimized ${SOQT_LIBRARY_RELEASE}
         ${QT_DEBUG_LIBRARIES}
         ${QT_LIBRARIES}
         QtUiTools.lib
         ${OPENGL_gl_LIBRARY}
    )
else(MSVC)
    set(FreeCADGui_LIBS
        FreeCADBase
        FreeCADApp
        ${COIN3D_LIBRARY}
        ${SOQT_LIBRARIES}
        ${QT_LIBRARIES}
        ${Boost_LIBRARIES}
        ${OPENGL_gl_LIBRARY}
    )
endif(MSVC)

IF(SPNAV_FOUND)
    add_definitions(-DSPNAV_FOUND)
    include_directories(
        ${SPNAV_INCLUDE_DIR}
    )
    set(FreeCADGui_LIBS
        ${FreeCADGui_LIBS}
        ${SPNAV_LIBRARIES}
    )
ENDIF(SPNAV_FOUND)

if(SHIBOKEN_INCLUDE_DIR)
    add_definitions(-DHAVE_SHIBOKEN)
    include_directories(
        ${SHIBOKEN_INCLUDE_DIR}
    )
    set(FreeCADGui_LIBS
        ${FreeCADGui_LIBS}
        ${SHIBOKEN_LIBRARY}
    )
endif(SHIBOKEN_INCLUDE_DIR)

if(PYSIDE_INCLUDE_DIR)
    add_definitions(-DHAVE_PYSIDE)
    include_directories(
        ${PYSIDE_INCLUDE_DIR}
        ${PYSIDE_INCLUDE_DIR}/QtCore
        ${PYSIDE_INCLUDE_DIR}/QtGui
    )
    set(FreeCADGui_LIBS
        ${FreeCADGui_LIBS}
        ${PYSIDE_LIBRARY}
    )
endif(PYSIDE_INCLUDE_DIR)

generate_from_xml(DocumentPy)
generate_from_xml(PythonWorkbenchPy)
generate_from_xml(ViewProviderPy)
generate_from_xml(ViewProviderDocumentObjectPy)
generate_from_xml(ViewProviderPythonFeaturePy)
generate_from_xml(WorkbenchPy)
generate_from_xml(SelectionObjectPy)

generate_from_py(FreeCADGuiInit GuiInitScript.h)

# The XML files
SET(FreeCADGui_XML_SRCS
    ViewProviderDocumentObjectPy.xml
    ViewProviderPythonFeaturePy.xml
    ViewProviderPy.xml
    PythonWorkbenchPy.xml
    WorkbenchPy.xml
    SelectionObjectPy.xml
    DocumentPy.xml
)
SOURCE_GROUP("XML" FILES ${FreeCADApp_XML_SRCS})

# The 3D Connexion SDK files
if(FREECAD_USE_3DCONNEXION)
SET(FreeCADGui_SDK_SRCS
    3Dconnexion/I3dMouseParams.h
    3Dconnexion/MouseParameters.cpp
    3Dconnexion/MouseParameters.h
    3Dconnexion/GuiApplicationNativeEventAwareWin32.cpp
)
SOURCE_GROUP("3D connexion SDK" FILES ${FreeCADGui_SDK_SRCS})
endif(FREECAD_USE_3DCONNEXION)

set(Gui_MOC_HDRS
    Action.h
    CallTips.h
    CombiView.h
    Control.h
    Clipping.h
    DemoMode.h
    DownloadDialog.h
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
    DlgPreferencesImp.h
    DlgProjectUtility.h
    DlgReportViewImp.h
    DlgSettings3DViewImp.h
    DlgSettingsViewColor.h
    DlgSettingsColorGradientImp.h
    DlgSettingsDocumentImp.h
    DlgSettingsImageImp.h
    DlgSettingsMacroImp.h
    DlgSettingsUnitsImp.h
    DlgTipOfTheDayImp.h
    DlgToolbarsImp.h
    TaskDlgRelocation.h
    DlgUndoRedo.h
    DockWindow.h
    DockWindowManager.h
    EditorView.h
    FileDialog.h
    Flag.h
    GuiApplicationNativeEventAware.h
    HelpView.h
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
    ReportView.h
    SceneInspector.h
    SelectionView.h
    SpinBox.h
    Splashscreen.h
    TaskPanelView.h
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
)
#qt4_wrap_cpp(Gui_MOC_SRCS ${Gui_MOC_HDRS})
fc_wrap_cpp(Gui_MOC_SRCS ${Gui_MOC_HDRS})
#SOURCE_GROUP("Moc" FILES ${Gui_MOC_SRCS})

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
    DlgPreferences.ui
    DlgProjectInformation.ui
    DlgProjectUtility.ui
    DlgReportView.ui
    DlgSettings3DView.ui
    DlgSettingsUnits.ui
    DlgSettingsViewColor.ui
    DlgSettingsColorGradient.ui
    DlgSettingsDocument.ui
    DlgSettingsImage.ui
    DlgSettingsMacro.ui
    DlgTipOfTheDay.ui
    DlgToolbars.ui
    DlgTreeWidget.ui
    DlgLocationAngle.ui
    DlgLocationPos.ui
    DownloadManager.ui
    DownloadItem.ui
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
)
QT4_ADD_RESOURCES(Gui_QRC_SRCS ${Gui_RES_SRCS})

QT4_WRAP_UI(Gui_UIC_HDRS ${Gui_UIC_SRCS})
SOURCE_GROUP("Uic" FILES ${Gui_UIC_HDRS})

# The command sources
SET(Command_CPP_SRCS
    Action.cpp
    Command.cpp
    CommandDoc.cpp
    CommandFeat.cpp
    CommandMacro.cpp
    CommandStd.cpp
    CommandWindow.cpp
    CommandTest.cpp
    CommandView.cpp
)
SET(Command_SRCS
    ${Command_CPP_SRCS}
    Action.h
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
    DlgProjectInformationImp.cpp
    DlgProjectUtility.cpp
    DlgTipOfTheDayImp.cpp
    TaskDlgRelocation.cpp
    DlgUndoRedo.cpp
    InputVector.cpp
    Placement.cpp
    PropertyPage.cpp
    SceneInspector.cpp
    TextureMapping.cpp
    Transform.cpp
    DownloadDialog.cpp
    DownloadItem.cpp
    DownloadManager.cpp
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
    DlgProjectInformationImp.h
    DlgProjectUtility.h
    DlgTipOfTheDayImp.h
    TaskDlgRelocation.h
    DlgUndoRedo.h
    InputVector.h
    Placement.h
    PropertyPage.h
    SceneInspector.h
    TextureMapping.h
    Transform.h
    DownloadDialog.h
    DownloadItem.h
    DownloadManager.h
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
    DlgProjectInformation.ui
    DlgProjectUtility.ui
    DlgTipOfTheDay.ui
    DlgTreeWidget.ui
    DownloadManager.ui
    DownloadItem.ui
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
)
SET(Dialog_Customize_HPP_SRCS
    DlgActionsImp.h
    DlgCommandsImp.h
    DlgCustomizeImp.h
    DlgCustomizeSpaceball.h
    DlgCustomizeSpNavSettings.h
    DlgKeyboardImp.h
    DlgToolbarsImp.h
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
    HelpView.cpp
    PropertyView.cpp
    ReportView.cpp
    SelectionView.cpp
    TaskPanelView.cpp
    ToolBox.cpp
    Tree.cpp
    TreeView.cpp
    ProjectView.cpp
)
SET(Dock_Windows_HPP_SRCS
    CombiView.h
    DockWindow.h
    HelpView.h
    PropertyView.h
    ReportView.h
    SelectionView.h
    TaskPanelView.h
    ToolBox.h
    Tree.h
    TreeView.h
    ProjectView.h
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
qt4_wrap_cpp(iis_MOC_SRCS ${iis_MOC_HDRS})
SOURCE_GROUP("Widget\\iisTaskPanel\\Mocs" FILES ${iis_MOC_SRCS})


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
    GLPainter.cpp
    MouseSelection.cpp
    NavigationStyle.cpp
    InventorNavigationStyle.cpp
    CADNavigationStyle.cpp
    BlenderNavigationStyle.cpp
    TouchpadNavigationStyle.cpp
    SplitView3DInventor.cpp
    View.cpp
    View3DInventor.cpp
    View3DInventorExamples.cpp
    View3DInventorViewer.cpp
    View3DPy.cpp
)
SET(View3D_SRCS
    ${View3D_CPP_SRCS}
    Flag.h
    GLPainter.h
    MouseSelection.h
    NavigationStyle.h
    SplitView3DInventor.h
    View.h
    View3DInventor.h
    View3DInventorExamples.h
    View3DInventorViewer.h
    View3DPy.h
)
SOURCE_GROUP("View3D" FILES ${View3D_SRCS})

# The view provider sources
SET(Viewprovider_CPP_SRCS
    ViewProvider.cpp
    ViewProviderAnnotation.cpp
    ViewProviderDocumentObject.cpp
    ViewProviderDocumentObjectGroup.cpp
    ViewProviderDocumentObjectPyImp.cpp
    ViewProviderPythonFeaturePyImp.cpp
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
    ViewProviderPlane.cpp
    ViewProviderMaterialObject.cpp
)
SET(Viewprovider_SRCS
    ${Viewprovider_CPP_SRCS}
    ViewProvider.h
    ViewProviderAnnotation.h
    ViewProviderDocumentObject.h
    ViewProviderDocumentObjectGroup.h
    ViewProviderExtern.h
    ViewProviderFeature.h
    ViewProviderGeometryObject.h
    ViewProviderInventorObject.h
    ViewProviderMeasureDistance.h
    ViewProviderPythonFeature.h
    ViewProviderVRMLObject.h
    ViewProviderBuilder.h
    ViewProviderPlacement.h
    ViewProviderPlane.h
    ViewProviderMaterialObject.h
)
SOURCE_GROUP("View3D\\Viewprovider" FILES ${Viewprovider_SRCS})

# The Inventor sources
SET(Inventor_CPP_SRCS
    Inventor/SoDrawingGrid.cpp
    Inventor/SoAutoZoomTranslation.cpp
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
)
SET(Inventor_SRCS
    ${Inventor_CPP_SRCS}
    Inventor/SoDrawingGrid.h
    Inventor/SoAutoZoomTranslation.h
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
)
SOURCE_GROUP("View3D\\Inventor" FILES ${Inventor_SRCS})

# The widget sources
SET(Widget_CPP_SRCS
    FileDialog.cpp
    MainWindow.cpp
    MDIView.cpp
    PrefWidgets.cpp
    InputField.cpp
    ProgressBar.cpp
    SpinBox.cpp
    Splashscreen.cpp
    WidgetFactory.cpp
    Widgets.cpp
    Window.cpp
)
SET(Widget_HPP_SRCS
    FileDialog.h
    MainWindow.h
    MDIView.h
    PrefWidgets.h
    InputField.h
    ProgressBar.h
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
    BitmapFactory.cpp
    Document.cpp
    DocumentModel.cpp
    DocumentPyImp.cpp
    GuiApplicationNativeEventAware.cpp
    GuiConsole.cpp
    Macro.cpp
    MergeDocuments.cpp
    resource.cpp
    Control.cpp
    SpaceballEvent.cpp
    Thumbnail.cpp
    Utilities.cpp
    WaitCursor.cpp
    ManualAlignment.cpp
)
SET(FreeCADGui_SRCS
    Application.h
    BitmapFactory.h
    Document.h
    DocumentModel.h
    FreeCADGuiInit.py
    GuiApplicationNativeEventAware.h
    GuiConsole.h
    InventorAll.h
    Macro.h
    MergeDocuments.h
    PreCompiled.cpp
    PreCompiled.h
    Qt4All.h
    Control.h
    SpaceballEvent.h
    Thumbnail.h
    Utilities.h
    WaitCursor.h
    ManualAlignment.h
)

SET(FreeCADGui_SRCS
    ${FreeCADGui_SDK_SRCS}
    ${FreeCADGui_CPP_SRCS}
    ${FreeCADGui_XML_SRCS}
    ${iis_MOC_SRCS}
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
    ${Resource_SRCS}
    ${View3D_SRCS}
    ${Viewprovider_SRCS}
    ${Widget_SRCS}
    ${Workbench_SRCS}
    ${Selection_SRCS}
    ${FreeCADGui_SRCS}
)

if(MSVC)
    add_definitions(-D_PreComp_)
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
    ADD_MSVC_PRECOMPILED_HEADER("PreCompiled.h" "PreCompiled.cpp" FreeCADGui_CPP_SRCS)
endif(MSVC)

add_library(FreeCADGui SHARED ${FreeCADGui_SRCS})

target_link_libraries(FreeCADGui ${FreeCADGui_LIBS})

# Note this is IDE specific, not necessarily platform specific
if(MSVC)
    set_target_properties(FreeCADGui PROPERTIES DEBUG_OUTPUT_NAME "FreeCADGuiD")
    set_target_properties(FreeCADGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    # dirty hack to avoid Debug/Release subdirectory
    set_target_properties(FreeCADGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(FreeCADGui PROPERTIES DEBUG_OUTPUT_NAME "FreeCADGuiD")
    set_target_properties(FreeCADGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else(MSVC)
    set_target_properties(FreeCADGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set_target_properties(FreeCADGui PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

if(WIN32)
    INSTALL(TARGETS FreeCADGui
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
    )
else(WIN32)
    INSTALL(TARGETS FreeCADGui
        LIBRARY DESTINATION lib
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

