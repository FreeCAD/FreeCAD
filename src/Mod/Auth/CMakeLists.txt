# SPDX-License-Identifier: LGPL-2.1-or-later

file(GLOB Auth_Binaries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.pyd"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.so"
)

SET(Auth_SRCS
    Init.py
    InitGui.py
    aqs.py
)

SET(Auth_ALL ${Auth_SRCS} ${Auth_Binaries})
SOURCE_GROUP("" FILES ${Auth_ALL})

ADD_CUSTOM_TARGET(Auth ALL
    SOURCES ${Auth_ALL}
)

fc_copy_sources(Auth "${CMAKE_BINARY_DIR}/Mod/Auth" ${Auth_ALL})

INSTALL(
    FILES
        ${Auth_ALL}
    DESTINATION
        Mod/Auth
)
