set(AddonManagerWidget_SRCS
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
    addonmanager_widget_view_selector.py)

source_group("" FILES ${AddonManagerWidget_SRCS})

add_custom_target(AddonManagerWidget ALL SOURCES ${AddonManagerWidget_SRCS})

fc_copy_sources(AddonManagerWidget "${CMAKE_BINARY_DIR}/Mod/AddonManager/Widgets"
                ${AddonManagerWidget_SRCS})

install(FILES ${AddonManagerWidget_SRCS} DESTINATION Mod/AddonManager/Widgets)
