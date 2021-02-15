if(MSVC)
    add_definitions(-DHAVE_ATANH -DHAVE_ASINH -DHAVE_ACOSH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)

if(BUILD_QT5)
    include_directories(
        ${Qt5XmlPatterns_INCLUDE_DIRS}
    )
else(BUILD_QT5)
    include_directories(
        ${QT_QTXMLPATTERNS_INCLUDE_DIR}
    )
endif(BUILD_QT5)

link_directories(${OCC_LIBRARY_DIR})

set(TechDrawGui_LIBS
    TechDraw
    FreeCADGui
)

set(TechDrawGui_MOC_HDRS
    DlgPrefsTechDrawAdvancedImp.h
    DlgPrefsTechDrawAnnotationImp.h
    DlgPrefsTechDrawColorsImp.h
    DlgPrefsTechDrawDimensionsImp.h
    DlgPrefsTechDrawGeneralImp.h
    DlgPrefsTechDrawHLRImp.h
    DlgPrefsTechDrawScaleImp.h
    DlgTemplateField.h
    MDIViewPage.h
    mrichtextedit.h
    mtextedit.h
    QGEPath.h
    QGIDrawingTemplate.h
    QGIGhostHighlight.h
    QGILeaderLine.h
    QGIRichAnno.h
    QGITemplate.h
    QGISVGTemplate.h
    QGIView.h
    QGIViewDimension.h
    QGIViewBalloon.h
    QGIWeldSymbol.h
    QGMText.h
    QGTracker.h
    QGVPage.h
    TaskActiveView.h
    TaskBalloon.h
    TaskCenterLine.h
    TaskCosmeticLine.h
    TaskCosVertex.h
    TaskDetail.h
    TaskDimension.h
    TaskGeomHatch.h
    TaskHatch.h
    TaskLeaderLine.h
    TaskLinkDim.h
    TaskLineDecor.h
    TaskProjGroup.h
    TaskRichAnno.h
    TaskSectionView.h
    TaskWeldingSymbol.h
    SymbolChooser.h   
)

