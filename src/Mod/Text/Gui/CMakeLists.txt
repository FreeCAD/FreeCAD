include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(TextGui_LIBS
    Text
    PartGui
    FreeCADGui
)

qt_add_resources(TextGui_SRCS Resources/Text.qrc ${Text_TR_QRC})

SET(TextGui_SRCS
    ${TextGui_SRCS}
    ${TextGui_UIC_HDRS}
    AppTextGui.cpp
    ViewProviderShapeText.cpp
    ViewProviderShapeText.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${TextGui_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(TextGui PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)


add_library(TextGui SHARED ${TextGui_SRCS})
target_link_libraries(TextGui ${TextGui_LIBS})


SET_BIN_DIR(TextGui TextGui /Mod/Text)
SET_PYTHON_PREFIX_SUFFIX(TextGui)

INSTALL(TARGETS TextGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
