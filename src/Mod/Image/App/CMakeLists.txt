if(WIN32)
    add_definitions(-DFCAppImage)
endif(WIN32)

include_directories(
    #${OPENCV_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
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
fc_copy_script("Mod/Image" "Image" Init.py)

if(MSVC)
    set_target_properties(Image PROPERTIES SUFFIX ".pyd")
    set_target_properties(Image PROPERTIES DEBUG_OUTPUT_NAME "Image_d")
    set_target_properties(Image PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Image)
    set_target_properties(Image PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Image PROPERTIES SUFFIX ".pyd")
    set_target_properties(Image PROPERTIES DEBUG_OUTPUT_NAME "Image_d")
    set_target_properties(Image PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Image)
    set_target_properties(Image PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Image PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Image)
    set_target_properties(Image PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS Image DESTINATION lib)
