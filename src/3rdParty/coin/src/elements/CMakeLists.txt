add_subdirectory(GL)

# source files
set(COIN_ELEMENTS_FILES
	SoAccumulatedElement.cpp
	SoAmbientColorElement.cpp
	SoAnnoText3CharOrientElement.cpp
	SoAnnoText3FontSizeHintElement.cpp
	SoAnnoText3RenderPrintElement.cpp
	SoBBoxModelMatrixElement.cpp
	SoBumpMapElement.cpp
	SoBumpMapCoordinateElement.cpp
	SoBumpMapMatrixElement.cpp
	SoBumpMappingPropertyElement.cpp
	SoCacheElement.cpp
	SoCacheHintElement.cpp
	SoClipPlaneElement.cpp
	SoComplexityElement.cpp
	SoComplexityTypeElement.cpp
	SoCoordinateElement.cpp
	SoCreaseAngleElement.cpp
	SoCullElement.cpp
	SoDecimationPercentageElement.cpp
	SoDecimationTypeElement.cpp
	SoDepthBufferElement.cpp
	SoDiffuseColorElement.cpp
	SoDrawStyleElement.cpp
	SoElement.cpp
	SoEmissiveColorElement.cpp
	SoEnvironmentElement.cpp
	SoFloatElement.cpp
	SoFocalDistanceElement.cpp
	SoFontNameElement.cpp
	SoFontSizeElement.cpp
	SoInt32Element.cpp
	SoLazyElement.cpp
	SoLightAttenuationElement.cpp
	SoLightElement.cpp
	SoLightModelElement.cpp
	SoLinePatternElement.cpp
	SoLineWidthElement.cpp
	SoLocalBBoxMatrixElement.cpp
	SoMaterialBindingElement.cpp
	SoVertexAttributeBindingElement.cpp
	SoMultiTextureCoordinateElement.cpp
	SoMultiTextureEnabledElement.cpp
	SoMultiTextureMatrixElement.cpp
	SoMultiTextureImageElement.cpp
	SoModelMatrixElement.cpp
	SoNormalBindingElement.cpp
	SoNormalElement.cpp
	SoOverrideElement.cpp
	SoPickRayElement.cpp
	SoPickStyleElement.cpp
	SoPointSizeElement.cpp
	SoPolygonOffsetElement.cpp
	SoProfileCoordinateElement.cpp
	SoProfileElement.cpp
	SoProjectionMatrixElement.cpp
	SoReplacedElement.cpp
	SoShapeHintsElement.cpp
	SoShapeStyleElement.cpp
	SoShininessElement.cpp
	SoSpecularColorElement.cpp
	SoSwitchElement.cpp
	SoTextOutlineEnabledElement.cpp
	SoTextureCombineElement.cpp
	SoTextureCoordinateBindingElement.cpp
	SoTextureOverrideElement.cpp
	SoTextureScalePolicyElement.cpp
	SoTextureScaleQualityElement.cpp
	SoTextureUnitElement.cpp
	SoTextureQualityElement.cpp
	SoTransparencyElement.cpp
	SoUnitsElement.cpp
	SoViewVolumeElement.cpp
	SoViewingMatrixElement.cpp
	SoViewportRegionElement.cpp
	SoWindowElement.cpp
	SoListenerPositionElement.cpp
	SoListenerOrientationElement.cpp
	SoListenerGainElement.cpp
	SoListenerDopplerElement.cpp
	SoSoundElement.cpp
	SoVertexAttributeElement.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_ELEMENTS_INTERNAL_FILES
	SoTextureScalePolicyElement.h
	SoTextureScalePolicyElement.cpp
	SoTextureScaleQualityElement.h
	SoTextureScaleQualityElement.cpp
	SoVertexAttributeData.h
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_ELEMENTS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} elements/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(elements OBJECT ${COIN_ELEMENTS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_ELEMENTS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/elements/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_ELEMENTS_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/elements/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
