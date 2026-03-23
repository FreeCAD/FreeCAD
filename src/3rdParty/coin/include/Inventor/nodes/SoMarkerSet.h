#ifndef COIN_SOMARKERSET_H
#define COIN_SOMARKERSET_H

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

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/fields/SoMFInt32.h>

class COIN_DLL_API SoMarkerSet : public SoPointSet {
  typedef SoPointSet inherited;

  SO_NODE_HEADER(SoMarkerSet);

public:
  static void initClass(void);
  SoMarkerSet(void);

  enum MarkerType {
    NONE = -1,
    CROSS_5_5, PLUS_5_5, MINUS_5_5, SLASH_5_5, BACKSLASH_5_5, BAR_5_5,
    STAR_5_5, Y_5_5, LIGHTNING_5_5, WELL_5_5,

    CIRCLE_LINE_5_5, SQUARE_LINE_5_5, DIAMOND_LINE_5_5, TRIANGLE_LINE_5_5,
    RHOMBUS_LINE_5_5, HOURGLASS_LINE_5_5, SATELLITE_LINE_5_5,
    PINE_TREE_LINE_5_5, CAUTION_LINE_5_5, SHIP_LINE_5_5,

    CIRCLE_FILLED_5_5, SQUARE_FILLED_5_5, DIAMOND_FILLED_5_5,
    TRIANGLE_FILLED_5_5, RHOMBUS_FILLED_5_5, HOURGLASS_FILLED_5_5,
    SATELLITE_FILLED_5_5, PINE_TREE_FILLED_5_5, CAUTION_FILLED_5_5,
    SHIP_FILLED_5_5,

    CROSS_7_7, PLUS_7_7, MINUS_7_7, SLASH_7_7, BACKSLASH_7_7, BAR_7_7,
    STAR_7_7, Y_7_7, LIGHTNING_7_7, WELL_7_7,

    CIRCLE_LINE_7_7, SQUARE_LINE_7_7, DIAMOND_LINE_7_7, TRIANGLE_LINE_7_7,
    RHOMBUS_LINE_7_7, HOURGLASS_LINE_7_7, SATELLITE_LINE_7_7,
    PINE_TREE_LINE_7_7, CAUTION_LINE_7_7, SHIP_LINE_7_7,

    CIRCLE_FILLED_7_7, SQUARE_FILLED_7_7, DIAMOND_FILLED_7_7,
    TRIANGLE_FILLED_7_7, RHOMBUS_FILLED_7_7, HOURGLASS_FILLED_7_7,
    SATELLITE_FILLED_7_7, PINE_TREE_FILLED_7_7, CAUTION_FILLED_7_7,
    SHIP_FILLED_7_7,

    CROSS_9_9, PLUS_9_9, MINUS_9_9, SLASH_9_9, BACKSLASH_9_9, BAR_9_9,
    STAR_9_9, Y_9_9, LIGHTNING_9_9, WELL_9_9,

    CIRCLE_LINE_9_9, SQUARE_LINE_9_9, DIAMOND_LINE_9_9, TRIANGLE_LINE_9_9,
    RHOMBUS_LINE_9_9, HOURGLASS_LINE_9_9, SATELLITE_LINE_9_9,
    PINE_TREE_LINE_9_9, CAUTION_LINE_9_9, SHIP_LINE_9_9,

    CIRCLE_FILLED_9_9, SQUARE_FILLED_9_9, DIAMOND_FILLED_9_9,
    TRIANGLE_FILLED_9_9, RHOMBUS_FILLED_9_9, HOURGLASS_FILLED_9_9,
    SATELLITE_FILLED_9_9, PINE_TREE_FILLED_9_9, CAUTION_FILLED_9_9,
    SHIP_FILLED_9_9,
    NUM_MARKERS /* must be last, and is _not_ a marker :) */
  };

  SoMFInt32 markerIndex;

  virtual void GLRender(SoGLRenderAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);

  static int getNumDefinedMarkers(void);
  static void addMarker(int idx, const SbVec2s & size,
                        const unsigned char * bytes, SbBool isLSBFirst = TRUE,
                        SbBool isUpToDown = TRUE);
  static SbBool getMarker(int idx, SbVec2s & size,
                          const unsigned char *& bytes, SbBool & isLSBFirst);
  static SbBool removeMarker(int idx);
  static SbBool isMarkerBitSet(int idx, int bitNumber);

protected:
  virtual ~SoMarkerSet();

private:
  enum Binding {
    OVERALL = 0,
    PER_VERTEX
  };
  Binding findMaterialBinding(SoState * const state) const;
};

#endif // !COIN_SOMARKERSET_H
