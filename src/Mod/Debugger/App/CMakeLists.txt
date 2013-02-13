
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${QT_QTCORE_INCLUDE_DIR}
    ${QT_QTNETWORK_INCLUDE_DIR}
)

set(Debugger_LIBS
    FreeCADApp
    ${QT_QTCORE_LIBRARY}
    ${QT_QTCORE_LIBRARY_DEBUG}
    ${QT_QTNETWORK_LIBRARY}
    ${QT_QTNETWORK_LIBRARY_DEBUG}
)

set(Debugger_MOC_HDRS
    DebugClient.h
    PythonDebugger.h
)
fc_wrap_cpp(Debugger_MOC_SRCS ${Debugger_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${Debugger_MOC_SRCS})

SET(Mod_SRCS
    AppDebugger.cpp
    PreCompiled.cpp
    PreCompiled.h
)

SET(Debugger_SRCS
    ${Mod_SRCS}
    DebugClient.cpp
    DebugClient.h
    PythonDebugger.cpp
    PythonDebugger.h
)

SOURCE_GROUP("Module" FILES ${Mod_SRCS})

add_library(Debugger SHARED ${Debugger_SRCS})
target_link_libraries(Debugger ${Debugger_LIBS})


fc_target_copy_resource(Debugger 
    ${CMAKE_SOURCE_DIR}/src/Mod/Debugger
    ${CMAKE_BINARY_DIR}/Mod/Debugger
    __init__.py)

if(MSVC)
    set_target_properties(Debugger PROPERTIES SUFFIX ".pyd")
    set_target_properties(Debugger PROPERTIES DEBUG_OUTPUT_NAME "Debugger_d")
    set_target_properties(Debugger PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Debugger)
    set_target_properties(Debugger PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Debugger PROPERTIES SUFFIX ".pyd")
    set_target_properties(Debugger PROPERTIES DEBUG_OUTPUT_NAME "Debugger_d")
    set_target_properties(Debugger PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Debugger)
    set_target_properties(Debugger PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Debugger PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Debugger)
    set_target_properties(Debugger PROPERTIES PREFIX "")
    set_target_properties(Debugger PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

INSTALL(TARGETS Debugger DESTINATION lib)
