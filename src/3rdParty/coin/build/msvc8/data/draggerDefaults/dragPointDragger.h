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

#ifndef SO_DRAGPOINTDRAGGER_IV_H
#define SO_DRAGPOINTDRAGGER_IV_H

static const char DRAGPOINTDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "DEF DRAGPOINT_ACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0  emissiveColor 0.5 0.5 0 }\n"
  "DEF DRAGPOINT_FEEDBACK_MATERIAL Material { diffuseColor 0.5 0 0.5  emissiveColor  0.5 0 0.5  transparency 0.2 }\n"
  "\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_STICK Group {\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   Cylinder { height 1.5 radius 0.2 }\n"
  "}\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_STICK Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "DEF DRAGPOINT_ACTIVE_STICK Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_STICK\n"
  "}\n"
  "\n"
  "DEF dragPointXTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK } \n"
  "DEF dragPointXTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "DEF dragPointYTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK }\n"
  "DEF dragPointYTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "DEF dragPointZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_STICK }\n"
  "DEF dragPointZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_STICK }\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_PLANE Group { Cube { width 1  height 1  depth .1 } }\n"
  "\n"
  "DEF DRAGPOINT_INACTIVE_PLANE Separator {\n"
  "   USE DRAGPOINT_INACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "DEF DRAGPOINT_ACTIVE_PLANE Separator {\n"
  "   USE DRAGPOINT_ACTIVE_MATERIAL\n"
  "   USE DRAGPOINT_PLANE\n"
  "}\n"
  "\n"
  "DEF dragPointXYTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointXYTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "DEF dragPointXZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointXZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "DEF dragPointYZTranslatorTranslator Separator { USE DRAGPOINT_INACTIVE_PLANE }\n"
  "DEF dragPointYZTranslatorTranslatorActive Separator { USE DRAGPOINT_ACTIVE_PLANE }\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_LINE Group {\n"
  "   Coordinate3 { point [ 0 -10 0, 0 10 0 ] }\n"
  "   LineSet { }\n"
  "\n"
  "   Transform { translation 0 10 0 }\n"
  "   DEF DRAGPOINT_FEEDBACK_ARROWHEAD Cone { height 0.5 bottomRadius 0.5 }\n"
  "   Transform { translation 0 -20 0 }\n"
  "   Rotation { rotation 0 0 1  3.14 }\n"
  "   USE DRAGPOINT_FEEDBACK_ARROWHEAD\n"
  "}\n"
  "\n"
  "DEF dragPointXFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 0 0 1 1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "DEF dragPointYFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "DEF dragPointZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 1 0 0 1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_LINE\n"
  "}\n"
  "\n"
  "\n"
  "DEF DRAGPOINT_FEEDBACK_PLANE Group {\n"
  "   ShapeHints { shapeType UNKNOWN_SHAPE_TYPE }\n"
  "   Coordinate3 { point [ -10 0 -10, -10 0 10, 10 0 10, 10 0 -10, -10 0 -10 ] }\n"
  "   FaceSet { }\n"
  "   Scale { scaleFactor 1.05 1 1.05 }\n"
  "   LineSet { }\n"
  "}\n"
  "\n"
  "DEF dragPointXYFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 1 0 0  1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n"
  "DEF dragPointXZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n"
  "DEF dragPointYZFeedback Separator {\n"
  "   USE DRAGPOINT_FEEDBACK_MATERIAL\n"
  "   Rotation { rotation 0 0 1  1.57 }\n"
  "   USE DRAGPOINT_FEEDBACK_PLANE\n"
  "}\n";

#endif /* ! SO_DRAGPOINTDRAGGER_IV_H */
