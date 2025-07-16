add_executable(Part_tests_run
        Attacher.cpp
        AttachExtension.cpp
        BRepMesh.cpp
        FeatureChamfer.cpp
        FeatureCompound.cpp
        FeatureExtrusion.cpp
        FeatureFillet.cpp
        FeatureMirroring.cpp
        FeatureOffset.cpp
        FeaturePartBoolean.cpp
        FeaturePartCommon.cpp
        FeaturePartCut.cpp
        FeaturePartFuse.cpp
        FeatureRevolution.cpp
        FuzzyBoolean.cpp
        Geometry.cpp
        PartFeature.cpp
        PartFeatures.cpp
        PartTestHelpers.cpp
        PropertyTopoShape.cpp
        TopoDS_Shape.cpp
        TopoShape.cpp
        TopoShapeCache.cpp
        TopoShapeExpansion.cpp
        TopoShapeMakeElementRefine.cpp
        TopoShapeMakeShapeWithElementMap.cpp
        TopoShapeMapper.cpp
        TopoShapeMakeShape.cpp
        WireJoiner.cpp
)

set(PartTestData_Files
        brepfiles/cylinder1.brep
        brepfiles/helix1.brep
)

ADD_CUSTOM_TARGET(PartTestData ALL
        SOURCES ${PartTestData_Files}
)

fc_target_copy_resource(PartTestData
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/tests
        ${PartTestData_Files})
