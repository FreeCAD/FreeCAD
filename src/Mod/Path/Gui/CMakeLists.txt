if(MSVC)
    add_definitions(-DFCGuiPath -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(PathGui_LIBS
    Path
    FreeCADGui
)

qt4_add_resources(PathResource_SRCS Resources/Path.qrc)

SOURCE_GROUP("Resources" FILES ${PathResource_SRCS})

set(PathGui_MOC_HDRS
    DlgSettingsPathColor.h
    TaskDlgPathCompound.h
    DlgProcessorChooser.h
)
fc_wrap_cpp(PathGui_MOC_SRCS ${PathGui_MOC_HDRS})

SOURCE_GROUP("Moc" FILES ${PathGui_MOC_SRCS})

set(PathGui_UIC_SRCS
    DlgSettingsPathColor.ui
    TaskDlgPathCompound.ui
    DlgProcessorChooser.ui
)
qt4_wrap_ui(PathGui_UIC_HDRS ${PathGui_UIC_SRCS})

SET(PathGui_SRCS_Module
    Command.cpp
    AppPathGui.cpp
    AppPathGuiPy.cpp
    Resources/Path.qrc
    PreCompiled.cpp
    PreCompiled.h
    DlgSettingsPathColor.ui
    DlgSettingsPathColor.cpp
    DlgSettingsPathColor.h
    TaskDlgPathCompound.ui
    TaskDlgPathCompound.cpp
    TaskDlgPathCompound.h
    DlgProcessorChooser.ui
    DlgProcessorChooser.cpp
    DlgProcessorChooser.h
)

SOURCE_GROUP("Module" FILES ${PathGui_SRCS_Module})

SET(PathGui_SRCS_ViewProvider
    ViewProviderPath.cpp
    ViewProviderPath.h
    ViewProviderPathCompound.cpp
    ViewProviderPathCompound.h
    ViewProviderPathShape.cpp
    ViewProviderPathShape.h
)

SOURCE_GROUP("ViewProvider" FILES ${PathGui_SRCS_ViewProvider})

SET(PathGui_SRCS
    ${PathResource_SRCS}
    ${PathGui_UIC_HDRS}
    ${PathGui_SRCS_Module}
    ${PathGui_SRCS_ViewProvider}
)

add_library(PathGui SHARED ${PathGui_SRCS})
target_link_libraries(PathGui ${PathGui_LIBS})


fc_target_copy_resource(PathGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Path
    ${CMAKE_BINARY_DIR}/Mod/Path
    InitGui.py)

SET_BIN_DIR(PathGui PathGui /Mod/Path)
SET_PYTHON_PREFIX_SUFFIX(PathGui)

INSTALL(TARGETS PathGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
