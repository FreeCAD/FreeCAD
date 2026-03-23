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

#ifndef SO_SCALE2UNIFORMDRAGGER_IV_H
#define SO_SCALE2UNIFORMDRAGGER_IV_H

static const char SCALE2UNIFORMDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "DEF SCALE2UNIFORM_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF SCALE2UNIFORM_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF SCALE2UNIFORM_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor 0.5 0 0.5 }\n"
  "\n"
  "DEF SCALE2UNIFORM_MARKER Group {\n"
  "   PickStyle { style SHAPE }\n"
  "   Translation { translation -0.025 0 0 }\n"
  "   Cube { width 0.05  height 0.10  depth 0.05 }\n"
  "   Translation { translation 0.05 -0.025 0 }\n"
  "   Cube { width 0.05  height 0.05  depth 0.05 }\n"
  "}\n"
  "\n"
  "DEF SCALE2UNIFORM_MARKERS Separator {\n"
  "   Separator {\n"
  "\n"
  "      Translation { translation -1.1 -1.1 0 }\n"
  "      USE SCALE2UNIFORM_MARKER\n"
  "   }\n"
  "   Separator {\n"
  "\n"
  "      Translation { translation 1.1 -1.1 0 }\n"
  "      Rotation { rotation 0 0 1  1.57 }\n"
  "      USE SCALE2UNIFORM_MARKER\n"
  "   }\n"
  "   Separator {\n"
  "\n"
  "      Translation { translation -1.1 1.1 0 }\n"
  "      Rotation { rotation 0 0 1  -1.57 }\n"
  "      USE SCALE2UNIFORM_MARKER\n"
  "   }\n"
  "\n"
  "   Translation { translation 1.1 1.1 0 }\n"
  "   Rotation { rotation 0 0 1  3.14 }\n"
  "   USE SCALE2UNIFORM_MARKER\n"
  "}\n"
  "\n"
  "DEF SCALE2UNIFORM_FRAME Separator {\n"
  "   DrawStyle { lineWidth 2 }\n"
  "   Coordinate3 { point [ 1.1 1.1 0, -1.1 1.1 0, -1.1 -1.1 0, 1.1 -1.1 0 ] }\n"
  "\n"
  "\n"
  "\n"
  "   PickStyle { style SHAPE }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, 2, 3, 0, -1 ] }\n"
  "}\n"
  "\n"
  "\n"
  "DEF scale2UniformScaler Separator {\n"
  "   USE SCALE2UNIFORM_INACTIVE_MATERIAL\n"
  "   USE SCALE2UNIFORM_FRAME\n"
  "   USE SCALE2UNIFORM_MARKERS\n"
  "}\n"
  "\n"
  "DEF scale2UniformScalerActive Separator {\n"
  "   USE SCALE2UNIFORM_ACTIVE_MATERIAL\n"
  "   USE SCALE2UNIFORM_FRAME\n"
  "   USE SCALE2UNIFORM_MARKERS\n"
  "\n"
  "   DrawStyle { style LINES  lineWidth 1 }\n"
  "   PickStyle { style UNPICKABLE }\n"
  "   Cube { width 2.2  height 2.2  depth 2.2 }\n"
  "}\n"
  "\n"
  "DEF SCALE2UNIFORM_FEEDBACK Group {\n"
  "   USE SCALE2UNIFORM_FEEDBACK_MATERIAL\n"
  "   DrawStyle { lineWidth 2 }\n"
  "   PickStyle { style UNPICKABLE }\n"
  "   Coordinate3 { point [ 1.2 0 0, -1.2 0 0, 0 1.2 0, 0 -1.2 0, 0 0 1.2, 0 0 -1.2 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1, 2, 3, -1, 4, 5, -1 ] }\n"
  "}\n"
  "\n"
  "DEF scale2UniformFeedback Separator { USE SCALE2UNIFORM_FEEDBACK }\n"
  "DEF scale2UniformFeedbackActive Separator { USE SCALE2UNIFORM_FEEDBACK }\n";

#endif /* ! SO_SCALE2UNIFORMDRAGGER_IV_H */
