
if(OPENCV2_FOUND)
    add_definitions(-DHAVE_OPENCV2)
endif(OPENCV2_FOUND)


include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${OPENCV2_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

if(MSVC)
    include_directories(
        ${CMAKE_SOURCE_DIR}/src/3rdParty/OpenGL/api
    )
endif(MSVC)

set(ImageGui_LIBS
    Image
    FreeCADGui
    ${OpenCV2_LIBRARIES}
    ${OPENGL_glu_LIBRARY}
)

set(ImageGui_MOC_HDRS
    ImageView.h
    ImageOrientationDialog.h
)

if(FREECAD_USE_QTOPENGL_WIDGET)
    list(APPEND ImageGui_MOC_HDRS
        OpenGLImageBox.h
    )
else()
    list(APPEND ImageGui_MOC_HDRS
        GLImageBox.h
    )
endif()

fc_wrap_cpp(ImageGui_MOC_SRCS ${ImageGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${ImageGui_MOC_SRCS})

SET(ImageGui_RES_SRCS
    Resources/Image.qrc
)

set(ImageGui_UIC_SRCS
    ImageOrientationDialog.ui
)

if(BUILD_QT5)
    qt5_add_resources(ImageGui_QRC_SRCS ${ImageGui_RES_SRCS})
    qt5_wrap_ui(ImageGui_UIC_HDRS ${ImageGui_UIC_SRCS})
else()
    qt4_add_resources(ImageGui_QRC_SRCS ${ImageGui_RES_SRCS})
    qt4_wrap_ui(ImageGui_UIC_HDRS ${ImageGui_UIC_SRCS})
endif()

SET(ImageGui_SRCS
#   ${ImageGui_MOC_SRCS}
    ${ImageGui_QRC_SRCS}
    ${ImageGui_UIC_HDRS}
    AppImageGui.cpp
    AppImageGuiPy.cpp
    Command.cpp
    ImageOrientationDialog.cpp
    ImageOrientationDialog.h
    ViewProviderImagePlane.cpp
    ViewProviderImagePlane.h
    Resources/Image.qrc
    ImageView.cpp
    ImageView.h
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
    XpmImages.h
)

if(FREECAD_USE_QTOPENGL_WIDGET)
    list(APPEND ImageGui_SRCS
        OpenGLImageBox.cpp
        OpenGLImageBox.h
    )
else()
    list(APPEND ImageGui_SRCS
        GLImageBox.cpp
        GLImageBox.h
    )
endif()

SET(ImageGuiIcon_SVG
    Resources/icons/ImageWorkbench.svg
)

add_library(ImageGui SHARED ${ImageGui_SRCS} ${ImageGuiIcon_SVG})
target_link_libraries(ImageGui ${ImageGui_LIBS})


SET_BIN_DIR(ImageGui ImageGui /Mod/Image)
SET_PYTHON_PREFIX_SUFFIX(ImageGui)

fc_copy_sources(ImageGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Image" ${ImageGuiIcon_SVG})

INSTALL(TARGETS ImageGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES ${ImageGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Image/Resources/icons")
