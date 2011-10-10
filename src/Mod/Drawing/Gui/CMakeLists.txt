if(MSVC)
    add_definitions(-DHAVE_ATANH -DHAVE_ASINH -DHAVE_ACOSH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${QT_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(DrawingGui_LIBS
    Drawing
    FreeCADGui
)

set(DrawingGui_MOC_HDRS
    DrawingView.h
    TaskDialog.h
)

fc_wrap_cpp(DrawingGui_MOC_SRCS ${DrawingGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${DrawingGui_MOC_SRCS})

qt4_add_resources(DrawingGui_SRCS Resources/Drawing.qrc)

SET(DrawingGui_SRCS
    ${DrawingGui_SRCS}
    AppDrawingGui.cpp
    AppDrawingGuiPy.cpp
    Command.cpp
    Resources/Drawing.qrc
    PreCompiled.cpp
    PreCompiled.h
    TaskDialog.cpp
    TaskDialog.h
    Workbench.cpp
    Workbench.h
)
SET(DrawingGuiView_SRCS
    DrawingView.cpp
    DrawingView.h
)
SET(DrawingGuiViewProvider_SRCS
    ViewProviderPage.cpp
    ViewProviderPage.h
    ViewProviderView.cpp
    ViewProviderView.h
)

SOURCE_GROUP("Mod" FILES ${DrawingGui_SRCS})
SOURCE_GROUP("SVG-View" FILES ${DrawingGuiView_SRCS})
SOURCE_GROUP("ViewProvider" FILES ${DrawingGuiViewProvider_SRCS})

if(MSVC)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${DrawingGui_SRCS} ${DrawingGuiView_SRCS} ${DrawingGuiViewProvider_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER("PreCompiled.h" "PreCompiled.cpp" PCH_SRCS)
endif(MSVC)

add_library(DrawingGui SHARED ${DrawingGui_SRCS} ${DrawingGuiView_SRCS} ${DrawingGuiViewProvider_SRCS})
target_link_libraries(DrawingGui ${DrawingGui_LIBS})
fc_copy_script("Mod/Drawing" "DrawingGui" InitGui.py)

if(MSVC)
    set_target_properties(DrawingGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(DrawingGui PROPERTIES DEBUG_OUTPUT_NAME "DrawingGui_d")
    set_target_properties(DrawingGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Drawing)
    set_target_properties(DrawingGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(DrawingGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(DrawingGui PROPERTIES DEBUG_OUTPUT_NAME "DrawingGui_d")
    set_target_properties(DrawingGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Drawing)
    set_target_properties(DrawingGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(DrawingGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Drawing)
    set_target_properties(DrawingGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS DrawingGui DESTINATION lib)
