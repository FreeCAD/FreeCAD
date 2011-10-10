if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
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

set(RaytracingGui_MOC_HDRS
    DlgSettingsRayImp.h
)
fc_wrap_cpp(RaytracingGui_MOC_SRCS ${RaytracingGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${RaytracingGui_MOC_SRCS})

fc_add_resources(Resource_SRCS Resources/Raytracing.qrc)
SET(Resource_SRCS
    Resources/Raytracing.qrc
)
SOURCE_GROUP("Resource" FILES ${Resource_SRCS})

set(RaytracingGui_UIC_SRCS
    DlgSettingsRay.ui
)
qt4_wrap_ui(RaytracingGui_UIC_HDRS ${RaytracingGui_UIC_SRCS})

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
    Workbench.cpp
    Workbench.h
)

add_library(RaytracingGui SHARED ${RaytracingGui_SRCS})
target_link_libraries(RaytracingGui ${RaytracingGui_LIBS})
fc_copy_script("Mod/Raytracing" "RaytracingGui" InitGui.py)

if(MSVC)
    set_target_properties(RaytracingGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(RaytracingGui PROPERTIES DEBUG_OUTPUT_NAME "RaytracingGui_d")
    set_target_properties(RaytracingGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Raytracing)
    set_target_properties(RaytracingGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(RaytracingGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(RaytracingGui PROPERTIES DEBUG_OUTPUT_NAME "RaytracingGui_d")
    set_target_properties(RaytracingGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Raytracing)
    set_target_properties(RaytracingGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(RaytracingGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Raytracing)
    set_target_properties(RaytracingGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS RaytracingGui DESTINATION lib)
