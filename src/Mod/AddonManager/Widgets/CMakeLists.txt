SET(AddonManagerWidget_SRCS
        __init__.py
        addonmanager_colors.py
        addonmanager_widget_addon_buttons.py
        addonmanager_widget_filter_selector.py
        addonmanager_widget_global_buttons.py
        addonmanager_widget_package_details_view.py
        addonmanager_widget_progress_bar.py
        addonmanager_widget_readme_browser.py
        addonmanager_widget_search.py
        addonmanager_widget_view_control_bar.py
        addonmanager_widget_view_selector.py
)

SOURCE_GROUP("" FILES ${AddonManagerWidget_SRCS})

ADD_CUSTOM_TARGET(AddonManagerWidget ALL
    SOURCES ${AddonManagerWidget_SRCS}
)

fc_copy_sources(AddonManagerWidget "${CMAKE_BINARY_DIR}/Mod/AddonManager/Widgets" ${AddonManagerWidget_SRCS})

INSTALL(
    FILES
        ${AddonManagerWidget_SRCS}
    DESTINATION
        Mod/AddonManager/Widgets
)
