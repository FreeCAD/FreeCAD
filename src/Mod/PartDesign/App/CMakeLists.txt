if(MSVC)
    add_definitions(-DFCAppPartDesign -DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${QT_INCLUDE_DIR}
)
link_directories(${OCC_LIBRARY_DIR})

generate_from_xml(BodyPy)
generate_from_xml(FeaturePy)


set(PartDesign_LIBS
    ${OCC_LIBRARIES}
    ${OCC_DEBUG_LIBRARIES}
    Part
    FreeCADApp
)

SET(Features_SRCS
    Feature.cpp
    Feature.h
    FeatureSolid.cpp
    FeatureSolid.h
    Body.cpp
    Body.h
    FeatureBase.h
    FeatureBase.cpp
)
SOURCE_GROUP("Features" FILES ${Features_SRCS})

SET(DatumFeatures_SRCS
    ShapeBinder.h
    ShapeBinder.cpp
    DatumPlane.cpp
    DatumPlane.h
    DatumLine.cpp
    DatumLine.h
    DatumPoint.cpp
    DatumPoint.h
    DatumCS.h
    DatumCS.cpp
)
SOURCE_GROUP("DatumFeatures" FILES ${DatumFeatures_SRCS})

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
    FeatureThickness.cpp
    FeatureThickness.h
)
SOURCE_GROUP("DressUpFeatures" FILES ${FeaturesDressUp_SRCS})

SET(FeaturesSketchBased_SRCS
    FeatureSketchBased.cpp
    FeatureSketchBased.h
    FeaturePad.cpp
    FeaturePad.h
    FeaturePocket.cpp
    FeaturePocket.h
    FeatureRevolution.cpp
    FeatureRevolution.h
    FeatureGroove.cpp
    FeatureGroove.h
    FeatureAddSub.cpp
    FeatureAddSub.h
    FeatureHole.h
    FeatureHole.cpp
    FeatureBoolean.h
    FeatureBoolean.cpp
    FeaturePrimitive.h
    FeaturePrimitive.cpp
    FeaturePipe.h
    FeaturePipe.cpp
    FeatureLoft.h
    FeatureLoft.cpp
)

SOURCE_GROUP("SketchBasedFeatures" FILES ${FeaturesSketchBased_SRCS})

SET(Module_SRCS
    AppPartDesign.cpp
    AppPartDesignPy.cpp
    PreCompiled.cpp
    PreCompiled.h
)
SOURCE_GROUP("Module" FILES ${Module_SRCS})


SET(Python_SRCS
    BodyPy.xml
    BodyPyImp.cpp
    FeaturePy.xml
    FeaturePyImp.cpp

)
SOURCE_GROUP("Python" FILES ${Python_SRCS})

SET(PartDesign_SRCS
    ${Features_SRCS}
    ${DatumFeatures_SRCS}
    ${FeaturesTransformed_SRCS}
    ${FeaturesSketchBased_SRCS}
    ${FeaturesDressUp_SRCS}
    ${Module_SRCS}
	${Python_SRCS}
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
    PartDesignTests/__init__.py
    PartDesignTests/TestDatum.py
    PartDesignTests/TestShapeBinder.py
    PartDesignTests/TestPad.py
    PartDesignTests/TestPocket.py
    PartDesignTests/TestHole.py
    PartDesignTests/TestRevolve.py
    PartDesignTests/TestLoft.py
    PartDesignTests/TestPipe.py
    PartDesignTests/TestPrimitive.py
    PartDesignTests/TestMirrored.py
    PartDesignTests/TestLinearPattern.py
    PartDesignTests/TestPolarPattern.py
    PartDesignTests/TestMultiTransform.py
    PartDesignTests/TestBoolean.py
    PartDesignTests/TestFillet.py
    PartDesignTests/TestChamfer.py
    PartDesignTests/TestDraft.py
    PartDesignTests/TestThickness.py
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

