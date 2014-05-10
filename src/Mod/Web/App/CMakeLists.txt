
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

SET_BIN_DIR(Web Web /Mod/Web)
SET_PYTHON_PREFIX_SUFFIX(Web)

install(TARGETS Web DESTINATION ${CMAKE_INSTALL_LIBDIR})
