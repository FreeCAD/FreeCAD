# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileNotice: Part of the FreeCAD project.

if (APPLE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPENSSL_CFLAGS}")
endif()

include_directories(
    SYSTEM
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
)

set(Cloud_LIBS
    FreeCADApp
    ${OPENSSL_LIBRARIES}
    ${CURL_LIBRARIES}
    ${XercesC_LIBRARIES}
    ${Boost_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

SET(Cloud_SRCS
    AppCloud.cpp
    PreCompiled.h
)

add_library(Cloud SHARED ${Cloud_SRCS})
target_link_libraries(Cloud ${Cloud_LIBS})

if(FREECAD_USE_PCH)
    target_precompile_headers(Cloud PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:"${CMAKE_CURRENT_LIST_DIR}/PreCompiled.h">
    )
endif(FREECAD_USE_PCH)


SET_BIN_DIR(Cloud Cloud /Mod/Cloud)
SET_PYTHON_PREFIX_SUFFIX(Cloud)

install(TARGETS Cloud DESTINATION ${CMAKE_INSTALL_LIBDIR})
