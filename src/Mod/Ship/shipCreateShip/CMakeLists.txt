SET(Ship_CreateShip
	__init__.py
	Preview.py
	TaskPanel.py
	TaskPanel.ui
)

SOURCE_GROUP("" FILES ${Ship_CreateShip})

ADD_CUSTOM_TARGET(ShipCreateShip ALL
    SOURCES ${Ship_CreateShip}
)

fc_copy_sources("Mod/Ship/shipCreateShip" "Ship/shipCreateShip" ${Ship_CreateShip})

INSTALL(
    FILES
		${Ship_CreateShip}
    DESTINATION
		Mod/Ship/shipCreateShip
)

