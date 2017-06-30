if(MSVC)
    add_definitions(-DFCAppPart -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
)

set(GDMLGui_LIBS
    ${OCC_LIBRARIES}
    Part
    FreeCADGui
)

qt4_add_resources(GDML_QRC_SRCS Resources/GDML.qrc)

######################
#BEGIN for G4GDML

set(GDML_SRCS
    GDML/G4FreeCAD.h
    GDML/G4FreeCAD.cpp
    )
SOURCE_GROUP("GDML" FILES ${GDML_SRCS})

set(GeometryManagement_SRCS
    GDML/geometry/management/G4SolidStore.hh
    GDML/geometry/management/G4SolidStore.cc
    GDML/geometry/management/G4VSolid.hh
    GDML/geometry/management/G4VSolid.icc
    GDML/geometry/management/G4VSolid.cc
    GDML/geometry/management/G4LogicalVolume.hh
    GDML/geometry/management/G4LogicalVolume.icc
    GDML/geometry/management/G4LogicalVolume.cc
    GDML/geometry/management/G4VPhysicalVolume.hh
    GDML/geometry/management/G4VPhysicalVolume.icc
    GDML/geometry/management/G4VPhysicalVolume.cc
    GDML/geometry/management/G4RotationMatrix.hh
    GDML/geometry/management/G4PhysicalVolumeStore.hh
    GDML/geometry/management/G4PhysicalVolumeStore.cc
    GDML/geometry/management/G4LogicalVolumeStore.hh
    GDML/geometry/management/G4LogicalVolumeStore.cc
    GDML/geometry/management/G4ReflectedSolid.hh
    GDML/geometry/management/G4ReflectedSolid.cc
    GDML/geometry/management/G4AffineTransform.hh
    GDML/geometry/management/G4AffineTransform.icc
    GDML/geometry/management/G4VPVParameterisation.cc
    GDML/geometry/management/G4VPVParameterisation.hh
    GDML/geometry/management/G4VoxelLimits.hh
    GDML/geometry/management/G4VoxelLimits.icc
    GDML/geometry/management/G4VoxelLimits.cc
    GDML/geometry/management/meshdefs.hh
    )
SOURCE_GROUP("GDML\\geometry\\management" FILES ${GeometryManagement_SRCS})

set(GeometrySolid_SRCS
    GDML/geometry/solid/G4Box.hh
    GDML/geometry/solid/G4Box.icc
    GDML/geometry/solid/G4Box.cc
    GDML/geometry/solid/G4Tubs.hh
    GDML/geometry/solid/G4Tubs.icc
    GDML/geometry/solid/G4Tubs.cc
    GDML/geometry/solid/G4CSGSolid.hh
    GDML/geometry/solid/G4CSGSolid.cc
    GDML/geometry/solid/G4Trap.hh
    GDML/geometry/solid/G4Trap.icc
    GDML/geometry/solid/G4Trap.cc
    GDML/geometry/solid/G4Cons.hh
    GDML/geometry/solid/G4Cons.icc
    GDML/geometry/solid/G4Cons.cc
    GDML/geometry/solid/G4Trd.hh
    GDML/geometry/solid/G4Trd.icc
    GDML/geometry/solid/G4Trd.cc
    )
SOURCE_GROUP("GDML\\geometry\\solid" FILES ${GeometrySolid_SRCS})

set(GeometrySolidBoolean_SRCS
    GDML/geometry/solid/Boolean/G4BooleanSolid.hh
    GDML/geometry/solid/Boolean/G4BooleanSolid.icc
    GDML/geometry/solid/Boolean/G4BooleanSolid.cc
    GDML/geometry/solid/Boolean/G4DisplacedSolid.hh
    GDML/geometry/solid/Boolean/G4DisplacedSolid.cc
    GDML/geometry/solid/Boolean/G4IntersectionSolid.hh
    GDML/geometry/solid/Boolean/G4IntersectionSolid.cc
    GDML/geometry/solid/Boolean/G4SubtractionSolid.hh
    GDML/geometry/solid/Boolean/G4SubtractionSolid.cc
    GDML/geometry/solid/Boolean/G4UnionSolid.hh
    GDML/geometry/solid/Boolean/G4UnionSolid.cc
    )
