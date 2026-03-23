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

#ifndef SO_TRACKBALLDRAGGER_IV_H
#define SO_TRACKBALLDRAGGER_IV_H

static const char TRACKBALLDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "DEF TRACKBALL_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF TRACKBALL_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF TRACKBALL_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor 0.5 0 0.5 }\n"
  "\n"
  "DEF TRACKBALL_USER_INACTIVE_MATERIAL Material { diffuseColor 0 0.7 0.1  emissiveColor 0 0.2 0.1 }\n"
  "DEF TRACKBALL_USER_ACTIVE_MATERIAL Material { diffuseColor 0 0.8 0.1  emissiveColor 0 0.3 0.1 }\n"
  "\n"
  "\n"
  "\n"
  "DEF TRACKBALL_BAND_MARKER Group {\n"
  "   DrawStyle { style LINES }\n"
  "   ShapeHints { vertexOrdering UNKNOWN_ORDERING }\n"
  "   Cylinder { parts SIDES  radius 1.05 height 0.1 }\n"
  "}\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF trackballXRotator Separator {\n"
  "   USE TRACKBALL_INACTIVE_MATERIAL\n"
  "   DEF TRACKBALL_X_BAND Group {\n"
  "      Rotation { rotation 0 0 1 1.57 }\n"
  "      USE TRACKBALL_BAND_MARKER\n"
  "   }\n"
  "}\n"
  "DEF trackballXRotatorActive Separator {\n"
  "   USE TRACKBALL_ACTIVE_MATERIAL\n"
  "   USE TRACKBALL_X_BAND\n"
  "}\n"
  "\n"
  "DEF trackballYRotator Separator {\n"
  "   USE TRACKBALL_INACTIVE_MATERIAL\n"
  "   USE TRACKBALL_BAND_MARKER\n"
  "}\n"
  "DEF trackballYRotatorActive Separator {\n"
  "   USE TRACKBALL_ACTIVE_MATERIAL\n"
  "   USE TRACKBALL_BAND_MARKER\n"
  "}\n"
  "\n"
  "DEF trackballZRotator Separator {\n"
  "   USE TRACKBALL_INACTIVE_MATERIAL\n"
  "   DEF TRACKBALL_Z_BAND Group {\n"
  "      Rotation { rotation 1 0 0 1.57 }\n"
  "      USE TRACKBALL_BAND_MARKER\n"
  "   }\n"
  "}\n"
  "DEF trackballZRotatorActive Separator {\n"
  "   USE TRACKBALL_ACTIVE_MATERIAL\n"
  "   USE TRACKBALL_Z_BAND\n"
  "}\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF TRACKBALL_USER_AXIS Group {\n"
  "   Coordinate3 { point [ 0 1.5 0, 0 -1.5 0 ] }\n"
  "   IndexedLineSet { coordIndex [ 0, 1, -1 ] }\n"
  "}\n"
  "\n"
  "DEF trackballUserAxis Separator {\n"
  "   USE TRACKBALL_FEEDBACK_MATERIAL\n"
  "   USE TRACKBALL_USER_AXIS\n"
  "}\n"
  "DEF trackballUserAxisActive Separator {\n"
  "   USE TRACKBALL_FEEDBACK_MATERIAL\n"
  "   USE TRACKBALL_USER_AXIS\n"
  "}\n"
  "\n"
  "DEF TRACKBALL_DRAGGER_USER_ROTATOR Group {\n"
  "   Scale { scaleFactor 1.05 1.05 1.05 } # outside the x, y and z bands\n"
  "   USE TRACKBALL_BAND_MARKER\n"
  "}\n"
  "\n"
  "DEF trackballUserRotator Separator {\n"
  "   USE TRACKBALL_USER_INACTIVE_MATERIAL\n"
  "   USE TRACKBALL_DRAGGER_USER_ROTATOR\n"
  "}\n"
  "\n"
  "DEF trackballUserRotatorActive Separator {\n"
  "   USE TRACKBALL_USER_ACTIVE_MATERIAL\n"
  "   USE TRACKBALL_DRAGGER_USER_ROTATOR\n"
  "}\n"
  "\n"
  "DEF TRACKBALL_CENTER Separator {\n"
  "   DrawStyle { style INVISIBLE }\n"
  "   Sphere { radius 1.02 }\n"
  "}\n"
  "\n"
  "DEF trackballRotator Separator { USE TRACKBALL_CENTER }\n"
  "DEF trackballRotatorActive Separator { USE TRACKBALL_CENTER }\n";

#endif /* ! SO_TRACKBALLDRAGGER_IV_H */
