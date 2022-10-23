include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

if (QtWebEngineWidgets_FOUND)
    add_definitions(-DQTWEBENGINE)
endif()

set(WebGui_LIBS
    FreeCADGui
)

include_directories(${QtWebEngineWidgets_INCLUDE_DIRS})
list(APPEND WebGui_LIBS ${QtWebEngineWidgets_LIBRARIES})
qt_add_resources(Web_QRC_SRCS Resources/Web.qrc)

SET(WebGui_SRCS
    ${Web_QRC_SRCS}
    AppWebGui.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
    BrowserView.h
    BrowserView.cpp
    CookieJar.cpp
    CookieJar.h
)

SET(WebGuiIcon_SVG
    Resources/icons/WebWorkbench.svg
)

add_library(WebGui SHARED ${WebGui_SRCS} ${WebGuiIcon_SVG})
target_link_libraries(WebGui ${WebGui_LIBS})


SET_BIN_DIR(WebGui WebGui /Mod/Web)
SET_PYTHON_PREFIX_SUFFIX(WebGui)

fc_copy_sources(WebGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Web" ${WebGuiIcon_SVG})

INSTALL(TARGETS WebGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

INSTALL(FILES ${WebGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Web/Resources/icons")
