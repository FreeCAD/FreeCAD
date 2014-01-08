
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${ZLIB_INCLUDE_DIR}
    ${XERCESC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
)

if(MSVC)
    set(Web_LIBS
         FreeCADApp
         ${QT_DEBUG_LIBRARIES}
         ${QT_LIBRARIES}
    )
else(MSVC)
    set(Web_LIBS
        FreeCADApp
        ${QT_LIBRARIES}
    )
endif(MSVC)

set(Web_MOC_HDRS
    Server.h
)
fc_wrap_cpp(Web_MOC_SRCS ${Web_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${Web_MOC_SRCS})

SET(Web_SRCS
    AppWeb.cpp
    AppWebPy.cpp
    PreCompiled.cpp
    PreCompiled.h
    Server.cpp
    Server.h
)

add_library(Web SHARED ${Web_SRCS})
target_link_libraries(Web ${Web_LIBS})


fc_target_copy_resource(Web 
    ${CMAKE_SOURCE_DIR}/src/Mod/Web
    ${CMAKE_BINARY_DIR}/Mod/Web
    Init.py)

if(MSVC)
    set_target_properties(Web PROPERTIES SUFFIX ".pyd")
    set_target_properties(Web PROPERTIES DEBUG_OUTPUT_NAME "Web_d")
    set_target_properties(Web PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Web)
    set_target_properties(Web PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Web PROPERTIES SUFFIX ".pyd")
    set_target_properties(Web PROPERTIES DEBUG_OUTPUT_NAME "Web_d")
    set_target_properties(Web PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Web)
    set_target_properties(Web PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Web PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Web)
    set_target_properties(Web PROPERTIES PREFIX "")
    set_target_properties(Web PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

install(TARGETS Web DESTINATION lib)
