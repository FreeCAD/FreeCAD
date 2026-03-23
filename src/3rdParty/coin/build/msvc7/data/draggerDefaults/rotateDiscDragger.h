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

#ifndef SO_ROTATEDISCDRAGGER_IV_H
#define SO_ROTATEDISCDRAGGER_IV_H

static const char ROTATEDISCDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "DEF ROTATEDISC_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF ROTATEDISC_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF ROTATEDISC_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor 0.5 0 0.5 }\n"
  "\n"
  "\n"
  "DEF ROTATEDISC_CYLINDER Separator {\n"
  "   DEF ROTATEDISC_CYLINDER_ROTATION Rotation { rotation 1 0 0  1.57 }\n"
  "   DEF ROTATEDISC_CYLINDER_SCALE Scale { scaleFactor 1.733 1 1.733 }\n"
  "   DEF ROTATEDISC_CYLINDER_SHAPEHINTS ShapeHints { vertexOrdering UNKNOWN_ORDERING }\n"
  "   DrawStyle { style LINES }\n"
  "   Cylinder { parts SIDES height 0.2 }\n"
  "}\n"
  "\n"
  "DEF ROTATEDISC_CYLINDER_PICK Separator {\n"
  "   USE ROTATEDISC_CYLINDER_ROTATION\n"
  "   USE ROTATEDISC_CYLINDER_SCALE\n"
  "   USE ROTATEDISC_CYLINDER_SHAPEHINTS\n"
  "   DrawStyle { style INVISIBLE }\n"
  "   Cylinder { parts ALL height 0.2 }\n"
  "}\n"
  "\n"
  "DEF ROTATEDISC_BOX Cube { width 2.2  height 2.2  depth 2.2 }\n"
  "\n"
  "DEF rotateDiscRotator Separator {\n"
  "   USE ROTATEDISC_INACTIVE_MATERIAL\n"
  "   USE ROTATEDISC_CYLINDER\n"
  "\n"
  "   USE ROTATEDISC_CYLINDER_PICK\n"
  "}\n"
  "\n"
  "DEF rotateDiscRotatorActive Separator {\n"
  "   USE ROTATEDISC_ACTIVE_MATERIAL\n"
  "   USE ROTATEDISC_CYLINDER\n"
  "   DrawStyle { style LINES  lineWidth 1 }\n"
  "   USE ROTATEDISC_BOX\n"
  "}\n"
  "\n"
  "DEF ROTATEDISC_AXIS_LINE Group {\n"
  "   Coordinate3 { point [ 0 0 1.1, 0 0 -1.1 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1 ] }\n"
  "}\n"
  "\n"
  "DEF rotateDiscFeedback Separator {\n"
  "   USE ROTATEDISC_FEEDBACK_MATERIAL\n"
  "   USE ROTATEDISC_AXIS_LINE\n"
  "}\n"
  "\n"
  "DEF rotateDiscFeedbackActive Separator {\n"
  "   USE ROTATEDISC_ACTIVE_MATERIAL\n"
  "   USE ROTATEDISC_AXIS_LINE\n"
  "}\n";

#endif /* ! SO_ROTATEDISCDRAGGER_IV_H */
