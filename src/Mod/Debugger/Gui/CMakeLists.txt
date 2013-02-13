
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
)

if(MSVC)
    SET(DebugServer_LIBS
        debug MSVCRTD.LIB
        debug MSVCPRTD.LIB
        optimized MSVCRT.LIB
        optimized MSVCPRT.LIB
        debug qtmaind.lib
        optimized qtmain.lib
        ${QT_LIBRARIES}
        ${QT_DEBUG_LIBRARIES}
    )
else(MSVC)
    SET(DebugServer_LIBS
        ${QT_LIBRARIES}
        ${QT_DEBUG_LIBRARIES}
    )
endif(MSVC)

set(DebugServer_MOC_HDRS
    DebugServer.h
    ServerWidget.h
)
fc_wrap_cpp(DebugServer_MOC_SRCS ${DebugServer_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${DebugServer_MOC_SRCS})

set(DebugServer_UIC_SRCS
    ServerWidget.ui
)

qt4_wrap_ui(DebugServer_UIC_HDRS ${DebugServer_UIC_SRCS})

SET(DebugServer_SRCS
    ${DebugServer_UIC_SRCS}
    PreCompiled.cpp
    PreCompiled.h
    main.cpp
    DebugServer.cpp
    DebugServer.h
    ServerWidget.cpp
    ServerWidget.h
)

add_executable(DebugServer WIN32 ${DebugServer_SRCS})
target_link_libraries(DebugServer ${DebugServer_LIBS})

# Note this is IDE specific, not necessarily platform specific
if(MSVC)
    set_target_properties(DebugServer PROPERTIES OUTPUT_NAME "DebugServer")
    set_target_properties(DebugServer PROPERTIES DEBUG_OUTPUT_NAME "DebugServerD")
    # dirty hack to avoid Debug/Release subdirectory
    set_target_properties(DebugServer PROPERTIES PREFIX "../")
    set_target_properties(DebugServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
elseif(MINGW)
    set_target_properties(DebugServer PROPERTIES OUTPUT_NAME "DebugServer")
    set_target_properties(DebugServer PROPERTIES DEBUG_OUTPUT_NAME "DebugServerD")
    set_target_properties(DebugServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else(MSVC)
    set_target_properties(DebugServer PROPERTIES OUTPUT_NAME "DebugServer")
    set_target_properties(DebugServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set_target_properties(DebugServer PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

if(WIN32)
    INSTALL(TARGETS DebugServer
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
    )
else(WIN32)
    INSTALL(TARGETS DebugServer 
        RUNTIME DESTINATION bin
    )
endif(WIN32)
