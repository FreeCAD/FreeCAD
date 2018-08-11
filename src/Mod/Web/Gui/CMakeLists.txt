include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

set(WebGui_LIBS
    FreeCADGui
)

if(BUILD_QT5)
    include_directories(
        ${Qt5WebKitWidgets_INCLUDE_DIRS}
    )
    list(APPEND WebGui_LIBS
        ${Qt5WebKitWidgets_LIBRARIES}
    )
    qt5_add_resources(Web_QRC_SRCS Resources/Web.qrc)
else()
    qt4_add_resources(Web_QRC_SRCS Resources/Web.qrc)
endif()

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

set(WebGui_MOC_HDRS
    BrowserView.h
    CookieJar.h
)
fc_wrap_cpp(WebGui_MOC_SRCS ${WebGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${SketcherGui_MOC_SRCS})

add_library(WebGui SHARED ${WebGui_SRCS})
target_link_libraries(WebGui ${WebGui_LIBS})


fc_target_copy_resource(WebGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Web
    ${CMAKE_BINARY_DIR}/Mod/Web
    InitGui.py)

SET_BIN_DIR(WebGui WebGui /Mod/Web)
SET_PYTHON_PREFIX_SUFFIX(WebGui)

INSTALL(TARGETS WebGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

SET(WebGuiIcon_SVG
    Resources/icons/WebWorkbench.svg
)

fc_copy_sources(WebGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Web" ${WebGuiIcon_SVG})

INSTALL(FILES ${WebGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Web/Resources/icons")
