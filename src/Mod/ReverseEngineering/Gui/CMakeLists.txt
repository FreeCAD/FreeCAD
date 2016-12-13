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
    ${COIN3D_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${XercesC_INCLUDE_DIRS}
)
link_directories(${OCC_LIBRARY_DIR})

set(ReenGui_LIBS
    ReverseEngineering
    FreeCADGui
)

if(BUILD_QT5)
    qt5_add_resources(ReenGui_QRC_SRCS Resources/ReverseEngineering.qrc)
else()
    qt4_add_resources(ReenGui_QRC_SRCS Resources/ReverseEngineering.qrc)
endif()

set(ReenGui_MOC_HDRS
    FitBSplineSurface.h
    Poisson.h
)
fc_wrap_cpp(ReenGui_MOC_SRCS ${ReenGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${ReenGui_MOC_SRCS})

set(Dialogs_UIC_SRCS
    FitBSplineSurface.ui
    Poisson.ui
)

if(BUILD_QT5)
    qt5_wrap_ui(Dialogs_UIC_HDRS ${Dialogs_UIC_SRCS})
else()
    qt4_wrap_ui(Dialogs_UIC_HDRS ${Dialogs_UIC_SRCS})
endif()

SET(Dialogs_SRCS
    ${Dialogs_UIC_HDRS}
    ${Dialogs_UIC_SRCS}
    FitBSplineSurface.cpp
    FitBSplineSurface.h
    Poisson.cpp
    Poisson.h
)
SOURCE_GROUP("Dialogs" FILES ${Dialogs_SRCS})

SET(ReenGui_SRCS
    ${ReenGui_QRC_SRCS}
    ${Dialogs_SRCS}
    AppReverseEngineeringGui.cpp
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

SET(ReverseEngineeringGuiIcon_SVG
    Resources/icons/ReverseEngineeringWorkbench.svg
)

fc_copy_sources(ReverseEngineeringGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/ReverseEngineering" ${ReverseEngineeringGuiIcon_SVG})

INSTALL(FILES ${ReverseEngineeringGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/ReverseEngineering/Resources/icons")
