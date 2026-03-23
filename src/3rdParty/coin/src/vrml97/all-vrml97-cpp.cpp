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
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

#include "vrml97/Anchor.cpp"
#include "vrml97/Appearance.cpp"
#include "vrml97/AudioClip.cpp"
#include "vrml97/Background.cpp"
#include "vrml97/Billboard.cpp"
#include "vrml97/Box.cpp"
#include "vrml97/Collision.cpp"
#include "vrml97/Color.cpp"
#include "vrml97/ColorInterpolator.cpp"
#include "vrml97/Cone.cpp"
#include "vrml97/Coordinate.cpp"
#include "vrml97/CoordinateInterpolator.cpp"
#include "vrml97/Cylinder.cpp"
#include "vrml97/CylinderSensor.cpp"
#include "vrml97/DirectionalLight.cpp"
#include "vrml97/DragSensor.cpp"
#include "vrml97/ElevationGrid.cpp"
#include "vrml97/Extrusion.cpp"
#include "vrml97/Fog.cpp"
#include "vrml97/FontStyle.cpp"
#include "vrml97/Geometry.cpp"
#include "vrml97/Group.cpp"
#include "vrml97/ImageTexture.cpp"
#include "vrml97/IndexedFaceSet.cpp"
#include "vrml97/IndexedLine.cpp"
#include "vrml97/IndexedLineSet.cpp"
#include "vrml97/IndexedShape.cpp"
#include "vrml97/Init.cpp"
#include "vrml97/Inline.cpp"
#include "vrml97/Interpolator.cpp"
#include "vrml97/LOD.cpp"
#include "vrml97/Light.cpp"
#include "vrml97/Material.cpp"
#include "vrml97/MovieTexture.cpp"
#include "vrml97/NavigationInfo.cpp"
#include "vrml97/Normal.cpp"
#include "vrml97/NormalInterpolator.cpp"
#include "vrml97/OrientationInterpolator.cpp"
#include "vrml97/Parent.cpp"
#include "vrml97/PixelTexture.cpp"
#include "vrml97/PlaneSensor.cpp"
#include "vrml97/PointLight.cpp"
#include "vrml97/PointSet.cpp"
#include "vrml97/PositionInterpolator.cpp"
#include "vrml97/ProximitySensor.cpp"
#include "vrml97/ScalarInterpolator.cpp"
#include "vrml97/Script.cpp"
#include "vrml97/Sensor.cpp"
#include "vrml97/Shape.cpp"
#include "vrml97/Sound.cpp"
#include "vrml97/Sphere.cpp"
#include "vrml97/SphereSensor.cpp"
#include "vrml97/SpotLight.cpp"
#include "vrml97/Switch.cpp"
#include "vrml97/Text.cpp"
#include "vrml97/Texture.cpp"
#include "vrml97/TextureCoordinate.cpp"
#include "vrml97/TextureTransform.cpp"
#include "vrml97/TimeSensor.cpp"
#include "vrml97/TouchSensor.cpp"
#include "vrml97/Transform.cpp"
#include "vrml97/VertexLine.cpp"
#include "vrml97/VertexPoint.cpp"
#include "vrml97/VertexShape.cpp"
#include "vrml97/Viewpoint.cpp"
#include "vrml97/VisibilitySensor.cpp"
#include "vrml97/WorldInfo.cpp"
#include "vrml97/JS_VRMLClasses.cpp"

#endif // HAVE_VRML97
