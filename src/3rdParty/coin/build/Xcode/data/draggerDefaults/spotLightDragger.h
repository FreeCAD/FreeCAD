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

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* !COIN_INTERNAL */

#ifndef SO_SPOTLIGHTDRAGGER_IV_H
#define SO_SPOTLIGHTDRAGGER_IV_H

static const char SPOTLIGHTDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "DEF SPOTLIGHT_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF SPOTLIGHT_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF spotLightOverallMaterial Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "\n"
  "\n"
  "DEF SPOTLIGHT_ARROW Separator {\n"
  "\n"
  "   Rotation { rotation 1 0 0  -1.57 }\n"
  "\n"
  "   Coordinate3 { point [ 0 0 0, 0 9 0 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1 ] }\n"
  "   Translation { translation 0 10 0 }\n"
  "   Cone { }\n"
  "   Translation { translation 0 -11 0 }\n"
  "   Cone { bottomRadius 0.1 }\n"
  "}\n"
  "\n"
  "DEF spotLightRotatorRotator Separator {\n"
  "   USE SPOTLIGHT_INACTIVE_MATERIAL\n"
  "   USE SPOTLIGHT_ARROW\n"
  "}\n"
  "DEF spotLightRotatorRotatorActive Separator {\n"
  "   USE SPOTLIGHT_ACTIVE_MATERIAL\n"
  "   USE SPOTLIGHT_ARROW\n"
  "}\n"
  "\n"
  "DEF spotLightRotatorFeedback Separator { }\n"
  "DEF spotLightRotatorFeedbackActive Separator { }\n"
  "\n"
  "\n"
  "DEF SPOTLIGHT_TRANSLATE_AXIS Group {\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   Cylinder { height 3.0  radius 0.2 }\n"
  "}\n"
  "\n"
  "DEF spotLightTranslatorLineTranslator Separator {\n"
  "   USE SPOTLIGHT_INACTIVE_MATERIAL\n"
  "   USE SPOTLIGHT_TRANSLATE_AXIS\n"
  "}\n"
  "DEF spotLightTranslatorLineTranslatorActive Separator {\n"
  "   USE SPOTLIGHT_ACTIVE_MATERIAL\n"
  "   USE SPOTLIGHT_TRANSLATE_AXIS\n"
  "}\n"
  "\n"
  "DEF spotLightTranslatorPlaneTranslator Separator {\n"
  "   USE SPOTLIGHT_INACTIVE_MATERIAL\n"
  "   Sphere { }\n"
  "}\n"
  "DEF spotLightTranslatorPlaneTranslatorActive Separator {\n"
  "   USE SPOTLIGHT_ACTIVE_MATERIAL\n"
  "   Sphere { }\n"
  "}\n"
  "\n"
  "\n"
  "DEF SPOTLIGHT_CONEANGLE Separator {\n"
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "   ShapeHints { vertexOrdering UNKNOWN_ORDERING }\n"
  "   Cone { parts SIDES  bottomRadius 2.0 }\n"
  "}\n"
  "\n"
  "DEF spotLightBeam Separator {\n"
  "   USE SPOTLIGHT_INACTIVE_MATERIAL\n"
  "   DrawStyle { style LINES  lineWidth 2 }\n"
  "   USE SPOTLIGHT_CONEANGLE\n"
  "}\n"
  "DEF spotLightBeamActive Separator {\n"
  "   USE SPOTLIGHT_ACTIVE_MATERIAL\n"
  "   DrawStyle { style LINES  lineWidth 3 }\n"
  "   USE SPOTLIGHT_CONEANGLE\n"
  "}\n"
  "\n"
  "DEF spotLightBeamPlacement Translation { translation 0 0 -1.5 }\n";

#endif /* ! SO_SPOTLIGHTDRAGGER_IV_H */
