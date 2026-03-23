#ifndef COIN_CC_GLYPH3D_H
#define COIN_CC_GLYPH3D_H

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
#endif /* ! COIN_INTERNAL */

/* ********************************************************************** */

#include "fontspec.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef struct cc_glyph3d cc_glyph3d;

  cc_glyph3d * cc_glyph3d_ref(uint32_t character,
                              const cc_font_specification * spec);
  void cc_glyph3d_unref(cc_glyph3d * glyph);

  const float * cc_glyph3d_getcoords(const cc_glyph3d * g);
  const int * cc_glyph3d_getfaceindices(const cc_glyph3d * g);
  const int * cc_glyph3d_getedgeindices(const cc_glyph3d * g);
  const int * cc_glyph3d_getnextcwedge(const cc_glyph3d * g, int edgeidx);
  const int * cc_glyph3d_getnextccwedge(const cc_glyph3d * g, int edgeidx);

  float cc_glyph3d_getwidth(const cc_glyph3d * g);
  /* FIXME: the interface of the getboundingbox() function should be
     fixed, either by passing in the float array, or by making a
     datatype abstraction for a bbox. 20030916 mortene. */
  const float * cc_glyph3d_getboundingbox(const cc_glyph3d * g);

  void cc_glyph3d_getadvance(const cc_glyph3d * g, float * x, float * y);
  void cc_glyph3d_getkerning(const cc_glyph3d * left, const cc_glyph3d * right,
                             float * x, float * y);

#ifdef __cplusplus
}
#endif
#endif /* !COIN_CC_GLYPH3D_H */