SOURCE_GROUP("GDML\\geometry\\solid\\Boolean" FILES ${GeometrySolidBoolean_SRCS})

set(GeometrySolidSpecific_SRCS
    GDML/geometry/solid/specific/G4Polycone.hh
    GDML/geometry/solid/specific/G4Polycone.icc
    GDML/geometry/solid/specific/G4Polycone.cc
    GDML/geometry/solid/specific/G4PolyconeSide.hh
    GDML/geometry/solid/specific/G4PolyconeSide.cc
    GDML/geometry/solid/specific/G4IntersectingCone.hh
    GDML/geometry/solid/specific/G4IntersectingCone.cc
    GDML/geometry/solid/specific/G4ReduciblePolygon.hh
    GDML/geometry/solid/specific/G4ReduciblePolygon.cc
    GDML/geometry/solid/specific/G4EnclosingCylinder.hh
    GDML/geometry/solid/specific/G4EnclosingCylinder.cc
    GDML/geometry/solid/specific/G4PolyPhiFace.hh
    GDML/geometry/solid/specific/G4PolyPhiFace.icc
    GDML/geometry/solid/specific/G4PolyPhiFace.cc
    GDML/geometry/solid/specific/G4VCSGface.hh
    GDML/geometry/solid/specific/G4VCSGfaceted.hh
    GDML/geometry/solid/specific/G4VCSGfaceted.cc
    )
SOURCE_GROUP("GDML\\geometry\\solid\\specific" FILES ${GeometrySolidSpecific_SRCS})

set(GeometryVolume_SRCS
    GDML/geometry/volume/G4PVPlacement.hh
    GDML/geometry/volume/G4PVPlacement.cc
#    GDML/geometry/volume/G4PVReplica.hh
#    GDML/geometry/volume/G4PVReplica.cc
    GDML/geometry/volume/G4ReflectionFactory.hh
    GDML/geometry/volume/G4ReflectionFactory.cc
    )
SOURCE_GROUP("GDML\\geometry\\volume" FILES ${GeometryVolume_SRCS})

set(CLHEPunits_SRCS
    GDML/CLHEP/units/PhysicalConstants.h
    GDML/CLHEP/units/defs.h
    GDML/CLHEP/units/SystemOfUnits.h
    )
SOURCE_GROUP("GDML\\CLHEP\\units" FILES ${CLHEPunits_SRCS})

set(CLHEPevaluator_SRCS
    GDML/CLHEP/evaluator/Evaluator.h
    GDML/CLHEP/evaluator/Evaluator.cc
    GDML/CLHEP/evaluator/defs.h
    GDML/CLHEP/evaluator/hash_map.src
    GDML/CLHEP/evaluator/stack.src
    GDML/CLHEP/evaluator/string.src
    GDML/CLHEP/evaluator/setSystemOfUnits.cc
    GDML/CLHEP/evaluator/setStdMath.cc
    )
SOURCE_GROUP("GDML\\CLHEP\\evaluator" FILES ${CLHEPevaluator_SRCS})

set(CLHEPvector_SRCS
    GDML/CLHEP/vector/ThreeVector.h
    GDML/CLHEP/vector/ThreeVector.icc
    GDML/CLHEP/vector/ThreeVector.cc
    GDML/CLHEP/vector/defs.h
    GDML/CLHEP/vector/ZMxpv.h
    GDML/CLHEP/vector/ZMxpv.cc
    GDML/CLHEP/vector/Rotation.h
    GDML/CLHEP/vector/Rotation.icc
    GDML/CLHEP/vector/Rotation.cc
    GDML/CLHEP/vector/RotationX.h
    GDML/CLHEP/vector/RotationX.icc
    GDML/CLHEP/vector/RotationX.cc
    GDML/CLHEP/vector/RotationY.h
    GDML/CLHEP/vector/RotationY.icc
    GDML/CLHEP/vector/RotationY.cc
    GDML/CLHEP/vector/RotationZ.h
    GDML/CLHEP/vector/RotationZ.icc
    GDML/CLHEP/vector/RotationZ.cc
    GDML/CLHEP/vector/LorentzVector.h
    GDML/CLHEP/vector/LorentzVector.icc
    GDML/CLHEP/vector/LorentzVector.cc
    GDML/CLHEP/vector/LorentzRotation.h
    GDML/CLHEP/vector/LorentzRotation.icc
    GDML/CLHEP/vector/LorentzRotation.cc
    GDML/CLHEP/vector/RotationInterfaces.h
    GDML/CLHEP/vector/RotationInterfaces.icc
    GDML/CLHEP/vector/RotationInterfaces.cc
    GDML/CLHEP/vector/AxisAngle.h
    GDML/CLHEP/vector/AxisAngle.icc
    GDML/CLHEP/vector/AxisAngle.cc
    GDML/CLHEP/vector/EulerAngles.h
    GDML/CLHEP/vector/EulerAngles.icc
    GDML/CLHEP/vector/EulerAngles.cc
    GDML/CLHEP/vector/TwoVector.h
    GDML/CLHEP/vector/TwoVector.icc
    GDML/CLHEP/vector/TwoVector.cc
    )
