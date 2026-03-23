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

#ifndef SO_JACKDRAGGER_IV_H
#define SO_JACKDRAGGER_IV_H

static const char JACKDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "DEF JACK_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF JACK_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF JACK_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor 0.5 0 0.5 }\n"
  "DEF JACK_GREENISH_MATERIAL Material { diffuseColor 0 0.3 0.2  emissiveColor 0 0.3 0.2  transparency 0.5 }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF JACK_TRANSLATOR_AXIS Cube { width 2.2  height 0.1  depth 0.1 }\n"
  "\n"
  "DEF jackTranslatorLineTranslator Separator {\n"
  "   USE JACK_INACTIVE_MATERIAL\n"
  "   USE JACK_TRANSLATOR_AXIS\n"
  "}\n"
  "\n"
  "DEF jackTranslatorLineTranslatorActive Separator {\n"
  "   USE JACK_ACTIVE_MATERIAL\n"
  "   USE JACK_TRANSLATOR_AXIS\n"
  "}\n"
  "\n"
  "DEF JACK_FEEDBACK_AXIS Group {\n"
  "   Coordinate3 { point [ 0 3 0, 0 -3 0 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1 ] }\n"
  "   Translation { translation 0 3 0 }\n"
  "   Cone { height 0.2 bottomRadius 0.1 }\n"
  "   Translation { translation 0 -6 0 }\n"
  "   Rotation { rotation 0 0 1  3.14 }\n"
  "   Cone { height 0.2 bottomRadius 0.1 }\n"
  "}\n"
  "\n"
  "DEF jackTranslatorXFeedback Separator {\n"
  "   USE JACK_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   USE JACK_FEEDBACK_AXIS\n"
  "}\n"
  "\n"
  "DEF jackTranslatorYFeedback Separator {\n"
  "   USE JACK_FEEDBACK_MATERIAL\n"
  "   USE JACK_FEEDBACK_AXIS\n"
  "}\n"
  "\n"
  "DEF jackTranslatorZFeedback Separator {\n"
  "   USE JACK_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "   USE JACK_FEEDBACK_AXIS\n"
  "}\n"
  "\n"
  "DEF JACK_TRANSLATOR_PLANE Group {\n"
  "   DrawStyle { style LINES  lineWidth 1 }\n"
  "   Cube { depth 0.2 }\n"
  "}\n"
  "\n"
  "DEF jackTranslatorPlaneTranslator Separator {\n"
  "   USE JACK_INACTIVE_MATERIAL\n"
  "   USE JACK_TRANSLATOR_PLANE\n"
  "}\n"
  "\n"
  "DEF jackTranslatorPlaneTranslatorActive Separator {\n"
  "   USE JACK_ACTIVE_MATERIAL\n"
  "   USE JACK_TRANSLATOR_PLANE\n"
  "}\n"
  "\n"
  "DEF JACK_FEEDBACK_PLANE Group {\n"
  "   Coordinate3 { point [ -3 0 -3, 3 0 -3 , 3 0 3, -3 0 3 ] }\n"
  "   IndexedFaceSet { coordIndex [ 0, 1, 2, 3, -1 ] }\n"
  "}\n"
  "\n"
  "DEF jackTranslatorYZFeedback Separator {\n"
  "   USE JACK_GREENISH_MATERIAL\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   USE JACK_FEEDBACK_PLANE\n"
  "}\n"
  "\n"
  "DEF jackTranslatorXZFeedback Separator {\n"
  "   USE JACK_GREENISH_MATERIAL\n"
  "   USE JACK_FEEDBACK_PLANE\n"
  "}\n"
  "\n"
  "DEF jackTranslatorXYFeedback Separator {\n"
  "   USE JACK_GREENISH_MATERIAL\n"
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "   USE JACK_FEEDBACK_PLANE\n"
  "}\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF JACK_ROTATE_AXES Group {\n"
  "   DrawStyle { lineWidth 2 }\n"
  "   Coordinate3 { point [ 1.5 0 0, -1.5 0 0, 0 1.5 0, 0 -1.5 0, 0 0 1.5, 0 0 -1.5 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1, 2, 3, -1, 4, 5, -1 ] }\n"
  "}\n"
  "\n"
  "DEF jackRotatorRotator Separator {\n"
  "   USE JACK_INACTIVE_MATERIAL\n"
  "   USE JACK_ROTATE_AXES\n"
  "}\n"
  "DEF jackRotatorRotatorActive Separator {\n"
  "   USE JACK_ACTIVE_MATERIAL\n"
  "   USE JACK_ROTATE_AXES\n"
  "}\n"
  "\n"
  "DEF jackRotatorFeedback Separator { }\n"
  "DEF jackRotatorFeedbackActive Separator { }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF JACK_SCALER Group {\n"
  "   Translation { translation -1.5 0 0 }\n"
  "   DEF JACK_SCALE_MARKER Cube { width 0.1  height 0.1  depth 0.1 }\n"
  "   Translation { translation 3 0 0 }\n"
  "   USE JACK_SCALE_MARKER\n"
  "   Translation { translation -1.5 -1.5 0 }\n"
  "   USE JACK_SCALE_MARKER\n"
  "   Translation { translation 0 3 0 }\n"
  "   USE JACK_SCALE_MARKER\n"
  "   Translation { translation 0 -1.5 -1.5 }\n"
  "   USE JACK_SCALE_MARKER\n"
  "   Translation { translation 0 0 3 }\n"
  "   USE JACK_SCALE_MARKER\n"
  "}\n"
  "\n"
  "DEF jackScalerScaler Separator {\n"
  "   USE JACK_INACTIVE_MATERIAL\n"
  "   USE JACK_SCALER\n"
  "}\n"
  "DEF jackScalerScalerActive Separator {\n"
  "   USE JACK_ACTIVE_MATERIAL\n"
  "   USE JACK_SCALER\n"
  "}\n"
  "\n"
  "DEF jackScalerFeedback Separator { }\n"
  "DEF jackScalerFeedbackActive Separator { }\n";

#endif /* ! SO_JACKDRAGGER_IV_H */
