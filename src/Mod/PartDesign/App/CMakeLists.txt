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
    ${CMAKE_SOURCE_DIR}/src/3rdParty/json/single_include/nlohmann/
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
    FeatureExtrude.cpp
    FeatureExtrude.h
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
    FeatureHelix.h
    FeatureHelix.cpp
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

add_library(PartDesign SHARED ${PartDesign_SRCS})
target_link_libraries(PartDesign ${PartDesign_LIBS})

if(FREECAD_USE_PCH)
    add_definitions(-D_PreComp_)
    GET_MSVC_PRECOMPILED_SOURCE("PreCompiled.cpp" PartDesign_CPP_SRCS ${PartDesign_SRCS})
    ADD_MSVC_PRECOMPILED_HEADER(PartDesign PreCompiled.h PreCompiled.cpp PartDesign_CPP_SRCS)
endif(FREECAD_USE_PCH)

SET_BIN_DIR(PartDesign _PartDesign /Mod/PartDesign)
SET_PYTHON_PREFIX_SUFFIX(PartDesign)

INSTALL(TARGETS PartDesign DESTINATION ${CMAKE_INSTALL_LIBDIR})

