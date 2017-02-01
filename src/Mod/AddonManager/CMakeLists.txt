SET(AddonManager_SRCS
    Init.py
    InitGui.py
    AddonManager.py
    AddonManagerGui.py
)

SOURCE_GROUP("" FILES ${AddonManager_SRCS})

ADD_CUSTOM_TARGET(AddonManager ALL
    SOURCES ${AddonManager_SRCS}
)

fc_copy_sources(AddonManager "${CMAKE_BINARY_DIR}/Mod/AddonManager" ${AddonManager_SRCS})

IF (BUILD_GUI)
    fc_target_copy_resource(AddonManager
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/AddonManager)
ENDIF (BUILD_GUI)

INSTALL(
    FILES
        ${AddonManager_SRCS}
    DESTINATION
        Mod/AddonManager
)
