SET(Ship_Utils
	__init__.py
	Math.py
	Paths.py
	Translator.py
)

SOURCE_GROUP("" FILES ${Ship_Utils})

ADD_CUSTOM_TARGET(ShipUtils ALL
    SOURCES ${Ship_Utils}
)

fc_copy_sources("Mod/Ship/shipUtils" "Ship/shipUtils" ${Ship_Utils})

INSTALL(
    FILES
		${Ship_Utils}
    DESTINATION
		Mod/Ship/shipUtils
)

