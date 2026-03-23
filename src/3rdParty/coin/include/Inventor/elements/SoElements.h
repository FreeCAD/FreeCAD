#ifndef COIN_SOELEMENTS_H
#define COIN_SOELEMENTS_H

/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include <Inventor/elements/SoElement.h>
#include <Inventor/elements/SoAccumulatedElement.h>
#include <Inventor/elements/SoClipPlaneElement.h>
#include <Inventor/elements/SoGLClipPlaneElement.h>
#include <Inventor/elements/SoLightElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoBBoxModelMatrixElement.h>
#include <Inventor/elements/SoGLModelMatrixElement.h>
#include <Inventor/elements/SoProfileElement.h>
#ifndef COIN_INTERNAL
#include <Inventor/elements/SoTextureMatrixElement.h>
#include <Inventor/elements/SoGLTextureMatrixElement.h>
#endif // COIN_INTERNAL
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoInt32Element.h>
#include <Inventor/elements/SoAnnoText3CharOrientElement.h>
#include <Inventor/elements/SoAnnoText3FontSizeHintElement.h>
#include <Inventor/elements/SoAnnoText3RenderPrintElement.h>
#include <Inventor/elements/SoComplexityTypeElement.h>
#include <Inventor/elements/SoDecimationTypeElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoGLDrawStyleElement.h>
#include <Inventor/elements/SoGLLightIdElement.h>
#ifndef COIN_INTERNAL
#include <Inventor/elements/SoGLTextureEnabledElement.h>
#endif // COIN_INTERNAL
#include <Inventor/elements/SoLinePatternElement.h>
#include <Inventor/elements/SoGLLinePatternElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoNormalBindingElement.h>
#include <Inventor/elements/SoPickStyleElement.h>
#include <Inventor/elements/SoSwitchElement.h>
#include <Inventor/elements/SoTextOutlineEnabledElement.h>
#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
#include <Inventor/elements/SoUnitsElement.h>
#include <Inventor/elements/SoFloatElement.h>
#include <Inventor/elements/SoComplexityElement.h>
#include <Inventor/elements/SoCreaseAngleElement.h>
#include <Inventor/elements/SoDecimationPercentageElement.h>
#include <Inventor/elements/SoFocalDistanceElement.h>
#include <Inventor/elements/SoFontSizeElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoGLLineWidthElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/elements/SoGLPointSizeElement.h>
#include <Inventor/elements/SoTextureQualityElement.h>
#include <Inventor/elements/SoGLRenderPassElement.h>
#include <Inventor/elements/SoGLUpdateAreaElement.h>
#include <Inventor/elements/SoLocalBBoxMatrixElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoPickRayElement.h>
#include <Inventor/elements/SoReplacedElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoGLCoordinateElement.h>
#include <Inventor/elements/SoEnvironmentElement.h>
#include <Inventor/elements/SoGLEnvironmentElement.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoLightAttenuationElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoGLNormalElement.h>
#include <Inventor/elements/SoPolygonOffsetElement.h>
#include <Inventor/elements/SoGLPolygonOffsetElement.h>
#include <Inventor/elements/SoProjectionMatrixElement.h>
#include <Inventor/elements/SoGLProjectionMatrixElement.h>
#include <Inventor/elements/SoProfileCoordinateElement.h>
#ifndef COIN_INTERNAL
#include <Inventor/elements/SoTextureCoordinateElement.h>
#include <Inventor/elements/SoGLTextureCoordinateElement.h>
#include <Inventor/elements/SoTextureImageElement.h>
#include <Inventor/elements/SoGLTextureImageElement.h>
#endif // COIN_INTERNAL
#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoGLViewingMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoGLShapeHintsElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoGLViewportRegionElement.h>
#include <Inventor/elements/SoWindowElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoGLColorIndexElement.h>
#include <Inventor/elements/SoListenerPositionElement.h>
#include <Inventor/elements/SoListenerOrientationElement.h>
#include <Inventor/elements/SoListenerDopplerElement.h>
#include <Inventor/elements/SoListenerGainElement.h>
#include <Inventor/elements/SoSoundElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/elements/SoGLDepthBufferElement.h>
#include <Inventor/elements/SoGLVertexAttributeElement.h>
#include <Inventor/elements/SoVertexAttributeBindingElement.h>

// elements still supported by Coin that are not in SGI Inventor >= 2.1
#include <Inventor/elements/SoAmbientColorElement.h>
#include <Inventor/elements/SoDiffuseColorElement.h>
#include <Inventor/elements/SoSpecularColorElement.h>
#include <Inventor/elements/SoEmissiveColorElement.h>
#include <Inventor/elements/SoShininessElement.h>
#include <Inventor/elements/SoTransparencyElement.h>
#include <Inventor/elements/SoLightModelElement.h>

#endif // !COIN_SOELEMENTS_H
