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

#ifndef SO_DIRECTIONALLIGHTDRAGGER_IV_H
#define SO_DIRECTIONALLIGHTDRAGGER_IV_H

static const char DIRECTIONALLIGHTDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "DEF DIRECTIONALLIGHT_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF DIRECTIONALLIGHT_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF directionalLightOverallMaterial Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "\n"
  "DEF DIRECTIONALLIGHT_ARROW Separator {\n"
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
  "DEF directionalLightRotatorRotator Separator {\n"
  "   USE DIRECTIONALLIGHT_INACTIVE_MATERIAL\n"
  "   USE DIRECTIONALLIGHT_ARROW\n"
  "}\n"
  "DEF directionalLightRotatorRotatorActive Separator {\n"
  "   USE DIRECTIONALLIGHT_ACTIVE_MATERIAL\n"
  "   USE DIRECTIONALLIGHT_ARROW\n"
  "}\n"
  "\n"
  "DEF directionalLightRotatorFeedback Separator { }\n"
  "DEF directionalLightRotatorFeedbackActive Separator { }\n"
  "\n"
  "\n"
  "DEF directionalLightTranslatorPlaneTranslator Separator {\n"
  "   USE DIRECTIONALLIGHT_INACTIVE_MATERIAL\n"
  "   Sphere { }\n"
  "}\n"
  "DEF directionalLightTranslatorPlaneTranslatorActive Separator {\n"
  "   USE DIRECTIONALLIGHT_ACTIVE_MATERIAL\n"
  "   Sphere { }\n"
  "}\n"
  "\n"
  "DEF DIRECTIONALLIGHT_TRANSLATE_AXIS Group {\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   Cylinder { height 3  radius 0.2 }\n"
  "}\n"
  "\n"
  "DEF directionalLightTranslatorLineTranslator Separator {\n"
  "   USE DIRECTIONALLIGHT_INACTIVE_MATERIAL\n"
  "   USE DIRECTIONALLIGHT_TRANSLATE_AXIS\n"
  "}\n"
  "DEF directionalLightTranslatorLineTranslatorActive Separator {\n"
  "   USE DIRECTIONALLIGHT_ACTIVE_MATERIAL\n"
  "   USE DIRECTIONALLIGHT_TRANSLATE_AXIS\n"
  "}\n";

#endif /* ! SO_DIRECTIONALLIGHTDRAGGER_IV_H */