SOURCE_GROUP("GDML\\CLHEP\\vector" FILES ${CLHEPvector_SRCS})

set(CLHEPgeometry_SRCS
    GDML/CLHEP/geometry/Transform3D.h
    GDML/CLHEP/geometry/Transform3D.icc
    GDML/CLHEP/geometry/Transform3D.cc
    GDML/CLHEP/geometry/Point3D.h
    GDML/CLHEP/geometry/Point3D.cc
    GDML/CLHEP/geometry/Normal3D.h
    GDML/CLHEP/geometry/Normal3D.cc
    GDML/CLHEP/geometry/Vector3D.h
    GDML/CLHEP/geometry/Vector3D.cc
    GDML/CLHEP/geometry/BasicVector3D.h
    GDML/CLHEP/geometry/BasicVector3D.cc
    GDML/CLHEP/geometry/Plane3D.h
    )
SOURCE_GROUP("GDML\\CLHEP\\geometry" FILES ${CLHEPgeometry_SRCS})

set(Global_SRCS
    GDML/global/G4Types.hh
    GDML/global/G4ios.hh
    GDML/global/G4ExceptionSeverity.hh
    GDML/global/G4String.hh
    GDML/global/G4String.icc
    GDML/global/geomdefs.hh
    GDML/global/templates.hh
    GDML/global/globals.hh
    GDML/global/G4ThreeVector.hh
    GDML/global/G4TwoVector.hh
    GDML/global/G4Transform3D.hh
    GDML/global/G4GeometryTolerance.hh 
    GDML/global/G4GeometryTolerance.cc
    GDML/global/G4Point3D.hh
    )
SOURCE_GROUP("GDML\\global" FILES ${Global_SRCS})

set(Graphics_Reps_SRCS
    GDML/graphics_reps/G4Polyhedron.hh 
    GDML/graphics_reps/G4Polyhedron.cc 
    GDML/graphics_reps/G4Visible.hh
    GDML/graphics_reps/G4Visible.icc
    GDML/graphics_reps/G4Visible.cc
    GDML/graphics_reps/HepPolyhedron.h 
    GDML/graphics_reps/HepPolyhedron.cc 
#    GDML/graphics_reps/G4VisAttributes.hh
#    GDML/graphics_reps/G4VisAttributes.icc
#    GDML/graphics_reps/G4VisAttributes.cc
	GDML/graphics_reps/BooleanProcessor.src
	GDML/graphics_reps/HepPolyhedronProcessor.h
	GDML/graphics_reps/HepPolyhedronProcessor.src
    )
SOURCE_GROUP("GDML\\graphics_reps" FILES ${Graphics_Reps_SRCS})

