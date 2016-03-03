
if(OPENCV2_FOUND)
    add_definitions(-DHAVE_OPENCV2)
endif(OPENCV2_FOUND)


include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${OPENCV2_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
)

set(ImageGui_LIBS
    Image
    FreeCADGui
    ${OpenCV2_LIBRARIES}
    ${OPENGL_glu_LIBRARY}
)

set(ImageGui_MOC_HDRS
    ImageView.h
    GLImageBox.h
    ImageOrientationDialog.h
)

fc_wrap_cpp(ImageGui_MOC_SRCS ${ImageGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${ImageGui_MOC_SRCS})

SET(ImageGui_RES_SRCS
    Resources/Image.qrc
)

qt4_add_resources(ImageGui_QRC_SRCS ${ImageGui_RES_SRCS})

set(ImageGui_UIC_SRCS
    ImageOrientationDialog.ui
)
qt4_wrap_ui(ImageGui_UIC_HDRS ${ImageGui_UIC_SRCS})

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
    GLImageBox.cpp
    GLImageBox.h
    Resources/Image.qrc
    ImageView.cpp
    ImageView.h
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
    XpmImages.h
)

add_library(ImageGui SHARED ${ImageGui_SRCS})
target_link_libraries(ImageGui ${ImageGui_LIBS})


fc_target_copy_resource(ImageGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Image
    ${CMAKE_BINARY_DIR}/Mod/Image
    InitGui.py)

SET_BIN_DIR(ImageGui ImageGui /Mod/Image)
SET_PYTHON_PREFIX_SUFFIX(ImageGui)

INSTALL(TARGETS ImageGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

SET(ImageGuiIcon_SVG
    Resources/icons/ImageWorkbench.svg
)

fc_copy_sources(ImageGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Image" ${ImageGuiIcon_SVG})

INSTALL(FILES ${ImageGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Image/Resources/icons")
