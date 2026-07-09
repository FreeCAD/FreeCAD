# SPDX-License-Identifier: LGPL-2.1-or-later

file(GLOB bridge_Binaries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.pyd"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.so"
)

if(NOT bridge_Binaries)
    message(FATAL_ERROR "No compiled bridge extensions (*.pyd/*.so) found in ${CMAKE_CURRENT_SOURCE_DIR}. "
                        "Run the Build Cython Modules workflow and sync the output before building.")
endif()

file(GLOB bridge_RpcMethods_Binaries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/rpc_methods/*.pyd"
    "${CMAKE_CURRENT_SOURCE_DIR}/rpc_methods/*.so"
)

if(NOT bridge_RpcMethods_Binaries)
    message(FATAL_ERROR "No compiled bridge rpc_methods extensions (*.pyd/*.so) found in ${CMAKE_CURRENT_SOURCE_DIR}/rpc_methods. "
                        "Run the Build Cython Modules workflow and sync the output before building.")
endif()

file(GLOB bridge_Tools_Binaries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.pyd"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/*.so"
)

if(NOT bridge_Tools_Binaries)
    message(FATAL_ERROR "No compiled bridge tools extensions (*.pyd/*.so) found in ${CMAKE_CURRENT_SOURCE_DIR}/tools. "
                        "Run the Build Cython Modules workflow and sync the output before building.")
endif()

SET(bridge_SRCS
    Init.py
    InitGui.py
    acs.py
)

SET(bridge_ALL ${bridge_SRCS} ${bridge_Binaries} ${bridge_RpcMethods_Binaries} ${bridge_Tools_Binaries})
SOURCE_GROUP("" FILES ${bridge_ALL})

ADD_CUSTOM_TARGET(bridge ALL
    SOURCES ${bridge_ALL}
)

fc_copy_sources(bridge "${CMAKE_BINARY_DIR}/Mod/bridge" ${bridge_ALL})

INSTALL(
    FILES
        ${bridge_SRCS} ${bridge_Binaries}
    DESTINATION
        Mod/bridge
)

INSTALL(
    FILES
        ${bridge_RpcMethods_Binaries}
    DESTINATION
        Mod/bridge/rpc_methods
)

INSTALL(
    FILES
        ${bridge_Tools_Binaries}
    DESTINATION
        Mod/bridge/tools
)
