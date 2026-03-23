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

#ifndef SO_POINTLIGHTDRAGGER_IV_H
#define SO_POINTLIGHTDRAGGER_IV_H

static const char POINTLIGHTDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF pointLightOverallMaterial Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF POINTLIGHT_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "\n"
  "\n"
  "\n"
  "DEF POINTLIGHT_AXIS_TRANSLATOR Cube { width 4  height 0.5  depth 0.5 }\n"
  "DEF POINTLIGHT_PLANE_TRANSLATOR Sphere { radius 1.0 }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF pointLightTranslatorLineTranslator Separator {\n"
  "   USE pointLightOverallMaterial\n"
  "   USE POINTLIGHT_AXIS_TRANSLATOR\n"
  "}\n"
  "\n"
  "DEF pointLightTranslatorLineTranslatorActive Separator {\n"
  "   USE POINTLIGHT_ACTIVE_MATERIAL\n"
  "   USE POINTLIGHT_AXIS_TRANSLATOR\n"
  "}\n"
  "\n"
  "DEF pointLightTranslatorPlaneTranslator Separator {\n"
  "   USE POINTLIGHT_PLANE_TRANSLATOR\n"
  "}\n"
  "\n"
  "DEF pointLightTranslatorPlaneTranslatorActive Separator {\n"
  "   USE POINTLIGHT_ACTIVE_MATERIAL\n"
  "   USE POINTLIGHT_PLANE_TRANSLATOR\n"
  "}\n";

#endif /* ! SO_POINTLIGHTDRAGGER_IV_H */
