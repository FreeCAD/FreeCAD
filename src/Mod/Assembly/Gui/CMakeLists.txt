if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    #${ODE_INCLUDE_DIRS}
)

set(AssemblyGui_LIBS
    #${ODE_LIBRARIES}
    Assembly
    PartGui
    FreeCADGui
)

qt4_add_resources(AssemblyGui_SRCS Resources/Assembly.qrc)


SET(AssemblyGuiViewProvider_SRCS
    ViewProvider.cpp
    ViewProvider.h
)
SOURCE_GROUP("ViewProvider" FILES ${AssemblyGuiViewProvider_SRCS})

SET(AssemblyGuiModule_SRCS
    AppAssemblyGui.cpp
    AppAssemblyGuiPy.cpp
    Command.cpp
    Resources/Assembly.qrc
    qrc_Assembly.cxx
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)
SOURCE_GROUP("Module" FILES ${AssemblyGuiModule_SRCS})


SET(AssemblyGui_SRCS
    ${AssemblyGui_SRCS}
    ${AssemblyGuiViewProvider_SRCS}
    ${AssemblyGuiModule_SRCS}
)

add_library(AssemblyGui SHARED ${AssemblyGui_SRCS})
target_link_libraries(AssemblyGui ${AssemblyGui_LIBS})


fc_target_copy_resource(AssemblyGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Assembly
    ${CMAKE_BINARY_DIR}/Mod/Assembly
    InitGui.py)

SET_BIN_DIR(AssemblyGui AssemblyGui /Mod/Assembly)
SET_PYTHON_PREFIX_SUFFIX(AssemblyGui)

INSTALL(TARGETS AssemblyGui DESTINATION ${CMAKE_INSTALL_LIBDIR})
