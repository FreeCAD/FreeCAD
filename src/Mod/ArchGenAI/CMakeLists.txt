IF (BUILD_GUI)
    PYSIDE_WRAP_RC(ArchGenAI_QRC_SRCS Resources/ArchGenAI.qrc)
ENDIF (BUILD_GUI)

SET(ArchGenAI_SRCS
    Init.py
    InitGui.py
    ArchGen_Global.py
    ArchGen_Command.py
    ArchGen_UI.py
)

SET(ArchGenAI_Backend_SRCS
    backend/README.md
)

SOURCE_GROUP("Module" FILES ${ArchGenAI_SRCS})
SOURCE_GROUP("Backend" FILES ${ArchGenAI_Backend_SRCS})

ADD_CUSTOM_TARGET(ArchGenAI ALL
    SOURCES ${ArchGenAI_SRCS} ${ArchGenAI_Backend_SRCS}
)

fc_copy_sources(ArchGenAI "${CMAKE_BINARY_DIR}/Mod/ArchGenAI" ${ArchGenAI_SRCS})
fc_copy_sources(ArchGenAI "${CMAKE_BINARY_DIR}/Mod/ArchGenAI/backend" ${ArchGenAI_Backend_SRCS})

INSTALL(
    FILES
        ${ArchGenAI_SRCS}
    DESTINATION
        Mod/ArchGenAI
)

INSTALL(
    FILES
        ${ArchGenAI_Backend_SRCS}
    DESTINATION
        Mod/ArchGenAI/backend
)

# Install backend Python files if they exist
IF(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/backend/main.py")
    INSTALL(
        FILES
            backend/main.py
            backend/api.py
        DESTINATION
            Mod/ArchGenAI/backend
    )
ENDIF()
