add_library(FreeCADGui SHARED)

if ( EXISTS "${CMAKE_SOURCE_DIR}/src/3rdParty/GSL/include" )
    include_directories( ${CMAKE_SOURCE_DIR}/src/3rdParty/GSL/include )
else()
    find_package(Microsoft.GSL)
    if( Microsoft.GSL_FOUND )
        message( STATUS "Found Microsoft.GSL: version ${Microsoft.GSL_VERSION}" )
    else()
        message( SEND_ERROR "The C++ Guidelines Support Library (GSL) submodule is not available. Please run git submodule update --init" )
    endif()
endif()

add_subdirectory(Stylesheets)
add_subdirectory(PreferencePacks)
add_subdirectory(PreferencePackTemplates)

if(BUILD_WITH_CONDA)
    add_definitions(-DFC_CONDA)
endif(BUILD_WITH_CONDA)

if(FC_FLATPAK)
    add_definitions(-DFC_FLATPAK)
endif(FC_FLATPAK)

if(WIN32)
    add_definitions(-DFCGui -DQSINT_MAKEDLL -DOVR_OS_WIN32 -DQUARTER_INTERNAL -DQUARTER_MAKE_DLL -DCOIN_DLL)
endif(WIN32)

IF(CMAKE_BUILD_TYPE)
    add_definitions(-DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
ENDIF(CMAKE_BUILD_TYPE)

if (FREECAD_USE_3DCONNEXION_LEGACY AND (MSVC OR APPLE))
    add_definitions(-D_USE_3DCONNEXION_SDK)
    if(APPLE)
        set(3DCONNEXION_LINKFLAGS "-F/Library/Frameworks -weak_framework 3DconnexionClient")
        list(APPEND 3DCONNEXION_INCLUDE_DIR ${3DCONNEXIONCLIENT_FRAMEWORK}/Headers
                                    ${3DCONNEXIONCLIENT_FRAMEWORK}/Headers/3DconnexionClient )
    endif(APPLE)
endif(FREECAD_USE_3DCONNEXION_LEGACY AND (MSVC OR APPLE))

if(FREECAD_USE_3DCONNEXION_NAVLIB AND (MSVC OR APPLE))
    add_definitions(-DUSE_3DCONNEXION_NAVLIB)
    if(APPLE)
        add_definitions(-D__APPLE__)
    endif(APPLE)
    list(APPEND 3DCONNEXION_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/3Dconnexion/inc)
endif(FREECAD_USE_3DCONNEXION_NAVLIB AND (MSVC OR APPLE))

if (BUILD_VR)
    add_definitions(-DBUILD_VR )
endif(BUILD_VR)

if (BUILD_ADDONMGR)
    add_definitions(-DBUILD_ADDONMGR )
endif(BUILD_ADDONMGR)

if (BUILD_TRACY_FRAME_PROFILER)
    add_definitions(-DBUILD_TRACY_FRAME_PROFILER)
endif()

target_include_directories(
    FreeCADGui
    PUBLIC
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Quarter
    ${CMAKE_CURRENT_SOURCE_DIR}/PreferencePages
    ${CMAKE_CURRENT_SOURCE_DIR}/Selection
    ${CMAKE_CURRENT_SOURCE_DIR}/StyleParameters
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/Language
    ${CMAKE_CURRENT_BINARY_DIR}/propertyeditor
    ${CMAKE_CURRENT_BINARY_DIR}/TaskView
    ${CMAKE_CURRENT_BINARY_DIR}/Quarter
    ${CMAKE_CURRENT_BINARY_DIR}/DAGView
)

target_include_directories(
    FreeCADGui
    SYSTEM
    PRIVATE
    ${FastSignals_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${3DCONNEXION_INCLUDE_DIR}
)

if(MSVC)
    target_include_directories(
        FreeCADGui
        SYSTEM
        PUBLIC
        ${CMAKE_SOURCE_DIR}/src/3rdParty/OpenGL/api
    )
endif(MSVC)

if(MSVC)
    set(FreeCADGui_LIBS
         FreeCADApp
         libfastsignals
         ${OPENGL_gl_LIBRARY}
    )

    if(FREECAD_USE_3DCONNEXION_LEGACY)
        list(APPEND FreeCADGui_LIBS
            hid
        )
    endif()

else(MSVC)
    set(FreeCADGui_LIBS
        FreeCADApp
        libfastsignals
        ${Boost_LIBRARIES}
        ${OPENGL_gl_LIBRARY}
        ${3DCONNEXION_LINKFLAGS}
    )
endif(MSVC)

if(BUILD_TRACY_FRAME_PROFILER)
    list(APPEND FreeCADGui_LIBS TracyClient)
endif()

if (TARGET Coin::Coin)
    list(APPEND FreeCADGui_LIBS Coin::Coin)
else()
    target_include_directories(FreeCADGui SYSTEM PUBLIC ${COIN3D_INCLUDE_DIRS})
    target_link_directories(FreeCADGui PUBLIC ${COIN3D_LIB_DIRS})
    list(APPEND FreeCADGui_LIBS ${COIN3D_LIBRARIES})
endif()

if (WIN32)
    # In order to make menus and tooltips usable in fullscreen under Windows we need to call setHasBorderInFullScreen(),
    # defined in Qt's private Gui code. See issue #7563, and the calls to QNativeInterface::Private in MainWindow.cpp
    if(FREECAD_QT_MAJOR_VERSION EQUAL 6)
        if(Qt6_VERSION VERSION_GREATER_EQUAL "6.10.0")
            find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)
        endif()
        list(APPEND FreeCADGui_LIBS
            Qt6::GuiPrivate
        )
    endif()
endif()

target_include_directories(
    FreeCADGui
    SYSTEM
    PUBLIC
    ${QtCore_INCLUDE_DIRS}
    ${QtWidgets_INCLUDE_DIRS}
    ${QtOpenGL_INCLUDE_DIRS}
    ${QtOpenGLWidgets_INCLUDE_DIRS}
    ${QtPrintSupport_INCLUDE_DIRS}
    ${QtSvg_INCLUDE_DIRS}
    ${QtSvgWidgets_INCLUDE_DIRS}
    ${QtNetwork_INCLUDE_DIRS}
    ${QtUiTools_INCLUDE_DIRS}
    ${QtXml_INCLUDE_DIRS}
    ${YAML_CPP_INCLUDE_DIRS}
)

list(APPEND FreeCADGui_LIBS
    ${QtCore_LIBRARIES}
    ${QtWidgets_LIBRARIES}
    ${QtOpenGL_LIBRARIES}
    ${QtOpenGLWidgets_LIBRARIES}
    ${QtPrintSupport_LIBRARIES}
    ${QtSvg_LIBRARIES}
    ${QtSvgWidgets_LIBRARIES}
    ${QtNetwork_LIBRARIES}
    ${QtUiTools_LIBRARIES}
)

link_directories(${YAML_CPP_LIBRARY_DIR})

if(yaml-cpp_VERSION VERSION_LESS 0.8.0)
    list(APPEND FreeCADGui_LIBS
            ${YAML_CPP_LIBRARIES}
    )
else()
    list(APPEND FreeCADGui_LIBS
            yaml-cpp::yaml-cpp
    )
endif()

if(${Qt5WinExtras_FOUND})
    target_include_directories(
        FreeCADGui
        SYSTEM
        PUBLIC
        ${Qt5WinExtras_INCLUDE_DIRS}
    )
    list(APPEND FreeCADGui_LIBS
        ${Qt5WinExtras_LIBRARIES}
    )
endif()

IF(SPNAV_FOUND AND FREECAD_USE_3DCONNEXION_LEGACY)
    add_definitions(-D_USE_3DCONNEXION_SDK)
    if(SPNAV_USE_X11)
        add_definitions(-DSPNAV_USE_X11)
        if (FREECAD_QT_MAJOR_VERSION EQUAL 5 AND UNIX AND NOT APPLE)
            find_package(Qt5X11Extras REQUIRED)
            target_include_directories(
                FreeCADGui
                SYSTEM
                PUBLIC
                ${Qt5X11Extras_INCLUDE_DIRS}
            )
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
    list(APPEND FreeCADGui_connexion_SRCS
        ${FreeCADGui_SDK_SRCS}
    )
    list(APPEND FreeCADGui_connexion_HDRS
        ${FreeCADGui_SDK_MOC_HDRS}
    )

    add_definitions(-DSPNAV_FOUND)
    target_include_directories(
        FreeCADGui
        SYSTEM
        PUBLIC
        ${SPNAV_INCLUDE_DIR}
    )
    list(APPEND FreeCADGui_LIBS
        ${SPNAV_LIBRARIES}
    )
ENDIF(SPNAV_FOUND AND FREECAD_USE_3DCONNEXION_LEGACY)

IF(OCULUS_FOUND)
    add_definitions(-DOCULUS_FOUND)
    target_include_directories(
        FreeCADGui
        SYSTEM
        PUBLIC
        ${OCULUS_INCLUDE_DIRS}
    )
    list(APPEND FreeCADGui_LIBS
        ${OCULUS_LIBRARIES}
    )
ENDIF(OCULUS_FOUND)

if(FREECAD_USE_SHIBOKEN)
    add_definitions(-DHAVE_SHIBOKEN${PYSIDE_MAJOR_VERSION})
    target_include_directories(
        FreeCADGui
        SYSTEM
        PUBLIC
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
    target_include_directories(
        FreeCADGui
        SYSTEM
        PUBLIC
        ${PYSIDE_INCLUDE_DIR}
        ${PYSIDE_INCLUDE_DIR}/QtCore
        ${PYSIDE_INCLUDE_DIR}/QtGui
        ${PYSIDE_INCLUDE_DIR}/QtWidgets
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

    add_definitions(-DHAVE_PYSIDE${PYSIDE_MAJOR_VERSION})
endif(FREECAD_USE_PYSIDE)

generate_from_py(Document)
generate_from_py(PythonWorkbench)
generate_from_py(ViewProvider)
generate_from_py(ViewProviderDocumentObject)
generate_from_py(ViewProviderGeometryObject)
generate_from_py(ViewProviderExtension)
generate_from_py(Workbench)
generate_from_py(Selection/SelectionObject)
generate_from_py(LinkView)
generate_from_py(ViewProviderLink)
generate_from_py(AxisOrigin)
generate_from_py(Command)
generate_from_py(Navigation/NavigationStyle)

generate_embed_from_py(FreeCADGuiInit GuiInitScript.h)

# The Pyi files
SET(FreeCADGui_Pyi_SRCS
    ViewProviderDocumentObject.pyi
    ViewProviderGeometryObject.pyi
    ViewProvider.pyi
    ViewProviderExtension.pyi
    PythonWorkbench.pyi
    Workbench.pyi
    Selection/SelectionObject.pyi
    Document.pyi
    LinkView.pyi
    ViewProviderLink.pyi
    AxisOrigin.pyi
    Command.pyi
)
SOURCE_GROUP("Pyi" FILES ${FreeCADGui_Pyi_SRCS})

# The 3D Connexion SDK files
if(FREECAD_USE_3DCONNEXION_LEGACY AND MSVC)
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
    list(APPEND FreeCADGui_connexion_SRCS
        ${FreeCADGui_SDK_SRCS}
    )
    list(APPEND FreeCADGui_connexion_HDRS
        ${FreeCADGui_SDK_MOC_HDRS}
    )
endif(FREECAD_USE_3DCONNEXION_LEGACY AND MSVC)

if(FREECAD_USE_3DCONNEXION_LEGACY AND APPLE)
    SET(FreeCADGui_SDK_SRCS
        3Dconnexion/GuiAbstractNativeEvent.cpp
        3Dconnexion/GuiNativeEventMac.cpp
    )
    SOURCE_GROUP("3D connexion SDK" FILES ${FreeCADGui_SDK_SRCS})
    SET(FreeCADGui_SDK_MOC_HDRS
      3Dconnexion/GuiAbstractNativeEvent.h
      3Dconnexion/GuiNativeEventMac.h
    )
    list(APPEND FreeCADGui_connexion_SRCS
        ${FreeCADGui_SDK_SRCS}
    )
    list(APPEND FreeCADGui_connexion_HDRS
        ${FreeCADGui_SDK_MOC_HDRS}
    )
endif(FREECAD_USE_3DCONNEXION_LEGACY AND APPLE)

if(FREECAD_USE_3DCONNEXION_NAVLIB AND (MSVC OR APPLE))
    SET(NAVLIB_STUB_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/3Dconnexion/src)
    SET(FreeCADGui_SDK_SRCS
        3Dconnexion/navlib/NavlibCmds.cpp
        3Dconnexion/navlib/NavlibNavigation.cpp
        3Dconnexion/navlib/NavlibPivot.cpp
        ${NAVLIB_STUB_DIR}/navlib_load.cpp
        ${NAVLIB_STUB_DIR}/navlib_stub.c
    )
    SOURCE_GROUP("3Dconnexion Navlib" FILES ${FreeCADGui_SDK_SRCS})
    SET(FreeCADGui_SDK_MOC_HDRS
        3Dconnexion/navlib/NavlibInterface.h
    )
    list(APPEND FreeCADGui_connexion_SRCS
        ${FreeCADGui_SDK_SRCS}
    )
    list(APPEND FreeCADGui_connexion_HDRS
        ${FreeCADGui_SDK_MOC_HDRS}
    )
endif(FREECAD_USE_3DCONNEXION_NAVLIB AND (MSVC OR APPLE))

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
    Dialogs/AboutApplication.ui
    Clipping.ui
    DemoMode.ui
    Dialogs/DlgActions.ui
    Dialogs/DlgActivateWindow.ui
    Dialogs/DlgUnitsCalculator.ui
    Dialogs/DlgAuthorization.ui
    Dialogs/DlgChooseIcon.ui
    Dialogs/DlgCreateNewPreferencePack.ui
    Dialogs/DlgInputDialog.ui
    Dialogs/DlgKeyboard.ui
    Dialogs/DlgMacroExecute.ui
    Dialogs/DlgRunExternal.ui
    Dialogs/DlgMacroRecord.ui
    Dialogs/DlgMaterialProperties.ui
    Dialogs/DlgOnlineHelp.ui
    Dialogs/DlgParameter.ui
    Dialogs/DlgParameterFind.ui
    Dialogs/DlgPreferencePackManagement.ui
    Dialogs/DlgPreferences.ui
    Dialogs/DlgProjectInformation.ui
    Dialogs/DlgProjectUtility.ui
    Dialogs/DlgPropertyLink.ui
    Dialogs/DlgRevertToBackupConfig.ui
    Dialogs/DlgThemeEditor.ui
    Dialogs/DlgVersionMigrator.ui
    PreferencePages/DlgSettings3DView.ui
    PreferencePages/DlgSettingsCacheDirectory.ui
    Dialogs/DlgSettingsColorGradient.ui
    PreferencePages/DlgSettingsDocument.ui
    PreferencePages/DlgSettingsEditor.ui
    PreferencePages/DlgSettingsGeneral.ui
    Dialogs/DlgSettingsImage.ui
    PreferencePages/DlgSettingsLightSources.ui
    PreferencePages/DlgSettingsMacro.ui
    PreferencePages/DlgSettingsNavigation.ui
    PreferencePages/DlgSettingsNotificationArea.ui
    PreferencePages/DlgSettingsPythonConsole.ui
    PreferencePages/DlgSettingsReportView.ui
    PreferencePages/DlgSettingsSelection.ui
    PreferencePages/DlgSettingsUI.ui
    PreferencePages/DlgSettingsViewColor.ui
    PreferencePages/DlgSettingsWorkbenches.ui
    PreferencePages/DlgSettingsPDF.ui
    Dialogs/DlgCheckableMessageBox.ui
    Dialogs/DlgToolbars.ui
    Dialogs/DlgTreeWidget.ui
    Dialogs/DlgLocationAngle.ui
    Dialogs/DlgLocationPos.ui
    DocumentRecovery.ui
    DownloadManager.ui
    DownloadItem.ui
    Dialogs/DlgExpressionInput.ui
    MouseButtons.ui
    SceneInspector.ui
    InputVector.ui
    Placement.ui
    TaskTransform.ui
    TextureMapping.ui
    TaskView/TaskAppearance.ui
    TaskView/TaskOrientation.ui
    TaskView/TaskImage.ui
    TaskView/TaskSolverMessages.ui
    TaskView/TaskSelectLinkProperty.ui
    TaskElementColors.ui
    Dialogs/DlgObjectSelection.ui
    Dialogs/DlgAddProperty.ui
    VectorListEditor.ui
)

if(FREECAD_USE_3DCONNEXION_LEGACY)
    list(APPEND Gui_UIC_SRCS Dialogs/DlgCustomizeSpNavSettings.ui)
endif(FREECAD_USE_3DCONNEXION_LEGACY)

set (FreeCAD_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Language/FreeCAD_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Language/FreeCAD_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Language)
qt_create_resource_file_prefix(${FreeCAD_TR_QRC} ${QM_SRCS})
set(Gui_RES_SRCS
    Icons/resource.qrc
    Language/translation.qrc
    ${FreeCAD_TR_QRC}
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
    Dialogs/DlgAbout.cpp
    Dialogs/DlgActivateWindowImp.cpp
    Dialogs/DlgCreateNewPreferencePackImp.cpp
    Dialogs/DlgUnitsCalculatorImp.cpp
    Dialogs/DlgInputDialogImp.cpp
    Dialogs/DlgMacroExecuteImp.cpp
    Dialogs/DlgRunExternal.cpp
    Dialogs/DlgEditFileIncludePropertyExternal.cpp
    Dialogs/DlgMacroRecordImp.cpp
    Dialogs/DlgMaterialPropertiesImp.cpp
    Dialogs/DlgParameterImp.cpp
    Dialogs/DlgParameterFind.cpp
    Dialogs/DlgPreferencePackManagementImp.cpp
    Dialogs/DlgProjectInformationImp.cpp
    Dialogs/DlgProjectUtility.cpp
    Dialogs/DlgPropertyLink.cpp
    Dialogs/DlgRevertToBackupConfigImp.cpp
    Dialogs/DlgExpressionInput.cpp
    Dialogs/DlgThemeEditor.cpp
    Dialogs/DlgVersionMigrator.cpp
    TaskDlgRelocation.cpp
    Dialogs/DlgCheckableMessageBox.cpp
    TaskTransform.cpp
    Dialogs/DlgUndoRedo.cpp
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
    Dialogs/DlgObjectSelection.cpp
    Dialogs/DlgAddProperty.cpp
    VectorListEditor.cpp
)

SET(Dialog_HPP_SRCS
    Clipping.h
    DemoMode.h
    Dialogs/DlgAbout.h
    Dialogs/DlgActivateWindowImp.h
    Dialogs/DlgCreateNewPreferencePackImp.h
    Dialogs/DlgUnitsCalculatorImp.h
    Dialogs/DlgInputDialogImp.h
    Dialogs/DlgMacroExecuteImp.h
    Dialogs/DlgRunExternal.h
    Dialogs/DlgEditFileIncludePropertyExternal.h
    Dialogs/DlgMacroRecordImp.h
    Dialogs/DlgMaterialPropertiesImp.h
    Dialogs/DlgParameterImp.h
    Dialogs/DlgParameterFind.h
    Dialogs/DlgPreferencePackManagementImp.h
    Dialogs/DlgProjectInformationImp.h
    Dialogs/DlgProjectUtility.h
    Dialogs/DlgPropertyLink.h
    Dialogs/DlgRevertToBackupConfigImp.h
    Dialogs/DlgCheckableMessageBox.h
    Dialogs/DlgExpressionInput.h
    Dialogs/DlgThemeEditor.h
    Dialogs/DlgVersionMigrator.h
    TaskDlgRelocation.h
    TaskTransform.h
    Dialogs/DlgUndoRedo.h
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
    Dialogs/DlgObjectSelection.h
    Dialogs/DlgAddProperty.h
    VectorListEditor.h
)

SET(Dialog_SRCS
    ${Dialog_CPP_SRCS}
    ${Dialog_HPP_SRCS}
    Dialogs/AboutApplication.ui
    Clipping.ui
    DemoMode.ui
    Dialogs/DlgActivateWindow.ui
    Dialogs/DlgUnitsCalculator.ui
    Dialogs/DlgAuthorization.ui
    Dialogs/DlgInputDialog.ui
    Dialogs/DlgAddProperty.ui
    Dialogs/DlgLocationAngle.ui
    Dialogs/DlgLocationPos.ui
    Dialogs/DlgMacroExecute.ui
    Dialogs/DlgRunExternal.ui
    Dialogs/DlgMacroRecord.ui
    Dialogs/DlgMaterialProperties.ui
    Dialogs/DlgParameter.ui
    Dialogs/DlgParameterFind.ui
    Dialogs/DlgPreferencePackManagement.ui
    Dialogs/DlgProjectInformation.ui
    Dialogs/DlgProjectUtility.ui
    Dialogs/DlgPropertyLink.ui
    Dialogs/DlgRevertToBackupConfig.ui
    Dialogs/DlgCheckableMessageBox.ui
    Dialogs/DlgTreeWidget.ui
    Dialogs/DlgExpressionInput.ui
    Dialogs/DlgCreateNewPreferencePack.ui
    Dialogs/DlgVersionMigrator.ui
    DownloadManager.ui
    DownloadItem.ui
    DocumentRecovery.ui
    MouseButtons.ui
    InputVector.ui
    Placement.ui
    SceneInspector.ui
    TextureMapping.ui
    TaskElementColors.ui
    Dialogs/DlgObjectSelection.ui
    VectorListEditor.ui
)
SOURCE_GROUP("Dialog" FILES ${Dialog_SRCS})

# The customize dialog sources
SET(Dialog_Customize_CPP_SRCS
    Dialogs/DlgActionsImp.cpp
    Dialogs/DlgCustomizeImp.cpp
    Dialogs/DlgKeyboardImp.cpp
    Dialogs/DlgToolbarsImp.cpp
    PreferencePages/ListWidgetDragBugFix.cpp
)
SET(Dialog_Customize_HPP_SRCS
    Dialogs/DlgActionsImp.h
    Dialogs/DlgCustomizeImp.h
    Dialogs/DlgKeyboardImp.h
    Dialogs/DlgToolbarsImp.h
    PreferencePages/ListWidgetDragBugFix.h
)

if(FREECAD_USE_3DCONNEXION_LEGACY)
    list(APPEND Dialog_Customize_CPP_SRCS Dialogs/DlgCustomizeSpaceball.cpp Dialogs/DlgCustomizeSpNavSettings.cpp)
    list(APPEND Dialog_Customize_HPP_SRCS Dialogs/DlgCustomizeSpaceball.h Dialogs/DlgCustomizeSpNavSettings.h)
endif(FREECAD_USE_3DCONNEXION_LEGACY)

SET(Dialog_Customize_SRCS
    ${Dialog_Customize_CPP_SRCS}
    ${Dialog_Customize_HPP_SRCS}
    Dialogs/DlgActions.ui
    Dialogs/DlgChooseIcon.ui
    Dialogs/DlgKeyboard.ui
    Dialogs/DlgToolbars.ui
)

if(FREECAD_USE_3DCONNEXION_LEGACY)
    list(APPEND Dialog_Customize_SRCS  Dialogs/DlgCustomizeSpNavSettings.ui)
endif(FREECAD_USE_3DCONNEXION_LEGACY)

SOURCE_GROUP("Dialog\\Customize" FILES ${Dialog_Customize_SRCS})

# The settings dialog sources
SET(Dialog_Settings_CPP_SRCS
    Dialogs/DlgOnlineHelpImp.cpp
    Dialogs/DlgPreferencesImp.cpp
    PreferencePages/DlgSettings3DViewImp.cpp
    PreferencePages/DlgSettingsPDF.cpp
    PreferencePages/DlgSettingsCacheDirectory.cpp
    Dialogs/DlgSettingsColorGradientImp.cpp
    PreferencePages/DlgSettingsDocumentImp.cpp
    PreferencePages/DlgSettingsEditor.cpp
    PreferencePages/DlgSettingsGeneral.cpp
    Dialogs/DlgSettingsImageImp.cpp
    PreferencePages/DlgSettingsLightSources.cpp
    PreferencePages/DlgSettingsMacroImp.cpp
    PreferencePages/DlgSettingsNavigation.cpp
    PreferencePages/DlgSettingsNotificationArea.cpp
    PreferencePages/DlgSettingsPythonConsole.cpp
    PreferencePages/DlgSettingsReportView.cpp
    PreferencePages/DlgSettingsSelection.cpp
    PreferencePages/DlgSettingsUI.cpp
    PreferencePages/DlgSettingsViewColor.cpp
    PreferencePages/DlgSettingsWorkbenchesImp.cpp
    PreferencePages/DlgSettingsAdvanced.cpp
)
SET(Dialog_Settings_HPP_SRCS
    Dialogs/DlgOnlineHelpImp.h
    Dialogs/DlgPreferencesImp.h
    PreferencePages/DlgSettings3DViewImp.h
    PreferencePages/DlgSettingsPDF.h
    PreferencePages/DlgSettingsCacheDirectory.h
    Dialogs/DlgSettingsColorGradientImp.h
    PreferencePages/DlgSettingsDocumentImp.h
    PreferencePages/DlgSettingsEditor.h
    PreferencePages/DlgSettingsGeneral.h
    Dialogs/DlgSettingsImageImp.h
    PreferencePages/DlgSettingsLightSources.h
    PreferencePages/DlgSettingsMacroImp.h
    PreferencePages/DlgSettingsNavigation.h
    PreferencePages/DlgSettingsNotificationArea.h
    PreferencePages/DlgSettingsPythonConsole.h
    PreferencePages/DlgSettingsReportView.h
    PreferencePages/DlgSettingsSelection.h
    PreferencePages/DlgSettingsUI.h
    PreferencePages/DlgSettingsViewColor.h
    PreferencePages/DlgSettingsWorkbenchesImp.h
    PreferencePages/DlgSettingsAdvanced.h
)
SET(Dialog_Settings_SRCS
    ${Dialog_Settings_CPP_SRCS}
    ${Dialog_Settings_HPP_SRCS}
    Dialogs/DlgOnlineHelp.ui
    Dialogs/DlgPreferences.ui
    PreferencePages/DlgSettings3DView.ui
    PreferencePages/DlgSettingsCacheDirectory.ui
    Dialogs/DlgSettingsColorGradient.ui
    PreferencePages/DlgSettingsDocument.ui
    PreferencePages/DlgSettingsEditor.ui
    PreferencePages/DlgSettingsGeneral.ui
    Dialogs/DlgSettingsImage.ui
    PreferencePages/DlgSettingsLightSources.ui
    PreferencePages/DlgSettingsMacro.ui
    PreferencePages/DlgSettingsNavigation.ui
    PreferencePages/DlgSettingsNotificationArea.ui
    PreferencePages/DlgSettingsPythonConsole.ui
    PreferencePages/DlgSettingsReportView.ui
    PreferencePages/DlgSettingsSelection.ui
    PreferencePages/DlgSettingsUI.ui
    PreferencePages/DlgSettingsViewColor.ui
    PreferencePages/DlgSettingsWorkbenches.ui
    PreferencePages/DlgSettingsPDF.ui
)
SOURCE_GROUP("Dialog\\Settings" FILES ${Dialog_Settings_SRCS})

# The dock windows sources
SET(Dock_Windows_CPP_SRCS
    ComboView.cpp
    DockWindow.cpp
    PropertyView.cpp
    ReportView.cpp
    Selection/SelectionView.cpp
    ToolBox.cpp
    Tree.cpp
    TreeView.cpp
    DAGView/DAGView.cpp
    DAGView/DAGModel.cpp
    DAGView/DAGRectItem.cpp
    DAGView/DAGModelGraph.cpp
)
SET(Dock_Windows_HPP_SRCS
    ComboView.h
    DockWindow.h
    PropertyView.h
    ReportView.h
    Selection/SelectionView.h
    ToolBox.h
    Tree.h
    TreeView.h
    DAGView/DAGView.h
    DAGView/DAGModel.h
    DAGView/DAGRectItem.h
    DAGView/DAGModelGraph.h
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
    TaskView/TaskSolverMessages.cpp
    TaskView/TaskSolverMessages.h
    TaskView/TaskSolverMessages.ui
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
    QSint/actionpanel/taskgroup_p.cpp
    QSint/actionpanel/taskgroup_p.h
    QSint/actionpanel/taskheader_p.cpp
    QSint/actionpanel/taskheader_p.h
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

# The 3d view
SET(View3D_CPP_SRCS
    Camera.cpp
    Flag.cpp
    GLBuffer.cpp
    GLPainter.cpp
    Multisample.cpp
    MouseSelection.cpp
    SplitView3DInventor.cpp
    View.cpp
    View3DInventor.cpp
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
    Multisample.h
    MouseSelection.h
    SplitView3DInventor.h
    View.h
    View3DInventor.h
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

SET(Navigation_CPP_SRCS
    Navigation/NavigationStateChart.cpp
    Navigation/NavigationStyle.cpp
    Navigation/NavigationStylePyImp.cpp
    Navigation/InventorNavigationStyle.cpp
    Navigation/CADNavigationStyle.cpp
    Navigation/RevitNavigationStyle.cpp
    Navigation/BlenderNavigationStyle.cpp
    Navigation/SiemensNXNavigationStyle.cpp
    Navigation/SolidWorksNavigationStyle.cpp
    Navigation/MayaGestureNavigationStyle.cpp
    Navigation/OpenCascadeNavigationStyle.cpp
    Navigation/OpenSCADNavigationStyle.cpp
    Navigation/TinkerCADNavigationStyle.cpp
    Navigation/TouchpadNavigationStyle.cpp
    Navigation/GestureNavigationStyle.cpp
    Navigation/NavigationAnimator.cpp
    Navigation/NavigationAnimation.cpp
)
SET(Navigation_SRCS
    ${Navigation_CPP_SRCS}
    Navigation/NavigationStateChart.h
    Navigation/NavigationStyle.h
    Navigation/NavigationStyle.pyi
    Navigation/GestureNavigationStyle.h
    Navigation/NavigationAnimator.h
    Navigation/NavigationAnimation.h
    Navigation/SiemensNXNavigationStyle.h
)
SOURCE_GROUP("Navigation" FILES ${Navigation_SRCS})

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
    ViewProviderSuppressibleExtension.cpp
    ViewProviderAnnotation.cpp
    ViewProviderDocumentObject.cpp
    ViewProviderDocumentObjectGroup.cpp
    ViewProviderDocumentObjectPyImp.cpp
    ViewProviderGeometryObjectPyImp.cpp
    ViewProviderDragger.cpp
    ViewProviderExtern.cpp
    ViewProviderFeature.cpp
    ViewProviderGeometryObject.cpp
    ViewProviderImagePlane.cpp
    ViewProviderInventorObject.cpp
    ViewProviderPyImp.cpp
    ViewProviderFeaturePython.cpp
    ViewProviderVRMLObject.cpp
    ViewProviderBuilder.cpp
    ViewProviderPlacement.cpp
    ViewProviderDatum.cpp
    ViewProviderPlane.cpp
    ViewProviderPoint.cpp
    ViewProviderLine.cpp
    ViewProviderGeoFeatureGroup.cpp
    ViewProviderOriginGroup.cpp
    ViewProviderPart.cpp
    ViewProviderCoordinateSystem.cpp
    ViewProviderMaterialObject.cpp
    ViewProviderTextDocument.cpp
    ViewProviderTextureExtension.cpp
    ViewProviderLink.cpp
    LinkViewPyImp.cpp
    ViewProviderLinkPyImp.cpp
    ViewProviderVarSet.cpp
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
    ViewProviderFeaturePython.h
    ViewProviderVRMLObject.h
    ViewProviderBuilder.h
    ViewProviderPlacement.h
    ViewProviderDatum.h
    ViewProviderPlane.h
    ViewProviderPoint.h
    ViewProviderLine.h
    ViewProviderGeoFeatureGroup.h
    ViewProviderOriginGroup.h
    ViewProviderPart.h
    ViewProviderCoordinateSystem.h
    ViewProviderMaterialObject.h
    ViewProviderTextDocument.h
    ViewProviderTextureExtension.h
    ViewProviderLink.h
    ViewProviderVarSet.h
    AxisOrigin.h
)
SOURCE_GROUP("View3D\\Viewprovider" FILES ${Viewprovider_SRCS})

# The Inventor sources
SET(Inventor_CPP_SRCS
    Inventor/MarkerBitmaps.cpp
    Inventor/SmSwitchboard.cpp
    Inventor/So3DAnnotation.cpp
    Inventor/SoAutoZoomTranslation.cpp
    Inventor/SoAxisCrossKit.cpp
    Inventor/SoFCPlacementIndicatorKit.cpp
    Inventor/SoDrawingGrid.cpp
    Inventor/SoFCBackgroundGradient.cpp
    Inventor/SoFCBoundingBox.cpp
    Inventor/SoMouseWheelEvent.cpp
    Inventor/SoFCTransform.cpp
    Inventor/SoToggleSwitch.cpp
    Inventor/Draggers/SoTransformDragger.cpp
    Inventor/Draggers/SoLinearDragger.cpp
    Inventor/Draggers/SoLinearDraggerGeometry.cpp
    Inventor/Draggers/SoPlanarDragger.cpp
    Inventor/Draggers/SoRotationDragger.cpp
    Inventor/Draggers/SoRotationDraggerGeometry.cpp
    Inventor/Draggers/Gizmo.cpp
    SoFCColorBar.cpp
    SoFCColorBarNotifier.cpp
    SoFCColorGradient.cpp
    SoFCColorLegend.cpp
    SoFCDB.cpp
    SoFCInteractiveElement.cpp
    SoFCOffscreenRenderer.cpp
    SoQtOffscreenRendererPy.cpp
    Selection/SoFCSelection.cpp
    Selection/SoFCUnifiedSelection.cpp
    Selection/SoFCSelectionContext.cpp
    Selection/SoFCSelectionAction.cpp
    SoFCVectorizeSVGAction.cpp
    SoFCVectorizeU3DAction.cpp
    SoDevicePixelRatioElement.cpp
    SoTextLabel.cpp
    SoDatumLabel.cpp
    SoTouchEvents.cpp
    ArcEngine.cpp
)
SET(Inventor_SRCS
    ${Inventor_CPP_SRCS}
    Inventor/MarkerBitmaps.h
    Inventor/SmSwitchboard.h
    Inventor/SoAutoZoomTranslation.h
    Inventor/SoAxisCrossKit.h
    Inventor/SoFCPlacementIndicatorKit.h
    Inventor/SoDrawingGrid.h
    Inventor/SoFCBackgroundGradient.h
    Inventor/SoFCBoundingBox.h
    Inventor/SoMouseWheelEvent.h
    Inventor/SoFCTransform.h
    Inventor/SoToggleSwitch.h
    Inventor/Draggers/SoTransformDragger.h
    Inventor/Draggers/SoLinearDragger.h
    Inventor/Draggers/SoLinearDraggerGeometry.h
    Inventor/Draggers/SoPlanarDragger.h
    Inventor/Draggers/SoRotationDragger.h
    Inventor/Draggers/SoRotationDraggerGeometry.h
    Inventor/Draggers/Gizmo.h
    Inventor/Draggers/GizmoStyleParameters.h
    SoFCColorBar.h
    SoFCColorBarNotifier.h
    SoFCColorGradient.h
    SoFCColorLegend.h
    SoFCDB.h
    SoFCInteractiveElement.h
    SoFCOffscreenRenderer.h
    SoQtOffscreenRendererPy.h
    Selection/SoFCSelection.h
    Selection/SoFCUnifiedSelection.h
    Selection/SoFCSelectionContext.h
    Selection/SoFCSelectionAction.h
    SoFCVectorizeSVGAction.h
    SoFCVectorizeU3DAction.h
    SoDevicePixelRatioElement.h
    SoTextLabel.h
    SoDatumLabel.h
    SoTouchEvents.h
    ArcEngine.h
)
SOURCE_GROUP("View3D\\Inventor" FILES ${Inventor_SRCS})

# The widget sources
SET(Widget_CPP_SRCS
    ComboLinks.cpp
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
    SplashScreen.cpp
    PythonWrapper.cpp
    UiLoader.cpp
    WidgetFactory.cpp
    Widgets.cpp
    Window.cpp
    WorkbenchSelector.cpp
    ElideLabel.cpp
    ElideCheckBox.cpp
    FontScaledSVG.cpp
    SplitButton.cpp
)
SET(Widget_HPP_SRCS
    ComboLinks.h
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
    SplashScreen.h
    PythonWrapper.h
    UiLoader.h
    WidgetFactory.h
    Widgets.h
    Window.h
    WorkbenchSelector.h
    ElideLabel.h
    ElideCheckBox.h
    FontScaledSVG.h
    SplitButton.h
)
SET(Widget_SRCS
    ${Widget_CPP_SRCS}
    ${Widget_HPP_SRCS}
)
SOURCE_GROUP("Widget" FILES ${Widget_SRCS})

SET(Params_CPP_SRCS
    TreeParams.cpp
    OverlayParams.cpp
    ParamHandler.cpp
    ViewParams.cpp
)

SET(Params_HPP_SRCS
    TreeParams.h
    OverlayParams.h
    ParamHandler.cpp
    ViewParams.h
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
    OverlayManager.cpp
    OverlayWidgets.cpp
    MenuManager.cpp
    PythonWorkbenchPyImp.cpp
    ToolBarAreaWidget.cpp
    ToolBarManager.cpp
    ToolBoxManager.cpp
    Workbench.cpp
    WorkbenchFactory.cpp
    WorkbenchManager.cpp
    WorkbenchManipulator.cpp
    WorkbenchManipulatorPython.cpp
    WorkbenchPyImp.cpp
)
SET(Workbench_SRCS
    ${Workbench_CPP_SRCS}
    DockWindowManager.h
    OverlayManager.h
    OverlayWidgets.h
    MenuManager.h
    ToolBarAreaWidget.h
    ToolBarManager.h
    ToolBoxManager.h
    Workbench.h
    WorkbenchFactory.h
    WorkbenchManager.h
    WorkbenchManipulator.h
    WorkbenchManipulatorPython.h
)
SOURCE_GROUP("Workbench" FILES ${Workbench_SRCS})

SET(Selection_SRCS
    Selection/SelectionObjectPyImp.cpp
    Selection/SelectionObject.h
    Selection/SelectionObject.cpp
    Selection/Selection.h
    Selection/Selection.cpp
    Selection/SelectionFilter.h
    Selection/SelectionFilter.cpp
    Selection/SelectionFilterPy.h
    Selection/SelectionFilterPy.cpp
    Selection/SelectionFilter.y
    Selection/SelectionFilter.l
    Selection/SelectionObserverPython.cpp
    Selection/SelectionObserverPython.h
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
    FreeCADStyle.cpp
    GuiApplication.cpp
    GuiApplicationNativeEventAware.cpp
    GuiConsole.cpp
    InputHintWidget.cpp
    InputHintPy.cpp
    Macro.cpp
    MergeDocuments.cpp
    ModuleIO.cpp
    Namespace.h
    resource.cpp
    Control.cpp
    SpaceballEvent.cpp
    StatusBarLabel.cpp
    PreferencePackManager.cpp
    Thumbnail.cpp
    Utilities.cpp
    WaitCursor.cpp
    ManualAlignment.cpp
    StartupProcess.cpp
    TransactionObject.cpp
    ToolHandler.cpp
    StyleParameters/Value.cpp
    StyleParameters/Parser.cpp
    StyleParameters/ParameterManager.cpp
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
    FreeCADStyle.h
    GraphicsViewZoom.h
    GuiApplication.h
    GuiApplicationNativeEventAware.h
    3Dconnexion/GuiAbstractNativeEvent.h
    GuiConsole.h
    InventorAll.h
    InputHint.h
    InputHintPy.h
    InputHintWidget.h
    Macro.h
    MergeDocuments.h
    MetaTypes.h
    ModuleIO.h
    Notifications.h
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
    StartupProcess.h
    StatusBarLabel.h
    TransactionObject.h
    ToolHandler.h
    StyleParameters/Value.h
    StyleParameters/Parser.h
    StyleParameters/ParameterManager.h
)

SET(FreeCADGui_SRCS
    ${FreeCADGui_connexion_SRCS}
    ${FreeCADGui_connexion_HDRS}
    ${FreeCADGui_CPP_SRCS}
    ${FreeCADGui_Pyi_SRCS}
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
    ${Inventor_CPP_SRCS}
    ${Language_SRCS}
    ${Propertyeditor_SRCS}
    ${Task_View_SRCS}
    ${qsintActionPanel_SRCS}
    ${Resource_SRCS}
    ${Quarter_SRCS}
    ${View3D_SRCS}
    ${Navigation_SRCS}
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
        TaskView/TaskSolverMessages.ui
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

if(FREECAD_USE_PCH)
    target_precompile_headers(FreeCADGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )

    set_source_files_properties(
            ${qsintActionPanel_SRCS}
            ${Quarter_SRCS}
            PROPERTIES SKIP_PRECOMPILE_HEADERS TRUE)
endif(FREECAD_USE_PCH)

if (FREECAD_USE_QT_DIALOGS)
    set_source_files_properties(FileDialog.cpp PROPERTIES COMPILE_FLAGS -DUSE_QT_DIALOGS)
endif()

# Suppress some very long Eigen3 warnings of older versions and
# suppress this warning for the SbMatrix class
if (EIGEN3_NO_DEPRECATED_COPY)
    set_source_files_properties(
        NaviCube.cpp
        Inventor/SoAutoZoomTranslation.cpp
        Inventor/Draggers/SoTransformDragger.cpp
        Inventor/Draggers/SoLinearDragger.cpp
        Inventor/Draggers/SoLinearDraggerGeometry.cpp
        Inventor/Draggers/SoPlanarDragger.cpp
        Inventor/Draggers/SoRotationDragger.cpp
        Inventor/Draggers/SoRotationDraggerGeometry.cpp
        Inventor/Draggers/Gizmo.cpp
        SoFCOffscreenRenderer.cpp
        Selection/SoFCSelectionAction.cpp
        Quarter/QuarterWidget.cpp
        View3DInventorViewer.cpp
        PROPERTIES COMPILE_FLAGS ${EIGEN3_NO_DEPRECATED_COPY})
endif ()

target_sources(FreeCADGui PRIVATE ${FreeCADGui_SRCS})
target_link_libraries(FreeCADGui PUBLIC ${FreeCADGui_LIBS})

if (MSVC)
    target_compile_definitions(FreeCADGui PRIVATE WIN32_LEAN_AND_MEAN)
    target_compile_options(FreeCADGui PRIVATE /wd4251 /wd4273 /wd4275)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(FreeCADGui)
endif()

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
    INSTALL(FILES Icons/freecad-icon-16.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps RENAME org.freecad.FreeCAD.png)
    INSTALL(FILES Icons/freecad-icon-32.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps RENAME org.freecad.FreeCAD.png)
    INSTALL(FILES Icons/freecad-icon-48.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps RENAME org.freecad.FreeCAD.png)
    INSTALL(FILES Icons/freecad-icon-64.png DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps RENAME org.freecad.FreeCAD.png)
    INSTALL(FILES Icons/freecad.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps RENAME org.freecad.FreeCAD.svg)
    INSTALL(FILES Icons/freecad.svg DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pixmaps)
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
