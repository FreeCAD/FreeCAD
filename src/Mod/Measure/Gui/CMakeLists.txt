include_directories (
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR})
link_directories (${OCC_LIBRARY_DIR})

link_directories (${OCC_LIBRARY_DIR})

set (
    MeasureGui_LIBS
    Measure
    # Part
    FreeCADGui)

if (MSVC)
    include_directories (${CMAKE_SOURCE_DIR}/src/3rdParty/OpenGL/api)
endif (MSVC)

set (Measure_TR_QRC ${CMAKE_CURRENT_BINARY_DIR}/Resources/Measure_translation.qrc)
qt_find_and_add_translation (QM_SRCS "Resources/translations/*_*.ts"
                             ${CMAKE_CURRENT_BINARY_DIR}/Resources/translations)
qt_create_resource_file (${Measure_TR_QRC} ${QM_SRCS})
qt_add_resources (MeasureGui_SRCS Resources/Measure.qrc ${Measure_TR_QRC})

set (MeasureGui_UIC_SRCS DlgPrefsMeasureAppearanceImp.ui)

generate_from_xml (QuickMeasurePy)

set (
    MeasureGui_SRCS
    ${CMAKE_SOURCE_DIR}/src/Mod/Measure/InitGui.py
    ${MeasureGui_SRCS}
    AppMeasureGui.cpp
    Command.cpp
    Resources/Measure.qrc
    PreCompiled.cpp
    PreCompiled.h
    QuickMeasurePy.xml
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
    WorkbenchManipulator.cpp
    WorkbenchManipulator.h
    DlgPrefsMeasureAppearanceImp.ui
    DlgPrefsMeasureAppearanceImp.cpp
    DlgPrefsMeasureAppearanceImp.h)

set (MeasureGuiTaskDlgs_SRCS DlgPrefsMeasureAppearanceImp.ui)

# set(MeasureGui_Scripts ../InitGui.py )

if (FREECAD_USE_PCH)
    add_definitions (-D_PreComp_)
    get_msvc_precompiled_source ("PreCompiled.cpp" PCH_SRCS ${MeasureGui_SRCS})
    add_msvc_precompiled_header (MeasureGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif (FREECAD_USE_PCH)

set (MeasureGuiIcon_SVG Resources/icons/preferences-measure.svg)

add_library (MeasureGui SHARED ${MeasureGui_SRCS} ${MeasureGuiIcon_SVG})
target_link_libraries (MeasureGui ${MeasureGui_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error (MeasureGui)
endif ()

set_bin_dir (MeasureGui MeasureGui /Mod/Measure)
set_python_prefix_suffix (MeasureGui)

fc_copy_sources (MeasureGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Measure/"
                 ${MeasureGuiIcon_SVG})

install (TARGETS MeasureGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
install (FILES ${MeasureGuiIcon_SVG}
         DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Measure/Resources/icons")
