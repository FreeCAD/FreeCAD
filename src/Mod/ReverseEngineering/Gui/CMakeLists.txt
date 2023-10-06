include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(ReenGui_LIBS
    ReverseEngineering
    MeshGui
)

set (ReverseEngineering_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/ReverseEngineering_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${ReverseEngineering_TR_QRC} ${QM_SRCS})
qt_add_resources(ReenGui_QRC_SRCS Resources/ReverseEngineering.qrc ${ReverseEngineering_TR_QRC})

set(Dialogs_UIC_SRCS
    FitBSplineSurface.ui
    Poisson.ui
    Segmentation.ui
    SegmentationManual.ui
)

SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    ${Dialogs_UIC_SRCS}
    FitBSplineSurface.cpp
    FitBSplineSurface.h
    Poisson.cpp
    Poisson.h
    Segmentation.cpp
    Segmentation.h
    SegmentationManual.cpp
    SegmentationManual.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

SET(ReenGui_SRCS
    ${ReenGui_QRC_SRCS}
    ${Dialogs_SRCS}
    AppReverseEngineeringGui.cpp
    Command.cpp
    Resources/ReverseEngineering.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

SET(ReverseEngineeringGuiIcon_SVG
    Resources/icons/ReverseEngineeringWorkbench.svg
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${ReenGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(ReverseEngineeringGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(ReverseEngineeringGui SHARED ${ReenGui_SRCS} ${ReverseEngineeringGuiIcon_SVG})
target_link_libraries(ReverseEngineeringGui ${ReenGui_LIBS})

SET_BIN_DIR(ReverseEngineeringGui ReverseEngineeringGui /Mod/ReverseEngineering)
SET_PYTHON_PREFIX_SUFFIX(ReverseEngineeringGui)

fc_copy_sources(ReverseEngineeringGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/ReverseEngineering" ${ReverseEngineeringGuiIcon_SVG})

install(TARGETS ReverseEngineeringGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${ReverseEngineeringGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/ReverseEngineering/Resources/icons")
