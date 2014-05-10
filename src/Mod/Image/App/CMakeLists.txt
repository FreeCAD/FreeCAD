if(WIN32)
    add_definitions(-DFCAppImage)
endif(WIN32)

include_directories(
    #${OPENCV_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
)

set(Image_LIBS
    #${OPENCV_LIBRARIES}
    FreeCADApp
)

set(Image_SRCS
    #CaptureClass.cpp
    #CaptureClass.h
    ImageBase.cpp
    ImageBase.h
    ImagePlane.cpp
    ImagePlane.h
    PreCompiled.cpp
    PreCompiled.h
    AppImage.cpp
)

add_library(Image SHARED ${Image_SRCS})
target_link_libraries(Image ${Image_LIBS})


fc_target_copy_resource(Image 
    ${CMAKE_SOURCE_DIR}/src/Mod/Image
    ${CMAKE_BINARY_DIR}/Mod/Image
    Init.py)

SET_BIN_DIR(Image Image /Mod/Image)
SET_PYTHON_PREFIX_SUFFIX(Image)

INSTALL(TARGETS Image DESTINATION ${CMAKE_INSTALL_LIBDIR})
