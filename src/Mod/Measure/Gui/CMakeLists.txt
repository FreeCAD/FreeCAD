# SPDX-License-Identifier: LGPL-2.1-or-later

set(MeasureGui_LIBS
    Measure
    #Part
    FreeCADGui
)

if(MSVC)
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/3rdParty/OpenGL/api
    )
endif(MSVC)


set (Measure_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Measure_translation.qrc)
qt_find_and_add_translation(QM_SRCS "Resources/translations/*_*.ts"
    ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file(${Measure_TR_QRC} ${QM_SRCS})
qt_add_resources(MeasureGui_SRCS Resources/Measure.qrc ${Measure_TR_QRC})

SET(MeasureGui_UIC_SRCS
    DlgPrefsMeasureAppearanceImp.ui
)

generate_from_py(QuickMeasure)

SET(MeasureGui_SRCS
    ${CMAKE_SOURCE_DIR}/src/Mod/Measure/InitGui.py
    ${MeasureGui_SRCS}
    AppMeasureGui.cpp
    Command.cpp
    Resources/Measure.qrc
    PreCompiled.h
    QuickMeasure.pyi
    QuickMeasurePyImp.cpp
    QuickMeasure.cpp
    QuickMeasure.h
    TaskMeasure.cpp
    TaskMeasure.h
    ViewProviderMeasureBase.cpp
    ViewProviderMeasureBase.h
    ViewProviderMeasureAngle.cpp
    ViewProviderMeasureAngle.h
    ViewProviderMeasureDistance.cpp
    ViewProviderMeasureDistance.h
    SoScreenSpaceScale.cpp
    SoScreenSpaceScale.h
    DlgPrefsMeasureAppearanceImp.ui
    DlgPrefsMeasureAppearanceImp.cpp
    DlgPrefsMeasureAppearanceImp.h

)

SET(MeasureGuiTaskDlgs_SRCS
    DlgPrefsMeasureAppearanceImp.ui
)


#set(MeasureGui_Scripts
#    ../InitGui.py
#)

SET(MeasureGuiIcon_SVG
    Resources/icons/preferences-measure.svg
)

add_library(MeasureGui SHARED ${MeasureGui_SRCS} ${MeasureGuiIcon_SVG})

if(FREECAD_USE_PCH)
    target_precompile_headers(MeasureGui PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)

target_include_directories(
    MeasureGui
    PRIVATE
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_link_libraries(MeasureGui ${MeasureGui_LIBS})

if (MSVC)
    target_compile_options(MeasureGui PRIVATE /wd4251 /wd4273)
endif()

if (FREECAD_WARN_ERROR)
    target_compile_warn_error(MeasureGui)
endif()

SET_BIN_DIR(MeasureGui MeasureGui /Mod/Measure)
SET_PYTHON_PREFIX_SUFFIX(MeasureGui)

fc_copy_sources(MeasureGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Measure/" ${MeasureGuiIcon_SVG})

INSTALL(TARGETS MeasureGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${MeasureGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Measure/Resources/icons")
