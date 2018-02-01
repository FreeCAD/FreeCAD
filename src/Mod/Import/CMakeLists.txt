
add_subdirectory(App)
if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

IF (BUILD_GUI)
    PYSIDE_WRAP_RC(Import_QRC_SRCS Resources/Import.qrc)
ENDIF (BUILD_GUI)

ADD_CUSTOM_TARGET(ImportRC ALL
    SOURCES ${Import_QRC_SRCS}
)


IF (BUILD_GUI)
    fc_target_copy_resource(ImportRC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/Mod/Import
        Import_rc.py)
ENDIF (BUILD_GUI)

INSTALL(
    FILES
        Init.py
        InitGui.py
        gzip_utf8.py
        stepZ.py
        ${Import_QRC_SRCS}
    DESTINATION
        Mod/Import
)   
