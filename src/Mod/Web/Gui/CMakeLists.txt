include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(WebGui_LIBS
    FreeCADGui
)

qt4_add_resources(Web_QRC_SRCS Resources/Web.qrc)

SET(WebGui_SRCS
    ${Web_QRC_SRCS}
    AppWebGui.cpp
    AppWebGuiPy.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
    BrowserView.h
    BrowserView.cpp
)

set(WebGui_MOC_HDRS
    BrowserView.h
)
fc_wrap_cpp(WebGui_MOC_SRCS ${WebGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${SketcherGui_MOC_SRCS})

add_library(WebGui SHARED ${WebGui_SRCS})
target_link_libraries(WebGui ${WebGui_LIBS})
fc_copy_script("Mod/Web" "WebGui" InitGui.py)
fc_copy_script("Mod/Web" "WebGui" Init.py)

if(MSVC)
    set_target_properties(WebGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(WebGui PROPERTIES DEBUG_OUTPUT_NAME "WebGui_d")
    set_target_properties(WebGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Web)
    set_target_properties(WebGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(WebGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(WebGui PROPERTIES DEBUG_OUTPUT_NAME "WebGui_d")
    set_target_properties(WebGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Web)
    set_target_properties(WebGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(WebGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Web)
    set_target_properties(WebGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS WebGui DESTINATION lib)
