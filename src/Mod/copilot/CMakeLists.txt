# SPDX-License-Identifier: LGPL-2.1-or-later

set(Copilot_Scripts
    Init.py
    InitGui.py
    commands.py
    constants.py
    executor.py
    panel.py
    provider.py
    tests.py
    Resources/icons/copilot.svg
)

add_custom_target(CopilotScripts ALL
    SOURCES ${Copilot_Scripts}
)

fc_target_copy_resource(CopilotScripts
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/copilot
    ${Copilot_Scripts}
)

install(
    FILES
        Init.py
        InitGui.py
        commands.py
        constants.py
        executor.py
        panel.py
        provider.py
        tests.py
    DESTINATION
        Mod/copilot
)

install(
    FILES
        Resources/icons/copilot.svg
    DESTINATION
        Mod/copilot/Resources/icons
)
