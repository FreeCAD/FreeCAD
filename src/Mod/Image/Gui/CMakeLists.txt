include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    #${OPENCV_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(ImageGui_LIBS
    Image
    FreeCADGui
    #${OpenCV_LIBRARIES}
    ${OPENGL_glu_LIBRARY}
)

set(ImageGui_MOC_HDRS
    ImageView.h
    GLImageBox.h
)

fc_wrap_cpp(ImageGui_MOC_SRCS ${ImageGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${ImageGui_MOC_SRCS})

SET(ImageGui_RES_SRCS
    Resources/Image.qrc
)

fc_add_resources(ImageGui_QRC_SRCS ${ImageGui_RES_SRCS})
#QT4_ADD_RESOURCES(ImageGui_QRC_SRCS ${ImageGui_RES_SRCS})

SET(ImageGui_SRCS
#   ${ImageGui_MOC_SRCS}
#   ${ImageGui_QRC_SRCS}
    AppImageGui.cpp
    AppImageGuiPy.cpp
    Command.cpp
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
fc_copy_script("Mod/Image" "ImageGui" InitGui.py)

if(MSVC)
    set_target_properties(ImageGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(ImageGui PROPERTIES DEBUG_OUTPUT_NAME "ImageGui_d")
    set_target_properties(ImageGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Image)
    set_target_properties(ImageGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(ImageGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(ImageGui PROPERTIES DEBUG_OUTPUT_NAME "ImageGui_d")
    set_target_properties(ImageGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Image)
    set_target_properties(ImageGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(ImageGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Image)
    set_target_properties(ImageGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS ImageGui DESTINATION lib)
