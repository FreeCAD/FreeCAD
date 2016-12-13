include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(InspectionGui_LIBS
    Inspection
    FreeCADGui
)

set(InspectionGui_MOC_HDRS
    VisualInspection.h
)
fc_wrap_cpp(InspectionGui_MOC_SRCS ${InspectionGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${InspectionGui_MOC_SRCS})

set(Dialogs_UIC_SRCS
    VisualInspection.ui
)

if(BUILD_QT5)
    qt5_add_resources(Inspection_QRC_SRCS Resources/Inspection.qrc)
    qt5_wrap_ui(Dialogs_UIC_HDRS ${Dialogs_UIC_SRCS})
else()
    qt4_add_resources(Inspection_QRC_SRCS Resources/Inspection.qrc)
    qt4_wrap_ui(Dialogs_UIC_HDRS ${Dialogs_UIC_SRCS})
endif()

SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    VisualInspection.cpp
    VisualInspection.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

SET(InspectionGui_SRCS
    ${Inspection_QRC_SRCS}
    ${Dialogs_SRCS}
    AppInspectionGui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    ViewProviderInspection.cpp
    ViewProviderInspection.h
    Workbench.cpp
    Workbench.h
)

add_library(InspectionGui SHARED ${InspectionGui_SRCS})
target_link_libraries(InspectionGui ${InspectionGui_LIBS})


fc_target_copy_resource(InspectionGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Inspection
    ${CMAKE_BINARY_DIR}/Mod/Inspection
    InitGui.py)

SET_BIN_DIR(InspectionGui InspectionGui /Mod/Inspection)
SET_PYTHON_PREFIX_SUFFIX(InspectionGui)

INSTALL(TARGETS InspectionGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

SET(InspectionGuiIcon_SVG
    Resources/icons/InspectionWorkbench.svg
)

fc_copy_sources(InspectionGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Inspection" ${InspectionGuiIcon_SVG})

INSTALL(FILES ${InspectionGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Inspection/Resources/icons")
