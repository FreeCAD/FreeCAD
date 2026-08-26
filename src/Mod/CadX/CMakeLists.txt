# SPDX-License-Identifier: LGPL-2.1-or-later

# Python remains the FreeCAD bootstrap/provider boundary.  Native graph and
# active-view services are built additively in App/ and Gui/.

set(CadXMain_SRCS
    Init.py
    InitGui.py
    CadXConfig.py
    CadXChatClient.py
    CadXSession.py
    CadXPanel.py
    CadXGui.py
    CadXGraphAudit.py
    CadXToolProtocol.py
    CadXToolBridge.py
    CadXNativeAssemblyBridge.py
)

set(CadXTests_SRCS
    cadx_tests/run_all.py
    cadx_tests/test_cadx_chat.py
    cadx_tests/test_cadx_session.py
    cadx_tests/test_cadx_graph_audit.py
    cadx_tests/test_cadx_tool_contracts.py
    cadx_tests/test_cadx_tool_turn.py
)

SOURCE_GROUP("" FILES ${CadXMain_SRCS})

ADD_CUSTOM_TARGET(CadX ALL
    SOURCES ${CadXMain_SRCS} ${CadXTests_SRCS}
)

fc_copy_sources(CadX "${CMAKE_BINARY_DIR}/Mod/CadX" ${CadXMain_SRCS})

fc_copy_sources(CadX "${CMAKE_BINARY_DIR}/Mod/CadX" ${CadXTests_SRCS})

INSTALL(
    FILES
        ${CadXMain_SRCS}
    DESTINATION
        Mod/CadX
)

INSTALL(
    FILES
        ${CadXTests_SRCS}
    DESTINATION
        Mod/CadX/cadx_tests
)

add_subdirectory(App)

if(BUILD_GUI)
    add_subdirectory(Gui)
endif()
