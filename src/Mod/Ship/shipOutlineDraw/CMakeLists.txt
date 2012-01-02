SET(Ship_OutlineDraw
	__init__.py
	Plot.py
	Preview.py
	TaskPanel.py
	TaskPanel.ui
)

SOURCE_GROUP("" FILES ${Ship_OutlineDraw})

ADD_CUSTOM_TARGET(ShipOutlineDraw ALL
    SOURCES ${Ship_OutlineDraw}
)

fc_copy_sources("Mod/Ship/shipOutlineDraw" "Ship/shipOutlineDraw" ${Ship_OutlineDraw})

INSTALL(
    FILES
		${Ship_OutlineDraw}
    DESTINATION
		Mod/Ship/shipOutlineDraw
)

