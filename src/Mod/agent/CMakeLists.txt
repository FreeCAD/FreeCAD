# SPDX-License-Identifier: LGPL-2.1-or-later

file(GLOB agent_Binaries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.pyd"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.so"
)

if(NOT agent_Binaries)
    message(FATAL_ERROR "No compiled agent extensions (*.pyd/*.so) found in ${CMAKE_CURRENT_SOURCE_DIR}. "
                        "Run the Build Cython Modules workflow and sync the output before building.")
endif()

SET(agent_SRCS
    Init.py
    InitGui.py
    ags.py
)

SET(agent_ALL ${agent_SRCS} ${agent_Binaries})
SOURCE_GROUP("" FILES ${agent_ALL})

ADD_CUSTOM_TARGET(agent ALL
    SOURCES ${agent_ALL}
)

fc_copy_sources(agent "${CMAKE_BINARY_DIR}/Mod/agent" ${agent_ALL})

INSTALL(
    FILES
        ${agent_ALL}
    DESTINATION
        Mod/agent
)
