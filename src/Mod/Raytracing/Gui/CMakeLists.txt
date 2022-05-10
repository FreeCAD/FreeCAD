if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(RaytracingGui_LIBS
    Raytracing
    FreeCADGui
)

macro(generate_from_py2 BASE_NAME OUTPUT_FILE)
    file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/src/Tools/PythonToCPP.py TOOL_PATH)
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME} SOURCE_PATH)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}
        COMMAND ${PYTHON_EXECUTABLE} ${TOOL_PATH} ${SOURCE_PATH} ${OUTPUT_FILE}
        MAIN_DEPENDENCY ${BASE_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT Building files out of ${BASE_NAME}.py)
endmacro(generate_from_py2)

generate_from_py2(FreeCADpov FreeCADpov.h)

SET(Resource_SRCS
    ${Resource_SRCS}
    Resources/Raytracing.qrc
)
SOURCE_GROUP("Resource" FILES ${Resource_SRCS})

set(RaytracingGui_UIC_SRCS
    DlgSettingsRay.ui
)

if (BUILD_QT5)
    qt5_add_resources(Resource_SRCS Resources/Raytracing.qrc)
    qt5_wrap_ui(RaytracingGui_UIC_HDRS ${RaytracingGui_UIC_SRCS})
else()
    qt4_add_resources(Resource_SRCS Resources/Raytracing.qrc)
    qt4_wrap_ui(RaytracingGui_UIC_HDRS ${RaytracingGui_UIC_SRCS})
endif()

SET(RaytracingGui_SRCS
    ${Resource_SRCS}
    ${RaytracingGui_UIC_HDRS}
    AppRaytracingGui.cpp
    AppRaytracingGuiPy.cpp
    Command.cpp
    DlgSettingsRay.ui
    DlgSettingsRayImp.cpp
    DlgSettingsRayImp.h
    FreeCADpov
    PovrayHighlighter.cpp
    PovrayHighlighter.h
    PreCompiled.cpp
    PreCompiled.h
    ViewProvider.cpp
    ViewProvider.h
    Workbench.cpp
    Workbench.h
)

SET(RaytracingGuiIcon_SVG
    Resources/icons/RaytracingWorkbench.svg
)

add_library(RaytracingGui SHARED ${RaytracingGui_SRCS} ${RaytracingGuiIcon_SVG})
target_link_libraries(RaytracingGui ${RaytracingGui_LIBS})


SET_BIN_DIR(RaytracingGui RaytracingGui /Mod/Raytracing)
SET_PYTHON_PREFIX_SUFFIX(RaytracingGui)

fc_copy_sources(RaytracingGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Raytracing" ${RaytracingGuiIcon_SVG})

INSTALL(TARGETS RaytracingGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${RaytracingGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Raytracing/Resources/icons")
