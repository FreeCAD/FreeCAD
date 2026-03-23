# source files
set(COIN_VRML97_FILES
	Anchor.cpp
	Appearance.cpp
	AudioClip.cpp
	Background.cpp
	Billboard.cpp
	Box.cpp
	Collision.cpp
	Color.cpp
	ColorInterpolator.cpp
	Cone.cpp
	Coordinate.cpp
	CoordinateInterpolator.cpp
	Cylinder.cpp
	CylinderSensor.cpp
	DirectionalLight.cpp
	DragSensor.cpp
	ElevationGrid.cpp
	Extrusion.cpp
	Fog.cpp
	FontStyle.cpp
	Geometry.cpp
	Group.cpp
	ImageTexture.cpp
	IndexedFaceSet.cpp
	IndexedLine.cpp
	IndexedLineSet.cpp
	IndexedShape.cpp
	Init.cpp
	Inline.cpp
	Interpolator.cpp
	LOD.cpp
	Light.cpp
	Material.cpp
	MovieTexture.cpp
	NavigationInfo.cpp
	Normal.cpp
	NormalInterpolator.cpp
	OrientationInterpolator.cpp
	Parent.cpp
	PixelTexture.cpp
	PlaneSensor.cpp
	PointLight.cpp
	PointSet.cpp
	PositionInterpolator.cpp
	ProximitySensor.cpp
	ScalarInterpolator.cpp
	Script.cpp
	Sensor.cpp
	Shape.cpp
	Sound.cpp
	Sphere.cpp
	SphereSensor.cpp
	SpotLight.cpp
	Switch.cpp
	Text.cpp
	Texture.cpp
	TextureCoordinate.cpp
	TextureTransform.cpp
	TimeSensor.cpp
	TouchSensor.cpp
	Transform.cpp
	VertexLine.cpp
	VertexPoint.cpp
	VertexShape.cpp
	Viewpoint.cpp
	VisibilitySensor.cpp
	WorldInfo.cpp
	JS_VRMLClasses.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_VRML97_INTERNAL_FILES
	JS_VRMLClasses.h
	JS_VRMLClasses.cpp
	SoVRMLSubInterpolatorP.h
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_VRML97_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} vrml97/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(vrml97 OBJECT ${COIN_VRML97_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_VRML97_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/vrml97/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_VRML97_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/vrml97/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
