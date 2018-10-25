SET(AddonManager_SRCS
    Init.py
    InitGui.py
    AddonManager.py
    AddonManagerGui.py
    addonmanager_macro.py
    addonmanager_utilities.py
)

SOURCE_GROUP("" FILES ${AddonManager_SRCS})

ADD_CUSTOM_TARGET(AddonManager ALL
    SOURCES ${AddonManager_SRCS}
)

fc_copy_sources(AddonManager "${CMAKE_BINARY_DIR}/Mod/AddonManager" ${AddonManager_SRCS})

INSTALL(
    FILES
        ${AddonManager_SRCS}
    DESTINATION
        Mod/AddonManager
)
