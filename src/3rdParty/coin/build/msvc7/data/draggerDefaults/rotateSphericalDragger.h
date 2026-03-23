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

#ifndef SO_ROTATESPHERICALDRAGGER_IV_H
#define SO_ROTATESPHERICALDRAGGER_IV_H

static const char ROTATESPHERICALDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "DEF ROTATESPHERICAL_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF ROTATESPHERICAL_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF ROTATESPHERICAL_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor 0.5 0 0.5 }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF ROTATESPHERICAL_FEEDBACK Group {\n"
  "   Coordinate3 { point [ 1.3 0 0, -1.3 0 0, 0 1.3 0, 0 -1.3 0, 0 0 1.3, 0 0 -1.3 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1, 2, 3, -1, 4, 5, -1 ] }\n"
  "}\n"
  "\n"
  "DEF rotateSphericalFeedback Separator {\n"
  "   USE ROTATESPHERICAL_FEEDBACK_MATERIAL\n"
  "   USE ROTATESPHERICAL_FEEDBACK\n"
  "}\n"
  "DEF rotateSphericalFeedbackActive Separator {\n"
  "   USE ROTATESPHERICAL_FEEDBACK_MATERIAL\n"
  "   USE ROTATESPHERICAL_FEEDBACK\n"
  "}\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF ROTATESPHERICAL_RING Group {\n"
  "\n"
  "\n"
  "   ShapeHints {\n"
  "      shapeType UNKNOWN_SHAPE_TYPE\n"
  "      vertexOrdering UNKNOWN_ORDERING\n"
  "   }\n"
  "\n"
  "   DrawStyle { style LINES lineWidth 2 }\n"
  "   Cylinder { parts SIDES height 0 }\n"
  "}\n"
  "\n"
  "DEF ROTATESPHERICAL_BALL Group {\n"
  "\n"
  "\n"
  "   Scale { scaleFactor 1.733 1.733 1.733 }\n"
  "\n"
  "\n"
  "   DrawStyle { style INVISIBLE }\n"
  "   Sphere { }\n"
  "\n"
  "\n"
  "   USE ROTATESPHERICAL_RING\n"
  "   Rotation { rotation 0 0 1 1.57 }\n"
  "   USE ROTATESPHERICAL_RING\n"
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "   USE ROTATESPHERICAL_RING\n"
  "}\n"
  "\n"
  "DEF rotateSphericalRotator Separator {\n"
  "   USE ROTATESPHERICAL_INACTIVE_MATERIAL\n"
  "   USE ROTATESPHERICAL_BALL\n"
  "}\n"
  "\n"
  "DEF rotateSphericalRotatorActive Separator {\n"
  "   USE ROTATESPHERICAL_ACTIVE_MATERIAL\n"
  "   USE ROTATESPHERICAL_BALL\n"
  "}\n";

#endif /* ! SO_ROTATESPHERICALDRAGGER_IV_H */
