SET(Surfaces_Icons
	BorderIco.png
	BorderIco.xpm
	ConvertIco.png
	ConvertTo4SidesIco.xpm
	Ico.xcf
	IsoCurveIco.png
	IsoCurveIco.xpm
	SliceIco.xcf
	BorderIco.xcf
	ConvertIco.xcf
	Ico.png
	Ico.xpm
	IsoCurveIco.xcf
	SliceIco.png
	SliceIco.xpm
)

SOURCE_GROUP("" FILES ${Surfaces_Icons})

ADD_CUSTOM_TARGET(SurfacesIcons ALL
    SOURCES ${Surfaces_Icons}
)

fc_copy_sources("Mod/Surfaces/Icons" "Surfaces/Icons" ${Surfaces_Icons})

INSTALL(
    FILES
		${Surfaces_Icons}
    DESTINATION
		Mod/Surfaces/Icons
)

