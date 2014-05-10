if(MSVC)
    add_definitions(-DFCAppPartDesign -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

set(PartDesign_LIBS
    ${OCC_LIBRARIES}
    ${OCC_DEBUG_LIBRARIES}
    Part
    FreeCADApp
)

SET(Features_SRCS
    Feature.cpp
    Feature.h
    Body.cpp
    Body.h
)
SOURCE_GROUP("Features" FILES ${Features_SRCS})

SET(FeaturesTransformed_SRCS
    FeatureTransformed.h
    FeatureTransformed.cpp
    FeatureMirrored.h
    FeatureMirrored.cpp
    FeatureLinearPattern.h
    FeatureLinearPattern.cpp
    FeaturePolarPattern.h
    FeaturePolarPattern.cpp
    FeatureScaled.h
    FeatureScaled.cpp
    FeatureMultiTransform.h
    FeatureMultiTransform.cpp
)
SOURCE_GROUP("FeaturesTransformed" FILES ${FeaturesTransformed_SRCS})

SET(FeaturesDressUp_SRCS
    FeatureDressUp.cpp
    FeatureDressUp.h
    FeatureFillet.cpp
    FeatureFillet.h
    FeatureChamfer.cpp
    FeatureChamfer.h
    FeatureDraft.cpp
    FeatureDraft.h
)
SOURCE_GROUP("DressUpFeatures" FILES ${FeaturesDressUp_SRCS})

SET(FeaturesSketchBased_SRCS
    FeatureSketchBased.cpp
    FeatureSketchBased.h
    FeatureFace.cpp
    FeatureFace.h
    FeaturePad.cpp
    FeaturePad.h
    FeaturePocket.cpp
    FeaturePocket.h
    FeatureRevolution.cpp
    FeatureRevolution.h
    FeatureGroove.cpp
    FeatureGroove.h
    FeatureAdditive.cpp
    FeatureAdditive.h
    FeatureSubtractive.h
    FeatureSubtractive.cpp
    FeatureHole.h
    FeatureHole.cpp
)

SOURCE_GROUP("SketchBasedFeatures" FILES ${FeaturesSketchBased_SRCS})

SET(Module_SRCS
    AppPartDesign.cpp
    AppPartDesignPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})

SET(PartDesign_SRCS
    ${Features_SRCS}
    ${FeaturesTransformed_SRCS}
    ${FeaturesSketchBased_SRCS}
    ${FeaturesDressUp_SRCS}
    ${Module_SRCS}
)

SET(PartDesign_Scripts
    __init__.py
    Init.py
    TestPartDesignApp.py
    InvoluteGearFeature.py
    InvoluteGearFeature.ui
    Scripts/__init__.py
    Scripts/Gear.py
    Scripts/DistanceBolt.py
    Scripts/Epitrochoid.py
    Scripts/FilletArc.py
    Scripts/RadialCopy.py
    Scripts/Parallelepiped.py
    Scripts/Spring.py
    fcgear/__init__.py
    fcgear/fcgear.py
    fcgear/fcgeardialog.py
    fcgear/involute.py
    fcgear/svggear.py
)

add_library(PartDesign SHARED ${PartDesign_SRCS})
target_link_libraries(PartDesign ${PartDesign_LIBS})


fc_target_copy_resource(PartDesign 
    ${CMAKE_SOURCE_DIR}/src/Mod/PartDesign
    ${CMAKE_BINARY_DIR}/Mod/PartDesign
    ${PartDesign_Scripts})

SET_BIN_DIR(PartDesign _PartDesign /Mod/PartDesign)
SET_PYTHON_PREFIX_SUFFIX(PartDesign)

INSTALL(TARGETS PartDesign DESTINATION ${CMAKE_INSTALL_LIBDIR})

