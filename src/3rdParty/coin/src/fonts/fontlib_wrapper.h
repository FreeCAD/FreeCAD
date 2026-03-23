#ifndef COIN_FLW_H
#define COIN_FLW_H

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

/*
  FLW is a Font Library Wrapper abstraction designed to allow any
  number of underlying font libraries to be used through the same API.

  Functions and datatypes are loosely modeled on the FreeType font
  library.

  Which underlying font library to use is currently determined at
  compile time.
*/

/* ********************************************************************** */

#ifndef COIN_INTERNAL
#error this is a private header file
#endif /* ! COIN_INTERNAL */

/* ********************************************************************** */

#include <Inventor/C/tidbits.h>
#include <Inventor/C/base/string.h>
#include <Inventor/C/base/list.h>
#include "fontspec.h"

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

  void cc_flw_ref_font(int fontid);
  void cc_flw_unref_font(int fontid);

  int cc_flw_get_font_id(const char * fontname, unsigned int sizey,
                         float angle, float complexity);

  unsigned int cc_flw_get_glyph(int font, unsigned int charidx);
  void cc_flw_done_glyph(int font, unsigned int glyphidx);

  struct cc_font_bitmap * cc_flw_get_bitmap(int font, unsigned int glyph);
  struct cc_font_vector_glyph * cc_flw_get_vector_glyph(int font, unsigned int glyph);

  const float * cc_flw_get_vector_glyph_coords(struct cc_font_vector_glyph * vecglyph);
  const int * cc_flw_get_vector_glyph_faceidx(struct cc_font_vector_glyph * vecglyph);
  const int * cc_flw_get_vector_glyph_edgeidx(struct cc_font_vector_glyph * vecglyph);

  void cc_flw_get_bitmap_advance(int font, unsigned int glyph, int * x, int * y);
  void cc_flw_get_bitmap_kerning(int font, unsigned int glyph1, unsigned int glyph2, int * x, int * y);

  void cc_flw_get_vector_advance(int font, unsigned int glyph, float * x, float * y);
  void cc_flw_get_vector_kerning(int font, unsigned int glyph1, unsigned int glyph2, float * x, float * y);

#ifdef __cplusplus
}
#endif

#endif /* !COIN_FLW_H */
