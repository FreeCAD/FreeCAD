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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

#include <Inventor/VRMLnodes/SoVRML.h>
#include <Inventor/VRMLnodes/SoVRMLNodes.h>

void
so_vrml_init(void)
{
  SoVRMLGeometry::initClass();
  SoVRMLVertexPoint::initClass();
  SoVRMLVertexShape::initClass();
  SoVRMLIndexedShape::initClass();

  SoVRMLParent::initClass();
  SoVRMLGroup::initClass();

  SoVRMLTexture::initClass();

  SoVRMLInterpolator::initClass();

  SoVRMLLight::initClass();
  
  SoVRMLSensor::initClass();
  SoVRMLDragSensor::initClass();

  SoVRMLAnchor::initClass();
  SoVRMLAppearance::initClass();
  SoVRMLAudioClip::initClass();
  SoVRMLBackground::initClass();
  SoVRMLBillboard::initClass();
  SoVRMLBox::initClass();
  SoVRMLCollision::initClass();
  SoVRMLColor::initClass();
  SoVRMLColorInterpolator::initClass();
  SoVRMLCone::initClass();
  SoVRMLCoordinate::initClass();
  SoVRMLCoordinateInterpolator::initClass();
  SoVRMLCylinder::initClass();
  SoVRMLCylinderSensor::initClass();
  SoVRMLDirectionalLight::initClass();
  SoVRMLElevationGrid::initClass();
  SoVRMLExtrusion::initClass();
  SoVRMLFog::initClass();
  SoVRMLFontStyle::initClass();
  SoVRMLImageTexture::initClass();
  SoVRMLIndexedFaceSet::initClass();

  SoVRMLVertexLine::initClass();
  SoVRMLIndexedLine::initClass();
  SoVRMLIndexedLineSet::initClass();
  SoVRMLInline::initClass();
  SoVRMLLOD::initClass();
  SoVRMLShape::initClass();
  SoVRMLMaterial::initClass();
  SoVRMLMovieTexture::initClass();
  SoVRMLNavigationInfo::initClass();
  SoVRMLNormal::initClass();
  SoVRMLNormalInterpolator::initClass();
  SoVRMLOrientationInterpolator::initClass();
  SoVRMLPixelTexture::initClass();
  SoVRMLPlaneSensor::initClass();
  SoVRMLPointLight::initClass();
  SoVRMLPointSet::initClass();
  SoVRMLPositionInterpolator::initClass();
  SoVRMLProximitySensor::initClass();
  SoVRMLScalarInterpolator::initClass();
  SoVRMLScript::initClass();
  SoVRMLSound::initClass();
  SoVRMLSphere::initClass();
  SoVRMLSphereSensor::initClass();
  SoVRMLSpotLight::initClass();
  SoVRMLSwitch::initClass();
  SoVRMLText::initClass();
  SoVRMLTextureCoordinate::initClass();
  SoVRMLTextureTransform::initClass();
  SoVRMLTimeSensor::initClass();
  SoVRMLTouchSensor::initClass();
  SoVRMLTransform::initClass();
  SoVRMLViewpoint::initClass();
  SoVRMLVisibilitySensor::initClass();
  SoVRMLWorldInfo::initClass();
}

#endif // HAVE_VRML97
