if (BUILD_GUI)
    pyside_wrap_rc (AddonManager_QRC_SRCS Resources/AddonManager.qrc)
    add_subdirectory (Widgets)
endif (BUILD_GUI)

set (
    AddonManager_SRCS
    add_toolbar_button_dialog.ui
    Addon.py
    AddonStats.py
    AddonManager.py
    AddonManager.ui
    addonmanager_cache.py
    addonmanager_connection_checker.py
    addonmanager_dependency_installer.py
    addonmanager_devmode.py
    addonmanager_devmode_add_content.py
    addonmanager_devmode_license_selector.py
    addonmanager_devmode_licenses_table.py
    addonmanager_devmode_metadata_checker.py
    addonmanager_devmode_person_editor.py
    addonmanager_devmode_people_table.py
    addonmanager_devmode_predictor.py
    addonmanager_devmode_validators.py
    addonmanager_firstrun.py
    addonmanager_freecad_interface.py
    addonmanager_git.py
    addonmanager_installer.py
    addonmanager_installer_gui.py
    addonmanager_licenses.py
    addonmanager_macro.py
    addonmanager_macro_parser.py
    addonmanager_metadata.py
    addonmanager_package_details_controller.py
    addonmanager_preferences_defaults.json
    addonmanager_pyside_interface.py
    addonmanager_readme_controller.py
    addonmanager_update_all_gui.py
    addonmanager_uninstaller.py
    addonmanager_uninstaller_gui.py
    addonmanager_utilities.py
    addonmanager_workers_installation.py
    addonmanager_workers_startup.py
    addonmanager_workers_utility.py
    AddonManagerOptions.ui
    AddonManagerOptions_AddCustomRepository.ui
    AddonManagerOptions.py
    ALLOWED_PYTHON_PACKAGES.txt
    change_branch.py
    change_branch.ui
    compact_view.py
    composite_view.py
    dependency_resolution_dialog.ui
    developer_mode.ui
    developer_mode_add_content.ui
    developer_mode_advanced_freecad_versions.ui
    developer_mode_copyright_info.ui
    developer_mode_dependencies.ui
    developer_mode_edit_dependency.ui
    developer_mode_freecad_versions.ui
    developer_mode_license.ui
    developer_mode_licenses_table.ui
    developer_mode_people.ui
    developer_mode_people_table.ui
    developer_mode_select_from_list.ui
    developer_mode_tags.ui
    expanded_view.py
    first_run.ui
    Init.py
    InitGui.py
    install_to_toolbar.py
    loading.html
    manage_python_dependencies.py
    NetworkManager.py
    package_list.py
    PythonDependencyUpdateDialog.ui
    select_toolbar_dialog.ui
    TestAddonManagerApp.py
    update_all.ui)
if (BUILD_GUI)
    list (APPEND AddonManager_SRCS TestAddonManagerGui.py)
endif (BUILD_GUI)

source_group ("" FILES ${AddonManager_SRCS})

set (AddonManagerTests_SRCS AddonManagerTest/__init__.py AddonManagerTest/test_information.md)

set (
    AddonManagerTestsApp_SRCS
    AddonManagerTest/app/__init__.py
    AddonManagerTest/app/mocks.py
    AddonManagerTest/app/test_addon.py
    AddonManagerTest/app/test_cache.py
    AddonManagerTest/app/test_dependency_installer.py
    AddonManagerTest/app/test_freecad_interface.py
    AddonManagerTest/app/test_git.py
    AddonManagerTest/app/test_installer.py
    AddonManagerTest/app/test_macro.py
    AddonManagerTest/app/test_macro_parser.py
    AddonManagerTest/app/test_metadata.py
    AddonManagerTest/app/test_utilities.py
    AddonManagerTest/app/test_uninstaller.py)

set (
    AddonManagerTestsGui_SRCS
    AddonManagerTest/gui/__init__.py
    AddonManagerTest/gui/gui_mocks.py
    AddonManagerTest/gui/test_gui.py
    AddonManagerTest/gui/test_installer_gui.py
    AddonManagerTest/gui/test_update_all_gui.py
    AddonManagerTest/gui/test_uninstaller_gui.py
    AddonManagerTest/gui/test_workers_startup.py
    AddonManagerTest/gui/test_workers_utility.py)

set (
    AddonManagerTestsFiles_SRCS
    AddonManagerTest/data/__init__.py
    AddonManagerTest/data/addon_update_stats.json
    AddonManagerTest/data/bad_macro_metadata.FCStd
    AddonManagerTest/data/combination.xml
    AddonManagerTest/data/corrupted_metadata.zip
    AddonManagerTest/data/depends_on_all_workbenches.xml
    AddonManagerTest/data/DoNothing.FCMacro
    AddonManagerTest/data/git_submodules.txt
    AddonManagerTest/data/good_macro_metadata.FCStd
    AddonManagerTest/data/good_package.xml
    AddonManagerTest/data/icon_cache.zip
    AddonManagerTest/data/icon_cache.zip.sha1
    AddonManagerTest/data/macro_only.xml
    AddonManagerTest/data/macro_template.FCStd
    AddonManagerTest/data/MacrosRecipesWikiPage.zip
    AddonManagerTest/data/metadata.zip
    AddonManagerTest/data/missing_macro_metadata.FCStd
    AddonManagerTest/data/prefpack_only.xml
    AddonManagerTest/data/test_addon_with_fcmacro.zip
    AddonManagerTest/data/test_github_style_repo.zip
    AddonManagerTest/data/test_repo.zip
    AddonManagerTest/data/test_simple_repo.zip
    AddonManagerTest/data/test_version_detection.xml
    AddonManagerTest/data/TestWorkbench.zip
    AddonManagerTest/data/workbench_only.xml)

set (AddonManagerTests_ALL ${AddonManagerTests_SRCS} ${AddonManagerTestsApp_SRCS}
                           ${AddonManagerTestsFiles_SRCS})

if (BUILD_GUI)
    list (APPEND AddonManagerTests_ALL ${AddonManagerTestsGui_SRCS})
endif (BUILD_GUI)

add_custom_target (AddonManager ALL SOURCES ${AddonManager_SRCS} ${AddonManager_QRC_SRCS})

add_custom_target (AddonManagerTests ALL SOURCES ${AddonManagerTests_ALL})

fc_copy_sources (AddonManager "${CMAKE_BINARY_DIR}/Mod/AddonManager" ${AddonManager_SRCS})

fc_copy_sources (AddonManagerTests "${CMAKE_BINARY_DIR}/Mod/AddonManager" ${AddonManagerTests_ALL})

if (BUILD_GUI)
    fc_target_copy_resource (AddonManager ${CMAKE_CURRENT_BINARY_DIR}
                             ${CMAKE_BINARY_DIR}/Mod/AddonManager AddonManager_rc.py)
endif (BUILD_GUI)

install (FILES ${AddonManager_SRCS} ${AddonManager_QRC_SRCS} DESTINATION Mod/AddonManager)

install (FILES ${AddonManagerTests_SRCS} DESTINATION Mod/AddonManager/AddonManagerTest)
install (FILES ${AddonManagerTestsApp_SRCS} DESTINATION Mod/AddonManager/AddonManagerTest/app)
install (FILES ${AddonManagerTestsGui_SRCS} DESTINATION Mod/AddonManager/AddonManagerTest/gui)
install (FILES ${AddonManagerTestsFiles_SRCS} DESTINATION Mod/AddonManager/AddonManagerTest/data)
