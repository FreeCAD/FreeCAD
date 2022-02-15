IF (BUILD_GUI)
    PYSIDE_WRAP_RC(AddonManager_QRC_SRCS Resources/AddonManager.qrc)
ENDIF (BUILD_GUI)

SET(AddonManager_SRCS
    add_toolbar_button_dialog.ui
    AddonManager.py
    AddonManager.ui
    addonmanager_macro.py
    addonmanager_utilities.py
    addonmanager_workers.py
    AddonManagerOptions.ui
    AddonManagerRepo.py
    ALLOWED_PYTHON_PACKAGES.txt
    change_branch.py
    change_branch.ui
    compact_view.py
    dependency_resolution_dialog.ui
    expanded_view.py
    first_run.ui
    Init.py
    InitGui.py
    install_to_toolbar.py
    loading.html
    NetworkManager.py
    package_details.py
    package_list.py
    select_toolbar_dialog.ui
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


