# SPDX-License-Identifier: LGPL-2.1-or-later

file(GLOB bridge_Binaries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.pyd"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.so"
)

if(NOT bridge_Binaries)
    message(FATAL_ERROR "No compiled bridge extensions (*.pyd/*.so) found in ${CMAKE_CURRENT_SOURCE_DIR}. "
                        "Run the Build Cython Modules workflow and sync the output before building.")
endif()

SET(bridge_SRCS
    Init.py
    InitGui.py
    acs.py
)

SET(bridge_ALL ${bridge_SRCS} ${bridge_Binaries})
SOURCE_GROUP("" FILES ${bridge_ALL})

ADD_CUSTOM_TARGET(bridge ALL
    SOURCES ${bridge_ALL}
)

fc_copy_sources(bridge "${CMAKE_BINARY_DIR}/Mod/bridge" ${bridge_ALL})

INSTALL(
    FILES
        ${bridge_ALL}
    DESTINATION
        Mod/bridge
)
