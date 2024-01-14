include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

set(MeasureGui_LIBS
    Measure
    FreeCADGui
)

set (Measure_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Measure_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Measure_TR_QRC} ${QM_SRCS})
qt_add_resources(MeasureGui_SRCS Resources/Measure.qrc ${Measure_TR_QRC})

SET(MeasureGui_UIC_SRCS
    DlgPrefsMeasureAppearanceImp.ui
)

SET(MeasureGui_SRCS
    ${CMAKE_SOURCE_DIR}/src/Mod/Measure/InitGui.py
    ${MeasureGui_SRCS}
    AppMeasureGui.cpp
    Command.cpp
    Resources/Measure.qrc
    PreCompiled.cpp
    PreCompiled.h
    ViewProviderMeasureBase.cpp
    ViewProviderMeasureBase.h
    ViewProviderMeasureAngle.cpp
    ViewProviderMeasureAngle.h
    ViewProviderMeasureDistance.cpp
    ViewProviderMeasureDistance.h
    # Workbench.cpp
    # Workbench.h
    DlgPrefsMeasureAppearanceImp.ui
    DlgPrefsMeasureAppearanceImp.cpp
    DlgPrefsMeasureAppearanceImp.h
    
)

SET(MeasureGuiTaskDlgs_SRCS
    DlgPrefsMeasureAppearanceImp.ui
)

SET(MeasureGuiIcon_SVG
    Resources/icons/preferences-measure.svg
)

#set(MeasureGui_Scripts
#    ../InitGui.py
#)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${MeasureGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(MeasureGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(MeasureGui SHARED ${MeasureGui_SRCS} ${MeasureGuiIcon_SVG})
target_link_libraries(MeasureGui ${MeasureGui_LIBS})

fc_copy_sources(MeasureGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Measure/" ${MeasureGuiIcon_SVG})
INSTALL(FILES ${MeasureGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Measure/Resources/icons")

SET_BIN_DIR(MeasureGui MeasureGui /Mod/Measure)
SET_PYTHON_PREFIX_SUFFIX(MeasureGui)


INSTALL(TARGETS MeasureGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
