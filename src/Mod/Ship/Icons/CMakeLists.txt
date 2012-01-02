SET(Ship_Icons
	AreaCurveIco.png
	AreaCurveIco.xcf
	AreaCurveIco.xpm
	DataIco.png
	DataIco.xcf
	DataIco.xpm
	DiscretizeIco.png
	DiscretizeIco.xcf
	DiscretizeIco.xpm
	HydrostaticsIco.png
	HydrostaticsIco.xcf
	HydrostaticsIco.xpm
	Ico.png
	Ico.xcf
	Ico.xpm
	OutlineDrawIco.png
	OutlineDrawIco.xcf
	OutlineDrawIco.xpm
	ReparametrizeIco.png
	ReparametrizeIco.xcf
	ReparametrizeIco.xpm
	Ship.xcf
	Ship.xpm
)

SOURCE_GROUP("" FILES ${Ship_Icons})

ADD_CUSTOM_TARGET(ShipIcons ALL
    SOURCES ${Ship_Icons}
)

fc_copy_sources("Mod/Ship/Icons" "Ship/Icons" ${Ship_Icons})

INSTALL(
    FILES
		${Ship_Icons}
    DESTINATION
		Mod/Ship/Icons
)