set(Persistency_SRCS
    GDML/persistency/G4GDMLEvaluator.cc
    GDML/persistency/G4GDMLEvaluator.hh
    GDML/persistency/G4GDMLParameterisation.cc
    GDML/persistency/G4GDMLParameterisation.hh
	GDML/persistency/G4GDMLParser.cc
	GDML/persistency/G4GDMLParser.hh
	GDML/persistency/G4GDMLParser.icc
	GDML/persistency/G4GDMLRead.cc
	GDML/persistency/G4GDMLRead.hh
	GDML/persistency/G4GDMLReadDefine.cc
	GDML/persistency/G4GDMLReadDefine.hh
	GDML/persistency/G4GDMLReadMaterials.cc
	GDML/persistency/G4GDMLReadMaterials.hh
	GDML/persistency/G4GDMLReadParamvol.cc
	GDML/persistency/G4GDMLReadParamvol.hh
	GDML/persistency/G4GDMLReadSetup.cc
	GDML/persistency/G4GDMLReadSetup.hh
	GDML/persistency/G4GDMLReadSolids.cc
	GDML/persistency/G4GDMLReadSolids.hh
	GDML/persistency/G4GDMLReadStructure.cc
	GDML/persistency/G4GDMLReadStructure.hh
	GDML/persistency/G4GDMLWrite.cc
	GDML/persistency/G4GDMLWrite.hh
	GDML/persistency/G4GDMLWriteDefine.cc
	GDML/persistency/G4GDMLWriteDefine.hh
	GDML/persistency/G4GDMLWriteMaterials.cc
	GDML/persistency/G4GDMLWriteMaterials.hh
	GDML/persistency/G4GDMLWriteParamvol.cc
	GDML/persistency/G4GDMLWriteParamvol.hh
	GDML/persistency/G4GDMLWriteSetup.cc
	GDML/persistency/G4GDMLWriteSetup.hh
	GDML/persistency/G4GDMLWriteSolids.cc
	GDML/persistency/G4GDMLWriteSolids.hh
	GDML/persistency/G4GDMLWriteStructure.cc
	GDML/persistency/G4GDMLWriteStructure.hh
)
SOURCE_GROUP("GDML\\persistency" FILES ${Persistency_SRCS})

set(Materials_SRCS
    GDML/materials/G4Material.cc
    GDML/materials/G4Material.hh
    GDML/materials/G4MaterialTable.hh
    GDML/materials/G4Element.cc
    GDML/materials/G4Element.hh
    GDML/materials/G4ElementTable.hh
    GDML/materials/G4ElementVector.hh
    GDML/materials/G4Isotope.cc
    GDML/materials/G4Isotope.hh
    GDML/materials/G4IsotopeVector.hh
    GDML/materials/G4MaterialPropertiesTable.hh
    GDML/materials/G4MaterialPropertiesTable.icc
    GDML/materials/G4MaterialPropertiesTable.cc
    GDML/materials/G4MaterialPropertyVector.hh
    GDML/materials/G4MaterialPropertyVector.icc
    GDML/materials/G4MaterialPropertyVector.cc
    GDML/materials/G4MPVEntry.hh
    GDML/materials/G4MPVEntry.cc
    )
SOURCE_GROUP("GDML\\materials" FILES ${Materials_SRCS})

#BEGIN for G4GDML
######################

SET(GDMLGui_SRCS
	${GDML_SRCS}
	${Graphics_Reps_SRCS}
	${GeometrySolid_SRCS}
	${GeometryManagement_SRCS}
	${GeometrySolidBoolean_SRCS}
	${GeometrySolidSpecific_SRCS}
	${GeometryVolume_SRCS}
	${CLHEPunits_SRCS}
	${CLHEPevaluator_SRCS}
	${CLHEPvector_SRCS}
	${CLHEPgeometry_SRCS}
	${Persistency_SRCS}
	${Materials_SRCS}
	${Global_SRCS}
    ${GDML_QRC_SRCS}
    AppGDMLGui.cpp
    AppGDMLGuiPy.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(GDMLGui SHARED ${GDMLGui_SRCS})
target_link_libraries(GDMLGui ${GDMLGui_LIBS})

fc_target_copy_resource(GDMLGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/GDML
    ${CMAKE_BINARY_DIR}/Mod/GDML
    Init.py)

fc_target_copy_resource(GDMLGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/GDML
    ${CMAKE_BINARY_DIR}/Mod/GDML
    InitGui.py)

if(MSVC)
    set_target_properties(GDMLGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(GDMLGui PROPERTIES DEBUG_OUTPUT_NAME "GDMLGui_d")
    set_target_properties(GDMLGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/GDML)
    set_target_properties(GDMLGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(GDMLGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(GDMLGui PROPERTIES DEBUG_OUTPUT_NAME "GDMLGui_d")
    set_target_properties(GDMLGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/GDML)
    set_target_properties(GDMLGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(GDMLGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/GDML)
    set_target_properties(GDMLGui PROPERTIES PREFIX "")
    set_target_properties(Fem PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

install(TARGETS GDMLGui DESTINATION lib)
