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

#ifndef COIN_SOVRMLNODES_H
#define COIN_SOVRMLNODES_H

#include <Inventor/VRMLnodes/SoVRMLAnchor.h>
#include <Inventor/VRMLnodes/SoVRMLAppearance.h>
#include <Inventor/VRMLnodes/SoVRMLAudioClip.h>
#include <Inventor/VRMLnodes/SoVRMLBackground.h>
#include <Inventor/VRMLnodes/SoVRMLBillboard.h>
#include <Inventor/VRMLnodes/SoVRMLBox.h>
#include <Inventor/VRMLnodes/SoVRMLCollision.h>
#include <Inventor/VRMLnodes/SoVRMLColor.h>
#include <Inventor/VRMLnodes/SoVRMLColorInterpolator.h>
#include <Inventor/VRMLnodes/SoVRMLCone.h>
#include <Inventor/VRMLnodes/SoVRMLCoordinate.h>
#include <Inventor/VRMLnodes/SoVRMLCoordinateInterpolator.h>
#include <Inventor/VRMLnodes/SoVRMLCylinder.h>
#include <Inventor/VRMLnodes/SoVRMLCylinderSensor.h>
#include <Inventor/VRMLnodes/SoVRMLDirectionalLight.h>
#include <Inventor/VRMLnodes/SoVRMLElevationGrid.h>
#include <Inventor/VRMLnodes/SoVRMLExtrusion.h>
#include <Inventor/VRMLnodes/SoVRMLFog.h>
#include <Inventor/VRMLnodes/SoVRMLFontStyle.h>
#include <Inventor/VRMLnodes/SoVRMLGeometry.h>
#include <Inventor/VRMLnodes/SoVRMLGroup.h>
#include <Inventor/VRMLnodes/SoVRMLImageTexture.h>
#include <Inventor/VRMLnodes/SoVRMLIndexedFaceSet.h>
#include <Inventor/VRMLnodes/SoVRMLIndexedLine.h>
#include <Inventor/VRMLnodes/SoVRMLIndexedLineSet.h>
#include <Inventor/VRMLnodes/SoVRMLIndexedShape.h>
#include <Inventor/VRMLnodes/SoVRMLInline.h>
//#include <Inventor/VRMLnodes/SoVRMLInterpOutput.h>
//#include <Inventor/VRMLnodes/SoVRMLInterpOutputData.h>
#include <Inventor/VRMLnodes/SoVRMLInterpolator.h>
#include <Inventor/VRMLnodes/SoVRMLLOD.h>
#include <Inventor/VRMLnodes/SoVRMLLight.h>
#include <Inventor/VRMLnodes/SoVRMLMaterial.h>
#include <Inventor/VRMLnodes/SoVRMLMovieTexture.h>
#include <Inventor/VRMLnodes/SoVRMLNavigationInfo.h>
#include <Inventor/VRMLnodes/SoVRMLNormal.h>
#include <Inventor/VRMLnodes/SoVRMLNormalInterpolator.h>
#include <Inventor/VRMLnodes/SoVRMLOrientationInterpolator.h>
// #include <Inventor/VRMLnodes/SoVRMLOutputData.h>
#include <Inventor/VRMLnodes/SoVRMLParent.h>
#include <Inventor/VRMLnodes/SoVRMLPixelTexture.h>
#include <Inventor/VRMLnodes/SoVRMLPlaneSensor.h>
#include <Inventor/VRMLnodes/SoVRMLPointLight.h>
#include <Inventor/VRMLnodes/SoVRMLPointSet.h>
#include <Inventor/VRMLnodes/SoVRMLPositionInterpolator.h>
#include <Inventor/VRMLnodes/SoVRMLProximitySensor.h>
#include <Inventor/VRMLnodes/SoVRMLScalarInterpolator.h>
#include <Inventor/VRMLnodes/SoVRMLScript.h>
#include <Inventor/VRMLnodes/SoVRMLShape.h>
#include <Inventor/VRMLnodes/SoVRMLSound.h>
#include <Inventor/VRMLnodes/SoVRMLSphere.h>
#include <Inventor/VRMLnodes/SoVRMLSphereSensor.h>
#include <Inventor/VRMLnodes/SoVRMLSpotLight.h>
#include <Inventor/VRMLnodes/SoVRMLSwitch.h>
#include <Inventor/VRMLnodes/SoVRMLText.h>
#include <Inventor/VRMLnodes/SoVRMLTexture.h>
#include <Inventor/VRMLnodes/SoVRMLTextureCoordinate.h>
#include <Inventor/VRMLnodes/SoVRMLTextureTransform.h>
#include <Inventor/VRMLnodes/SoVRMLTimeSensor.h>
#include <Inventor/VRMLnodes/SoVRMLTouchSensor.h>
#include <Inventor/VRMLnodes/SoVRMLTransform.h>
#include <Inventor/VRMLnodes/SoVRMLVertexLine.h>
#include <Inventor/VRMLnodes/SoVRMLVertexPoint.h>
#include <Inventor/VRMLnodes/SoVRMLVertexShape.h>
#include <Inventor/VRMLnodes/SoVRMLViewpoint.h>
#include <Inventor/VRMLnodes/SoVRMLVisibilitySensor.h>
#include <Inventor/VRMLnodes/SoVRMLWorldInfo.h>

#endif // COIN_SOVRMLNODES
