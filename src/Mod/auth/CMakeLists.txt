# SPDX-License-Identifier: LGPL-2.1-or-later

file(GLOB auth_Binaries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.pyd"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.so"
)

if(NOT auth_Binaries)
    message(FATAL_ERROR "No compiled auth extensions (*.pyd/*.so) found in ${CMAKE_CURRENT_SOURCE_DIR}. "
                        "Run the Build Cython Modules workflow and sync the output before building.")
endif()

file(GLOB auth_Data CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/data/*"
)

if(NOT auth_Data)
    message(FATAL_ERROR "No auth data assets found in ${CMAKE_CURRENT_SOURCE_DIR}/data. "
                        "The Auth status icons must be present before building.")
endif()

SET(auth_SRCS
    Init.py
    InitGui.py
    aqs.py
)

SET(auth_ALL ${auth_SRCS} ${auth_Binaries} ${auth_Data})
SOURCE_GROUP("" FILES ${auth_ALL})

ADD_CUSTOM_TARGET(auth ALL
    SOURCES ${auth_ALL}
)

fc_copy_sources(auth "${CMAKE_BINARY_DIR}/Mod/auth" ${auth_ALL})

INSTALL(
    FILES
        ${auth_SRCS} ${auth_Binaries}
    DESTINATION
        Mod/auth
)

INSTALL(
    FILES
        ${auth_Data}
    DESTINATION
        Mod/auth/data
)
