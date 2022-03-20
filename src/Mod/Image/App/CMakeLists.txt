if(WIN32)
    add_definitions(-DFCAppImage)
endif(WIN32)

if(OPENCV2_FOUND)
    add_definitions(-DHAVE_OPENCV2)
endif(OPENCV2_FOUND)

include_directories(
    ${OPENCV_INCLUDE2_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
)

set(Image_LIBS
    ${OPENCV2_LIBRARIES}
    FreeCADApp
)

set(Image_SRCS
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


SET_BIN_DIR(Image Image /Mod/Image)
SET_PYTHON_PREFIX_SUFFIX(Image)

INSTALL(TARGETS Image DESTINATION ${CMAKE_INSTALL_LIBDIR})
