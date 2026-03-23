# source files
set(COIN_NODES_FILES
	SoAlphaTest.cpp
	SoAnnotation.cpp
	SoAntiSquish.cpp
	SoArray.cpp
	SoBaseColor.cpp
	SoBlinker.cpp
	SoBumpMap.cpp
	SoBumpMapCoordinate.cpp
	SoBumpMapTransform.cpp
	SoCallback.cpp
	SoCacheHint.cpp
	SoCamera.cpp
	SoClipPlane.cpp
	SoColorIndex.cpp
	SoComplexity.cpp
	SoCoordinate3.cpp
	SoCoordinate4.cpp
	SoDepthBuffer.cpp
	SoDirectionalLight.cpp
	SoDrawStyle.cpp
	SoEnvironment.cpp
	SoEventCallback.cpp
	SoExtSelection.cpp
	SoFile.cpp
	SoFont.cpp
	SoFontStyle.cpp
	SoFrustumCamera.cpp
	SoGroup.cpp
	SoInfo.cpp
	SoLOD.cpp
	SoLabel.cpp
	SoLevelOfDetail.cpp
	SoLight.cpp
	SoLightModel.cpp
	SoLinearProfile.cpp
	SoListener.cpp
	SoLocateHighlight.cpp
	SoMaterial.cpp
	SoMaterialBinding.cpp
	SoMatrixTransform.cpp
	SoMultipleCopy.cpp
	SoNode.cpp
	SoNormal.cpp
	SoNormalBinding.cpp
	SoNurbsProfile.cpp
	SoOrthographicCamera.cpp
	SoPackedColor.cpp
	SoPathSwitch.cpp
	SoPendulum.cpp
	SoPerspectiveCamera.cpp
	SoPickStyle.cpp
	SoPointLight.cpp
	SoPolygonOffset.cpp
	SoProfile.cpp
	SoProfileCoordinate2.cpp
	SoProfileCoordinate3.cpp
	SoResetTransform.cpp
	SoReversePerspectiveCamera.cpp
	SoRotation.cpp
	SoRotationXYZ.cpp
	SoRotor.cpp
	SoScale.cpp
	SoSceneTexture2.cpp
	SoSceneTextureCubeMap.cpp
	SoSelection.cpp
	SoSeparator.cpp
	SoShapeHints.cpp
	SoShuttle.cpp
	SoSpotLight.cpp
	SoSurroundScale.cpp
	SoSwitch.cpp
	SoTexture.cpp
	SoTexture2.cpp
	SoTexture3.cpp
	SoTexture2Transform.cpp
	SoTexture3Transform.cpp
	SoTextureCombine.cpp
	SoTextureCoordinate2.cpp
	SoTextureCoordinate3.cpp
	SoTextureCoordinateBinding.cpp
	SoTextureCoordinateCube.cpp
	SoTextureCoordinateCylinder.cpp
	SoTextureCoordinateSphere.cpp
	SoTextureCoordinateDefault.cpp
	SoTextureCoordinateEnvironment.cpp
	SoTextureCoordinateFunction.cpp
	SoTextureCoordinatePlane.cpp
	SoTextureCoordinateNormalMap.cpp
	SoTextureCoordinateReflectionMap.cpp
	SoTextureCoordinateObject.cpp
	SoTextureCubeMap.cpp
	SoTextureMatrixTransform.cpp
	SoTextureScalePolicy.cpp
	SoTextureUnit.cpp
	SoTransform.cpp
	SoTransparencyType.cpp
	SoTransformSeparator.cpp
	SoTransformation.cpp
	SoTranslation.cpp
	SoUnits.cpp
	SoUnknownNode.cpp
	SoVertexProperty.cpp
	SoVertexAttribute.cpp
	SoVertexAttributeBinding.cpp
	SoWWWAnchor.cpp
	SoWWWInline.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_NODES_INTERNAL_FILES
	SoSoundElementHelper.h
	SoSubNodeP.h
	SoUnknownNode.h
	SoUnknownNode.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_NODES_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} nodes/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(nodes OBJECT ${COIN_NODES_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_NODES_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/nodes/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_NODES_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/nodes/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
