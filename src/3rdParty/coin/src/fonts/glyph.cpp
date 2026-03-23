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
  Collects functionality for glyph-handling common for 2D and 3D
  glyphs.
*/

/* ********************************************************************** */

#include "glyph.h"

#include <cassert>

#include <Inventor/C/base/list.h>

#include "fontlib_wrapper.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using namespace std;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

/* ********************************************************************** */

void 
cc_glyph_unref(cc_dict * dict, cc_glyph * glyph, cc_glyph_finalize * f)
{
  cc_list * glyphlist;
  int ret;
  void * tmp;
  int i;

  glyph->refcount--;

  assert(glyph->refcount >= 0);
  if (glyph->refcount > 0) { return; }

  /* external finalizing: */
  if (f) { (*f)(glyph); }

  /* handling of common data: */

  ret = cc_dict_get(dict, (uintptr_t)glyph->character, &tmp);
  assert(ret);
  glyphlist = (cc_list *)tmp;
    
  for (i = 0; i < cc_list_get_length(glyphlist); i++) {
    if (glyph == (cc_glyph *)cc_list_get(glyphlist, i)) break;
  }    
  assert(i < cc_list_get_length(glyphlist));

  cc_list_remove_fast(glyphlist, i);
  if (cc_list_get_length(glyphlist) == 0) {
    (void)cc_dict_remove(dict, (uintptr_t)glyph->character);
    cc_list_destruct(glyphlist);
  }

  cc_fontspec_clean(glyph->fontspec);
  free(glyph->fontspec);

  cc_flw_done_glyph(glyph->fontidx, glyph->glyphidx);
  cc_flw_unref_font(glyph->fontidx);

  free(glyph);
}

/* ********************************************************************** */
