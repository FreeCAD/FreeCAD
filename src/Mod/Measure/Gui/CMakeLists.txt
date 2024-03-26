include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${OCC_INCLUDE_DIR}
)

set(MeasureGui_LIBS
    Measure
    Part
    FreeCADGui
)

generate_from_xml(QuickMeasurePy)

SET(Python_SRCS
    QuickMeasurePy.xml
    QuickMeasurePyImp.cpp
)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(MeasureGui_SRCS_Module
    AppMeasureGui.cpp
    AppMeasureGuiPy.cpp
    PreCompiled.cpp
    PreCompiled.h
    QuickMeasure.cpp
    QuickMeasure.h
    ${Measure_QRC_SRCS}
)

SOURCE_GROUP("Module" FILES ${MeasureGui_SRCS_Module})


SET(MeasureGui_SRCS
    ${MeasureGui_UIC_HDRS}
    ${MeasureGui_SRCS_Module}
    ${Python_SRCS}
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${MeasureGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(PathGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(MeasureGui SHARED ${MeasureGui_SRCS})
target_link_libraries(MeasureGui ${MeasureGui_LIBS})

SET_BIN_DIR(MeasureGui MeasureGui /Mod/Measure)
SET_PYTHON_PREFIX_SUFFIX(MeasureGui)


INSTALL(TARGETS MeasureGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${Measure_QRC_SRCS} DESTINATION Mod/Measure)