
target_sources(
    Part_tests_run
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/FeatureChamfer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/FeatureCompound.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/FeatureExtrusion.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/FeatureFillet.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/FeaturePartBoolean.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/FeaturePartCommon.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/FeaturePartCut.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/FeaturePartFuse.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/FeatureRevolution.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/PartTestHelpers.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/TopoShape.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/TopoShapeCache.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/TopoShapeExpansion.cpp
)
