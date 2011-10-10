include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
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
fc_copy_script("Mod/Assembly" "AssemblyGui" InitGui.py)

if(MSVC)
    set_target_properties(AssemblyGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(AssemblyGui PROPERTIES DEBUG_OUTPUT_NAME "AssemblyGui_d")
    set_target_properties(AssemblyGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Assembly)
    set_target_properties(AssemblyGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(AssemblyGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(AssemblyGui PROPERTIES DEBUG_OUTPUT_NAME "AssemblyGui_d")
    set_target_properties(AssemblyGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Assembly)
    set_target_properties(AssemblyGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(AssemblyGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Assembly)
    set_target_properties(AssemblyGui PROPERTIES PREFIX "")
endif(MSVC)

install(TARGETS AssemblyGui DESTINATION lib)
