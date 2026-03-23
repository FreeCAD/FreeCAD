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

#ifndef SO_TRANSFORMBOXDRAGGER_IV_H
#define SO_TRANSFORMBOXDRAGGER_IV_H

static const char TRANSFORMBOXDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "DEF TRANSFORMBOX_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF TRANSFORMBOX_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF TRANSFORMBOX_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor 0.5 0 0.5 }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF TRANSFORMBOX_SOLIDMARKER Cube { width 0.1  height 0.1  depth 0.1 }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF TRANSFORMBOX_SCALEDRAGPOINTS Group {\n"
  "   Separator {\n"
  "      Translation { translation 1.1 1.1 1.1 }\n"
  "      USE TRANSFORMBOX_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation 1.1 1.1 -1.1 }\n"
  "      USE TRANSFORMBOX_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation 1.1 -1.1 1.1 }\n"
  "      USE TRANSFORMBOX_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation 1.1 -1.1 -1.1 }\n"
  "      USE TRANSFORMBOX_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation -1.1 1.1 1.1 }\n"
  "      USE TRANSFORMBOX_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation -1.1 1.1 -1.1 }\n"
  "      USE TRANSFORMBOX_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation -1.1 -1.1 1.1 }\n"
  "      USE TRANSFORMBOX_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation -1.1 -1.1 -1.1 }\n"
  "      USE TRANSFORMBOX_SOLIDMARKER\n"
  "   }\n"
  "}\n"
  "\n"
  "DEF transformBoxScalerScaler Separator {\n"
  "   USE TRANSFORMBOX_INACTIVE_MATERIAL\n"
  "   USE TRANSFORMBOX_SCALEDRAGPOINTS\n"
  "}\n"
  "\n"
  "DEF transformBoxScalerScalerActive Separator {\n"
  "   USE TRANSFORMBOX_ACTIVE_MATERIAL\n"
  "   USE TRANSFORMBOX_SCALEDRAGPOINTS\n"
  "}\n"
  "\n"
  "DEF transformBoxScalerFeedback Separator { }\n"
  "DEF transformBoxScalerFeedbackActive Separator { }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF TRANSFORMBOX_ROTATION_MARKER Cube { width 0.04  height 2.2  depth 0.04 }\n"
  "\n"
  "DEF TRANSFORMBOX_ROTATE_SIDE Group {\n"
  "   Separator {\n"
  "      Translation { translation 1.1 0 1.1 }\n"
  "      USE TRANSFORMBOX_ROTATION_MARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation 1.1 0 -1.1 }\n"
  "      USE TRANSFORMBOX_ROTATION_MARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation -1.1 0 1.1 }\n"
  "      USE TRANSFORMBOX_ROTATION_MARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation -1.1 0 -1.1 }\n"
  "      USE TRANSFORMBOX_ROTATION_MARKER\n"
  "   }\n"
  "}\n"
  "\n"
  "DEF transformBoxRotatorRotator Separator {\n"
  "   USE TRANSFORMBOX_INACTIVE_MATERIAL\n"
  "   USE TRANSFORMBOX_ROTATE_SIDE \n"
  "}\n"
  "DEF transformBoxRotatorRotatorActive Separator {\n"
  "   USE TRANSFORMBOX_ACTIVE_MATERIAL\n"
  "   USE TRANSFORMBOX_ROTATE_SIDE \n"
  "}\n"
  "\n"
  "DEF transformBoxRotatorFeedback Separator { }\n"
  "\n"
  "DEF transformBoxRotatorFeedbackActive Separator {\n"
  "   USE TRANSFORMBOX_FEEDBACK_MATERIAL\n"
  "   Coordinate3 { point [ 0 1.2 0, 0 -1.2 0 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1 ] }\n"
  "}\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF TRANSFORMBOX_TRANSLATIONSIDE Separator {\n"
  "   DrawStyle { style INVISIBLE }\n"
  "   Coordinate3 { point [ 1.1 1.1 1.1, -1.1 1.1 1.1, -1.1 -1.1 1.1, 1.1 -1.1 1.1 ] }\n"
  "   IndexedFaceSet { coordIndex [ 0, 1, 2, 3, -1 ] }\n"
  "}\n"
  "\n"
  "DEF transformBoxTranslatorTranslator Separator {\n"
  "   USE TRANSFORMBOX_TRANSLATIONSIDE\n"
  "}\n"
  "\n"
  "DEF transformBoxTranslatorTranslatorActive Separator {\n"
  "   USE TRANSFORMBOX_TRANSLATIONSIDE\n"
  "}\n"
  "\n"
  "DEF transformBoxTranslatorXAxisFeedback Separator {\n"
  "   DEF TRANSFORMBOX_FEEDBACK_AXIS Group {\n"
  "      USE TRANSFORMBOX_FEEDBACK_MATERIAL\n"
  "      Coordinate3 { point [ 0.5 0 1.1, -0.5 0 1.1 ] }\n"
  "      IndexedLineSet { coordIndex [ 0, 1, -1 ] }\n"
  "      Separator {\n"
  "         Rotation { rotation 0 0 1  1.57 }\n"
  "         DEF TRANSFORMBOX_FEEDBACK_MARKER Group {\n"
  "            Translation { translation 0 0.5 1.1 }\n"
  "            Cone { bottomRadius 0.04  height 0.08 }\n"
  "         }\n"
  "      }\n"
  "      Separator {\n"
  "         Rotation { rotation 0 0 1  -1.57 }\n"
  "         USE TRANSFORMBOX_FEEDBACK_MARKER\n"
  "      }\n"
  "   }\n"
  "}\n"
  "\n"
  "DEF transformBoxTranslatorYAxisFeedback Separator {\n"
  "   Rotation { rotation 0 0 1  -1.57 }\n"
  "   USE TRANSFORMBOX_FEEDBACK_AXIS\n"
  "}\n";

#endif /* ! SO_TRANSFORMBOXDRAGGER_IV_H */
