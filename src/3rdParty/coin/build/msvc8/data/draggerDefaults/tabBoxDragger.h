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

#ifndef SO_TABBOXDRAGGER_IV_H
#define SO_TABBOXDRAGGER_IV_H

static const char TABBOXDRAGGER_draggergeometry[] =
  "#Inventor V2.1 ascii\n"
  "\n"
  "\n"
  "DEF TABBOX_INACTIVE_MATERIAL Material { diffuseColor 0.5 0.5 0.5  emissiveColor 0.5 0.5 0.5 }\n"
  "\n"
  "\n"
  "\n"
  "\n"
  "DEF tabBoxScaleTabMaterial Material { diffuseColor 0 0.5 0  emissiveColor 0 0.5 0 }\n"
  "DEF tabBoxScaleTabHints ShapeHints {\n"
  "\n"
  "   vertexOrdering COUNTERCLOCKWISE\n"
  "   shapeType SOLID\n"
  "}\n"
  "\n"
  "\n"
  "DEF tabBoxBoxGeom Separator {\n"
  "   USE TABBOX_INACTIVE_MATERIAL\n"
  "   DrawStyle { style LINES }\n"
  "   PickStyle { style UNPICKABLE }\n"
  "   Cube { }\n"
  "}\n"
  "\n"
  "\n"
  "DEF tabBoxTranslator Separator {\n"
  "   DrawStyle { style INVISIBLE }\n"
  "   Coordinate3 { point [ -1 -1 0, 1 -1 0, 1 1 0, -1 1 0 ] }\n"
  "   IndexedFaceSet { coordIndex [ 0, 1, 2, 3, -1 ] }\n"
  "}\n";

#endif /* ! SO_TABBOXDRAGGER_IV_H */
