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

#ifndef SO_CENTERBALLDRAGGER_IV_H
#define SO_CENTERBALLDRAGGER_IV_H

static const char CENTERBALLDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "DEF CENTERBALL_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF CENTERBALL_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF CENTERBALL_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor 0.5 0 0.5 }\n"
  "DEF CENTERBALL_TRANSLATION_MATERIAL Material { diffuseColor 0 0.8 0.5  emissiveColor 0 0.2 0.0 }\n"
  "\n"
  "\n"
  "\n"
  "DEF CENTERBALL_BALL Separator {\n"
  "   DrawStyle { style INVISIBLE }\n"
  "   Sphere { }\n"
  "}\n"
  "DEF centerballRotator Separator { USE CENTERBALL_BALL }\n"
  "DEF centerballRotatorActive Separator { USE CENTERBALL_BALL }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF CENTERBALL_SCALE Scale { scaleFactor 1.02 1.02 1.02 }\n"
  "\n"
  "DEF CENTERBALL_STRIPE Separator {\n"
  "   USE CENTERBALL_SCALE\n"
  "\n"
  "\n"
  "\n"
  "   ShapeHints {\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "      shapeType UNKNOWN_SHAPE_TYPE\n"
  "      vertexOrdering UNKNOWN_ORDERING\n"
  "   }\n"
  "\n"
  "   DrawStyle { style LINES  lineWidth 2 }\n"
  "   Cylinder { parts SIDES  height 0.0 }\n"
  "}\n"
  "\n"
  "DEF centerballStripe Separator {\n"
  "\n"
  "   DrawStyle { style INVISIBLE }\n"
  "   Cylinder { parts SIDES  height 0.1 }\n"
  "\n"
  "   USE CENTERBALL_INACTIVE_MATERIAL\n"
  "   USE CENTERBALL_STRIPE\n"
  "}\n"
  "DEF centerballStripeActive Separator {\n"
  "   USE CENTERBALL_ACTIVE_MATERIAL\n"
  "   USE CENTERBALL_STRIPE\n"
  "}\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF CENTERBALL_DOUBLEHEAD_ARROW Separator {\n"
  "   Coordinate3 { point [ 0 0.1 1, 0 -0.1  1, 0 0.1 -1, 0 -0.1 -1 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1, 2, 3, -1 ] }\n"
  "   \n"
  "   Complexity { value 0.1 }\n"
  "\n"
  "   Separator {\n"
  "     Translation { translation 0 0.12 1 }\n"
  "     DEF CENTERBALL_TRANSLATOR_HEAD Cone { height 0.05  bottomRadius 0.025 }\n"
  "     Translation { translation 0 -0.24 0 }\n"
  "     Rotation { rotation 1 0 0  3.14 }\n"
  "     USE CENTERBALL_TRANSLATOR_HEAD\n"
  "   }\n"
  "   Separator {\n"
  "     Translation { translation 0 0.12 -1 }\n"
  "     USE CENTERBALL_TRANSLATOR_HEAD\n"
  "     Translation { translation 0 -0.24 0 }\n"
  "     Rotation { rotation 1 0 0  3.14 }\n"
  "     USE CENTERBALL_TRANSLATOR_HEAD\n"
  "   }\n"
  "}\n"
  "\n"
  "DEF CENTERBALL_AXIS_CROSS Separator {\n"
  "   DrawStyle { lineWidth 3 }\n"
  "   USE CENTERBALL_DOUBLEHEAD_ARROW\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   USE CENTERBALL_DOUBLEHEAD_ARROW\n"
  "}\n"
  "\n"
  "DEF centerballCenterChanger Separator {\n"
  "\n"
  "   Separator {\n"
  "      DrawStyle { style INVISIBLE }\n"
  "      Translation { translation 0 0 1 }\n"
  "      DEF CENTERBALL_TRANSLATOR_MARKER Sphere { radius 0.1 }\n"
  "      Translation { translation 0 0 -2 }\n"
  "      USE CENTERBALL_TRANSLATOR_MARKER\n"
  "   }\n"
  "\n"
  "   USE CENTERBALL_TRANSLATION_MATERIAL\n"
  "   USE CENTERBALL_SCALE\n"
  "   USE CENTERBALL_AXIS_CROSS\n"
  "   Rotation { rotation 0 1 0  3.14 }\n"
  "   USE CENTERBALL_AXIS_CROSS\n"
  "}\n"
  "DEF centerballCenterChangerActive Separator { }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF CENTERBALL_FEEDBACK_AXIS Group {\n"
  "   USE CENTERBALL_FEEDBACK_MATERIAL\n"
  "   DrawStyle { lineWidth 3 }\n"
  "   Scale { scaleFactor 3 5 1.05 }\n"
  "   USE CENTERBALL_DOUBLEHEAD_ARROW\n"
  "}\n"
  "\n"
  "DEF centerballCenterXAxisFeedback Separator {\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   USE CENTERBALL_FEEDBACK_AXIS\n"
  "}\n"
  "\n"
  "DEF centerballCenterYAxisFeedback Separator {\n"
  "   USE CENTERBALL_FEEDBACK_AXIS\n"
  "}\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF CENTERBALL_BALLAXES_COMMON Group {\n"
  "   USE CENTERBALL_ACTIVE_MATERIAL\n"
  "   USE CENTERBALL_SCALE\n"
  "   Coordinate3 { point [ 1 0 0, -1 0 0, 0 1 0, 0 -1 0, 0 0 1, 0 0 -1 ] }\n"
  "}\n"
  "\n"
  "DEF centerballXAxis Separator {\n"
  "   USE CENTERBALL_BALLAXES_COMMON\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1 ] }\n"
  "}\n"
  "DEF centerballYAxis Separator {\n"
  "   USE CENTERBALL_BALLAXES_COMMON\n"
  "   IndexedLineSet { coordIndex [ 2, 3, -1 ] }\n"
  "}\n"
  "DEF centerballZAxis Separator {\n"
  "   USE CENTERBALL_BALLAXES_COMMON\n"
  "   IndexedLineSet { coordIndex [ 4, 5, -1 ] }\n"
  "}\n";

#endif /* ! SO_CENTERBALLDRAGGER_IV_H */
