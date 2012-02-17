if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ATANH -DHAVE_ASINH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(PartDesignGui_LIBS
    PartDesign
    PartGui
    FreeCADGui
)

set(PartDesignGui_MOC_HDRS
    TaskPadParameters.h
    TaskPatternRectangularParameters.h
    TaskPocketParameters.h
    TaskChamferParameters.h
    TaskFilletParameters.h
    TaskHoleParameters.h
    TaskRevolutionParameters.h
)
fc_wrap_cpp(PartDesignGui_MOC_SRCS ${PartDesignGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${PartDesignGui_MOC_SRCS})

qt4_add_resources(PartDesignGui_SRCS Resources/PartDesign.qrc)

set(PartDesignGui_UIC_SRCS
    TaskPadParameters.ui
    TaskPatternRectangularParameters.ui
    TaskPocketParameters.ui
    TaskChamferParameters.ui
    TaskFilletParameters.ui
    TaskHoleParameters.ui
    TaskRevolutionParameters.ui
)
qt4_wrap_ui(PartDesignGui_UIC_HDRS ${PartDesignGui_UIC_SRCS})

SET(PartDesignGuiViewProvider_SRCS
    ViewProvider.cpp
    ViewProvider.h
    ViewProviderPad.cpp
    ViewProviderPad.h
    ViewProviderHole.cpp
    ViewProviderHole.h
    ViewProviderPocket.cpp
    ViewProviderPocket.h
    ViewProviderChamfer.cpp
    ViewProviderChamfer.h
    ViewProviderFillet.cpp
    ViewProviderFillet.h
    ViewProviderRevolution.cpp
    ViewProviderRevolution.h
    ViewProviderPatternRectangular.cpp
    ViewProviderPatternRectangular.h
)
SOURCE_GROUP("ViewProvider" FILES ${PartDesignGuiViewProvider_SRCS})

SET(PartDesignGuiTaskDlgs_SRCS
    TaskPadParameters.ui
    TaskPadParameters.cpp
    TaskPadParameters.h
    TaskPatternRectangularParameters.ui
    TaskPatternRectangularParameters.cpp
    TaskPatternRectangularParameters.h
    TaskPocketParameters.ui
    TaskPocketParameters.cpp
    TaskPocketParameters.h
    TaskChamferParameters.ui
    TaskChamferParameters.cpp
    TaskChamferParameters.h
    TaskFilletParameters.ui
    TaskFilletParameters.cpp
    TaskFilletParameters.h
    TaskRevolutionParameters.ui
    TaskRevolutionParameters.cpp
    TaskRevolutionParameters.h
    TaskHoleParameters.ui
    TaskHoleParameters.cpp
    TaskHoleParameters.h
)
SOURCE_GROUP("TaskDialogs" FILES ${PartDesignGuiTaskDlgs_SRCS})

SET(PartDesignGuiModule_SRCS
    AppPartDesignGui.cpp
    AppPartDesignGuiPy.cpp
    Command.cpp
    Resources/PartDesign.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)
SOURCE_GROUP("Module" FILES ${PartDesignGuiModule_SRCS})

SET(PartDesignGui_Scripts
    InitGui.py
    TestPartDesignGui.py
)

SET(PartDesignGui_SRCS
    ${PartDesignGui_SRCS}
    ${PartDesignGui_UIC_HDRS}
    ${PartDesignGuiModule_SRCS}
    ${PartDesignGuiTaskDlgs_SRCS}
    ${PartDesignGuiViewProvider_SRCS}
)

add_library(PartDesignGui SHARED ${PartDesignGui_SRCS})
target_link_libraries(PartDesignGui ${PartDesignGui_LIBS})
fc_copy_script("Mod/PartDesign" "PartDesignGui" ${PartDesignGui_Scripts})

if(MSVC)
    set_target_properties(PartDesignGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(PartDesignGui PROPERTIES DEBUG_OUTPUT_NAME "PartDesignGui_d")
    set_target_properties(PartDesignGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/PartDesign)
    set_target_properties(PartDesignGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(PartDesignGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(PartDesignGui PROPERTIES DEBUG_OUTPUT_NAME "PartDesignGui_d")
    set_target_properties(PartDesignGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/PartDesign)
    set_target_properties(PartDesignGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(PartDesignGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/PartDesign)
    set_target_properties(PartDesignGui PROPERTIES PREFIX "")
    set_target_properties(PartDesignGui PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/Mod/PartDesign)
endif(MSVC)

# The PartDesign module must go to where the __init__.py file is
INSTALL(TARGETS PartDesignGui DESTINATION Mod/PartDesign)

