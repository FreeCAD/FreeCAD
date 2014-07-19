include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    #${ODE_INCLUDE_DIRS}
)

set(AssemblyGui_LIBS
    #${ODE_LIBRARIES}
    Assembly
    FreeCADGui
)

qt4_add_resources(AssemblyGui_SRCS Resources/Assembly.qrc)

SET(AssemblyGui_SRCS
    ${AssemblyGui_SRCS}
    AppAssemblyGui.cpp
    AppAssemblyGuiPy.cpp
    Command.cpp
    Resources/Assembly.qrc
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
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
