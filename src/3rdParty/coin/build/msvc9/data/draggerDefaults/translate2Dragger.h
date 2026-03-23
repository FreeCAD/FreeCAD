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

#ifndef SO_TRANSLATE2DRAGGER_IV_H
#define SO_TRANSLATE2DRAGGER_IV_H

static const char TRANSLATE2DRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "DEF TRANSLATE2_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF TRANSLATE2_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "\n"
  "\n"
  "DEF TRANSLATE2_GEOM Separator {\n"
  "\n"
  "   Cube { width 2  height 0.1  depth 0.1 } # Horizontal\n"
  "   Cube { width 0.1  height 2  depth 0.1 } # Vertical\n"
  "\n"
  "   Separator {\n"
  "      Translation { translation 1.25 0 0 }\n"
  "      RotationXYZ { axis Z  angle -1.57 }\n"
  "      DEF TRANSLATE2_ARROWHEAD Cone { height 0.5  bottomRadius 0.25 }\n"
  "   }\n"
  "\n"
  "   Separator {\n"
  "      Translation { translation -1.25 0 0 }\n"
  "      RotationXYZ { axis Z  angle 1.57 }\n"
  "      USE TRANSLATE2_ARROWHEAD\n"
  "   }\n"
  "\n"
  "   Separator {\n"
  "      Translation { translation 0 1.25 0 }\n"
  "      USE TRANSLATE2_ARROWHEAD\n"
  "   }\n"
  "\n"
  "   Separator {\n"
  "      Translation { translation 0 -1.25 0 }\n"
  "      RotationXYZ { axis X  angle 3.14 }\n"
  "      USE TRANSLATE2_ARROWHEAD\n"
  "   }\n"
  "}\n"
  "\n"
  "DEF translate2Translator Separator {\n"
  "   USE TRANSLATE2_INACTIVE_MATERIAL\n"
  "   USE TRANSLATE2_GEOM\n"
  "}\n"
  "\n"
  "DEF translate2TranslatorActive Separator {\n"
  "   USE TRANSLATE2_ACTIVE_MATERIAL\n"
  "   USE TRANSLATE2_GEOM\n"
  "}\n"
  "\n"
  "DEF translate2Feedback Separator { }\n"
  "DEF translate2FeedbackActive Separator { }\n"
  "\n"
  "DEF translate2XAxisFeedback Separator {\n"
  "  USE TRANSLATE2_ACTIVE_MATERIAL\n"
  "  DrawStyle { lineWidth 2 }\n"
  "  Coordinate3 { point [ -3 0 0, 3 0 0 ] }\n"
  "  LineSet { }\n"
  "}\n"
  "DEF translate2YAxisFeedback Separator {\n"
  "  USE TRANSLATE2_ACTIVE_MATERIAL\n"
  "  DrawStyle { lineWidth 2 }\n"
  "  Coordinate3 { point [ 0 -3 0, 0 3 0 ] }\n"
  "  LineSet { }\n"
  "}\n";

#endif /* ! SO_TRANSLATE2DRAGGER_IV_H */
