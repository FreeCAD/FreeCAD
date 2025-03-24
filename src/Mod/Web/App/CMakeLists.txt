include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

include_directories(
    SYSTEM
    ${QtNetwork_INCLUDE_DIRS}
)
set(Web_LIBS
    FreeCADApp
    ${QtNetwork_LIBRARIES}
)

SET(Web_SRCS
    AppWeb.cpp
    PreCompiled.cpp
    PreCompiled.h
    Server.cpp
    Server.h
)

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PCH_SRCS ${Web_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(Web PreCompiled.h PreCompiled.cpp PCH_SRCS)
endif(FREECAD_USE_PCH)

add_library(Web SHARED ${Web_SRCS})
target_link_libraries(Web ${Web_LIBS})
if (FREECAD_WARN_ERROR)
    target_compile_warn_error(Web)
endif()

SET_BIN_DIR(Web Web /Mod/Web)
SET_PYTHON_PREFIX_SUFFIX(Web)

install(TARGETS Web DESTINATION ${CMAKE_INSTALL_LIBDIR})
