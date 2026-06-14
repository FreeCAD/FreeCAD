# SPDX-License-Identifier: LGPL-2.1-or-later

file(GLOB Agent_Binaries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.pyd"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.so"
)

SET(Agent_SRCS
    __init__.py
    Init.py
    InitGui.py
    agent_config.py
    agent_embed.py
)

SET(Agent_ALL ${Agent_SRCS} ${Agent_Binaries})
SOURCE_GROUP("" FILES ${Agent_ALL})

ADD_CUSTOM_TARGET(Agent ALL
    SOURCES ${Agent_ALL}
)

fc_copy_sources(Agent "${CMAKE_BINARY_DIR}/Mod/Agent" ${Agent_ALL})

INSTALL(
    FILES
        ${Agent_ALL}
    DESTINATION
        Mod/Agent
)
