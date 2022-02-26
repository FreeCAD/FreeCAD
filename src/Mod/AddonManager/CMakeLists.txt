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
    change_branch.py
    change_branch.ui
    first_run.ui
    compact_view.py
    dependency_resolution_dialog.ui
    expanded_view.py
    NetworkManager.py
    package_list.py
    package_details.py
    loading.html
    TestAddonManagerApp.py
)
IF (BUILD_GUI)
	LIST(APPEND AddonManager_SRCS TestAddonManagerGui.py)
ENDIF (BUILD_GUI)

SOURCE_GROUP("" FILES ${AddonManager_SRCS})

SET(AddonManagerTests_SRCS
    AddonManagerTest/__init__.py
    AddonManagerTest/test_information.md
)

SET(AddonManagerTestsApp_SRCS
    AddonManagerTest/app/__init__.py
    AddonManagerTest/app/test_utilities.py
)

SET(AddonManagerTestsGui_SRCS
    AddonManagerTest/gui/__init__.py
    AddonManagerTest/gui/test_gui.py
)

SET(AddonManagerTestsFiles_SRCS
    AddonManagerTest/data/__init__.py
    AddonManagerTest/data/bad_macro_metadata.FCStd
    AddonManagerTest/data/good_macro_metadata.FCStd
    AddonManagerTest/data/missing_macro_metadata.FCStd
)

SET(AddonManagerTests_ALL
	${AddonManagerTests_SRCS}
	${AddonManagerTestsApp_SRCS}
	${AddonManagerTestsFiles_SRCS}
	)

IF (BUILD_GUI)
	LIST(APPEND AddonManagerTests_ALL ${AddonManagerTestsGui_SRCS})
ENDIF (BUILD_GUI)

ADD_CUSTOM_TARGET(AddonManager ALL
    SOURCES ${AddonManager_SRCS} ${AddonManager_QRC_SRCS}
)

ADD_CUSTOM_TARGET(AddonManagerTests ALL
    SOURCES ${AddonManagerTests_ALL}
)

fc_copy_sources(AddonManager "${CMAKE_BINARY_DIR}/Mod/AddonManager" ${AddonManager_SRCS})

fc_copy_sources(AddonManagerTests "${CMAKE_BINARY_DIR}/Mod/AddonManager" ${AddonManagerTests_ALL})

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

INSTALL(FILES ${AddonManagerTests_SRCS} DESTINATION Mod/AddonManager/AddonManagerTest)
INSTALL(FILES ${AddonManagerTestsApp_SRCS} DESTINATION Mod/AddonManager/AddonManagerTest/app)
INSTALL(FILES ${AddonManagerTestsGui_SRCS} DESTINATION Mod/AddonManager/AddonManagerTest/gui)
INSTALL(FILES ${AddonManagerTestsFiles_SRCS} DESTINATION Mod/AddonManager/AddonManagerTest/data)
