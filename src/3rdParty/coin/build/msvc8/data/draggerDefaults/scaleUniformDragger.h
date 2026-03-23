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

#ifndef SO_SCALEUNIFORMDRAGGER_IV_H
#define SO_SCALEUNIFORMDRAGGER_IV_H

static const char SCALEUNIFORMDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "DEF SCALEUNIFORM_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF SCALEUNIFORM_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF SCALEUNIFORM_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor 0.5 0 0.5 }\n"
  "\n"
  "\n"
  "\n"
  "DEF SCALEUNIFORM_SOLIDMARKER Cube { width 0.1  height 0.1  depth 0.1 }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF SCALEUNIFORM_DRAGPOINTS Group {\n"
  "   Separator {\n"
  "      Translation { translation -1.1 1.1 1.1 }\n"
  "      USE SCALEUNIFORM_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation 1.1 1.1 1.1 }\n"
  "      USE SCALEUNIFORM_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation 1.1 -1.1 1.1 }\n"
  "      USE SCALEUNIFORM_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation -1.1 -1.1 1.1 }\n"
  "      USE SCALEUNIFORM_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation -1.1 1.1 -1.1 }\n"
  "      USE SCALEUNIFORM_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation 1.1 1.1 -1.1 }\n"
  "      USE SCALEUNIFORM_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation 1.1 -1.1 -1.1 }\n"
  "      USE SCALEUNIFORM_SOLIDMARKER\n"
  "   }\n"
  "   Separator {\n"
  "      Translation { translation -1.1 -1.1 -1.1 }\n"
  "      USE SCALEUNIFORM_SOLIDMARKER\n"
  "   }\n"
  "}\n"
  "\n"
  "DEF scaleUniformScaler Separator {\n"
  "   USE SCALEUNIFORM_INACTIVE_MATERIAL\n"
  "   USE SCALEUNIFORM_DRAGPOINTS\n"
  "}\n"
  "\n"
  "DEF scaleUniformScalerActive Separator {\n"
  "   USE SCALEUNIFORM_ACTIVE_MATERIAL\n"
  "   USE SCALEUNIFORM_DRAGPOINTS\n"
  "\n"
  "\n"
  "   DrawStyle { style LINES  lineWidth 1 }\n"
  "   Cube { width 2.25  height 2.25  depth 2.25 }\n"
  "}\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF SCALEUNIFORM_FEEDBACK_AXES Group {\n"
  "   USE SCALEUNIFORM_FEEDBACK_MATERIAL\n"
  "   DrawStyle { lineWidth 2 }\n"
  "   Coordinate3 { point [ 1 0 0, -1 0 0, 0 1 0, 0 -1 0, 0 0 1, 0 0 -1 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1, 2, 3, -1, 4, 5, -1 ] }\n"
  "}\n"
  "\n"
  "DEF scaleUniformFeedback Separator {\n"
  "   USE SCALEUNIFORM_FEEDBACK_AXES\n"
  "}\n"
  "DEF scaleUniformFeedbackActive Separator {\n"
  "   USE SCALEUNIFORM_FEEDBACK_AXES\n"
  "}\n";

#endif /* ! SO_SCALEUNIFORMDRAGGER_IV_H */