fc_wrap_cpp(TechDrawGui_MOC_SRCS ${TechDrawGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${TechDrawGui_MOC_SRCS})

if(BUILD_QT5)
    qt5_add_resources(TechDrawGui_SRCS Resources/TechDraw.qrc)
else()
    qt4_add_resources(TechDrawGui_SRCS Resources/TechDraw.qrc)
endif()

set(TechDrawGui_UIC_SRCS
    DlgPrefsTechDrawAdvanced.ui
    DlgPrefsTechDrawAnnotation.ui
    DlgPrefsTechDrawColors.ui
    DlgPrefsTechDrawDimensions.ui
    DlgPrefsTechDrawGeneral.ui
    DlgPrefsTechDrawHLR.ui
    DlgPrefsTechDrawScale.ui
    DlgTemplateField.ui
    mrichtextedit.ui
    TaskActiveView.ui
    TaskBalloon.ui
    TaskCosmeticLine.ui
    TaskCosVertex.ui
    TaskCenterLine.ui
    TaskDetail.ui
    TaskDimension.ui
    TaskGeomHatch.ui
    TaskHatch.ui
    TaskLeaderLine.ui
    TaskLineDecor.ui
    TaskLinkDim.ui
    TaskProjGroup.ui
    TaskRestoreLines.ui
    TaskRichAnno.ui
    TaskSectionView.ui
    TaskWeldingSymbol.ui
    SymbolChooser.ui
)

if(BUILD_QT5)
    qt5_wrap_ui(TechDrawGui_UIC_HDRS ${TechDrawGui_UIC_SRCS})
else()
    qt4_wrap_ui(TechDrawGui_UIC_HDRS ${TechDrawGui_UIC_SRCS})
endif()

SET(MRTE_SRCS 
    mrichtextedit.ui
    mrichtextedit.cpp
    mrichtextedit.h
    mtextedit.cpp
    mtextedit.h
)

SET(MRTE_HDRS 
    mtextedit.h
)


SET(TechDrawGui_SRCS
    ${CMAKE_SOURCE_DIR}/src/Mod/TechDraw/InitGui.py
    ${TechDrawGui_SRCS}
    ${MRTE_SRCS}
    AppTechDrawGui.cpp
    AppTechDrawGuiPy.cpp
    Command.cpp
    CommandCreateDims.cpp
    CommandDecorate.cpp
    CommandAnnotate.cpp
    Resources/TechDraw.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
    TaskProjGroup.ui
    TaskProjGroup.cpp
    TaskProjGroup.h
    DlgPrefsTechDrawGeneral.ui
    DlgPrefsTechDrawGeneralImp.cpp
    DlgPrefsTechDrawGeneralImp.h
    DlgPrefsTechDrawScale.ui
    DlgPrefsTechDrawScaleImp.cpp
    DlgPrefsTechDrawScaleImp.h
    DlgPrefsTechDrawAnnotation.ui
    DlgPrefsTechDrawAnnotationImp.cpp
    DlgPrefsTechDrawAnnotationImp.h
    DlgPrefsTechDrawDimensions.ui
    DlgPrefsTechDrawDimensionsImp.cpp
    DlgPrefsTechDrawDimensionsImp.h
    DlgPrefsTechDrawColors.ui
    DlgPrefsTechDrawColorsImp.cpp
    DlgPrefsTechDrawColorsImp.h
    DlgPrefsTechDrawAdvanced.ui
    DlgPrefsTechDrawAdvancedImp.cpp
    DlgPrefsTechDrawAdvancedImp.h
    DlgPrefsTechDrawHLR.ui
    DlgPrefsTechDrawHLRImp.cpp
    DlgPrefsTechDrawHLRImp.h
    TaskLinkDim.ui
    TaskLinkDim.cpp
    TaskLinkDim.h
    DlgTemplateField.ui
    DlgTemplateField.cpp
    DlgTemplateField.h
    TaskSectionView.ui
    TaskSectionView.cpp
    TaskSectionView.h
    TaskBalloon.ui
    TaskBalloon.cpp
    TaskBalloon.h
    TaskDimension.ui
    TaskDimension.cpp
    TaskDimension.h
    TaskGeomHatch.ui
    TaskGeomHatch.cpp
    TaskGeomHatch.h
    TaskHatch.ui
    TaskHatch.cpp
    TaskHatch.h
    TaskLeaderLine.ui
    TaskLeaderLine.cpp
    TaskLeaderLine.h
    TaskRichAnno.ui
    TaskRichAnno.cpp
    TaskRichAnno.h
    TaskCosVertex.ui
    TaskCosVertex.cpp
    TaskCosVertex.h
    TaskCenterLine.ui
    TaskCenterLine.cpp
    TaskCenterLine.h
    TaskLineDecor.ui
    TaskLineDecor.cpp
    TaskLineDecor.h
    TaskRestoreLines.ui
    TaskWeldingSymbol.ui
    TaskWeldingSymbol.cpp
    TaskWeldingSymbol.h
    DrawGuiUtil.cpp
    DrawGuiUtil.h
    Rez.cpp
    Rez.h
    SymbolChooser.ui
    SymbolChooser.cpp
    SymbolChooser.h
    TaskActiveView.ui
    TaskActiveView.cpp
    TaskActiveView.h
    Grabber3d.cpp
    Grabber3d.h
    TaskDetail.ui
    TaskDetail.cpp
    TaskDetail.h
    PreferencesGui.cpp
    PreferencesGui.h
    TaskCosmeticLine.ui
    TaskCosmeticLine.cpp
    TaskCosmeticLine.h
)

SET(TechDrawGuiView_SRCS
    MDIViewPage.cpp
    MDIViewPage.h
    QGVPage.cpp
    QGVPage.h
    QGCustomText.cpp
    QGCustomText.h
    QGICaption.cpp
    QGICaption.h
    QGCustomRect.cpp
    QGCustomRect.h
    QGCustomSvg.cpp
    QGCustomSvg.h
    QGCustomClip.cpp
    QGCustomClip.h
    QGCustomLabel.cpp
    QGCustomLabel.h
    QGCustomBorder.cpp
    QGCustomBorder.h
    QGCustomImage.cpp
    QGCustomImage.h
    QGDisplayArea.cpp
    QGDisplayArea.h
    QGIView.cpp
    QGIView.h
    QGIArrow.cpp
    QGIArrow.h
    QGIEdge.cpp
    QGIEdge.h
    QGIFace.cpp
    QGIFace.h
    QGISVGTemplate.cpp
    QGISVGTemplate.h
    QGIVertex.cpp
    QGIVertex.h
    QGIMatting.cpp
    QGIMatting.h
    QGIDrawingTemplate.cpp
    QGIDrawingTemplate.h
    QGITemplate.cpp
    QGITemplate.h
    QGIViewCollection.cpp
    QGIViewCollection.h
    QGIViewDimension.cpp
    QGIViewDimension.h
    QGIViewBalloon.cpp
    QGIViewBalloon.h
    QGIProjGroup.cpp
    QGIProjGroup.h
    QGIViewPart.cpp
    QGIViewPart.h
    QGIViewSection.cpp
    QGIViewSection.h
    QGIViewAnnotation.cpp
    QGIViewAnnotation.h
    QGIViewSymbol.cpp
    QGIViewSymbol.h
    QGIViewSpreadsheet.cpp
    QGIViewSpreadsheet.h
    QGIViewImage.cpp
    QGIViewImage.h
    QGIViewClip.cpp
    QGIViewClip.h
    QGIPrimPath.cpp
    QGIPrimPath.h
    QGICMark.cpp
    QGICMark.h
    QGIDimLines.cpp
    QGIDimLines.h
    QGISectionLine.cpp
    QGISectionLine.h
    QGIDecoration.cpp
    QGIDecoration.h
    QGICenterLine.cpp
    QGICenterLine.h
    QGIHighlight.cpp
    QGIHighlight.h
    QGEPath.cpp
    QGEPath.h
    QGTracker.cpp
    QGTracker.h
    QGILeaderLine.cpp
    QGILeaderLine.h
    QGIRichAnno.cpp
    QGIRichAnno.h
    QGMText.h
    QGMText.cpp
    QGIWeldSymbol.h
    QGIWeldSymbol.cpp
    QGITile.h
    QGITile.cpp
    TemplateTextField.cpp
    TemplateTextField.h
    ZVALUE.h
    QGIGhostHighlight.cpp
    QGIGhostHighlight.h
)
SET(TechDrawGuiViewProvider_SRCS
    ViewProviderPage.cpp
    ViewProviderPage.h
    ViewProviderDrawingView.cpp
    ViewProviderDrawingView.h
    ViewProviderProjGroupItem.cpp
    ViewProviderProjGroupItem.h
    ViewProviderTemplate.cpp
    ViewProviderTemplate.h
    ViewProviderDimension.cpp
    ViewProviderDimension.h
    ViewProviderBalloon.cpp
    ViewProviderBalloon.h
    ViewProviderViewPart.cpp
    ViewProviderViewPart.h
    ViewProviderProjGroup.cpp
    ViewProviderProjGroup.h
    ViewProviderViewSection.cpp
    ViewProviderViewSection.h
    ViewProviderAnnotation.cpp
    ViewProviderAnnotation.h
    ViewProviderSymbol.cpp
    ViewProviderSymbol.h
    ViewProviderSpreadsheet.cpp
    ViewProviderSpreadsheet.h
    ViewProviderViewClip.cpp
    ViewProviderViewClip.h
    ViewProviderGeomHatch.cpp
    ViewProviderGeomHatch.h
    ViewProviderHatch.cpp
    ViewProviderHatch.h
    ViewProviderImage.cpp
    ViewProviderImage.h
    ViewProviderLeader.cpp
    ViewProviderLeader.h
    ViewProviderRichAnno.cpp
    ViewProviderRichAnno.h
    ViewProviderTile.cpp
    ViewProviderTile.h
    ViewProviderWeld.cpp
    ViewProviderWeld.h
    ViewProviderCosmeticExtension.cpp
    ViewProviderCosmeticExtension.h 
)

SOURCE_GROUP("MRTE" FILES ${MRTE_SRCS})
SOURCE_GROUP("Mod" FILES ${TechDrawGui_SRCS})
SOURCE_GROUP("SVG-View" FILES ${TechDrawGuiView_SRCS})
SOURCE_GROUP("ViewProvider" FILES ${TechDrawGuiViewProvider_SRCS})

SET(TechDrawGuiTaskDlgs_SRCS
    TaskProjGroup.ui
    TaskLinkDim.ui
    TaskSectionView.ui
    TaskGeomHatch.ui
    TaskHatch.ui
    TaskLeaderLine.ui
    TaskRichAnno.ui
    TaskCosVertex.ui
    mrichtextedit.ui
    TaskBalloon.ui
    TaskCenterLine.ui
    TaskDimension.ui
    TaskLineDecor.ui
    TaskRestoreLines.ui
    TaskWeldingSymbol.ui
    SymbolChooser.ui
    TaskActiveView.ui
    TaskDetail.ui
    TaskCosmeticLine.ui
)
SOURCE_GROUP("TaskDialogs" FILES ${TechDrawGuiTaskDlgs_SRCS})

SET(TechDrawGuiIcon_SVG
    Resources/icons/preferences-techdraw.svg
)

##install OSIFONT
SET(TechDrawGuiFonts
    Resources/fonts/osifont-lgpl3fe.ttf
    Resources/fonts/osifont.license
)

add_library(TechDrawGui SHARED ${TechDrawGui_SRCS} ${TechDrawGuiView_SRCS} ${TechDrawGuiViewProvider_SRCS}
                               ${TechDrawGuiIcon_SVG} ${TechDrawGuiFonts} ${MRTE_SRCS})
target_link_libraries(TechDrawGui ${TechDrawGui_LIBS})

fc_copy_sources(TechDrawGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw/" ${TechDrawGuiIcon_SVG})
INSTALL(FILES ${TechDrawGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/TechDraw/Resources/icons")

fc_copy_sources(TechDrawGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/TechDraw" ${TechDrawGuiFonts})
INSTALL(FILES ${TechDrawGuiFonts} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/TechDraw/Resources/fonts")

fc_copy_sources(TechDrawGui "${CMAKE_BINARY_DIR}/src/Mod/TechDraw/Gui" ${MRTE_HDRS})

SET_BIN_DIR(TechDrawGui TechDrawGui /Mod/TechDraw)
SET_PYTHON_PREFIX_SUFFIX(TechDrawGui)

INSTALL(TARGETS TechDrawGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
