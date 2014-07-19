if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(ReenGui_LIBS
    ReverseEngineering
    FreeCADGui
)

qt4_add_resources(ReenGui_QRC_SRCS Resources/ReverseEngineering.qrc)

SET(ReenGui_SRCS
    ${ReenGui_QRC_SRCS}
    AppReverseEngineeringGui.cpp
    AppReverseEngineeringGuiPy.cpp
    Command.cpp
    Resources/ReverseEngineering.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(ReverseEngineeringGui SHARED ${ReenGui_SRCS})
target_link_libraries(ReverseEngineeringGui ${ReenGui_LIBS})


fc_target_copy_resource(ReverseEngineeringGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/ReverseEngineering
    ${CMAKE_BINARY_DIR}/Mod/ReverseEngineering
    InitGui.py)

SET_BIN_DIR(ReverseEngineeringGui ReverseEngineeringGui /Mod/ReverseEngineering)
SET_PYTHON_PREFIX_SUFFIX(ReverseEngineeringGui)

install(TARGETS ReverseEngineeringGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
