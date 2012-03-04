SET(SnapMain_SRCS
	InitGui.py
	SnapGui.py
	Tools.py
)
SOURCE_GROUP("" FILES ${SnapMain_SRCS})

SET(SnapIcons_SRCS
	Icons/Ico.xcf
	Icons/Ico.png
	Icons/Ico.xpm
)
SOURCE_GROUP("snapicons" FILES ${SnapIcons_SRCS})

SET(SnapWidget_SRCS
	snapWidget/__init__.py
	snapWidget/Widget.py
)
SOURCE_GROUP("snapwidget" FILES ${SnapWidget_SRCS})

SET(SnapUtils_SRCS
	snapUtils/__init__.py
	snapUtils/Paths.py
	snapUtils/Translator.py
)
SOURCE_GROUP("snaputils" FILES ${SnapUtils_SRCS})

SET(SnapTest_SRCS
	snapTest/__init__.py
	snapTest/TaskPanel.py
	snapTest/TaskPanel.ui
)
SOURCE_GROUP("snaptest" FILES ${SnapTest_SRCS})

SET(all_files ${SnapMain_SRCS} ${SnapIcons_SRCS} ${SnapUtils_SRCS} ${SnapWidget_SRCS} ${SnapTest_SRCS})

ADD_CUSTOM_TARGET(Snap ALL
    SOURCES ${all_files}
)

fc_copy_sources(Mod/Snap "${CMAKE_BINARY_DIR}/Mod/Snap" ${all_files})

INSTALL(
    FILES
        ${SnapIcons_SRCS}
    DESTINATION
        Mod/Snap/Icons
)
INSTALL(
    FILES
        ${SnapWidget_SRCS}
    DESTINATION
        Mod/Snap/snapWidget
)
INSTALL(
    FILES
        ${SnapUtils_SRCS}
    DESTINATION
        Mod/Snap/snapUtils
)
INSTALL(
    FILES
        ${SnapMain_SRCS}
    DESTINATION
        Mod/Snap
)
INSTALL(
    FILES
        ${SnapTest_SRCS}
    DESTINATION
        Mod/Snap/snapTest
)

