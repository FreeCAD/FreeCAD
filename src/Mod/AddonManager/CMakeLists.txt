IF (BUILD_GUI)
    PYSIDE_WRAP_RC(AddonManager_QRC_SRCS Resources/AddonManager.qrc)
ENDIF (BUILD_GUI)

SET(AddonManager_SRCS
    Init.py
    InitGui.py
    AddonManager.py
    AddonManagerRepo.py
    addonmanager_macro.py
    addonmanager_utilities.py
    addonmanager_workers.py
    AddonManager.ui
    AddonManagerOptions.ui
    ALLOWED_PYTHON_PACKAGES.txt
    first_run.ui
    compact_view.py
    dependency_resolution_dialog.ui
    expanded_view.py
    NetworkManager.py
    package_list.py
    package_details.py
)

SOURCE_GROUP("" FILES ${AddonManager_SRCS})

ADD_CUSTOM_TARGET(AddonManager ALL
    SOURCES ${AddonManager_SRCS} ${AddonManager_QRC_SRCS}
)

fc_copy_sources(AddonManager "${CMAKE_BINARY_DIR}/Mod/AddonManager" ${AddonManager_SRCS})

IF (BUILD_GUI)
    fc_target_copy_resource(AddonManager
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/AddonManager
        AddonManager_rc.py)
ENDIF (BUILD_GUI)

INSTALL(
    FILES
        ${AddonManager_SRCS}
        ${AddonManager_QRC_SRCS}
    DESTINATION
        Mod/AddonManager
)


