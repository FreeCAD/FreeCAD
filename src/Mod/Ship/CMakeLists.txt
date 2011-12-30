add_subdirectory(Icons)
add_subdirectory(shipCreateShip)
add_subdirectory(shipOutlineDraw)
add_subdirectory(shipUtils)

SET(Ship_SRCS
        InitGui.py
        ShipGui.py
        Instance.py
)
SOURCE_GROUP("" FILES ${Ship_SRCS})

SET(all_files ${Ship_SRCS})

ADD_CUSTOM_TARGET(Ship ALL
    SOURCES ${all_files}
)

fc_copy_sources("Mod/Ship" "Ship" ${all_files})

INSTALL(
    FILES
		${Ship_SRCS}
		README
    DESTINATION
		Mod/Ship
)






